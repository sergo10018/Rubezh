# Промпты для Google Flow (Veo): б-ролл рилсов №1–3

> Версия 0.1 · 2026-11-01
> Людьми снимается только то, что нельзя генерировать: **мастер-дубли** (голос и лицо продюсера) и **5 лиц команды** (№1, S02). Записи экрана (№1 S03, S08; №2 S01) я рисую сам из реальных файлов проекта. Всё остальное — Flow.

## Настройки Flow
- Режим: **Text to Video** (для S08_zip №3 — **Frames to Video**, см. ниже).
- Соотношение сторон: **9:16 (портрет)**.
- Модель: Veo (Quality для героев-планов S01/S08 №3 и S06 №2, Fast для остального).
- 2 варианта на промпт. Скачивать в **1080p**, MP4.
- Имя файла — как в таблице, класть в `marketing/reels/NN_*/raw/`.
- Звук генерации не нужен (в монтаже он выключен).

## Общий стиль (добавляется в конец каждого промпта)
```
Vertical 9:16. Photorealistic documentary footage, shot on a 35mm cinema camera, natural overcast light, cold desaturated color grade with muted teal and grey tones, subtle film grain, shallow depth of field. Realistic, grounded, no fantasy elements. No text overlays, no logos, no watermarks, no visible faces unless specified.
```

## Канон-фильтр (проверено)
Никаких мутантов, аномалий, светящихся объектов, оружия в кадре. Собаки — обычные, без «монстрового» вида. Если Flow выдаёт что-то фантастическое (светящиеся глаза, уродство), вариант выбрасываем.

---

## №1 · «Пятеро против MMO»

| Файл | Промпт |
|---|---|
| `S05_ruins.mp4` | Slow handheld dolly forward through an abandoned Soviet-era industrial workshop, broken windows, rusted steel beams, concrete dust floating in grey daylight, puddles on the cracked floor, quiet and empty. |
| `S06_tool.mp4` | Close-up, side angle: a worker's hands in worn fingerless gloves place a heavy rusty adjustable wrench onto a scratched steel workbench. Slow deliberate movement, cold workshop light, metal shavings on the bench. |
| `S07_plant.mp4` | Static wide shot from a distance: an old working thermal power plant on the edge of a small town at dusk, two chimneys releasing steady white steam into a grey sky, power lines in the foreground, a few lit windows. |

## №2 · «Постапокалипсис без мутантов»

> Кириллица в генерации получается плохо. Сначала пробуем с текстом; если буквы кривые — берём вариант «blank» (без надписи), слово поставлю карточкой.

| Файл | Промпт |
|---|---|
| `S02_cross.mp4` | Top-down shot of a sheet of white paper on a dark wooden desk with the Russian word "МУТАНТЫ" handwritten in large black block letters. A hand with a thick red marker crosses the word out with one fast, decisive stroke. *(blank-вариант: «…with a single handwritten word in black block letters…»)* |
| `S03_cut.mp4` | Top-down shot on the same dark wooden desk: steel scissors cut a narrow strip of white paper with a handwritten black word on it, the strip falls away. Close-up, crisp paper texture. |
| `S04_trash.mp4` | Side view of a black metal office trash bin on a concrete floor. Three narrow strips of white paper with handwritten words fall into the bin one after another, slow motion. Dim cold light. |
| `S06_dogs.mp4` | A pack of five large ordinary stray dogs, mixed breeds, scruffy fur, trotting purposefully across a snowy abandoned industrial yard at dawn, low camera angle from a distance, telephoto lens. Realistic ordinary dogs, no aggression close to camera. |
| `S07_mask.mp4` | Close-up on a dark background: a hand unscrews a round filter canister from an old rubber Soviet-style gas mask lying on a metal table. Cold side light, dust particles in the air. |
| `S08_snow.mp4` | Heavy blizzard on an empty street of a small industrial town at night, a single streetlight, snow blowing sideways, abandoned car half buried in snow. |
| `S08_fuel.mp4` | Extreme close-up of an old car's analog fuel gauge, the needle drops slowly to empty, dim dashboard backlight, frost on the edges of the windshield visible out of focus. |
| `S09_people.mp4` | Three people in dark hooded jackets stand as silhouettes against a cold low sunset behind a chain-link fence, faces completely in shadow, they slowly turn their heads toward the camera. Static camera, backlit, no visible faces. |

## №3 · «Главный лут не влезет в рюкзак»

| Файл | Промпт |
|---|---|
| `S01_backpack.mp4` | Side view, medium close-up: a worn olive-green hiking backpack on a wooden table. Hands try to stuff a printed A4 photograph of a large factory into the backpack, the paper bends and crumples and does not fit, the hands push harder. Action starts in the very first frame. |
| `S02_pump.mp4` | Slow lateral tracking shot of an old brick water pumping station with a tall water tower, overgrown with dry grass, overcast autumn day. |
| `S02_factory.mp4` | Slow lateral tracking shot along the fence of a large old industrial factory with rusted pipes, a tall chimney and corrugated metal walls, overcast cold day. |
| `S02_rail.mp4` | Wide shot from a footbridge over a railway junction: several tracks merging, an old diesel locomotive standing still, rusted freight wagons, grey sky, light fog. |
| `S04_fix.mp4` | Extreme close-up: a wrench tightens a large bolt on a rusty industrial pipe flange, gloved hand, a few drops of water on the metal. |
| `S04_supply.mp4` | Close-up of gloved hands carrying a battered wooden supply crate through a doorway of a concrete building, handheld following camera. |
| `S04_guard.mp4` | A person in a dark padded jacket stands guard at an old industrial gate at dusk, seen from behind, watching an empty road, breath visible in cold air. Static camera. |
| `S08_zip.mp4` | **Frames to Video**: первый кадр — последний кадр выбранного `S01_backpack`. Промпт: The same backpack on the same table. Hands close the zipper; a crumpled corner of the factory photograph sticks out of the closed backpack. Static camera, same angle. |

---

## Честность и правила площадок
- Instagram и YouTube требуют отмечать **реалистичный ИИ-контент**: при публикации включить «ИИ-метку» (Instagram: «Дополнительные настройки → Метка ИИ»; YouTube Shorts: «Изменённый или синтетический контент → Да»).
- В подписи к ролику: «б-ролл сгенерирован в Google Flow». Для девлога про команду с ИИ это плюс, а не минус, — и это часть нашей позиции «показываем всё».
- Сгенерированные кадры не выдаём за кадры игры. Это иллюстрации мира.
