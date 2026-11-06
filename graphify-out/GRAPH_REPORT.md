# Graph Report - Rubezh  (2026-11-06)

## Corpus Check
- 25 files · ~53,740 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 617 nodes · 993 edges · 50 communities (37 shown, 13 thin omitted)
- Extraction: 86% EXTRACTED · 14% INFERRED · 0% AMBIGUOUS · INFERRED: 137 edges (avg confidence: 0.87)
- Token cost: 706,764 input · 0 output

## Community Hubs (Navigation)
- Скрипты ig-скиллов
- Instagram: контент и метрики
- Маркетинг и конвейер рилсов
- Процесс и этапы разработки
- Скиллы проверки и плейтестов
- Архитектура и компоненты кода
- Реестр фактов
- Детектор ИИ-текста
- Оценка хуков
- Журнал и вопросы канона
- Визуальные концепты рилсов
- Оружие и лечение (GDD)
- Подключение Unreal MCP
- Сеть и античит
- Жёсткие правила проекта
- Реализм мира и опасности
- Персонаж и экономика
- Смерть и выпадение лута
- UE: Lyra и GAS
- Снимок: выпадение патронов
- Дизайн уровней и столпы
- Core loop и прототип
- Контекст UE-проекта
- Экономика: стоки и крафт
- Лор и варианты сценария
- Инструменты и C++/Blueprint
- Петли игры и критерии
- Снимок: окно оружия
- Реестр, шаблоны, правила
- Инвентарь и вес
- Снимок: выпадение стопок
- Сервер и GAS: решения
- Агенты и ревью
- Агент геймдизайна
- Скиллы UE (quodsoler)
- История мира
- Незакрытое в прототипе
- Скорость и спринт
- Рилс про противогаз
- Съёмка: правила
- Мастер-дубль
- Запрет выхода из боя
- Источник ig-скиллов
- Ассеты
- Этап 5: бета
- Рилс про классы
- Рилс про Синдикат
- Стиль концепт-арта
- Озвучка

## God Nodes (most connected - your core abstractions)
1. `Канон мира (единственный источник правды)` - 23 edges
2. `ig-reel: сценарий Reel из идеи` - 19 edges
3. `Процесс, инструменты и план разработки` - 16 edges
4. `ig-plan: недельный контент-план` - 13 edges
5. `ig-human: очистка текста от признаков ИИ` - 13 edges
6. `Агент economy-designer` - 12 edges
7. `ig-viral: поиск вирусных Reels и swipe-файл` - 12 edges
8. `Публикует только человек (скиллы не постят)` - 11 edges
9. `Жёсткие правила проекта` - 11 edges
10. `Системы прототипа (Этап 2, «серый бокс»)` - 11 edges

## Surprising Connections (you probably didn't know these)
- `Правило: мир реалистичный` --semantically_similar_to--> `Столп 1: Реалистичный мир`  [INFERRED] [semantically similar]
  CLAUDE.md → docs/01_Канон.md
- `Сервер-авторитетность` --semantically_similar_to--> `Dedicated Server + replication/Iris, серверная авторитетность`  [INFERRED] [semantically similar]
  CLAUDE.md → docs/00_Процесс_и_план_разработки.md
- `Крафтовые цепочки (ресурсы минимум из двух источников)` --semantically_similar_to--> `Защита от монополии`  [INFERRED] [semantically similar]
  .claude/agents/economy-designer.md → docs/01_Канон.md
- `Рилс №6 «Противогаз не спасёт от всего»` --semantically_similar_to--> `Техногенные и природные опасности (канон §3.2)`  [INFERRED] [semantically similar]
  docs/marketing/02_Сценарии_рилсов.md → marketing/reels/_screens/canon.txt
- `Агент network-programmer` --conceptually_related_to--> `Секреты только в переменных окружения`  [INFERRED]
  .claude/agents/network-programmer.md → CLAUDE.md

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **Цикл доказательств: viral -> swipe.md -> plan/reel -> log.md -> audit** — claude_skills_ig_viral_skill_ig_viral, claude_instagram_swipe, claude_skills_ig_plan_skill_ig_plan, claude_skills_ig_reel_skill_ig_reel, claude_instagram_log, claude_skills_ig_audit_skill_ig_audit [EXTRACTED 1.00]
- **Система 26 формул хуков** — claude_skills_ig_reel_hooks, claude_skills_ig_reel_hookscore, claude_skills_ig_viral_swipe, claude_skills_ig_reel_skill_ig_reel, claude_skills_ig_viral_skill_ig_viral, claude_skills_ig_reply_skill_reply_with_reel [EXTRACTED 1.00]
- **Все тексты проходят через /ig-human перед показом** — claude_skills_ig_human_skill_ig_human, claude_skills_ig_caption_skill_ig_caption, claude_skills_ig_reel_skill_ig_reel, claude_skills_ig_comment_skill_ig_comment, claude_skills_ig_dm_skill_ig_dm, claude_skills_ig_profile_skill_ig_profile, claude_skills_ig_carousel_skill_ig_carousel [EXTRACTED 1.00]
- **Кандидаты визуального стиля A/B/C и выбор C v2** — docs_marketing_05_etalony_stilya_stil_a_kinofoto, docs_marketing_05_etalony_stilya_stil_b_kontsept_art, docs_marketing_05_etalony_stilya_stil_c_graficheskiy_roman, docs_marketing_05_etalony_stilya_stil_c_v2, docs_marketing_voice_vizual [EXTRACTED 1.00]
- **Варианты визуального стиля маркетинга: фотореализм / концепт-арт / графический роман** — marketing_style_v1_a1_station_style_photoreal, marketing_style_v1_b1_station_style_concept_art, marketing_style_v1_c1_station_style_graphic_novel [INFERRED 0.85]
- **Конвейер производства рилса: сценарий → тайминг → съёмка → монтаж** — docs_marketing_01_strategiya_prodvizheniya_konveyer_rilsov, tools_reels_beats_ru, tools_reels_make_reel, tools_reels_readme_edit_json, docs_marketing_03_shot_list_rilsy_1_3_master_dubl, docs_marketing_01_strategiya_prodvizheniya_instagram_agent_skill [INFERRED 0.85]
- **Рилсы недели 1: крючок позиционирования → сценарий → script.txt** — docs_marketing_02_stsenarii_rilsov_rils_01_pyatero_protiv_mmo, docs_marketing_02_stsenarii_rilsov_rils_02_bez_mutantov, docs_marketing_02_stsenarii_rilsov_rils_03_glavnyy_lut_zavod, marketing_reels_01_pyatero_script, marketing_reels_02_bez_mutantov_script, marketing_reels_03_zavod_script, docs_marketing_03_shot_list_rilsy_1_3 [INFERRED 0.85]
- **Сцена «Противогаз» в трёх стилевых вариантах A/B/C** — marketing_style_v1_a2_mask_image, marketing_style_v1_b2_mask_image, marketing_style_v1_c2_mask_image, marketing_style_v1_a2_mask_gas_mask_repair [INFERRED 0.95]
- **Сцена «Станция» в трёх стилевых вариантах A/B/C** — marketing_style_v1_a1_station_image, marketing_style_v1_b1_station_image, marketing_style_v1_c1_station_image, marketing_style_v1_a1_station_station [INFERRED 0.95]
- **Столпы игры** — docs_01_kanon_pillar_realistic_world, docs_01_kanon_pillar_player_economy, docs_01_kanon_pillar_meaningful_territory, docs_01_kanon_pillar_risk_consequences [EXTRACTED 1.00]
- **Четыре фракции мира** — docs_01_kanon_military_protectorate, docs_01_kanon_free_settlements, docs_01_kanon_syndicate, docs_01_kanon_independent_expeditions [EXTRACTED 1.00]
- **Этапы плана разработки с gate-проверками** — docs_00_protsess_i_plan_razrabotki_stage_0_preparation, docs_00_protsess_i_plan_razrabotki_stage_1_preproduction, docs_00_protsess_i_plan_razrabotki_stage_2_graybox_prototype, docs_00_protsess_i_plan_razrabotki_stage_3_vertical_slice, docs_00_protsess_i_plan_razrabotki_stage_4_alpha, docs_00_protsess_i_plan_razrabotki_stage_5_beta, docs_00_protsess_i_plan_razrabotki_stage_6_early_access_liveops [EXTRACTED 1.00]
- **Правила потери лута при смерти** — docs_gdd_00_core_loop_poterya_po_tipu, docs_gdd_00_core_loop_shans_30, docs_gdd_00_core_loop_poterya_patronov, docs_gdd_00_core_loop_telo, docs_gdd_01_kategoriya_predmeta [EXTRACTED 1.00]
- **Проверка выпадения патронов и стопок автотестами** — docs_qa_evidence_death_ammo_loss_report_lostondeath, docs_qa_evidence_death_ammo_loss_report_ammolossamount, docs_qa_evidence_death_ammo_loss_report_ammolosstake [EXTRACTED 1.00]
- **Связь Claude с редактором Unreal через MCP** — mcp, docs_tech_unreal_mcp_setup_proxy, docs_tech_unreal_mcp_setup_enginewire, docs_tech_unreal_mcp_setup_capability_token [EXTRACTED 1.00]
- **Фракции мира в таблице отношений** — docs_01_kanon_military_protectorate, docs_01_kanon_free_settlements, docs_01_kanon_syndicate, docs_01_kanon_independent_expeditions [EXTRACTED 1.00]
- **Набор шаблонов документов проекта** — docs_templates_bug_report, docs_templates_test_evidence, docs_templates_playtest_report, docs_templates_economy_model, docs_templates_faction_design, docs_templates_level_design_document [EXTRACTED 1.00]
- **Проверка вылазки: замысел зоны, признаки вылазки и критерий успеха** — docs_templates_level_design_document_etalonnyy_den, docs_templates_playtest_report_pyat_priznakov, docs_templates_playtest_report_kriteriy_uspekha [INFERRED 0.85]

## Communities (50 total, 13 thin omitted)

### Community 0 - "Скрипты ig-скиллов"
Cohesion: 0.07
Nodes (32): analyse(), main(), render(), render_box(), visible_window(), _cp(), humanize(), load_lexicon() (+24 more)

### Community 1 - "Instagram: контент и метрики"
Cohesion: 0.06
Nodes (46): ~/.claude/instagram/log.md (журнал постов и формул), ~/.claude/instagram/plan.md (недельный план), ~/.claude/instagram/swipe.md (swipe-файл), ~/.claude/instagram/voice.md (профиль голоса), Hold at 3s (оценка хука), ig-audit: пост-мортем опубликованного контента, Sends per reach (сильнейший сигнал), Первые 125 символов до «... more» (+38 more)

### Community 2 - "Маркетинг и конвейер рилсов"
Cohesion: 0.06
Nodes (38): Аудитория: ядро, геймдев-зрители, ИИ-любопытные, Безопасная зона Reels 1080×1920, instagram-agent-skill (.claude/skills/ig-*), Конвейер производства рилсов, Метрики роста (удержание 3 с, пересылки, подписки), Ритм недели (Фаза А), 26 формул хуков (ig-reel/hooks.json), Анимация картинок (Ken Burns, панорама, параллакс, частицы) (+30 more)

### Community 3 - "Процесс и этапы разработки"
Cohesion: 0.06
Nodes (44): Агент producer, Проверка gate с доказательствами, Декомпозиция этапов на задачи 0.5–3 дня, Конвейер: Концепт → Пре-продакшн → Прототип → Вертикальный срез → Альфа → Бета → Live-ops, Процесс, инструменты и план разработки, Gate-проверки между этапами, Главные риски, Распределение ролей команды (5 человек + Claude) (+36 more)

### Community 4 - "Скиллы проверки и плейтестов"
Cohesion: 0.07
Nodes (34): Скилл consistency-check (проверка согласованности), Реестр фактов docs/registry/facts.yaml, Скилл playtest-report (отчёт о плейтесте), Разбор заметок плейтеста и раскладка находок, Цель: не меньше 70 % (7 из 10) хотят ещё одну вылазку, Источник заимствования: Claude Code Game Studios (коммит 05ad7c8), Вольные поселения, Главный конфликт: четыре взгляда на будущее (+26 more)

### Community 5 - "Архитектура и компоненты кода"
Cohesion: 0.09
Nodes (26): Бэкенд Nakama + PostgreSQL, Архитектура: клиент → сервер зоны → Nakama → PostgreSQL, Данные в data/ (CSV/JSON для DataTable), Снаряжение: ERubezhEquipSlot и правила Items/RubezhEquipment.h, URubezhInjuryComponent: травмы и лечение, URubezhInteractionComponent: подбор по E, URubezhInventoryComponent на LyraPlayerState, FRubezhInventoryGrid: ячейки и стопки как в WoW (+18 more)

### Community 6 - "Реестр фактов"
Cohesion: 0.07
Nodes (27): factions, added, name, source, status, items, added, id (+19 more)

### Community 7 - "Детектор ИИ-текста"
Cohesion: 0.16
Nodes (15): bar(), check_burstiness(), check_fingerprint(), check_slop(), check_specificity(), check_voice(), clamp(), main() (+7 more)

### Community 8 - "Оценка хуков"
Cohesion: 0.22
Nodes (12): bar(), check_address(), check_frontload(), check_length(), check_specificity(), check_stakes(), clamp(), main() (+4 more)

### Community 9 - "Журнал и вопросы канона"
Cohesion: 0.12
Nodes (13): Расширение канона через журнал изменений, Ещё нет: падение пули, компенсация лага, ремонт, выход из игры, Этап 1. Пре-продакшн, Журнал изменений канона, Правила смерти: частичная потеря по типу предмета, Открытые вопросы канона, Тон: суровый, приземлённый, без героизма, Канон мира (единственный источник правды) (+5 more)

### Community 10 - "Визуальные концепты рилсов"
Cohesion: 0.17
Nodes (16): Фон: заброшенный заводской цех со ржавыми станками, светом сквозь крышу и дымкой, Кандидаты шрифтов: Bahnschrift, Arial Narrow, Franklin Gothic, Century Gothic, Impact, Consolas, Calibri, Segoe UI, Сравнение шрифтов субтитров для Reels: 8 вариантов на кадре заброшенного цеха, Слоган «Самое ценное в этом мире — не оружие.», Вооружённые охранники в балаклавах у сетчатых ворот (АК, бочка с огнём), A1: Станция (фотореализм) — кирпичная насосная станция с водонапорной башней, охрана у ворот, снег, сумерки, Станция-инфраструктура (кирпичное здание, водонапорная башня, дым из труб), Стиль A: кинематографичный фотореализм (холодный сине-серый тон, тёплые окна, огонь в бочке) (+8 more)

### Community 11 - "Оружие и лечение (GDD)"
Cohesion: 0.19
Nodes (12): Стоки денег вместо страховки, Аптечки: лечение сразу и через 5 с, Износ оружия и осечки, Патроны-предметы и калибры, Слоты оружия (два основных и пистолет), Стрельба, Спад урона на дистанции, Зоны попадания (голова ×2.5, торс ×1.0, конечности ×0.7) (+4 more)

### Community 12 - "Подключение Unreal MCP"
Cohesion: 0.18
Nodes (9): Подключение Claude к редактору Unreal (MCP), Плагин EngineWire 0.6.0, Прокси unreal-engine-mcp-server, ChiR24/Unreal_mcp (MIT), UE_PROJECT_PATH, npx, uvx, blender (+1 more)

### Community 13 - "Сеть и античит"
Cohesion: 0.18
Nodes (8): Защита от эксплойтов (дюп, циклы, альты), Агент network-programmer, Клиентское предсказание + серверная проверка попаданий (rewind), Бюджет: 30 Гц тикрейт на 64 игрока, Агент qa-lead, Автотесты Automation/CQTest для логики без визуала, Шаблон баг-репорта, Тест-план системы docs/qa/<система>.md

### Community 14 - "Жёсткие правила проекта"
Cohesion: 0.22
Nodes (8): Текущий этап: Этап 0 «Подготовка», git config lfs.setlockablereadonly false, docs/marketing/ — продвижение (Reels/Shorts), скиллы ig-*, РУБЕЖ: Земля после разлома (CLAUDE.md проекта), Командная работа (2+ человека, у каждого свой Claude), Решения, которые нужны сейчас, Этап 0. Подготовка, VCS: Perforce Helix Core или Git + LFS

### Community 15 - "Реализм мира и опасности"
Cohesion: 0.22
Nodes (9): Этап 2. Прототип «серый бокс», Этап 6. Ранний доступ и Live-ops, Глава IV «Закрытые архивы», Одичавшая фауна (единственный тип существ), Независимые экспедиции, Регион II. Затопленные земли, Регион III. Выжженный пояс, Сюжет по главам (I–V) (+1 more)

### Community 16 - "Персонаж и экономика"
Cohesion: 0.18
Nodes (10): Этап 4. Альфа, Характеристики: Сила, Ловкость, Выносливость, Интеллект, Предыстории (не классы), Климат и экипировка, Ценности исследования (зачем покидать поселение), Профессии и экономика, Регион I. Пограничье, Регион IV. Северные территории (+2 more)

### Community 17 - "Смерть и выпадение лута"
Cohesion: 0.25
Nodes (10): Выпадение 5–7 % патронов из рюкзака, Шанс выпадения стопки 30 %, Тело с выпавшим лутом (15 минут), Лут и обыск (data/loot_tables.csv), Магазины как предметы, Проверка в игре: что выпадает при смерти, Автотест Rubezh.Loot.AmmoLossAmount, Автотест Rubezh.Loot.AmmoLossTake (+2 more)

### Community 18 - "UE: Lyra и GAS"
Cohesion: 0.27
Nodes (9): Агент unreal-specialist, GAS для всех характеристик и состояний (Gameplay Tags), Проект Game/Rubezh.uproject на основе Lyra (UE 5.8.4), Game Feature плагин RubezhCore (модуль RubezhCoreRuntime), Префикс кода Rubezh, URubezhSurvivalSet: выносливость и вес (GAS), Gameplay Ability System (GAS), Lyra Starter Game (+1 more)

### Community 19 - "Снимок: выпадение патронов"
Cohesion: 0.28
Nodes (9): Сетка «Вещмешок · 3/12», Контейнер «Тело» с кнопкой «Забрать всё», Колонка «Снаряжение», Снаряжение остаётся после смерти, Окно рюкзака в три колонки, Частичная потеря патронов при смерти, Сводка защиты, тепла и лимита веса, Скриншот: обыск своего тела после смерти (PIE) (+1 more)

### Community 20 - "Дизайн уровней и столпы"
Cohesion: 0.32
Nodes (4): Агент level-designer, Документ локации docs/levels/<зона>.md, Эталонный игровой день, Территории и войны гильдий

### Community 21 - "Core loop и прототип"
Cohesion: 0.29
Nodes (7): Core loop — «Вылазка», Канон (docs/01_Канон.md, столпы и эталонный день), Запрет выхода 15 с после урона, Системы прототипа (Этап 2, «серый бокс»), ИИ-животные: собаки и кабан, Камера: третье лицо, прицеливание от первого, Погода: день/ночь и дождь

### Community 22 - "Контекст UE-проекта"
Cohesion: 0.29
Nodes (7): Опыт B_Rubezh_Proto, Карта L_Rubezh_Proto, Модуль LyraGame, Операции EngineWire только для /Game и /Engine, Game Feature плагин RubezhCore, Модуль RubezhCoreRuntime, ULyraRangedWeaponInstance и ULyraGameplayAbility_RangedWeapon

### Community 23 - "Экономика: стоки и крафт"
Cohesion: 0.29
Nodes (5): Агент economy-designer, Симуляции экономики tools/econ_sim/ (Python), docs/gdd/economy.md, Карта потоков: источники и стоки, Лут-таблицы с явными вероятностями

### Community 24 - "Лор и варианты сценария"
Cohesion: 0.29
Nodes (6): Агент lore-keeper, Проверка на соответствие канону, Формат квеста (заказчик, мотивация, исходы, награда), Вариант 1 сценария (полная концепция), Вариант 2 сценария (только лор, без фантастики), Удалено: мутанты, лабораторные организмы, «Глубинные»

### Community 25 - "Инструменты и C++/Blueprint"
Cohesion: 0.29
Nodes (5): Плагин EngineWire (Unreal MCP), ChiR24/Unreal_mcp, Ограничения Claude (бинарные ассеты, нет арта, не чувствует игру), Отклонённые ресурсы (chongdashu, flopperam, dstn2000), Рабочий процесс: бэклог → Claude → человек → ревью → плейтест

### Community 26 - "Петли игры и критерии"
Cohesion: 0.29
Nodes (5): Критерии плейтеста вылазки (≥ 7 из 10), Навыки растут от действия, Петля профессии, Петля территории, Выделенный сервер прототипа на 16 игроков

### Community 27 - "Снимок: окно оружия"
Cohesion: 0.38
Nodes (7): HUD Lyra под окном: быстрый доступ, счётчик патронов, здоровье, счёт и таймер матча, Колонка «Снаряжение»: слоты оружия 1–3, голова, бронежилет, одежда, рюкзак, Магазины ПМ 9×18 как предметы с показом патронов (8/8, 5/8), Скриншот: окно рюкзака с оружием в PIE, Дробовик помповый 12 к. в слоте 1: 6 патронов, прочность 100 %, Сетка «Вещмешок · 4/12» с подсказкой управления и полосой веса, Сводка защиты по частям тела, тепло и лимит веса

### Community 28 - "Реестр, шаблоны, правила"
Cohesion: 0.33
Nodes (5): Скилл consistency-check, Шаблоны документов docs/templates/, Реестр фактов docs/registry/facts.yaml, Скилл playtest-report, Что взято из Game Studios: правила, шаблоны, реестр фактов

### Community 29 - "Инвентарь и вес"
Cohesion: 0.40
Nodes (5): Категория предмета, Вес и уровни перегруза (лимит 30 кг), URubezhSurvivalSet, Слоты экипировки, Выносливость

### Community 30 - "Снимок: выпадение стопок"
Cohesion: 0.47
Nodes (6): Сетка «Вещмешок» 3/12 с магазином, бинтом и металлоломом, Контейнер «Тело» с тушёнкой и кнопкой «Забрать всё», Колонка «Снаряжение»: слоты оружия и одежды, защита и тепло, Окно рюкзака в три колонки (снаряжение, вещмешок, обыск), Частичная потеря стопок при смерти, Скриншот проверки: стопки на теле после смерти

### Community 31 - "Сервер и GAS: решения"
Cohesion: 0.40
Nodes (4): UE Project Context, ULyraAbilitySystemComponent на PlayerState, Iris (включение не решено), Выделенный сервер на зону (80–150 игроков)

### Community 32 - "Агенты и ревью"
Cohesion: 0.40
Nodes (5): Готовность задачи: компиляция Editor+Server, автотест, инструкция, ревью, Ревьюеры ECC: cpp-reviewer, security-reviewer, database-reviewer, Агенты проекта (.claude/agents/), Donchitos/Claude-Code-Game-Studios, Компактный состав из 8 агентов

### Community 34 - "Скиллы UE (quodsoler)"
Cohesion: 0.50
Nodes (4): Коммит quodsoler/unreal-engine-skills f3742d7, .agents/ue-project-context.md, Скиллы ue-* из quodsoler/unreal-engine-skills, quodsoler/unreal-engine-skills

### Community 35 - "История мира"
Cohesion: 0.50
Nodes (4): Тёмные годы и годы становления, Великий разлом (Год Разлома), Работающая инфраструктура — главная ценность мира, Мир через 25 лет после Разлома

### Community 36 - "Незакрытое в прототипе"
Cohesion: 0.67
Nodes (4): Баллистика и компенсация лага (rewind ≤ 200 мс), Definition of Done прототипа, Не проверено: перетаскивание, настоящий клиент, надпись «Осечка», Python в сетевом PIE вешает редактор

### Community 37 - "Скорость и спринт"
Cohesion: 1.00
Nodes (3): Rubezh::Encumbrance: уровни перегруза, Rubezh::Movement::RefreshWalkSpeed: база × перегруз × спринт, Спринт URubezhGameplayAbility_Sprint

## Ambiguous Edges - Review These
- `Баллистика и компенсация лага (rewind ≤ 200 мс)` → `Не проверено: перетаскивание, настоящий клиент, надпись «Осечка»`  [AMBIGUOUS]
  docs/qa/evidence/weapons/report.md · relation: conceptually_related_to

## Knowledge Gaps
- **120 isolated node(s):** `Документ локации docs/levels/<зона>.md`, `Проверка на соответствие канону`, `Формат квеста (заказчик, мотивация, исходы, награда)`, `Рилс №10 «Итоги опроса»`, `Общий стиль промпта (35mm, холодный грейд)` (+115 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 205 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **13 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Сценарии рилсов, Фаза А (12 роликов)` connect `Процесс и этапы разработки` to `Журнал и вопросы канона`, `Маркетинг и конвейер рилсов`?**
  _High betweenness centrality (0.296) - this node is a cross-community bridge._
- **Are the 3 inferred relationships involving `Канон мира (единственный источник правды)` (e.g. with `«Постапокалипсис без мутантов»` and `Канон-фильтр генераций`) actually correct?**
  _`Канон мира (единственный источник правды)` has 3 INFERRED edges - model-reasoned connections that need verification._
- **What connects `Документ локации docs/levels/<зона>.md`, `Проверка на соответствие канону`, `Формат квеста (заказчик, мотивация, исходы, награда)` to the rest of the system?**
  _120 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Скрипты ig-скиллов` be split into smaller, more focused modules?**
  _Cohesion score 0.06838106370543542 - nodes in this community are weakly interconnected._
- **Why does `Процесс, инструменты и план разработки` connect `Процесс и этапы разработки` to `Архитектура и компоненты кода`, `Сеть и античит`, `Жёсткие правила проекта`?**
  _High betweenness centrality (0.240) - this node is a cross-community bridge._
- **Should `Instagram: контент и метрики` be split into smaller, more focused modules?**
  _Cohesion score 0.06233766233766234 - nodes in this community are weakly interconnected._

### Low-confidence Hints
_AMBIGUOUS edges — the extractor was unsure. Verify before acting on these._

- **What is the exact relationship between `Баллистика и компенсация лага (rewind ≤ 200 мс)` and `Не проверено: перетаскивание, настоящий клиент, надпись «Осечка»`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._