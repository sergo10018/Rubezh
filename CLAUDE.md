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
- **Запусти и посмотри**: задача, меняющая то, что видит игрок (интерфейс, персонаж, карта, эффекты), не готова, пока игру не запустили и не посмотрели результат. Скриншот и короткий отчёт (`docs/templates/test-evidence.md`) — в `docs/qa/evidence/<задача>/`. Автотест и успешная сборка этого не заменяют. Не удалось запустить — прямо пиши «в игре не проверено» и не называй задачу готовой.

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
- Шаблоны документов — `docs/templates/` (баг, проверка в игре, плейтест, модель экономики, фракция, зона; что куда класть — в `README.md` там же). Новый такой документ начинать с шаблона, не дожидаясь просьбы.
- Реестр фактов — `docs/registry/facts.yaml`: числа и названия, которые встречаются в нескольких документах или в коде. Изменил такой факт (GDD, канон, `data/`, `*Rules.h`) — запусти скилл `consistency-check`; он же — перед проверкой gate. Новый общий факт — занести в реестр.
- После плейтеста — скилл `playtest-report`.

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
- Спринт `URubezhGameplayAbility_Sprint` на Shift (`InputTag.Ability.Dash`): ×1.5 скорости, тратит выносливость (`URubezhGE_SprintStaminaDrain`), не стартует ниже 15 ед. и при перегрузе; восстановление — `URubezhGE_StaminaRegen`. Числа — `Survival/RubezhStaminaRules.h`. Рывок Lyra убран: наш набор `AbilitySet_RubezhHero` = копия `AbilitySet_ShooterHero` без `GA_Hero_Dash`; у `IA_Ability_Dash` снят триггер Pressed (иначе «отпускание» приходит сразу).
- Инвентарь: предметы — `data/items.csv` → DataTable `/RubezhCore/Data/DT_Items` (строка `FRubezhItemDefinition`; после правки CSV — переимпорт в редакторе). Таблица прописана в `URubezhSettings` (`Config/DefaultGame.ini`). Ячейки `FRubezhInventoryGrid` (как в WoW: любой предмет — одна ячейка, одинаковые — стопкой до `MaxStack`; перенос на такой же предмет докладывает, на другой — меняет местами) — чистая логика с тестами; `URubezhInventoryComponent` на `LyraPlayerState` (добавляет `URubezhComponentInjector`, без правки ассетов), меняется только на сервере, виден только владельцу, пишет вес в `CarriedWeight`.
- Скорость считает только `Rubezh::Movement::RefreshWalkSpeed` (база × перегруз × спринт). При «неподъёмно» обнуляется и `MinAnalogWalkSpeed` (иначе персонаж идёт со скоростью 200; тег Lyra `Gameplay.MovementStopped` этого не лечит).
- Консоль для проверки (не в Shipping): `Rubezh.Give <id> [n]`, `Rubezh.Take <id> [n]`, `Rubezh.Inventory`, `Rubezh.Spawn <id> [n]` (положить на землю перед собой).
- Окно рюкзака (клавиша `I`, задаётся в `URubezhSettings::InventoryKey`): `URubezhInventoryWindow` — собрано кодом без ассетов виджетов, открывается на слое Lyra `UI.Layer.GameMenu`; ячейки рисует `URubezhSlotGridWidget` (по 6 в ряд), плитки `URubezhInventoryItemWidget` (ЛКМ — перетащить, ПКМ — в обыскиваемое и обратно, двойной клик — применить/надеть/забрать). Клавиши I и E подключает `URubezhPlayerInput` по событию Lyra `BindInputsNow` (Lyra очищает раскладки при каждом возрождении). Предмет, отпущенный за сеткой, выбрасывается (`ServerDropEntry`). Размеры и цвета — `Private/UI/RubezhInventoryStyle.h`. В PIE Esc останавливает игру, закрывать окно клавишей I.
- Предметы в мире: `ARubezhWorldItem` (стопка на земле; пока коробка, тем больше, чем тяжелее предмет, цвет по категории; можно ставить на карту, задав Item Id и Count). Подбор — `URubezhInteractionComponent` на `LyraCharacter` (добавляет инжектор): у своего игрока ищет предмет в радиусе 2 м по направлению взгляда, показывает подсказку под прицелом (`URubezhPickupPrompt`, «Нет места», если не влезает), по E шлёт `ServerPickUp`; сервер сам проверяет расстояние. Числа — `Items/RubezhPickupRules.h`.
- Травмы и лечение: `URubezhInjuryComponent` на `LyraCharacter`. Сервер ловит урон через `ULyraHealthSet::OnHealthChanged` и с шансом выдаёт травмы (кровотечение, сильное — от попадания ≥25 урона, перелом — попадание в ногу или падение >4 м, боль на 90 с). Кровотечение отнимает здоровье эффектом `URubezhGE_InjuryDamage` с тегом `Rubezh.Damage.Injury` (такой урон новых травм не даёт). Травмы зеркалятся тегами `Status.Injury.*` на ASC; спринт заблокирован при переломе и лечении. Лечение: двойной клик по медицине в рюкзаке или H (быстрое — сам выбирает жгут → бинт → шина → обезболивающее → аптечка); пока идёт, заблокированы стрельба, прицел, перезарядка, граната, удар, спринт; предмет тратится в конце. Что лечит предмет — столбцы `UseSeconds, HealPercent, DelayedHealPercent, DelayedHealSeconds, Cures, PainReliefSeconds` в `data/items.csv` (аптечка: `HealPercent` % от максимума сразу по окончании применения, ещё `DelayedHealPercent` % через `DelayedHealSeconds`; урон отложенное лечение не отменяет, смерть — отменяет); числа травм — `Health/RubezhInjuryRules.h`. Консоль: `Rubezh.Injure bleeding|heavy|fracture|pain|all`, `Rubezh.Hurt <урон>`.
- Снаряжение: слоты голова / бронежилет / одежда верх / низ / рюкзак (`ERubezhEquipSlot`), надетое хранит `URubezhInventoryComponent` (массив `Equipped`, порядок `Rubezh::Equipment::AllSlots`). Правила — чистая логика `Items/RubezhEquipment.h` с тестами: надеть/снять/сбросить, защита по частям тела (голова, корпус, руки, ноги; слои перемножаются), число ячеек от рюкзака (без него — карманы `PocketSlots` = 4), прибавка к лимиту веса (`MaxCarryWeight` = 30 + рюкзак), вес надетого входит в `CarriedWeight`. Характеристики — столбцы `EquipSlot, Protection, BleedProtection, Warmth, BackpackSlots, CarryBonusKg` в `data/items.csv`; стартовое снаряжение — `URubezhSettings::StartingEquipment`. Броня уменьшает урон через хук в Lyra: `ULyraDamageExecution::GetIncomingDamageMultiplier()` (единственная наша правка в `Source/LyraGame`), его привязывает модуль `RubezhCoreRuntime`; защита от кровотечения уменьшает шанс кровотечения в `RollHitInjuries`. В окне рюкзака слева колонка снаряжения: перетащить в слот или двойной клик — надеть, из слота в рюкзак или двойной клик — снять, за окно — бросить (рюкзак с вещами падает вместе с ними — контейнер, который можно обыскать или надеть обратно кнопкой «Надеть этот рюкзак»). Рюкзак меняется/снимается в рюкзак, только если вещи влезут. Консоль: `Rubezh.Equip <id>`, `Rubezh.Unequip head|armor|top|bottom|backpack`, `Rubezh.Shoot <урон> [head|torso|arms|legs]` (попадание через расчёт урона Lyra). 3D-моделей брони/одежды/рюкзаков в проекте нет — на земле это цветные коробки; тепло (`Warmth`) пока ни на что не влияет.
- ASC живёт на PlayerState и переживает персонажа: всё, что компонент персонажа ставит на ASC (теги, блокировки), он обязан снять в `EndPlay`.
- Обыск: `ARubezhLootContainer` — ящик на карте (поля Loot Table = military/medical/civil из `data/loot_tables.csv` → `/RubezhCore/Data/DT_LootTables`, строка `FRubezhLootTableRow`, прописана в `URubezhSettings::LootTable`; Slot Count; Search Seconds), тело или брошенный рюкзак. Содержимое знает только сервер; обыскивающему он присылает копию (`LootContainer`/`LootGrid` в `URubezhInventoryComponent`, только владельцу). E на ящик → `ServerStartSearch`: первый обыск — с задержкой (ящик 2 с, тело 3 с, рюкзак 1 с), повторный — сразу; окно рюкзака открывается само, справа — содержимое и «Забрать всё». Сервер проверяет расстояние на каждом переносе и закрывает обыск, если отошёл. Правила (таблицы лута, перенос, что теряется при смерти) — `Items/RubezhLootRules.h` с тестами; обыск — `RubezhInventoryComponent_Loot.cpp`. При смерти всё, что теряется (расходники, хлам, ценности), ложится «телом» (`Тело: <имя>`, 15 минут — `BodyLifetimeSeconds`), экипировка остаётся. Пустое тело исчезает, пустой брошенный рюкзак становится обычным предметом. На `L_Rubezh_Proto` у точки появления (6000, 0) стоят три ящика. Консоль: `Rubezh.Crate <таблица> [ячеек]`, `Rubezh.Search`, `Rubezh.TakeAll`, `Rubezh.DropBag`, `Rubezh.WearBag`.
- Ещё нет: выхода из игры, анимаций лечения и обыска, восстановления лута в ящиках. Скорость меняется без saved moves — на сервере с лагом возможны откаты (задача Этапа 1).

## Командная работа (2+ человека, у каждого свой Claude)
- Перед работой — `git pull`; после каждого законченного куска — коммит и `git push` маленькими порциями.
- `.uasset`/`.umap` не сливаются: заранее договариваться, кто правит какой ассет. Пока сервер блокировок не используется, локально стоит `git config lfs.setlockablereadonly false` (иначе ассеты после коммита становятся read-only и редактор не может сохранить). Каждому участнику выполнить эту команду у себя.
- Пути `E:\Rubezh` и `E:\UE_5.8` прописаны здесь и в `.mcp.json`; при других путях поправить у себя.
- Скомпилированные бинарники не хранятся в git: после `git pull` с изменениями C++ — собрать редактор.
- Владелец проекта (лид-геймдизайнер) — новичок в UE: объяснять каждое действие простыми словами.
- Запуск игры без редактора — только из PowerShell/cmd: Git Bash портит путь карты `/RubezhCore/...`.
