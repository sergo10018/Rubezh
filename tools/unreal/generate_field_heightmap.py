"""Рельеф тестовой карты 1×1 км: пишет сырую 16-битную карту высот (запускать обычным Python, не в редакторе).

Что получается: холмистое поле, края карты поднимаются холмами (естественная граница), под поселением,
деревней и заводом ровные площадки на высоте 0, вдоль дороги земля сглажена. Места — в field_layout.py.
Результат: Game/Saved/Field/field_height.r16 (в git не хранится, собирается заново).
"""
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import field_layout as layout

OUTPUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "Game", "Saved", "Field", "field_height.r16")

SEED = 7
ROLLING_HEIGHT_M = 7.0
EDGE_HILL_HEIGHT_M = 32.0
EDGE_HILL_START_M = 90.0  # столько метров от края занимает подъём
FLAT_BLEND_M = 60.0
ROAD_HALF_WIDTH_M = 7.0
ROAD_BLEND_M = 30.0
ROAD_HEIGHT_SHARE = 0.4  # дорога идёт по сглаженному рельефу: высоты вдоль неё сильно уменьшены
UNITS_PER_METER = 128.0  # масштаб ландшафта по высоте 100: одна единица = 1/128 м
SEA_LEVEL = 32768


def smooth_noise(size, cells, rng):
    """Сглаженный шум: случайная редкая сетка, растянутая на всю карту."""
    grid = rng.random((cells + 2, cells + 2))
    axis = np.linspace(0.0, cells, size)
    index = axis.astype(int)
    t = axis - index
    t = t * t * (3.0 - 2.0 * t)
    top = grid[index][:, index] * (1.0 - t) + grid[index][:, index + 1] * t
    bottom = grid[index + 1][:, index] * (1.0 - t) + grid[index + 1][:, index + 1] * t
    return top * (1.0 - t)[:, None] + bottom * t[:, None]


def rolling_hills(size, rng):
    total = np.zeros((size, size))
    for cells, weight in ((4, 1.0), (9, 0.45), (22, 0.18), (60, 0.05)):
        total += smooth_noise(size, cells, rng) * weight
    total -= total.mean()
    return total / np.abs(total).max() * ROLLING_HEIGHT_M


def smoothstep(values):
    values = np.clip(values, 0.0, 1.0)
    return values * values * (3.0 - 2.0 * values)


def distance_to_segment(x, y, a, b):
    ax, ay = a
    bx, by = b
    dx, dy = bx - ax, by - ay
    t = np.clip(((x - ax) * dx + (y - ay) * dy) / max(dx * dx + dy * dy, 1e-6), 0.0, 1.0)
    return np.hypot(x - (ax + t * dx), y - (ay + t * dy))


def flatten_areas(height, x, y, blend):
    for (cx, cy), radius in layout.FLAT_AREAS_M:
        distance = np.hypot(x - cx, y - cy)
        height = height * smoothstep((distance - radius) / blend)
    return height


def main():
    size = layout.LANDSCAPE_VERTICES
    half = (size - 1) * 0.5  # одна клетка — метр
    axis = np.linspace(-half, half, size)
    x, y = np.meshgrid(axis, axis)  # строки — Y, столбцы — X

    height = rolling_hills(size, np.random.default_rng(SEED))

    edge = np.minimum(half - np.abs(x), half - np.abs(y))
    height += EDGE_HILL_HEIGHT_M * smoothstep(1.0 - edge / EDGE_HILL_START_M)

    height = flatten_areas(height, x, y, FLAT_BLEND_M)

    road_distance = np.full_like(height, np.inf)
    for a, b in zip(layout.ROAD_POINTS_M, layout.ROAD_POINTS_M[1:]):
        road_distance = np.minimum(road_distance, distance_to_segment(x, y, a, b))
    near_road = 1.0 - smoothstep((road_distance - ROAD_HALF_WIDTH_M) / ROAD_BLEND_M)
    height = height * (1.0 - near_road * (1.0 - ROAD_HEIGHT_SHARE))

    # Площадки ещё раз: сглаживание у дороги не должно поднять их края.
    height = flatten_areas(height, x, y, FLAT_BLEND_M * 0.5)

    raw = np.clip(np.round(height * UNITS_PER_METER + SEA_LEVEL), 0, 65535).astype(np.uint16)
    os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
    raw.tofile(OUTPUT)
    print("heightmap: %d x %d, от %.1f до %.1f м -> %s" % (size, size, height.min(), height.max(), os.path.normpath(OUTPUT)))


if __name__ == "__main__":
    main()
