"""Шаг 1 сборки карты 1×1 км: новый пустой уровень со светом и режимом игры, материал земли.

Запуск в редакторе; повторно не нужен. Шаг 2 — ландшафт (Unreal MCP: create_landscape, import_heightmap),
шаг 3 — build_field_map.py.
"""
import unreal

MAP = "/RubezhCore/Maps/L_Rubezh_Field"
MATERIAL = "/RubezhCore/Environment/M_RubezhGround"
EXPERIENCE = "/RubezhCore/Experiences/B_Rubezh_Proto.B_Rubezh_Proto_C"

assets = unreal.EditorAssetLibrary
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def build_material():
    if assets.does_asset_exist(MATERIAL):
        return
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = tools.create_asset("M_RubezhGround", "/RubezhCore/Environment", unreal.Material, unreal.MaterialFactoryNew())
    lib = unreal.MaterialEditingLibrary

    def node(cls, x, y):
        return lib.create_material_expression(material, cls, x, y)

    def color(rgb, x, y):
        result = node(unreal.MaterialExpressionConstant3Vector, x, y)
        result.set_editor_property("constant", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
        return result

    grass = color((0.075, 0.105, 0.035), -900, -300)
    dry = color((0.17, 0.14, 0.07), -900, -150)
    rock = color((0.13, 0.12, 0.11), -900, 0)

    position = node(unreal.MaterialExpressionWorldPosition, -1400, -300)
    patches = node(unreal.MaterialExpressionNoise, -1150, -420)
    patches.set_editor_property("scale", 0.002)
    patches.set_editor_property("levels", 3)
    patches.set_editor_property("output_min", 0.0)
    patches.set_editor_property("output_max", 1.0)
    lib.connect_material_expressions(position, "", patches, "Position")

    ground = node(unreal.MaterialExpressionLinearInterpolate, -600, -280)
    lib.connect_material_expressions(grass, "", ground, "A")
    lib.connect_material_expressions(dry, "", ground, "B")
    lib.connect_material_expressions(patches, "", ground, "Alpha")

    # Склоны — камень: чем меньше нормаль смотрит вверх, тем больше камня.
    normal = node(unreal.MaterialExpressionVertexNormalWS, -1150, 100)
    up = node(unreal.MaterialExpressionComponentMask, -950, 100)
    up.set_editor_property("r", False)
    up.set_editor_property("g", False)
    up.set_editor_property("b", True)
    lib.connect_material_expressions(normal, "", up, "")
    power = node(unreal.MaterialExpressionPower, -750, 100)
    exponent = node(unreal.MaterialExpressionConstant, -900, 220)
    exponent.set_editor_property("r", 12.0)
    lib.connect_material_expressions(up, "", power, "Base")
    lib.connect_material_expressions(exponent, "", power, "Exponent")

    final = node(unreal.MaterialExpressionLinearInterpolate, -350, -100)
    lib.connect_material_expressions(rock, "", final, "A")
    lib.connect_material_expressions(ground, "", final, "B")
    lib.connect_material_expressions(power, "", final, "Alpha")
    lib.connect_material_property(final, "", unreal.MaterialProperty.MP_BASE_COLOR)

    roughness = node(unreal.MaterialExpressionConstant, -350, 100)
    roughness.set_editor_property("r", 0.95)
    lib.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)

    lib.recompile_material(material)
    assets.save_loaded_asset(material)


def spawn(cls, location, rotation=None):
    return actors.spawn_actor_from_class(cls, unreal.Vector(*location), rotation or unreal.Rotator(0.0, 0.0, 0.0))


def build_level():
    if assets.does_asset_exist(MAP):
        raise RuntimeError(MAP + " уже есть")
    if not levels.new_level(MAP):
        raise RuntimeError("не удалось создать " + MAP)

    sun = spawn(unreal.DirectionalLight, (0.0, 0.0, 5000.0), unreal.Rotator(0.0, -40.0, 30.0))
    sun.light_component.set_editor_property("atmosphere_sun_light", True)
    sun.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    spawn(unreal.SkyAtmosphere, (0.0, 0.0, 0.0))
    sky = spawn(unreal.SkyLight, (0.0, 0.0, 6000.0))
    sky.light_component.set_editor_property("real_time_capture", True)
    sky.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    spawn(unreal.ExponentialHeightFog, (0.0, 0.0, 0.0))
    spawn(unreal.VolumetricCloud, (0.0, 0.0, 0.0))

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    settings = world.get_world_settings()
    print(settings.get_class().get_name())
    settings.set_editor_property("DefaultGameplayExperience", unreal.load_class(None, EXPERIENCE))
    levels.save_current_level()


build_material()
build_level()
print("build_field_base: done")
