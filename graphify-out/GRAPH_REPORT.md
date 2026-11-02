# Graph Report - MMORPG (мини разрабы)  (2026-11-01)

## Corpus Check
- Corpus is ~43,909 words - fits in a single context window. You may not need a graph.

## Summary
- 419 nodes · 805 edges · 20 communities (12 shown, 8 thin omitted)
- Extraction: 84% EXTRACTED · 16% INFERRED · 0% AMBIGUOUS · INFERRED: 129 edges (avg confidence: 0.88)
- Token cost: 500,605 input · 0 output

## Community Hubs (Navigation)
- Линтер подписей (caption.py)
- Канон, уровни и квесты
- Экономика и геймдизайн (агенты)
- Аналитика Instagram и данные
- Архитектура, стек и план этапов
- Сборка рилсов и стиль C
- Подписи, карусели, хуманайзер
- Стратегия продвижения
- Фазы продвижения и сценарии
- Оценка хуков (hookscore)
- Эталоны стиля (изображения)
- Unreal MCP конфигурация
- Рилс «Противогаз» и опасности
- Правила съёмки
- Мастер-дубль и нарезка
- Рилс «Нет классов»
- Рилс «Синдикат»
- Стиль B концепт-арт
- Озвучка Chatterbox

## God Nodes (most connected - your core abstractions)
1. `Канон мира (единственный источник правды)` - 52 edges
2. `Процесс, инструменты и план разработки` - 42 edges
3. `РУБЕЖ: Земля после разлома (CLAUDE.md проекта)` - 24 edges
4. `ig-reel: сценарий Reel из идеи` - 19 edges
5. `Агент economy-designer` - 14 edges
6. `ig-human: очистка текста от признаков ИИ` - 13 edges
7. `ig-plan: недельный контент-план` - 13 edges
8. `ig-viral: поиск вирусных Reels и swipe-файл` - 12 edges
9. `Агент game-designer` - 11 edges
10. `Агент level-designer` - 11 edges

## Surprising Connections (you probably didn't know these)
- `Крафтовые цепочки (ресурсы минимум из двух источников)` --semantically_similar_to--> `Защита от монополии`  [INFERRED] [semantically similar]
  .claude/agents/economy-designer.md → docs/01_Канон.md
- `Нет универсального персонажа: у каждого преимущества цена` --semantically_similar_to--> `Фундаментальные принципы (чек-лист)`  [INFERRED] [semantically similar]
  .claude/agents/game-designer.md → docs/01_Канон.md
- `Рилс №6 «Противогаз не спасёт от всего»` --semantically_similar_to--> `Техногенные и природные опасности (канон §3.2)`  [INFERRED] [semantically similar]
  docs/marketing/02_Сценарии_рилсов.md → marketing/reels/_screens/canon.txt
- `РУБЕЖ: Земля после разлома (CLAUDE.md проекта)` --references--> `Коммит quodsoler/unreal-engine-skills f3742d7`  [EXTRACTED]
  CLAUDE.md → .claude/skills/SOURCE-quodsoler.txt
- `Жёсткие правила проекта` --conceptually_related_to--> `Dedicated Server + replication/Iris, серверная авторитетность`  [INFERRED]
  CLAUDE.md → docs/00_Процесс_и_план_разработки.md

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **Четыре столпа игры** — docs_01_kanon_pillar_realistic_world, docs_01_kanon_pillar_player_economy, docs_01_kanon_pillar_meaningful_territory, docs_01_kanon_pillar_risk_consequences [EXTRACTED 1.00]
- **Четыре фракции главного конфликта** — docs_01_kanon_military_protectorate, docs_01_kanon_free_settlements, docs_01_kanon_syndicate, docs_01_kanon_independent_expeditions, docs_01_kanon_main_conflict [EXTRACTED 1.00]
- **8 агентов проекта** — claude_agents_economy_designer_agent, claude_agents_game_designer_agent, claude_agents_level_designer_agent, claude_agents_lore_keeper_agent, claude_agents_network_programmer_agent, claude_agents_producer_agent, claude_agents_qa_lead_agent, claude_agents_unreal_specialist_agent [EXTRACTED 1.00]
- **Конвейер производства рилса: сценарий → тайминг → съёмка → монтаж** — docs_marketing_01_strategiya_prodvizheniya_konveyer_rilsov, tools_reels_beats_ru, tools_reels_make_reel, tools_reels_readme_edit_json, docs_marketing_03_shot_list_rilsy_1_3_master_dubl, docs_marketing_01_strategiya_prodvizheniya_instagram_agent_skill [INFERRED 0.85]
- **Рилсы недели 1: крючок позиционирования → сценарий → script.txt** — docs_marketing_02_stsenarii_rilsov_rils_01_pyatero_protiv_mmo, docs_marketing_02_stsenarii_rilsov_rils_02_bez_mutantov, docs_marketing_02_stsenarii_rilsov_rils_03_glavnyy_lut_zavod, marketing_reels_01_pyatero_script, marketing_reels_02_bez_mutantov_script, marketing_reels_03_zavod_script, docs_marketing_03_shot_list_rilsy_1_3 [INFERRED 0.85]
- **Кандидаты визуального стиля A/B/C и выбор C v2** — docs_marketing_05_etalony_stilya_stil_a_kinofoto, docs_marketing_05_etalony_stilya_stil_b_kontsept_art, docs_marketing_05_etalony_stilya_stil_c_graficheskiy_roman, docs_marketing_05_etalony_stilya_stil_c_v2, docs_marketing_voice_vizual [EXTRACTED 1.00]
- **Все тексты проходят через /ig-human перед показом** — claude_skills_ig_human_skill_ig_human, claude_skills_ig_caption_skill_ig_caption, claude_skills_ig_reel_skill_ig_reel, claude_skills_ig_comment_skill_ig_comment, claude_skills_ig_dm_skill_ig_dm, claude_skills_ig_profile_skill_ig_profile, claude_skills_ig_carousel_skill_ig_carousel [EXTRACTED 1.00]
- **Цикл доказательств: viral -> swipe.md -> plan/reel -> log.md -> audit** — claude_skills_ig_viral_skill_ig_viral, claude_instagram_swipe, claude_skills_ig_plan_skill_ig_plan, claude_skills_ig_reel_skill_ig_reel, claude_instagram_log, claude_skills_ig_audit_skill_ig_audit [EXTRACTED 1.00]
- **Система 26 формул хуков** — claude_skills_ig_reel_hooks, claude_skills_ig_reel_hookscore, claude_skills_ig_viral_swipe, claude_skills_ig_reel_skill_ig_reel, claude_skills_ig_viral_skill_ig_viral, claude_skills_ig_reply_skill_reply_with_reel [EXTRACTED 1.00]
- **Сцена «Станция» в трёх стилевых вариантах A/B/C** — marketing_style_v1_a1_station_image, marketing_style_v1_b1_station_image, marketing_style_v1_c1_station_image, marketing_style_v1_a1_station_station [INFERRED 0.95]
- **Сцена «Противогаз» в трёх стилевых вариантах A/B/C** — marketing_style_v1_a2_mask_image, marketing_style_v1_b2_mask_image, marketing_style_v1_c2_mask_image, marketing_style_v1_a2_mask_gas_mask_repair [INFERRED 0.95]
- **Варианты визуального стиля маркетинга: фотореализм / концепт-арт / графический роман** — marketing_style_v1_a1_station_style_photoreal, marketing_style_v1_b1_station_style_concept_art, marketing_style_v1_c1_station_style_graphic_novel [INFERRED 0.85]

## Communities (20 total, 8 thin omitted)

### Community 0 - "Линтер подписей (caption.py)"
Cohesion: 0.07
Nodes (32): analyse(), main(), render(), render_box(), visible_window(), _cp(), humanize(), load_lexicon() (+24 more)

### Community 1 - "Канон, уровни и квесты"
Cohesion: 0.07
Nodes (40): Агент level-designer, Документ локации docs/levels/<зона>.md, Агент lore-keeper, Проверка на соответствие канону, Расширение канона через журнал изменений, Формат квеста (заказчик, мотивация, исходы, награда), Вариант 1 сценария (полная концепция), Журнал изменений канона (+32 more)

### Community 2 - "Экономика и геймдизайн (агенты)"
Cohesion: 0.07
Nodes (35): Агент economy-designer, Симуляции экономики tools/econ_sim/ (Python), docs/gdd/economy.md, Защита от эксплойтов (дюп, циклы, альты), Карта потоков: источники и стоки, Лут-таблицы с явными вероятностями, Агент game-designer, Шаблон раздела GDD (цель, правила, связи, параметры, эталонный день, MVP) (+27 more)

### Community 3 - "Аналитика Instagram и данные"
Cohesion: 0.07
Nodes (37): ~/.claude/instagram/log.md (журнал постов и формул), ~/.claude/instagram/plan.md (недельный план), ~/.claude/instagram/swipe.md (swipe-файл), ~/.claude/instagram/voice.md (профиль голоса), Hold at 3s (оценка хука), ig-audit: пост-мортем опубликованного контента, Sends per reach (сильнейший сигнал), Отступ 120px и формат 1080x1350 (+29 more)

### Community 4 - "Архитектура, стек и план этапов"
Cohesion: 0.08
Nodes (37): Архитектура: клиент → сервер зоны → Nakama → PostgreSQL, Агент producer, Проверка gate с доказательствами, Декомпозиция этапов на задачи 0.5–3 дня, GAS для всех характеристик и состояний (Gameplay Tags), Текущий этап: Этап 0 «Подготовка», Коммит quodsoler/unreal-engine-skills f3742d7, Ассеты: Fab, Megascans, MetaHuman, Mixamo (+29 more)

### Community 5 - "Сборка рилсов и стиль C"
Cohesion: 0.10
Nodes (23): Анимация картинок (Ken Burns, панорама, параллакс, частицы), Стиль C · графический роман, Стиль C v2 (выбран продюсером), Визуал: стиль C, Montserrat Bold, Pop 0,2 с, Лицензия шрифта Montserrat (OFL), Шрифт Montserrat, SIL Open Font License 1.1, build() (+15 more)

### Community 6 - "Подписи, карусели, хуманайзер"
Cohesion: 0.10
Nodes (24): Первые 125 символов до «... more», Лимит 5 хэштегов (с 18.12.2025), ig-caption: подпись к посту Instagram, Поисковые фразы в тексте подписи, ig-carousel: карусель 1080x1350, Структура карусели: обложка, ставка, слайды, recap, CTA, bar(), check_burstiness() (+16 more)

### Community 7 - "Стратегия продвижения"
Cohesion: 0.09
Nodes (23): Стратегия продвижения разработки (Reels/Shorts), Аудитория: ядро, геймдев-зрители, ИИ-любопытные, Безопасная зона Reels 1080×1920, instagram-agent-skill (.claude/skills/ig-*), Конвейер производства рилсов, Метрики роста (удержание 3 с, пересылки, подписки), Площадки: Instagram Reels, YouTube Shorts, Telegram/Discord, Ритм недели (Фаза А) (+15 more)

### Community 8 - "Фазы продвижения и сценарии"
Cohesion: 0.10
Nodes (24): Фаза А «Нулевой километр», Фаза Б «Серый бокс», Фаза Г «Альфа», Фаза В «Вертикальный срез», «Главный лут — инфраструктура», «Постапокалипсис без мутантов», «Пятеро против жанра», Рубрики R1–R9 (+16 more)

### Community 9 - "Оценка хуков (hookscore)"
Cohesion: 0.22
Nodes (12): bar(), check_address(), check_frontload(), check_length(), check_specificity(), check_stakes(), clamp(), main() (+4 more)

### Community 10 - "Эталоны стиля (изображения)"
Cohesion: 0.17
Nodes (16): Фон: заброшенный заводской цех со ржавыми станками, светом сквозь крышу и дымкой, Кандидаты шрифтов: Bahnschrift, Arial Narrow, Franklin Gothic, Century Gothic, Impact, Consolas, Calibri, Segoe UI, Сравнение шрифтов субтитров для Reels: 8 вариантов на кадре заброшенного цеха, Слоган «Самое ценное в этом мире — не оружие.», Вооружённые охранники в балаклавах у сетчатых ворот (АК, бочка с огнём), A1: Станция (фотореализм) — кирпичная насосная станция с водонапорной башней, охрана у ворот, снег, сумерки, Станция-инфраструктура (кирпичное здание, водонапорная башня, дым из труб), Стиль A: кинематографичный фотореализм (холодный сине-серый тон, тёплые окна, огонь в бочке) (+8 more)

### Community 11 - "Unreal MCP конфигурация"
Cohesion: 0.50
Nodes (3): UE_PROJECT_PATH, npx, unreal-engine

## Knowledge Gaps
- **66 isolated node(s):** `npx`, `UE_PROJECT_PATH`, `Unreal Engine 5 (одна версия, сборка из исходников)`, `Ассеты: Fab, Megascans, MetaHuman, Mixamo`, `Рабочий процесс: бэклог → Claude → человек → ревью → плейтест` (+61 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 119 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **8 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Сценарии рилсов, Фаза А (12 роликов)` connect `Фазы продвижения и сценарии` to `Канон, уровни и квесты`, `Архитектура, стек и план этапов`, `Стратегия продвижения`?**
  _High betweenness centrality (0.330) - this node is a cross-community bridge._
- **Are the 3 inferred relationships involving `Канон мира (единственный источник правды)` (e.g. with `«Постапокалипсис без мутантов»` and `Канон-фильтр генераций`) actually correct?**
  _`Канон мира (единственный источник правды)` has 3 INFERRED edges - model-reasoned connections that need verification._
- **What connects `npx`, `UE_PROJECT_PATH`, `Unreal Engine 5 (одна версия, сборка из исходников)` to the rest of the system?**
  _66 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Линтер подписей (caption.py)` be split into smaller, more focused modules?**
  _Cohesion score 0.07142857142857142 - nodes in this community are weakly interconnected._
- **Why does `Канон мира (единственный источник правды)` connect `Канон, уровни и квесты` to `Фазы продвижения и сценарии`, `Экономика и геймдизайн (агенты)`, `Архитектура, стек и план этапов`, `Стратегия продвижения`?**
  _High betweenness centrality (0.279) - this node is a cross-community bridge._
- **Should `Канон, уровни и квесты` be split into smaller, more focused modules?**
  _Cohesion score 0.06826241134751773 - nodes in this community are weakly interconnected._
- **Why does `Процесс, инструменты и план разработки` connect `Архитектура, стек и план этапов` to `Фазы продвижения и сценарии`, `Канон, уровни и квесты`, `Экономика и геймдизайн (агенты)`, `Стратегия продвижения`?**
  _High betweenness centrality (0.226) - this node is a cross-community bridge._