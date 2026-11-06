"""Шаг 3 сборки карты 1×1 км: ставит на рельеф дорогу, деревню, завод, лес, ящики, логова животных и точки появления ботов.

Запуск в редакторе при открытой /RubezhCore/Maps/L_Rubezh_Field. Повторный запуск безопасен: всё с меткой RubezhField
сначала удаляется. Поселение строит отдельно build_settlement.py. Места — field_layout.py.
Серый бокс: постройки — коробки; деревья и кусты берутся из TREE_MESHES и BUSH_MESHES, пока это фигуры движка.
"""
import math
import os
import random
import sys

import unreal

sys.path.insert(0, os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "..", "tools", "unreal"))
import importlib

import field_layout as layout

importlib.reload(layout)

MAP = "/RubezhCore/Maps/L_Rubezh_Field"
EXPERIENCE = "/RubezhCore/Experiences/B_Rubezh_Proto.B_Rubezh_Proto_C"
TAG = "RubezhField"
SEED = 11
CM = 100.0

ROAD_WIDTH = 800.0
ROAD_PIECE = 2000.0
WALL = 40.0

# Заменить на модели с Fab: пути к StaticMesh. Пусто — ствол и конус из фигур движка.
TREE_MESHES = []
BUSH_MESHES = []

FOREST_PATCHES = [  # центр в метрах, радиус, сколько деревьев
    ((-380.0, 180.0), 110.0, 70), ((-180.0, 320.0), 120.0, 70), ((120.0, 350.0), 90.0, 40),
    ((360.0, -80.0), 110.0, 70), ((180.0, -330.0), 120.0, 70), ((-120.0, -360.0), 90.0, 40), ((30.0, 130.0), 50.0, 20),
]
BUSHES = 160

SPAWNERS = [  # вид, сколько, метры, через сколько секунд новые
    ("dog", 4, (-120.0, 70.0), 600.0), ("dog", 3, (130.0, 110.0), 600.0), ("wolf", 3, (-330.0, 170.0), 900.0),
    ("boar", 1, (-190.0, 280.0), 600.0), ("boar", 1, (180.0, -280.0), 600.0), ("bear", 1, (360.0, -60.0), 1200.0),
]

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
assets = unreal.EditorAssetLibrary
cube = unreal.load_asset("/Engine/BasicShapes/Cube")
cone = unreal.load_asset("/Engine/BasicShapes/Cone")
cylinder = unreal.load_asset("/Engine/BasicShapes/Cylinder")
sphere = unreal.load_asset("/Engine/BasicShapes/Sphere")


def flat_material(name, rgb):
    """Простой материал одного цвета для заготовок."""
    path = "/RubezhCore/Environment/" + name
    if assets.does_asset_exist(path):
        return unreal.load_asset(path)
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, "/RubezhCore/Environment", unreal.Material, unreal.MaterialFactoryNew())
    lib = unreal.MaterialEditingLibrary
    value = lib.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -300, 0)
    value.set_editor_property("constant", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    lib.connect_material_property(value, "", unreal.MaterialProperty.MP_BASE_COLOR)
    lib.recompile_material(material)
    assets.save_loaded_asset(material)
    return material


ROAD = flat_material("M_Flat_Road", (0.045, 0.04, 0.035))
CONCRETE = flat_material("M_Flat_Concrete", (0.28, 0.27, 0.25))
BRICK = flat_material("M_Flat_Brick", (0.22, 0.09, 0.06))
RUST = flat_material("M_Flat_Rust", (0.2, 0.1, 0.05))
TRUNK = flat_material("M_Flat_Trunk", (0.07, 0.045, 0.025))
LEAVES = flat_material("M_Flat_Leaves", (0.025, 0.07, 0.025))
BUSH = flat_material("M_Flat_Bush", (0.05, 0.09, 0.03))


def ground_z(x, y):
    """Высота земли в см; x, y — в см. None — под точкой нет земли."""
    world = editor.get_editor_world()
    hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, 20000.0), unreal.Vector(x, y, -20000.0),
        unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
    return hit.to_tuple()[4].z if hit else None


def put(mesh, material, location, size, label, rotation=None, folder="Field"):
    """Фигура движка размера size (см); location — центр. Куб, конус и цилиндр движка — 100 см."""
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location), rotation or unreal.Rotator(0.0, 0.0, 0.0))
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.static_mesh_component.set_material(0, material)
    actor.set_actor_scale3d(unreal.Vector(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    actor.set_actor_label(label)
    actor.set_folder_path(folder)
    actor.tags = [TAG]
    return actor


def place(cls, location, label, yaw=0.0, folder="Field"):
    actor = actors.spawn_actor_from_class(cls, unreal.Vector(*location), unreal.Rotator(0.0, 0.0, yaw))
    actor.set_actor_label(label)
    actor.set_folder_path(folder)
    actor.tags = [TAG]
    return actor


def build_road():
    points = layout.ROAD_POINTS_M
    index = 0
    for (ax, ay), (bx, by) in zip(points, points[1:]):
        ax, ay, bx, by = ax * CM, ay * CM, bx * CM, by * CM
        length = math.hypot(bx - ax, by - ay)
        pieces = max(int(round(length / ROAD_PIECE)), 1)
        yaw = math.degrees(math.atan2(by - ay, bx - ax))
        for piece in range(pieces):
            start = piece / pieces
            end = (piece + 1) / pieces
            x0, y0 = ax + (bx - ax) * start, ay + (by - ay) * start
            x1, y1 = ax + (bx - ax) * end, ay + (by - ay) * end
            z0, z1 = ground_z(x0, y0), ground_z(x1, y1)
            if z0 is None or z1 is None:
                continue
            piece_length = length / pieces
            pitch = math.degrees(math.atan2(z1 - z0, piece_length))
            put(cube, ROAD, ((x0 + x1) * 0.5, (y0 + y1) * 0.5, (z0 + z1) * 0.5 - 5.0 + 3.0 * (index % 2)), (piece_length + 60.0, ROAD_WIDTH, 30.0),
                "Road_%03d" % index, unreal.Rotator(0.0, pitch, yaw), "Field/Road")
            index += 1


def building(prefix, cx, cy, width, depth, height, material, door_width=300.0, folder="Field/Buildings"):
    """Коробка с дверным проёмом на южной стороне и крышей. cx, cy — центр, см; пол — на высоте 0 (ровная площадка)."""
    half_w, half_d = width * 0.5, depth * 0.5
    z = height * 0.5
    put(cube, material, (cx, cy + half_d, z), (width, WALL, height), prefix + "_N", folder=folder)
    put(cube, material, (cx - half_w, cy, z), (WALL, depth - WALL, height), prefix + "_W", folder=folder)
    put(cube, material, (cx + half_w, cy, z), (WALL, depth - WALL, height), prefix + "_E", folder=folder)
    side = (width - door_width) * 0.5
    offset = (door_width + side) * 0.5
    put(cube, material, (cx - offset, cy - half_d, z), (side, WALL, height), prefix + "_S1", folder=folder)
    put(cube, material, (cx + offset, cy - half_d, z), (side, WALL, height), prefix + "_S2", folder=folder)
    put(cube, CONCRETE, (cx, cy, height + 15.0), (width + 60.0, depth + 60.0, 30.0), prefix + "_Roof", folder=folder)


def crate(label, x, y, table, slots):
    box = place(unreal.RubezhLootContainer, (x, y, 50.0), label, folder="Field/Loot")
    box.set_editor_property("LootTable", table)
    box.set_editor_property("SlotCount", slots)
    return box


def build_village():
    cx, cy = layout.VILLAGE_M[0] * CM, layout.VILLAGE_M[1] * CM
    houses = [(-3000, 1800), (-3200, -1600), (-800, 3000), (2600, 2600), (3200, -600), (800, -3200)]
    for index, (dx, dy) in enumerate(houses):
        building("Village_House_%d" % index, cx + dx, cy + dy, 900.0, 700.0, 320.0, BRICK)
        crate("Village_Crate_%d" % index, cx + dx, cy + dy, "medical" if index == 3 else "civil", 8)


def build_factory():
    cx, cy = layout.FACTORY_M[0] * CM, layout.FACTORY_M[1] * CM
    building("Factory_Hall", cx, cy + 2500, 6000.0, 3000.0, 1400.0, CONCRETE, door_width=1000.0)
    building("Factory_Office", cx - 5000, cy - 2000, 2000.0, 1200.0, 900.0, BRICK)
    building("Factory_Store", cx + 4500, cy - 2500, 2400.0, 1400.0, 700.0, RUST, door_width=600.0)
    put(cylinder, BRICK, (cx + 4500, cy + 4500, 2500.0), (600.0, 600.0, 5000.0), "Factory_Chimney", folder="Field/Buildings")
    for index, (dx, dy) in enumerate([(-1500, -3500), (800, -4000), (-3000, 300), (3000, 600), (1500, -2000)]):
        put(cube, RUST, (cx + dx, cy + dy, 130.0), (600.0, 240.0, 260.0), "Factory_Container_%d" % index, unreal.Rotator(0.0, 0.0, 25.0 * index), "Field/Buildings")
    crate("Factory_Crate_Hall_0", cx - 2000, cy + 2500, "military", 12)
    crate("Factory_Crate_Hall_1", cx + 2000, cy + 3300, "military", 12)
    crate("Factory_Crate_Office", cx - 5000, cy - 2000, "medical", 8)
    crate("Factory_Crate_Store", cx + 4500, cy - 2500, "civil", 10)


def is_clear(x_m, y_m, margin):
    for (cx, cy), radius in layout.FLAT_AREAS_M:
        if math.hypot(x_m - cx, y_m - cy) < radius + margin:
            return False
    for (ax, ay), (bx, by) in zip(layout.ROAD_POINTS_M, layout.ROAD_POINTS_M[1:]):
        dx, dy = bx - ax, by - ay
        t = max(0.0, min(1.0, ((x_m - ax) * dx + (y_m - ay) * dy) / (dx * dx + dy * dy)))
        if math.hypot(x_m - (ax + t * dx), y_m - (ay + t * dy)) < margin:
            return False
    half = (layout.LANDSCAPE_VERTICES - 1) * 0.5 - 5.0
    return abs(x_m) < half and abs(y_m) < half


def tree(rng, index, x_m, y_m):
    x, y = x_m * CM, y_m * CM
    z = ground_z(x, y)
    if z is None:
        return
    scale = rng.uniform(0.8, 1.4)
    if TREE_MESHES:
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z), unreal.Rotator(0.0, 0.0, rng.uniform(0.0, 360.0)))
        actor.static_mesh_component.set_static_mesh(unreal.load_asset(rng.choice(TREE_MESHES)))
        actor.set_actor_scale3d(unreal.Vector(scale, scale, scale))
        actor.set_actor_label("Tree_%03d" % index)
        actor.set_folder_path("Field/Forest")
        actor.tags = [TAG]
        return
    trunk_height, crown_height = 500.0 * scale, 900.0 * scale
    put(cylinder, TRUNK, (x, y, z + trunk_height * 0.5), (50.0 * scale, 50.0 * scale, trunk_height), "Tree_%03d_Trunk" % index, folder="Field/Forest")
    crown = put(cone, LEAVES, (x, y, z + trunk_height + crown_height * 0.5 - 60.0), (420.0 * scale, 420.0 * scale, crown_height), "Tree_%03d_Crown" % index, folder="Field/Forest")
    crown.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)


def bush(rng, index, x_m, y_m):
    x, y = x_m * CM, y_m * CM
    z = ground_z(x, y)
    if z is None:
        return
    size = rng.uniform(150.0, 300.0)
    actor = put(sphere, BUSH, (x, y, z + size * 0.3), (size, size, size * 0.7), "Bush_%03d" % index, folder="Field/Forest")
    actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    actor.static_mesh_component.set_editor_property("can_ever_affect_navigation", False)


def build_forest():
    rng = random.Random(SEED)
    index = 0
    for (cx, cy), radius, count in FOREST_PATCHES:
        placed = 0
        attempts = 0
        while placed < count and attempts < count * 5:
            attempts += 1
            angle, distance = rng.uniform(0.0, math.tau), radius * math.sqrt(rng.random())
            x_m, y_m = cx + math.cos(angle) * distance, cy + math.sin(angle) * distance
            if is_clear(x_m, y_m, 14.0):
                tree(rng, index, x_m, y_m)
                index += 1
                placed += 1
    half = (layout.LANDSCAPE_VERTICES - 1) * 0.5 - 30.0
    for index in range(BUSHES):
        x_m, y_m = rng.uniform(-half, half), rng.uniform(-half, half)
        if is_clear(x_m, y_m, 12.0):
            bush(rng, index, x_m, y_m)


def build_animals():
    for index, (kind, count, (x_m, y_m), respawn) in enumerate(SPAWNERS):
        z = ground_z(x_m * CM, y_m * CM) or 0.0
        spawner = place(unreal.RubezhAnimalSpawner, (x_m * CM, y_m * CM, z + 100.0), "Lair_%d_%s" % (index, kind), folder="Field/Animals")
        spawner.set_editor_property("AnimalId", kind)
        spawner.set_editor_property("Count", count)
        spawner.set_editor_property("RespawnSeconds", respawn)


def build_bot_starts():
    """Боты появляются только вне поселения (ARubezhSafeZone::FilterPlayerStarts)."""
    spots = [(layout.VILLAGE_M[0], layout.VILLAGE_M[1] + 25.0), (layout.VILLAGE_M[0] + 25.0, layout.VILLAGE_M[1]),
             (layout.FACTORY_M[0], layout.FACTORY_M[1] - 60.0), (layout.FACTORY_M[0] - 25.0, layout.FACTORY_M[1] - 60.0)]
    for index, (x_m, y_m) in enumerate(spots):
        place(unreal.LyraPlayerStart, (x_m * CM, y_m * CM, 100.0), "BotStart_%d" % index, folder="Field/Starts")


def build_navigation():
    half = (layout.LANDSCAPE_VERTICES - 1) * 0.5 * CM
    volume = place(unreal.NavMeshBoundsVolume, (0.0, 0.0, 1500.0), "Field_NavBounds", folder="Field")
    origin, extent = volume.get_actor_bounds(False)
    if extent.x < 1.0:
        raise RuntimeError("у объёма навигации нет формы: создать его через MCP")
    volume.set_actor_scale3d(unreal.Vector(half / extent.x, half / extent.y, 6000.0 / extent.z))


if editor.get_editor_world().get_path_name().split(".")[0] != MAP:
    raise RuntimeError("открой " + MAP)

for actor in actors.get_all_level_actors():
    if TAG in [str(tag) for tag in actor.tags]:
        actors.destroy_actor(actor)

editor.get_editor_world().get_world_settings().set_editor_property("DefaultGameplayExperience", unreal.load_class(None, EXPERIENCE))
build_road()
build_village()
build_factory()
build_forest()
build_animals()
build_bot_starts()
build_navigation()
levels.save_current_level()
print("build_field_map: done, actors", len(actors.get_all_level_actors()))
