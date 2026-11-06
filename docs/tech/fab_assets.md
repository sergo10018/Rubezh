# Модели с Fab

Что взято с Fab, по какой лицензии и где используется. Новую модель заносить сюда сразу: по этому списку потом собираются титры игры.

Правило отбора: бесплатно и для личного, и для профессионального использования. Товары, где бесплатна только личная лицензия, а профессиональная платная, не берём без решения лида.

- **Стандартная лицензия Fab:** указывать автора не нужно.
- **CC-BY:** автора нужно указать в титрах игры.

## В проекте и используется на карте `L_Rubezh_Field`
| Модель | Автор | Лицензия | Папка | Для чего |
|---|---|---|---|---|
| German Shepherd 3D Dog Model | RetroStyle Games | стандартная | `/Game/German_Shepherd_3D_Model` | собака (нет анимаций укуса и смерти) |
| Animal Variety Pack | PROTOFACTOR INC | стандартная | `/Game/AnimalVarietyPack` | волк; свинью лид не утвердил, не используем |
| Free Birch Biome Sample | FreshCan | стандартная | `/Game/FC_BirchTreeSample` | берёзы |
| DZ Sample Trees | DropZone Studios | стандартная | `/Game/DZ_Assets` | сосна и осина |
| Free Shrubs Pack | Greenleaf Vision | стандартная | `/Game/GV_FreeShrubsPack` | кусты |
| Factory Pack Vol.1 | MeikWModels | **CC-BY** | `/Game/Factory_Pack_V1` | опоры ЛЭП и баки у завода |
| Abandoned House | jourverse | **CC-BY** | `/Game/Fab/AbandonedHouse` | дома деревни |
| Warehouse Building | Hephaestus Studio | **CC-BY** | `/Game/Fab/WarehouseBuilding` | цех завода |
| Soviet-Style Apartment Building | Daonware | стандартная | `/Game/Fab/SovietBlock` | панельки у завода, стены перекрашены в бетон |

## В проекте, пока не используется
| Модель | Автор | Лицензия | Папка | Для чего |
|---|---|---|---|---|
| AK-47 with animations | A.D | стандартная | `/Game/AK-47` | автомат |
| Makarov Pistol (PM) | TaigaForest | **CC-BY** | `/Game/TF_Russian_Pistols` | пистолет |
| Survival Character | Arberry | стандартная | `/Game/Survival_Character` | персонаж: модульный, с рюкзаком |
| Megaplants: Norway Spruce, Silver Birch, Common Heather | Quixel Megaplants | стандартная | `/Game/Megaplant_Library` | процедурные растения; проверить, потянет ли карта |

## Скачано, в проект не импортировано (`Game/Saved/FabImport`, в git нет)
| Модель | Автор | Лицензия | Для чего |
|---|---|---|---|
| Model 12 Pump Action Shotgun | the 9th Earl | стандартная | дробовик |
| SV-98 Sniper Rifle | SpatialNeglect | **CC-BY** | снайперская винтовка; СВД бесплатно не нашлось |
| Battle Vest | Barnus Model's | стандартная | бронежилет |
| Military Helmet | CG Vertexium | стандартная | шлем |
| Free sample Squad Of Surviving Stalkers | Oleg Ushenok | стандартная | персонаж |
| Dead Shrubs, Rough Meadow Grass | Quixel Megascans | стандартная | сухие кусты, трава |

## Только в библиотеке Fab
Не скачаны: SKS, jacket pants balaclava and bags (**CC-BY**, marko1937), Quantum Modular Character, Unfinished Building, Free Nanite Plants, Megaplants: Norway Maple, European Hornbeam, Black Alder, Wild Grass.

## Нет бесплатной замены
Реалистичных кабана и медведя с анимациями среди бесплатных моделей нет, на карте они пока коробки. Платный вариант — Forest Animals: перед покупкой проверить цену и получить решение лида.

## Как добавить
- **Формат Unreal:** Epic Games Launcher → Unreal Engine → Библиотека → «Добавить к проекту». Если проекта нет в списке, отметить «Показать все проекты» и выбрать самую новую версию из предложенных.
- **FBX:** скачать с сайта Fab, распаковать в `Game/Saved/FabImport/<имя>`, дописать в `tools/unreal/import_fab_fbx.py` и запустить его в редакторе.
