"""Строит поселение на карте прототипа (запускать через Unreal MCP или Tools → Execute Python Script).

Повторный запуск безопасен: всё, что поставил этот скрипт (метка RubezhSettlement), сначала удаляется.
Что ставит: безопасную зону, стены с двумя КПП, четырёх торговцев и точки появления игроков.
Числа поселения меняются здесь. Серый бокс: модели зданий и NPC появятся позже.
"""
import unreal

MAP = "/RubezhCore/Maps/L_Rubezh_Proto"
TAG = "RubezhSettlement"
FOLDER = "Settlement"

# Поселение стоит у края поля, в стороне от укрытий: на точке (8000, 0), 36×36 м внутри стен.
CENTER_X, CENTER_Y = 8000.0, 0.0
HALF_SIZE = 1800.0
WALL_HEIGHT = 500.0
WALL_THICKNESS = 50.0
GATE_WIDTH = 500.0
BOOTH_SIZE = 250.0
BOOTH_HEIGHT = 300.0
ZONE_HALF_HEIGHT = 1000.0
START_Z = 100.0

TRADERS = [
    ("Gunsmith", unreal.RubezhTraderKind.GUNSMITH, 1100.0, 1100.0),
    ("Medic", unreal.RubezhTraderKind.MEDIC, 1100.0, -1100.0),
    ("Armorer", unreal.RubezhTraderKind.ARMORER, -1100.0, -1100.0),
    ("Scavenger", unreal.RubezhTraderKind.SCAVENGER, 1100.0, 0.0),
]

# Смещения от центра: игроки появляются лицом к западному КПП.
STARTS = [(0.0, 300.0), (0.0, -300.0), (300.0, 500.0), (300.0, -500.0)]
START_YAW = 180.0

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
cube = unreal.load_asset("/Engine/BasicShapes/Cube")
material = unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial")


def spawn(actor_class, x, y, z, label, yaw=0.0):
    actor = actors.spawn_actor_from_class(actor_class, unreal.Vector(CENTER_X + x, CENTER_Y + y, z), unreal.Rotator(0.0, 0.0, yaw))
    if not actor:
        raise RuntimeError("cannot spawn " + label)
    actor.set_actor_label(label)
    actor.set_folder_path(FOLDER)
    actor.tags = [TAG]
    return actor


def box(label, x, y, size_x, size_y, size_z):
    """Серый блок: x, y — центр от середины поселения, стоит на земле. Куб движка — 100 см."""
    actor = spawn(unreal.StaticMeshActor, x, y, size_z * 0.5, label)
    actor.static_mesh_component.set_static_mesh(cube)
    actor.static_mesh_component.set_material(0, material)
    actor.set_actor_scale3d(unreal.Vector(size_x / 100.0, size_y / 100.0, size_z / 100.0))
    return actor


def wall_with_gate(label, along_x, offset):
    """Стена на расстоянии offset от центра; посередине проход.
    Углы занимают стены вдоль X, а стены вдоль Y упираются в них: иначе в углах блоки пересекаются и мерцают."""
    half_length = HALF_SIZE + WALL_THICKNESS if along_x else HALF_SIZE
    length = half_length - GATE_WIDTH * 0.5
    shift = GATE_WIDTH * 0.5 + length * 0.5
    for side, sign in (("A", 1.0), ("B", -1.0)):
        if along_x:
            box("%s_%s" % (label, side), sign * shift, offset, length, WALL_THICKNESS, WALL_HEIGHT)
        else:
            box("%s_%s" % (label, side), offset, sign * shift, WALL_THICKNESS, length, WALL_HEIGHT)


def solid_wall(label, along_x, offset):
    if along_x:
        box(label, 0.0, offset, (HALF_SIZE + WALL_THICKNESS) * 2.0, WALL_THICKNESS, WALL_HEIGHT)
    else:
        box(label, offset, 0.0, WALL_THICKNESS, HALF_SIZE * 2.0, WALL_HEIGHT)


levels.load_level(MAP)

for actor in actors.get_all_level_actors():
    if TAG in [str(tag) for tag in actor.tags]:
        actors.destroy_actor(actor)

zone = spawn(unreal.RubezhSafeZone, 0.0, 0.0, ZONE_HALF_HEIGHT * 0.5, "Settlement_SafeZone")
zone.set_editor_property("HalfSizeCm", unreal.Vector(HALF_SIZE, HALF_SIZE, ZONE_HALF_HEIGHT))

wall_offset = HALF_SIZE + WALL_THICKNESS * 0.5
wall_with_gate("Settlement_WallWest", False, -wall_offset)
wall_with_gate("Settlement_WallNorth", True, wall_offset)
solid_wall("Settlement_WallEast", False, wall_offset)
solid_wall("Settlement_WallSouth", True, -wall_offset)

# Будка часового снаружи у прохода: так КПП видно издалека.
booth_out = HALF_SIZE + WALL_THICKNESS + BOOTH_SIZE * 0.5
booth_side = GATE_WIDTH * 0.5 + BOOTH_SIZE * 0.5 + WALL_THICKNESS
box("Settlement_Checkpoint_West", -booth_out, booth_side, BOOTH_SIZE, BOOTH_SIZE, BOOTH_HEIGHT)
box("Settlement_Checkpoint_North", booth_side, booth_out, BOOTH_SIZE, BOOTH_SIZE, BOOTH_HEIGHT)

for name, kind, x, y in TRADERS:
    trader = spawn(unreal.RubezhTrader, x, y, 0.0, "Settlement_Trader_" + name)
    trader.set_editor_property("Kind", kind)

for index, (x, y) in enumerate(STARTS):
    spawn(unreal.LyraPlayerStart, x, y, START_Z, "Settlement_Start_%d" % index, START_YAW)

levels.save_current_level()
print("build_settlement: done")
