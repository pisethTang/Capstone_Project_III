from __future__ import annotations

import math
from typing import List, Tuple

from .obj_writer import Face, ObjMesh, Vector3


def sphere_uv(radius: float, slices: int, stacks: int) -> ObjMesh:
    vertices: List[Vector3] = []
    faces: List[Face] = []

    for i in range(stacks + 1):
        v = i / stacks
        phi = v * math.pi  # 0..pi
        sin_phi = math.sin(phi)
        cos_phi = math.cos(phi)
        for j in range(slices + 1):
            u = j / slices
            theta = u * 2.0 * math.pi
            sin_theta = math.sin(theta)
            cos_theta = math.cos(theta)
            x = radius * sin_phi * cos_theta
            y = radius * sin_phi * sin_theta
            z = radius * cos_phi
            vertices.append((x, y, z))

    def idx(i: int, j: int) -> int:
        return i * (slices + 1) + j + 1  # OBJ 1-based

    for i in range(stacks):
        for j in range(slices):
            a = idx(i, j)
            b = idx(i + 1, j)
            c = idx(i + 1, j + 1)
            d = idx(i, j + 1)
            if i != 0:
                faces.append((a, b, d))
            if i != stacks - 1:
                faces.append((b, c, d))

    return ObjMesh(vertices, faces)


def torus(radius_major: float, radius_minor: float, segments_major: int, segments_minor: int) -> ObjMesh:
    vertices: List[Vector3] = []
    faces: List[Face] = []

    for i in range(segments_major + 1):
        u = i / segments_major
        theta = u * 2.0 * math.pi
        cos_t = math.cos(theta)
        sin_t = math.sin(theta)
        for j in range(segments_minor + 1):
            v = j / segments_minor
            phi = v * 2.0 * math.pi
            cos_p = math.cos(phi)
            sin_p = math.sin(phi)
            x = (radius_major + radius_minor * cos_p) * cos_t
            y = (radius_major + radius_minor * cos_p) * sin_t
            z = radius_minor * sin_p
            vertices.append((x, y, z))

    def idx(i: int, j: int) -> int:
        return i * (segments_minor + 1) + j + 1

    for i in range(segments_major):
        for j in range(segments_minor):
            a = idx(i, j)
            b = idx(i + 1, j)
            c = idx(i + 1, j + 1)
            d = idx(i, j + 1)
            faces.append((a, b, d))
            faces.append((b, c, d))

    return ObjMesh(vertices, faces)


def saddle_grid(size: float, divisions: int, height: float = 1.0) -> ObjMesh:
    vertices: List[Vector3] = []
    faces: List[Face] = []

    step = (size * 2.0) / divisions
    for i in range(divisions + 1):
        y = -size + i * step
        for j in range(divisions + 1):
            x = -size + j * step
            z = height * (x * x - y * y)
            vertices.append((x, y, z))

    def idx(i: int, j: int) -> int:
        return i * (divisions + 1) + j + 1

    for i in range(divisions):
        for j in range(divisions):
            a = idx(i, j)
            b = idx(i + 1, j)
            c = idx(i + 1, j + 1)
            d = idx(i, j + 1)
            faces.append((a, b, d))
            faces.append((b, c, d))

    return ObjMesh(vertices, faces)


__all__ = ["sphere_uv", "torus", "saddle_grid"]
