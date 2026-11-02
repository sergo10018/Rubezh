#!/usr/bin/env python3
"""
beats_ru.py - тайминг русского сценария рилса до съёмки.

Аналог beats.py из instagram-agent-skill (он не считает кириллицу).
Одна строка сценария = один бит. Оценка по числу слов и темпу речи.

Флаги:
  - хук (первая строка) длиннее 3 с;
  - бит длиннее 4 с (статичный кадр - место, где уходят);
  - нет закольцовки (последняя строка не повторяет слово из хука);
  - расхождение с целевой длиной.

Использование:
  python beats_ru.py script.txt
  python beats_ru.py script.txt --target 25 --wpm 150
  python beats_ru.py script.txt --srt draft.srt   # черновые субтитры по оценке
"""

import argparse
import re
import sys
from pathlib import Path

WORD_RE = re.compile(r"[0-9A-Za-zА-Яа-яЁё%₽$]+(?:[-'][0-9A-Za-zА-Яа-яЁё]+)*")
CONCRETE_RE = re.compile(r"\d|«|[A-ZА-ЯЁ][a-zа-яё]{2,}")

HOOK_WINDOW = 3.0
MAX_BEAT = 4.0
PAUSE = 0.25          # пауза между фразами, с
STEM_LEN = 5          # совпадение основы слова для проверки закольцовки
DEFAULT_WPM = 140     # русская речь в кадре обычно 130-160 слов/мин


def words(line):
    return WORD_RE.findall(line)


def stems(line):
    return {w.lower()[:STEM_LEN] for w in words(line) if len(w) > 3}


def analyse(lines, wpm, target):
    wps = wpm / 60.0
    beats, t = [], 0.0
    for i, line in enumerate(lines):
        dur = max(len(words(line)) / wps, 0.6) + PAUSE
        beats.append({"start": t, "dur": dur, "text": line,
                      "tag": "ХУК" if i == 0 else ("ФИНАЛ" if i == len(lines) - 1 else "")})
        t += dur
    flags = []
    if beats and beats[0]["dur"] > HOOK_WINDOW:
        flags.append(f"Хук {beats[0]['dur']:.1f} с > {HOOK_WINDOW:.0f} с. Сократите первую строку.")
    for i, b in enumerate(beats):
        if b["dur"] > MAX_BEAT:
            flags.append(f"Бит {i + 1}: {b['dur']:.1f} с на одной фразе. Разбейте или смените кадр внутри.")
    abstract_run = 0
    for i, b in enumerate(beats):
        abstract_run = 0 if CONCRETE_RE.search(b["text"]) else abstract_run + 1
        if abstract_run == 3:
            flags.append(f"Биты {i - 1}-{i + 1}: три строки подряд без цифр, имён и названий.")
    if len(beats) > 1:
        shared = stems(lines[0]) & stems(lines[-1])
        if shared:
            flags.append(f"Закольцовка есть: финал повторяет «{sorted(shared)[0]}…» из хука.")
        else:
            flags.append("Нет закольцовки: повторите в финале слово из хука.")
    if target:
        diff = t - target
        if abs(diff) > 2:
            flags.append(f"Длина {t:.1f} с против цели {target:.0f} с ({diff:+.1f} с).")
    return beats, t, flags


def fmt(t):
    return f"{int(t // 60)}:{t % 60:04.1f}"


def srt_time(t):
    ms = int(round(t * 1000))
    return f"{ms // 3600000:02}:{ms // 60000 % 60:02}:{ms // 1000 % 60:02},{ms % 1000:03}"


def write_srt(beats, path):
    out = []
    for i, b in enumerate(beats, 1):
        out.append(f"{i}\n{srt_time(b['start'])} --> {srt_time(b['start'] + b['dur'] - PAUSE)}\n{b['text']}\n")
    Path(path).write_text("\n".join(out), encoding="utf-8")


def main():
    ap = argparse.ArgumentParser(description="Тайминг русского сценария рилса")
    ap.add_argument("script", help="файл сценария, одна фраза на строку ('-' = stdin)")
    ap.add_argument("--wpm", type=float, default=DEFAULT_WPM)
    ap.add_argument("--target", type=float)
    ap.add_argument("--srt", help="записать черновые субтитры в этот файл")
    args = ap.parse_args()

    raw = sys.stdin.read() if args.script == "-" else Path(args.script).read_text(encoding="utf-8")
    lines = [l.strip() for l in raw.splitlines() if l.strip() and not l.strip().startswith("```")]
    if not lines:
        sys.exit("Пустой сценарий.")

    beats, total, flags = analyse(lines, args.wpm, args.target)
    n_words = sum(len(words(l)) for l in lines)
    head = f"БИТЫ · {n_words} слов · ~{total:.1f} с при {args.wpm:.0f} сл/мин"
    print(head + (f" · цель {args.target:.0f} с" if args.target else ""))
    print("=" * 76)
    for b in beats:
        print(f"  {fmt(b['start']):>6}  {b['dur']:4.1f}s  {b['tag']:<6} {b['text'][:52]}")
    print("-" * 76)
    for f in flags:
        print("  - " + f)
    if args.srt:
        write_srt(beats, args.srt)
        print(f"\nСубтитры: {args.srt}")


if __name__ == "__main__":
    main()
