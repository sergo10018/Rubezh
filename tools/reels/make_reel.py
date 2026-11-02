#!/usr/bin/env python3
"""
make_reel.py - сборка вертикального рилса 1080x1920 из исходников по JSON-монтажному листу.

Нужен ffmpeg в PATH. Ничего не загружает в сеть.

Монтажный лист (пути - относительно файла листа):
{
  "output": "out/reel_01.mp4",
  "font": "C:/Windows/Fonts/arialbd.ttf",
  "clips": [
    {"src": "raw/face.mp4", "in": 0.0, "out": 3.5},              # вертикальное видео: кроп под 9:16
    {"src": "raw/screen.mp4", "in": 10, "out": 14, "fit": "blur", "speed": 4},  # горизонтальное: на размытом фоне
    {"image": "raw/factory.jpg", "dur": 3.0, "anim": "in"},       # картинка: in/out/up/down/left/right, "amount": 0.12
    {"color": "#0d0f10", "dur": 1.5}                              # заливка под титр
  ],
  "cards": [{"text": "50-300 ЧЕЛОВЕК. НАС 5", "start": 0, "end": 3.5}],
  "srt": "subs.srt",          # вшитые субтитры (необязательно)
  "voice": "raw/vo.wav",      # озвучка (необязательно)
  "music": "raw/music.mp3",   # музыка (необязательно)
  "music_volume": 0.12,
  "clip_audio": true           # оставить звук исходников
}

Использование:
  python make_reel.py edit.json
"""

import json
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

W, H, FPS = 1080, 1920, 30
# Безопасная зона Reels: не выше y=230, не ниже y=1440, правые 230 px свободны.
SAFE_LEFT, SAFE_RIGHT, SAFE_TOP, SAFE_BOTTOM = 80, W - 230, 230, 1440
# Текст: Montserrat Bold, белый, тонкая тёмная обводка, появление «Pop».
TEXT_BORDER, TEXT_BORDER_COLOR = 3, "0x111111@0.9"
POP_TIME, POP_FROM = 0.2, 0.6   # с; стартовый масштаб
CARD_SIZE, SUB_SIZE = 72, 54
CARD_WRAP, SUB_WRAP = 15, 24   # символов в строке (Montserrat шире Arial)
DEFAULT_FONT = Path(__file__).resolve().parent / "fonts" / "Montserrat-Bold.ttf"


def run(cmd):
    cmd = [cmd[0], "-hide_banner", *cmd[1:]]
    res = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace")
    if res.returncode != 0:
        sys.exit(f"ffmpeg завершился с ошибкой:\n{' '.join(map(str, cmd))}\n{res.stderr[-2000:]}")


def esc(path):
    """Путь для значения опции внутри filtergraph (двоеточие и кавычки экранируются)."""
    return str(path).replace("\\", "/").replace(":", "\\:").replace("'", "\\'")


def wrap(text, width):
    lines, cur = [], ""
    # Тире не переносится в начало строки: склеиваем его с предыдущим словом неразрывным пробелом.
    text = re.sub(r"\s+([—–-])(?=\s)", " \\1", " ".join(text.split()))
    for word in text.split(" "):
        if cur and len(cur) + 1 + len(word) > width:
            lines.append(cur)
            cur = word
        else:
            cur = f"{cur} {word}".strip()
    return "\n".join(lines + [cur])


def segment(clip, base, work, idx):
    """Нормализует один клип в сегмент 1080x1920@30 с AAC-дорожкой (тишина, если звука нет)."""
    out = work / f"seg_{idx:03}.mp4"
    silence = ["-f", "lavfi", "-i", "anullsrc=r=48000:cl=stereo"]
    if "color" in clip:
        dur = float(clip.get("dur", 2))
        cmd = ["ffmpeg", "-y", "-f", "lavfi", "-i", f"color=c={clip['color']}:s={W}x{H}:r={FPS}:d={dur}",
               *silence, "-vf", "format=yuv420p", "-map", "0:v", "-map", "1:a", "-t", str(dur)]
    elif "image" in clip:
        dur = float(clip.get("dur", 3))
        frames = int(dur * FPS)
        anim = clip.get("anim", "in" if clip.get("zoom") else "none")
        move = image_motion(anim, frames, float(clip.get("amount", 0.12)))
        vf = (f"scale={W*2}:{H*2}:force_original_aspect_ratio=increase,crop={W*2}:{H*2},"
              f"zoompan={move}:d={frames}:s={W}x{H}:fps={FPS},setsar=1,format=yuv420p")
        cmd = ["ffmpeg", "-y", "-i", str(base / clip["image"]), *silence,
               "-vf", vf, "-map", "0:v", "-map", "1:a", "-t", str(dur)]
    else:
        src = base / clip["src"]
        start, end = float(clip.get("in", 0)), clip.get("out")
        speed = float(clip.get("speed", 1))
        trim = ["-ss", str(start)] + (["-to", str(float(end))] if end is not None else [])
        pts = f"setpts=PTS/{speed}," if speed != 1 else ""
        if clip.get("fit") == "blur":
            fc = (f"[0:v]{pts}split[a][b];"
                  f"[a]scale={W}:{H}:force_original_aspect_ratio=increase,crop={W}:{H},boxblur=30:5[bg];"
                  f"[b]scale={W}:-2[fg];[bg][fg]overlay=(W-w)/2:(H-h)/2,setsar=1,fps={FPS},format=yuv420p[v]")
        else:
            fc = (f"[0:v]{pts}scale={W}:{H}:force_original_aspect_ratio=increase,crop={W}:{H},"
                  f"setsar=1,fps={FPS},format=yuv420p[v]")
        has_audio = speed == 1 and probe_audio(src)
        if has_audio:
            fc += ";[0:a]aresample=48000,aformat=channel_layouts=stereo[a]"
            amap, extra = "[a]", []
        else:
            amap, extra = "1:a", silence
        cmd = ["ffmpeg", "-y", *trim, "-i", str(src), *extra, "-filter_complex", fc,
               "-map", "[v]", "-map", amap, "-shortest"]
    run(cmd + ["-c:v", "libx264", "-preset", "veryfast", "-crf", "18",
               "-c:a", "aac", "-b:a", "192k", "-ar", "48000", str(out)])
    return out


def image_motion(anim, frames, amount):
    """Выражения zoompan для анимации картинки: in/out - наезд/отъезд, up/down/left/right - панорама."""
    p = f"(on/{max(frames - 1, 1)})"            # прогресс 0..1
    center = "x='iw/2-iw/zoom/2':y='ih/2-ih/zoom/2'"
    if anim == "in":
        return f"z='1+{amount}*{p}':{center}"
    if anim == "out":
        return f"z='1+{amount}*(1-{p})':{center}"
    pans = {"down": "x='iw/2-iw/zoom/2':y='(ih-ih/zoom)*{p}'",
            "up": "x='iw/2-iw/zoom/2':y='(ih-ih/zoom)*(1-{p})'",
            "right": "x='(iw-iw/zoom)*{p}':y='ih/2-ih/zoom/2'",
            "left": "x='(iw-iw/zoom)*(1-{p})':y='ih/2-ih/zoom/2'"}
    if anim in pans:
        return f"z='1+{amount}':" + pans[anim].format(p=p)
    return f"z=1:{center}"


def probe_audio(path):
    res = subprocess.run(["ffprobe", "-v", "error", "-select_streams", "a", "-show_entries",
                          "stream=index", "-of", "csv=p=0", str(path)], capture_output=True, text=True)
    return bool(res.stdout.strip())


def parse_srt(path):
    def secs(ts):
        h, m, rest = ts.replace(",", ".").split(":")
        return int(h) * 3600 + int(m) * 60 + float(rest)
    cues = []
    for block in re.split(r"\n\s*\n", Path(path).read_text(encoding="utf-8-sig").strip()):
        lines = block.strip().splitlines()
        timing = next((l for l in lines if "-->" in l), None)
        if not timing:
            continue
        a, b = (s.strip() for s in timing.split("-->"))
        text = " ".join(lines[lines.index(timing) + 1:])
        cues.append({"text": text, "start": secs(a), "end": secs(b), "kind": "sub"})
    return cues


def pop_progress(start):
    return f"clip((t-{start:.3f})/{POP_TIME},0,1)"


def pop_size(size, start):
    """«Pop»: размер растёт от POP_FROM до 100% с небольшим перескоком (easeOutBack) за POP_TIME."""
    p = pop_progress(start)
    ease = f"(1+2.70158*pow({p}-1,3)+1.70158*pow({p}-1,2))"
    return f"{size}*({POP_FROM}+(1-{POP_FROM})*{ease})"


def pop_alpha(start):
    return f"min(1,2*{pop_progress(start)})"


def drawtext_filters(items, font, work):
    """Текстовые карточки и субтитры через drawtext; текст пишется в файлы, чтобы не экранировать."""
    filters = []
    for i, it in enumerate(items):
        is_sub = it.get("kind") == "sub"
        size = SUB_SIZE if is_sub else int(it.get("size", CARD_SIZE))
        txt = work / f"text_{i:03}.txt"
        txt.write_text(wrap(it["text"], SUB_WRAP if is_sub else CARD_WRAP), encoding="utf-8", newline="\n")
        start, end = float(it["start"]), float(it["end"])
        box_w = SAFE_RIGHT - SAFE_LEFT
        x = f"{SAFE_LEFT}+({box_w}-text_w)/2"
        # Якорь - центр блока по вертикали, чтобы «Pop» рос из центра, а не из угла.
        if is_sub:
            cy = SAFE_BOTTOM - 40 - size
        elif it.get("pos") == "center":
            cy = (SAFE_TOP + SAFE_BOTTOM) // 2
        else:
            cy = SAFE_TOP + 40 + size
        filters.append(
            f"drawtext=fontfile='{esc(font)}':textfile='{esc(txt)}':fontsize='{pop_size(size, start)}':"
            f"fontcolor=white:alpha='{pop_alpha(start)}':borderw={TEXT_BORDER}:bordercolor={TEXT_BORDER_COLOR}:"
            f"shadowx=0:shadowy=4:shadowcolor=black@0.45:"
            f"text_align=C:line_spacing=6:x={x}:y={cy}-text_h/2:"
            f"enable='between(t,{start:.3f},{end:.3f})'")
    return ",".join(filters) if filters else "null"


def build(spec_path):
    spec_path = Path(spec_path).resolve()
    base = spec_path.parent
    spec = json.loads(spec_path.read_text(encoding="utf-8"))
    if not spec.get("clips"):
        sys.exit("В монтажном листе нет clips.")
    font = spec.get("font", DEFAULT_FONT)
    output = base / spec.get("output", "reel.mp4")
    output.parent.mkdir(parents=True, exist_ok=True)

    work = Path(tempfile.mkdtemp(prefix="reel_"))
    try:
        segs = [segment(c, base, work, i) for i, c in enumerate(spec["clips"])]
        concat_list = work / "list.txt"
        concat_list.write_text("".join(f"file '{s.as_posix()}'\n" for s in segs), encoding="utf-8")
        joined = work / "joined.mp4"
        run(["ffmpeg", "-y", "-f", "concat", "-safe", "0", "-i", str(concat_list), "-c", "copy", str(joined)])

        # drawtext не открывает шрифт по пути с кириллицей - копируем во временную папку.
        font = shutil.copy(font, work / ("font" + Path(font).suffix))
        items = list(spec.get("cards", []))
        if spec.get("srt"):
            items += parse_srt(base / spec["srt"])
        vf = drawtext_filters(items, font, work)

        inputs, mix, n = ["-i", str(joined)], [], 1
        clip_vol = 1.0 if spec.get("clip_audio", True) else 0.0
        mix.append(f"[0:a]volume={clip_vol}[a0]")
        labels = ["[a0]"]
        if spec.get("voice"):
            inputs += ["-i", str(base / spec["voice"])]
            mix.append(f"[{n}:a]aresample=48000,volume=1.0[a{n}]")
            labels.append(f"[a{n}]")
            n += 1
        if spec.get("music"):
            inputs += ["-stream_loop", "-1", "-i", str(base / spec["music"])]
            mix.append(f"[{n}:a]aresample=48000,volume={float(spec.get('music_volume', 0.12))}[a{n}]")
            labels.append(f"[a{n}]")
            n += 1
        mix.append(f"{''.join(labels)}amix=inputs={len(labels)}:duration=first:normalize=0,"
                   f"loudnorm=I=-14:TP=-1.5:LRA=11[aout]")
        fc = f"[0:v]{vf}[vout];" + ";".join(mix)
        run(["ffmpeg", "-y", *inputs, "-filter_complex", fc, "-map", "[vout]", "-map", "[aout]",
             "-c:v", "libx264", "-preset", "medium", "-crf", "18", "-profile:v", "high",
             "-pix_fmt", "yuv420p", "-r", str(FPS), "-c:a", "aac", "-b:a", "192k", "-ar", "48000",
             "-movflags", "+faststart", str(output)])
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print(f"Готово: {output}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    build(sys.argv[1])
