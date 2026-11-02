# Подключение Claude к редактору Unreal (MCP)

Используем [ChiR24/Unreal_mcp](https://github.com/ChiR24/Unreal_mcp) (MIT): плагин **EngineWire** внутри редактора + прокси `unreal-engine-mcp-server` для Claude Code. Версии плагина и прокси должны совпадать — сейчас **0.6.0**.

## Что уже сделано
- Исходники плагина лежат в `tools/third_party/EngineWire/` (коммит — в `SOURCE.txt`).
- В `.mcp.json` прописан сервер `unreal-engine` (прокси через `npx`, версия зафиксирована).
- В `.claude/settings.json` запрещено редактировать `.uasset`/`.umap` напрямую — только через редактор.

## Состояние (2026-11-02): подключено
Репозиторий — `E:\Rubezh`, проект — `E:\Rubezh\Game\Rubezh.uproject` (UE 5.8.4), плагин — `Game/Plugins/EngineWire`. Редактор собран, связь проверена (482 операции). Плагин слушает `127.0.0.1:8090/8091`; прокси из `.mcp.json` подключается сам, как только редактор запущен.

## Настройка на компьютере участника
1. **Путь без кириллицы и пробелов** (UE и его сборка ненадёжно работают с такими путями). Если клонируете не в `E:\Rubezh`, поправьте `UE_PROJECT_PATH` в `.mcp.json` у себя.
2. Плагин уже лежит в `Game/Plugins/EngineWire` (копия исходников — `tools/third_party/EngineWire`).
3. Собрать редактор: `Build.bat LyraEditor Win64 Development -Project=<путь>\Game\Rubezh.uproject -WaitMutex` или из Visual Studio.
4. Открыть редактор (плитка в лаунчере или `UnrealEditor.exe <путь к .uproject>`); в логе должно быть `EngineWire listening on 127.0.0.1`. Если нет — Project Settings → EngineWire → Enable Native MCP Server.
5. Проверить связь:
   ```bash
   npx unreal-engine-mcp-server@0.6.0 doctor
   ```
6. Перезапустить сессию Claude Code и одобрить сервер `unreal-engine`.

## Безопасность
- Сервер слушает только `127.0.0.1`; токен лежит в `Game/Saved/MCP/capability-token` — не коммитить (`Game/Saved/` в `.gitignore`).
- Каждый участник ставит плагин у себя; токены у всех разные.
