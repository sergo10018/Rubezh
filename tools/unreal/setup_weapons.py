"""Настройка оружия в редакторе (запускать через Unreal MCP или Tools → Execute Python Script).

Что делает (повторный запуск безопасен):
1. Импортирует data/items.csv, data/loot_tables.csv и data/weapons.csv в таблицы /RubezhCore/Data.
2. Создаёт снайперскую винтовку: копии ассетов автомата в /RubezhCore/Weapons/Sniper (своей модели пока нет).
3. Применяет к ассетам Lyra темп стрельбы, режим огня и число дробин из data/weapons.csv.
4. Убирает у оружия Lyra свои множители попаданий и спад урона: их считает Rubezh::Weapons.
5. Убирает стартовый пистолет Lyra: оружие теперь выдаёт URubezhInventoryComponent.
"""
import csv

import unreal

DATA = "E:/Rubezh/data/"
SNIPER = "/RubezhCore/Weapons/Sniper/"
RIFLE = "/ShooterCore/Weapons/Rifle/"
# Дальше этого расстояния (см) пуля не летит; спад урона считает Rubezh::Weapons.
MAX_TRACE_RANGE_CM = 100000.0

# Какие ассеты Lyra соответствуют какому оружию из data/weapons.csv.
ASSETS = {
    "pistol_9x18": ("/ShooterCore/Weapons/Pistol/GA_Weapon_Fire_Pistol", "/ShooterCore/Weapons/Pistol/B_WeaponInstance_Pistol"),
    "rifle_762x39": (RIFLE + "GA_Weapon_Fire_Rifle_Auto", RIFLE + "B_WeaponInstance_Rifle"),
    "shotgun_12": ("/ShooterCore/Weapons/Shotgun/GA_Weapon_Fire_Shotgun", "/ShooterCore/Weapons/Shotgun/B_WeaponInstance_Shotgun"),
    "rifle_762x54r": (SNIPER + "GA_RubezhSniper_Fire", SNIPER + "B_WeaponInstance_RubezhSniper"),
}


def import_table(file_name, asset_name, row_struct):
    task = unreal.AssetImportTask()
    task.filename = DATA + file_name
    task.destination_path = "/RubezhCore/Data"
    task.destination_name = asset_name
    task.replace_existing = True
    task.automated = True
    task.save = True
    settings = unreal.CSVImportSettings()
    settings.import_type = unreal.CSVImportType.ECSV_DATA_TABLE
    settings.import_row_struct = unreal.load_object(None, "/Script/RubezhCoreRuntime." + row_struct)
    factory = unreal.CSVImportFactory()
    factory.automated_import_settings = settings
    task.factory = factory
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    table = unreal.load_asset("/RubezhCore/Data/" + asset_name)
    print(asset_name, "rows:", len(unreal.DataTableFunctionLibrary.get_data_table_row_names(table)))


def cdo(path):
    return unreal.get_default_object(unreal.load_asset(path).generated_class())


def duplicate(source, name):
    target = SNIPER + name
    if not unreal.EditorAssetLibrary.does_asset_exist(target):
        if not unreal.EditorAssetLibrary.duplicate_asset(source, target):
            raise RuntimeError("cannot duplicate " + source)
    return unreal.load_asset(target)


def create_sniper():
    fire = duplicate(RIFLE + "GA_Weapon_Fire_Rifle_Auto", "GA_RubezhSniper_Fire")
    instance = duplicate(RIFLE + "B_WeaponInstance_Rifle", "B_WeaponInstance_RubezhSniper")
    abilities = duplicate(RIFLE + "AbilitySet_ShooterRifle", "AbilitySet_RubezhSniper")
    equipment = duplicate(RIFLE + "WID_Rifle", "WID_RubezhSniper")
    item = duplicate(RIFLE + "ID_Rifle", "ID_RubezhSniper")

    granted = abilities.get_editor_property("GrantedGameplayAbilities")
    # Поля этой структуры закрыты для прямой записи из Python: заменяем класс способности в её текстовом виде.
    rifle_fire = RIFLE + "GA_Weapon_Fire_Rifle_Auto.GA_Weapon_Fire_Rifle_Auto_C"
    sniper_fire = SNIPER + "GA_RubezhSniper_Fire.GA_RubezhSniper_Fire_C"
    # Элемент массива приходит копией, поэтому после правки его надо записать обратно.
    entry = granted[0]
    entry.import_text(entry.export_text().replace(rifle_fire, sniper_fire))
    granted[0] = entry
    abilities.set_editor_property("GrantedGameplayAbilities", granted)
    fire_class = abilities.get_editor_property("GrantedGameplayAbilities")[0].get_editor_property("Ability")
    if fire_class != fire.generated_class():
        raise RuntimeError("sniper fire ability was not assigned: %s" % fire_class)

    equipment_cdo = unreal.get_default_object(equipment.generated_class())
    equipment_cdo.set_editor_property("InstanceType", instance.generated_class())
    equipment_cdo.set_editor_property("AbilitySetsToGrant", [abilities])

    item_cdo = unreal.get_default_object(item.generated_class())
    item_cdo.set_editor_property("DisplayName", unreal.Text("Снайперская винтовка"))
    for fragment in item_cdo.get_editor_property("Fragments"):
        if fragment.get_class().get_name() == "InventoryFragment_EquippableItem":
            fragment.set_editor_property("EquipmentDefinition", equipment.generated_class())
    return [fire, instance, abilities, equipment, item]


def apply_stats():
    touched = []
    with open(DATA + "weapons.csv", encoding="utf-8", newline="") as stream:
        for row in csv.DictReader(stream):
            fire_path, instance_path = ASSETS[row["---"]]
            fire = cdo(fire_path)
            fire.set_editor_property("FireDelayTimeSecs", 60.0 / float(row["ShotsPerMinute"]))
            semi_auto = row["bSemiAuto"].strip().lower() == "true"
            fire.set_editor_property("ActivationPolicy", unreal.LyraAbilityActivationPolicy.ON_INPUT_TRIGGERED if semi_auto else unreal.LyraAbilityActivationPolicy.WHILE_INPUT_ACTIVE)

            instance = cdo(instance_path)
            instance.set_editor_property("BulletsPerCartridge", int(row["Pellets"]))
            instance.set_editor_property("MaxDamageRange", MAX_TRACE_RANGE_CM)
            instance.set_editor_property("MaterialDamageMultiplier", {})
            instance.set_editor_property("DistanceDamageFalloff", unreal.RuntimeFloatCurve())
            touched += [fire_path, instance_path]
            print(row["---"], "delay", fire.get_editor_property("FireDelayTimeSecs"), "semi", semi_auto, "pellets", row["Pellets"])
    return touched


def remove_lyra_starting_pistol():
    path = "/ShooterCore/Game/B_Hero_ShooterMannequin"
    cdo(path).set_editor_property("InitialInventoryItems", [])
    return path


import_table("items.csv", "DT_Items", "RubezhItemDefinition")
import_table("loot_tables.csv", "DT_LootTables", "RubezhLootTableRow")
created = create_sniper()
import_table("weapons.csv", "DT_Weapons", "RubezhWeaponDefinition")
paths = apply_stats() + [remove_lyra_starting_pistol()]

for asset in created:
    unreal.EditorAssetLibrary.save_loaded_asset(asset, False)
for path in paths:
    unreal.EditorAssetLibrary.save_asset(path, False)
print("setup_weapons: done")
