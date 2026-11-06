"""Импорт FBX, скачанных с Fab, в /Game/Fab/<имя> (запускать в редакторе; повторный запуск заменяет ассеты).

Архивы распаковать в Game/Saved/FabImport/<имя>. Текстуры лежат рядом с FBX и подключаются при импорте.
Статичным постройкам задаётся точная коллизия, иначе двери и проёмы будут закрыты простой формой.
"""
import os

import unreal

SOURCE = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "FabImport")

# (папка в проекте, файл относительно SOURCE)
STATIC_MESHES = [
    ("AbandonedHouse", "abandoned-house/source/abandonhouse.fbx"),
    ("WarehouseBuilding", "warehouse-building/source/Warehouse-open.fbx"),
    ("SovietBlock", "blockhaus/Blockhaus.fbx"),
]


def import_static(folder, relative):
    path = os.path.join(SOURCE, relative)
    if not os.path.exists(path):
        raise RuntimeError("нет файла " + path)
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_textures", True)
    data = options.static_mesh_import_data
    data.set_editor_property("combine_meshes", True)
    data.set_editor_property("auto_generate_collision", False)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", path)
    task.set_editor_property("destination_path", "/Game/Fab/" + folder)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    for imported in task.get_editor_property("imported_object_paths"):
        mesh = unreal.load_asset(imported)
        if isinstance(mesh, unreal.StaticMesh):
            setup = mesh.get_editor_property("body_setup")
            setup.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
            unreal.EditorAssetLibrary.save_loaded_asset(mesh)
            box = mesh.get_bounding_box()
            print(folder, imported, "размер, см:", box.max - box.min, "минимум:", box.min,
                  "материалов:", len(mesh.static_materials))


for folder_name, file_name in STATIC_MESHES:
    import_static(folder_name, file_name)
print("import_fab_fbx: done")


def build_house_material():
    """У этого FBX текстуры лежат отдельно: собрать материал и назначить модели."""
    folder = "/Game/Fab/AbandonedHouse"
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    textures = {}
    for name in ("Base_Color", "Normal", "Roughness"):
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", os.path.join(SOURCE, "abandoned-house/textures/AbHouse_%s.png" % name))
        task.set_editor_property("destination_path", folder)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        tools.import_asset_tasks([task])
        texture = unreal.load_asset(task.get_editor_property("imported_object_paths")[0])
        if name == "Normal":
            texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        elif name == "Roughness":
            texture.set_editor_property("srgb", False)
            texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        unreal.EditorAssetLibrary.save_loaded_asset(texture)
        textures[name] = texture

    path = folder + "/M_AbandonedHouse"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    material = tools.create_asset("M_AbandonedHouse", folder, unreal.Material, unreal.MaterialFactoryNew())
    lib = unreal.MaterialEditingLibrary
    material.set_editor_property("two_sided", True)
    links = ((textures["Base_Color"], unreal.MaterialProperty.MP_BASE_COLOR, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR),
             (textures["Normal"], unreal.MaterialProperty.MP_NORMAL, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL),
             (textures["Roughness"], unreal.MaterialProperty.MP_ROUGHNESS, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS))
    for index, (texture, target, sampler) in enumerate(links):
        node = lib.create_material_expression(material, unreal.MaterialExpressionTextureSample, -400, 300 * index)
        node.set_editor_property("texture", texture)
        node.set_editor_property("sampler_type", sampler)
        lib.connect_material_property(node, "R" if target == unreal.MaterialProperty.MP_ROUGHNESS else "RGB", target)
    lib.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)

    mesh = unreal.load_asset(folder + "/abandonhouse")
    mesh.set_material(0, material)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)


build_house_material()
print("house material done")
