# UE Project Context

*Engine: UE 5.8.4 · Last updated: 2026-11-02*

## Engine & Project
**Engine version:** 5.8.4 (CL 58902052) — launcher build at `E:\UE_5.8` (`EngineAssociation: "5.8"`). Source build planned for Stage 1 (dedicated server target).
**Project name:** Rubezh («РУБЕЖ: Земля после разлома») · **Type:** game · **Genre:** MMO-lite online shooter with survival, player economy, guild territory wars
**Description:** Realistic post-collapse world (no mutants/anomalies); zone dedicated servers + persistent backend (Nakama + PostgreSQL). Canon: `docs/01_Канон.md`.
**Base:** Lyra Starter Game (`.uproject` still says `ProjectName=Lyra` in `DefaultGame.ini`; `Category: Samples`).
**Target platforms:** Win64 (dedicated servers: Win64/Linux — [unknown, not decided])
**Hardware baseline (lead dev PC):** Ryzen 7 5700X, 32 GB RAM, GeForce GTX 1660 SUPER (no hardware ray tracing → Lumen software path only).

## Modules
**Primary game module:** `LyraGame` (Lyra modules not renamed yet).
**Project code:** Game Feature plugin `Plugins/GameFeatures/RubezhCore` (mount point `/RubezhCore`), module `RubezhCoreRuntime` (Runtime; public deps Core, LyraGame, ModularGameplay; private deps CoreUObject, Engine, GameplayTags, GameplayTasks, GameplayAbilities, GameplayMessageRuntime, EnhancedInput, NetCore). `BuiltInInitialFeatureState: Registered`, activated by experience `B_Rubezh_Proto`. Asset scan rules copied from ShooterCore (Experiences, Maps, …).
**Prototype map:** `/RubezhCore/Maps/L_Rubezh_Proto` (200×200 m grey box, 8 `ALyraPlayerStart`, NavMeshBoundsVolume) → experience `/RubezhCore/Experiences/B_Rubezh_Proto` (copy of `B_ShooterGame_Elimination` + `RubezhCore` feature).
**MCP note:** EngineWire level/save operations accept only `/Game` and `/Engine` paths; for plugin content (`/RubezhCore`) use `system_control.execute_python` (each call needs a fresh consent from `describe`).

| Module | Host type | Key deps | Notes |
|---|---|---|---|
| LyraGame | Runtime | Core, CoreUObject, Engine, GameplayTags, GameplayTasks, GameplayAbilities, AIModule, ModularGameplay, ModularGameplayActors, DataRegistry, ReplicationGraph, GameFeatures, SignificanceManager, EnhancedInput, CommonUI, CommonInput, CommonGame, CommonUser, GameSettings, UIExtension, GameplayMessageRuntime, Niagara, NetCore, UMG, AudioModulation, EngineSettings, Json | `SetupIrisSupport(Target)`; defines `SHIPPING_DRAW_DEBUG_ERROR=1`, `WITH_RPC_REGISTRY`, `WITH_HTTPSERVER_LISTENERS`, `WITH_AUTOMATION_DRIVER` |
| LyraEditor | Editor | UnrealEd, EditorFramework, GameplayTagsEditor, GameplayAbilitiesEditor, DataValidation, ToolMenus, LyraGame, SourceControl | Lyra editor utilities |

## Plugins
| Plugin | Maturity | Used for |
|---|---|---|
| GameplayAbilities | stable | combat, attributes, effects |
| GameFeatures | Beta | Lyra experiences / feature plugins |
| ModularGameplay | Beta | component injection (Lyra) |
| EnhancedInput | stable | input |
| CommonUI | stable | UI stack |
| ReplicationGraph | Beta | present but **disabled** (`bDisableReplicationGraph=True`) |
| Iris | Beta | `SetupIrisSupport` in Build.cs; IrisCore sections in DefaultEngine.ini — runtime enablement [unknown] |
| DataRegistry | Beta | Lyra data |
| SignificanceManager | stable | LOD/significance |
| SmartObjects, GameplayStateTree, GameplayBehaviors | stable/Beta | AI (Lyra samples) |
| Niagara, Metasound, AudioModulation | stable | FX, audio |
| OnlineSubsystemSteam, SteamSockets, OnlineServicesNull, OnlineServicesOSSAdapter | stable | Lyra defaults; real backend will be Nakama |
| AIAssistant, ToolsetRegistry, AllToolsets | [unknown] | enabled by the 5.8 Lyra template |

**In-house plugins (Plugins/):** Lyra: AsyncMixin, CommonGame, CommonLoadingScreen, CommonStartupLoadingScreen, CommonUser, GameSettings, GameSubtitles, GameplayMessageRouter, ModularGameplayActors, PocketWorlds, UIExtension, LyraExampleContent, LyraExtTool, GreenRoom, RedRoom. Game features: ShooterCore, ShooterExplorer, ShooterMaps, ShooterTests, TopDownArena.
**Editor tooling:** EngineWire 0.6.0 (ChiR24/Unreal_mcp, MIT) — Unreal MCP bridge, Editor-only, listens on 127.0.0.1.
**Planned:** heroiclabs/nakama-unreal — [not installed].

## Coding conventions
**Prefixes:** Epic standard F/U/A/E/I; project classes `Rubezh` (e.g. `URubezhInventoryComponent`). Lyra code keeps `Lyra`.
**Object references:** `TObjectPtr` in `UPROPERTY`.
**API macro style:** Lyra uses per-header `#define UE_API LYRAGAME_API`; Rubezh module style [not yet established].
**Log categories:** Lyra: `LogLyra`, `LogLyraAbilitySystem`, `LogLyraExperience`, `LogLyraTeams`, `LogLyraRepGraph`… Rubezh: `LogRubezh` (declared in `RubezhCoreRuntimeModule.h`, exported `RUBEZHCORERUNTIME_API`); per-system categories later.
**Assertions:** [unknown]
**Enforced rules (CLAUDE.md):** C++ for logic, Blueprint for configuration only; server-authoritative damage/loot/inventory/craft/money; never edit `.uasset`/`.umap` as files (editor, Python API or MCP only); game data in `data/` as CSV/JSON → DataTable; secrets in env vars; C++ reviewed by `ecc:cpp-reviewer`.

## Gameplay framework
| Role | Class |
|---|---|
| GameMode | `ALyraGameMode` (BP: `/Game/B_LyraGameMode`) |
| GameState | `ALyraGameState` |
| PlayerController | `ALyraPlayerController` |
| PlayerState | `ALyraPlayerState` |
| Pawn / Character | `ALyraCharacter` |
| GameInstance | `ULyraGameInstance` (BP: `/Game/B_LyraGameInstance`) |

**Experiences:** `ULyraExperienceDefinition` in `/Game/System/Experiences`; game data `/Game/DefaultGameData`.
**Maps:** default `/Game/System/FrontEnd/Maps/L_LyraFrontEnd`, editor startup `L_DefaultEditorOverview`; `ServerDefaultMap` not set.

## GAS
**Enabled:** yes
**AbilitySystemComponent:** `ULyraAbilitySystemComponent` — lives on PlayerState (`ALyraPlayerState`)
**AttributeSets:** `ULyraHealthSet`, `ULyraCombatSet` (Rubezh attributes — stats/skills/climate — [not yet designed])
**Ability base:** `ULyraGameplayAbility`
**Tag source:** `Config/DefaultGameplayTags.ini` (`ImportTagsFromConfig=True`) + tag tables `DT_AnimEffectTags`, `DT_SurfaceTypes`; Lyra native tags in C++.

## Networking
**Model:** dedicated server per zone (`LyraServer` target), 80–150 players per zone process; realm = cluster of zone servers + layering + shared backend (Nakama + PostgreSQL). See `docs/00_Процесс_и_план_разработки.md`.
**Replication stack:** classic replication with Iris support compiled in; Iris runtime switch [unknown / to decide in Stage 1]
**Push model:** [unknown] · **ReplicationGraph:** compiled, disabled in config
**Budget:** [not yet established — tick rate to be set in Stage 1]

## Input
**Stack:** EnhancedInput (`LyraPlayerInput`, `LyraInputComponent` in DefaultInput.ini)
**Mapping contexts:** Lyra `InputData` / ShooterCore assets · **Added by:** Lyra hero component (`ULyraHeroComponent`)

## UI
**Stack:** CommonUI (+ UIExtension, CommonGame primary game layout)

## AI
**Stack:** Lyra sample bots (Behavior Trees) + StateTree/SmartObjects plugins enabled. Planned: wild-animal AI — [not yet designed].

## Streaming
**Model:** Lyra sample maps are non-World-Partition; the open world will use World Partition — [not yet set up].

## Saves
Persistent player data lives in the backend (Nakama), not `USaveGame`. Local `USaveGame` only for client settings (Lyra `ULyraSettingsLocal`).

## Build
**Targets:** LyraEditor (Editor), LyraGame (Game), LyraClient (Client), LyraServer (Server) + EOS/Steam variants. `DefaultBuildSettings = V7`, `IncludeOrderVersion = Latest`.
**Editor build command:** `E:\UE_5.8\Engine\Build\BatchFiles\Build.bat LyraEditor Win64 Development -Project=E:\Rubezh\Game\Rubezh.uproject -WaitMutex`
**Engine modifications:** none (launcher build).

## Team
**Size & roles:** 5 people + Claude (lead/game designer = project owner; other roles TBD after team survey).
**Source control:** Git + Git LFS (`.uasset`/`.umap`/art marked `lockable`); repo root `E:\Rubezh`, game in `Game/`. Remote host [not chosen]. Branching [not yet established].
**Docs:** `docs/` (Russian).
