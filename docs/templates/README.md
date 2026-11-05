# Шаблоны документов

Заготовки, которые Claude и агенты берут сами, когда приходит время. Помнить о них не нужно: указатель есть в `CLAUDE.md`.

Шаблон — это подсказка, а не анкета. Что к задаче не относится, удаляй. Числа придумывать нельзя: если значения ещё нет, пиши «не решено». Копию заполняем в папке из таблицы, сам шаблон не меняем.

| Шаблон | Когда нужен | Куда класть заполненный | Кто ведёт |
|---|---|---|---|
| `bug-report.md` | Найден баг | `docs/qa/bugs/BUG-<номер>.md` | `qa-lead` |
| `test-evidence.md` | Закрываем задачу, которую видит игрок | `docs/qa/evidence/<задача>/отчёт.md`, рядом скриншоты | исполнитель |
| `playtest-report.md` | После плейтеста (скилл `playtest-report`) | `docs/qa/playtests/<дата>-<кто>.md` | `qa-lead` |
| `economy-model.md` | Проектируем экономику системы | `docs/gdd/economy_<система>.md` (общая картина — `docs/gdd/economy.md`) | `economy-designer` |
| `faction-design.md` | Подробно описываем фракцию | `docs/narrative/factions/<фракция>.md` | `lore-keeper` |
| `level-design-document.md` | Проектируем зону | `docs/levels/<зона>.md` | `level-designer` |

Шаблоны переработаны под «Рубеж» на основе [Claude Code Game Studios](https://github.com/Donchitos/Claude-Code-Game-Studios) (MIT, см. `.claude/skills/LICENSE-Donchitos-claude-code-game-studios`).
