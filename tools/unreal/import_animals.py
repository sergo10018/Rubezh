"""Создаёт или обновляет таблицу животных /RubezhCore/Data/DT_Animals из data/animals.csv (запускать в редакторе)."""
import os

import unreal

TABLE = "/RubezhCore/Data/DT_Animals"
CSV = os.path.normpath(os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "..", "data", "animals.csv"))

assets = unreal.EditorAssetLibrary
if assets.does_asset_exist(TABLE):
    table = unreal.load_asset(TABLE)
else:
    factory = unreal.DataTableFactory()
    factory.set_editor_property("struct", unreal.RubezhAnimalDefinition.static_struct())
    table = unreal.AssetToolsHelpers.get_asset_tools().create_asset("DT_Animals", "/RubezhCore/Data", unreal.DataTable, factory)

if not unreal.DataTableFunctionLibrary.fill_data_table_from_csv_file(table, CSV):
    raise RuntimeError("не удалось прочитать " + CSV)
assets.save_loaded_asset(table)
print("import_animals:", [str(name) for name in unreal.DataTableFunctionLibrary.get_data_table_row_names(table)])
