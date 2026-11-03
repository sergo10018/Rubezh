# РУБЕЖ: Земля после разлома

MMO-lite онлайн-шутер с выживанием, экономикой игроков и войнами гильдий за инфраструктуру. Unreal Engine 5 (C++ + Blueprints, на основе Lyra), выделенные серверы зон + бэкенд Nakama/PostgreSQL. Команда: 5 человек + Claude. Язык общения и документов — русский.

## Главные документы (читать перед работой)
- `docs/01_Канон.md` — **единственный источник правды о мире**. Столпы игры — раздел 0.
- `docs/00_Процесс_и_план_разработки.md` — этапы, роли, gate-критерии, риски.
- `docs/gdd/` — геймдизайн по системам; `docs/tech/` — архитектура; `docs/levels/`, `docs/narrative/`, `docs/qa/`, `docs/backlog/`.
- `docs/marketing/` — продвижение разработки (Reels/Shorts): стратегия, сценарии, голос аккаунта. Скиллы `.claude/skills/ig-*`, сборка видео — `tools/reels/`. Публикует только человек.
- `.agents/ue-project-context.md` — контекст UE-проекта для скиллов (создать скиллом `ue-project-context`, когда появится `.uproject`).

## Жёсткие правила
- **Мир реалистичный**: никаких мутантов, аномалий, артефактов, лабораторных организмов, «Глубинных». Опасны люди, одичавшие реальные животные, техногенная среда.
- Канон меняется только с одобрения лид-геймдизайнера и с записью в журнал изменений канона.
- **C++ — логика, Blueprint — настройка.** Сложная логика не живёт в графах.
- **Сервер-авторитетность**: урон, лут, инвентарь, крафт, деньги решает сервер; постоянные данные — только через бэкенд.
- **Не редактировать `.uasset`/`.umap` файлами** (запрещено в `.claude/settings.json`) — только редактор, его Python API или Unreal MCP.
- Префикс кода проекта — `Rubezh`. Данные (предметы, рецепты, цены) — в `data/` как CSV/JSON для DataTable.
- Секреты — только в переменных окружения.
- Тесты пишем для систем без визуала (урон, инвентарь, крафт, экономика, бэкенд), а не для каждого визуального эффекта; глобальное правило 80% покрытия к ассетам и Blueprint не применяется.

## Агенты проекта (`.claude/agents/`)
| Агент | Когда |
|---|---|
| `producer` | Спринты, бэклог, распределение задач, проверка gate |
| `game-designer` | Механики, разделы GDD |
| `economy-designer` | Цены, лут, крафт, стоки, симуляции экономики |
| `lore-keeper` | Тексты, квесты, проверка соответствия канону |
| `unreal-specialist` | Игровой C++ / GAS / Lyra |
| `network-programmer` | Репликация, серверы, античит, Nakama, БД |
| `level-designer` | Документы локаций, блокаут через MCP |
| `qa-lead` | Тест-планы, плейтесты, баги, автотесты |

Ревью C++ — `ecc:cpp-reviewer`; безопасность бэкенда и античита — `ecc:security-reviewer`; БД — `ecc:database-reviewer` (плагин ECC).

## Скиллы и инструменты
- `.claude/skills/ue-*` — 28 скиллов по UE5 C++ из [quodsoler/unreal-engine-skills](https://github.com/quodsoler/unreal-engine-skills) (MIT, коммит в `SOURCE-quodsoler.txt`). Использовать вместо памяти об API движка.
- Unreal MCP (`.mcp.json`, плагин в `tools/third_party/EngineWire`) — настройка в `docs/tech/unreal_mcp_setup.md`.

## Текущий этап
Этап 0 «Подготовка» (см. план).
- Корень репозитория — `E:\Rubezh` (Git + LFS, ассеты `lockable`). Удалённый репозиторий — приватный GitHub `https://github.com/sergo10018/Rubezh`, ветка `main`.
- UE 5.8.4 из лаунчера: `E:\UE_5.8`. Проект — `Game/Rubezh.uproject` на основе Lyra (модули `LyraGame`/`LyraEditor` пока не переименованы; наш код будет в отдельном модуле/плагине с префиксом `Rubezh`).
- Плагин EngineWire (Unreal MCP) установлен в `Game/Plugins/EngineWire`. Операции уровней MCP работают только с `/Game`; для контента плагинов — Python через MCP.
- Наш код и контент — Game Feature плагин `Game/Plugins/GameFeatures/RubezhCore` (модуль `RubezhCoreRuntime`, лог `LogRubezh`). Тестовая карта — `/RubezhCore/Maps/L_Rubezh_Proto`, режим — `B_Rubezh_Proto`.
- Геймдизайн: `docs/gdd/00_Core_loop.md` и `docs/gdd/01_Системы_прототипа.md` — решения лида внесены (смерть, выход из боя 15 с, КПП, сетка-инвентарь, камера, голос).
- Сборка редактора: `E:\UE_5.8\Engine\Build\BatchFiles\Build.bat LyraEditor Win64 Development -Project=E:\Rubezh\Game\Rubezh.uproject -WaitMutex`. При открытом редакторе с Live Coding сборка не идёт — редактор надо закрыть.
- Автотесты: `UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests Rubezh; Quit" -unattended -nullrhi -nosplash -nosound -stdout`.
- Для выделенного сервера понадобится движок, собранный из исходников (Этап 1).

## Что уже сделано в коде (RubezhCore)
- `URubezhSurvivalSet` — выносливость и вес (GAS), выдаётся игроку через `AbilitySet_RubezhSurvival` → `HeroData_Rubezh` (свои данные персонажа, копия `HeroData_ShooterGame`) → режим `B_Rubezh_Proto`.
- `Rubezh::Encumbrance` — уровни перегруза; `ERubezhItemCategory` + `Rubezh::Items::IsLostOnDeath`; `URubezhCombatLogoutSubsystem` — запрет выхода 15 с после урона (только сервер).
- Ещё не связано с игрой: вес не замедляет движение, бег не тратит выносливость, выхода из игры нет.

## Командная работа (2+ человека, у каждого свой Claude)
- Перед работой — `git pull`; после каждого законченного куска — коммит и `git push` маленькими порциями.
- `.uasset`/`.umap` не сливаются: заранее договариваться, кто правит какой ассет. Пока сервер блокировок не используется, локально стоит `git config lfs.setlockablereadonly false` (иначе ассеты после коммита становятся read-only и редактор не может сохранить). Каждому участнику выполнить эту команду у себя.
- Пути `E:\Rubezh` и `E:\UE_5.8` прописаны здесь и в `.mcp.json`; при других путях поправить у себя.
- Скомпилированные бинарники не хранятся в git: после `git pull` с изменениями C++ — собрать редактор.
- Владелец проекта (лид-геймдизайнер) — новичок в UE: объяснять каждое действие простыми словами.
- Запуск игры без редактора — только из PowerShell/cmd: Git Bash портит путь карты `/RubezhCore/...`.
