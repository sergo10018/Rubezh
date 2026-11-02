#!/usr/bin/env python3
"""
render_screen.py - вертикальный клип «экран редактора/терминала» из реального текстового файла.

Используется вместо записи экрана, когда нужно показать настоящий файл проекта
или настоящий вывод инструмента крупно и читаемо на телефоне.

  python render_screen.py text.txt out.mp4 --dur 3 --title "01_Канон.md" --reveal
  python render_screen.py text.txt out.mp4 --dur 3 --highlight 6 --hl-at 0.4 --numbers 84
"""

import argparse
import subprocess
import sys
import tempfile
from pathlib import Path

W, H, FPS = 1080, 1920, 30
FONT = "C:/Windows/Fonts/consola.ttf"
FONT_UI = "C:/Windows/Fonts/segoeuib.ttf"
SIZE, LINE_H, WRAP = 36, 54, 44
TOP, LEFT = 560, 60
BG, FG, DIM, HL, ERR = "0x0d1117", "0xd7dde4", "0x6b7280", "0x7c2d12", "0xf87171"


def esc(path):
    return str(path).replace("\\", "/").replace(":", "\\:").replace("'", "\\'")


def wrap_lines(raw):
    out = []
    for src_no, line in enumerate(raw.splitlines()):
        line = line.rstrip() or " "
        while len(line) > WRAP:
            out.append((src_no, line[:WRAP]))
            line = "  " + line[WRAP:]
        out.append((src_no, line))
    return out


def build(args):
    raw = Path(args.text).read_text(encoding="utf-8")
    lines = wrap_lines(raw)
    max_rows = (H - TOP - 200) // LINE_H
    lines = lines[:max_rows]
    work = Path(tempfile.mkdtemp(prefix="screen_"))
    f = []
    # Шапка окна.
    f.append(f"drawbox=x=0:y={TOP - 110}:w={W}:h=80:color=0x161b22:t=fill")
    for i, c in enumerate(("0xff5f57", "0xfebc2e", "0x28c840")):
        f.append(f"drawbox=x={LEFT + i * 44}:y={TOP - 82}:w=24:h=24:color={c}:t=fill")
    if args.title:
        t = work / "title.txt"
        t.write_text(args.title, encoding="utf-8")
        f.append(f"drawtext=fontfile='{esc(FONT_UI)}':textfile='{esc(t)}':fontsize=30:fontcolor={DIM}:"
                 f"x={LEFT + 150}:y={TOP - 86}")
    n_src = max(n for n, _ in lines) + 1
    step = (args.dur * 0.8) / max(n_src, 1)
    for row, (src_no, text) in enumerate(lines):
        y = TOP + row * LINE_H
        shown = f"gte(t,{src_no * step:.3f})" if args.reveal else "1"
        if args.highlight and src_no + 1 == args.highlight:
            f.append(f"drawbox=x=0:y={y - 8}:w={W}:h={LINE_H}:color={HL}@0.85:t=fill:"
                     f"enable='gte(t,{args.hl_at:.3f})'")
        tf = work / f"l{row:03}.txt"
        tf.write_text(text, encoding="utf-8")
        color = ERR if args.error_words and any(w in text for w in args.error_words.split(",")) else FG
        f.append(f"drawtext=fontfile='{esc(FONT)}':textfile='{esc(tf)}':fontsize={SIZE}:fontcolor={color}:"
                 f"x={LEFT + (90 if args.numbers else 0)}:y={y}:enable='{shown}'")
        if args.numbers and not text.startswith("  "):
            nf = work / f"n{row:03}.txt"
            nf.write_text(str(args.numbers + src_no), encoding="utf-8")
            f.append(f"drawtext=fontfile='{esc(FONT)}':textfile='{esc(nf)}':fontsize={SIZE}:fontcolor={DIM}:"
                     f"x={LEFT}:y={y}:enable='{shown}'")
    # Курсор мигает после последней строки.
    cy = TOP + len(lines) * LINE_H
    f.append(f"drawbox=x={LEFT}:y={cy}:w=20:h=40:color={FG}:t=fill:enable='lt(mod(t,1),0.5)'")
    cmd = ["ffmpeg", "-y", "-v", "error", "-f", "lavfi", "-i", f"color=c={BG}:s={W}x{H}:r={FPS}:d={args.dur}",
           "-vf", ",".join(f) + ",format=yuv420p", "-c:v", "libx264", "-crf", "18", args.out]
    res = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace")
    if res.returncode != 0:
        sys.exit(res.stderr[-2000:])
    print(f"Готово: {args.out}")


def main():
    ap = argparse.ArgumentParser(description="Клип «экран» из текстового файла")
    ap.add_argument("text")
    ap.add_argument("out")
    ap.add_argument("--dur", type=float, default=3.0)
    ap.add_argument("--title", default="")
    ap.add_argument("--reveal", action="store_true", help="строки появляются по очереди")
    ap.add_argument("--highlight", type=int, help="номер строки исходника для подсветки (с 1)")
    ap.add_argument("--hl-at", type=float, default=0.4)
    ap.add_argument("--numbers", type=int, help="показывать номера строк, начиная с этого")
    ap.add_argument("--error-words", default="", help="слова через запятую: такие строки красные")
    build(ap.parse_args())


if __name__ == "__main__":
    main()
