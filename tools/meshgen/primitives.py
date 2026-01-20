from __future__ import annotations

import math
from typing import List, Tuple

from .obj_writer import Face, ObjMesh, Vector3


def sphere_uv(radius: float, slices: int, stacks: int) -> ObjMesh:
    """Generate a UV sphere without seam/pole duplicate vertices.

    The previous implementation created (slices+1) duplicates at each ring and
    then selectively omitted triangles at the poles, which can leave at least
    one vertex never referenced by any face (e.g., OBJ vertex 1).

    This version creates:
    - 1 top pole vertex
    - (stacks-1) latitude rings, each with exactly `slices` vertices
    - 1 bottom pole vertex
    and uses modulo wrap-around for connectivity.
    """

    if slices < 3:
        raise ValueError("slices must be >= 3")
    if stacks < 2:
        raise ValueError("stacks must be >= 2")

    vertices: List[Vector3] = []
    faces: List[Face] = []

    # Top pole
    vertices.append((0.0, 0.0, radius))

    # Rings (exclude poles)
    for i in range(1, stacks):
        v = i / stacks
        phi = v * math.pi  # (0, pi)
        sin_phi = math.sin(phi)
        cos_phi = math.cos(phi)
        for j in range(slices):
            u = j / slices
            theta = u * 2.0 * math.pi
            sin_theta = math.sin(theta)
            cos_theta = math.cos(theta)
            x = radius * sin_phi * cos_theta
            y = radius * sin_phi * sin_theta
            z = radius * cos_phi
            vertices.append((x, y, z))

    # Bottom pole
    vertices.append((0.0, 0.0, -radius))

    top = 1  # OBJ 1-based
    bottom = len(vertices)

    def ring_index(ring: int, j: int) -> int:
        """ring in [0, stacks-2] (there are stacks-1 rings), j in [0, slices-1]."""
        return 2 + ring * slices + (j % slices)

    ring_count = stacks - 1

    # Top cap: top + first ring
    if ring_count >= 1:
        for j in range(slices):
            a = top
            b = ring_index(0, j)
            c = ring_index(0, j + 1)
            faces.append((a, b, c))

    # Middle quads between rings
    for ring in range(ring_count - 1):
        for j in range(slices):
            a = ring_index(ring, j)
            b = ring_index(ring + 1, j)
            c = ring_index(ring + 1, j + 1)
            d = ring_index(ring, j + 1)
            faces.append((a, b, d))
            faces.append((b, c, d))

    # Bottom cap: last ring + bottom
    if ring_count >= 1:
        last_ring = ring_count - 1
        for j in range(slices):
            a = ring_index(last_ring, j)
            b = bottom
            c = ring_index(last_ring, j + 1)
            faces.append((a, b, c))

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


def plane_grid(size: float, divisions: int) -> ObjMesh:
    """Generate a triangulated square grid in the XY plane (Z=0)."""
    vertices: List[Vector3] = []
    faces: List[Face] = []

    if divisions < 1:
        raise ValueError("divisions must be >= 1")

    step = (size * 2.0) / divisions
    for i in range(divisions + 1):
        y = -size + i * step
        for j in range(divisions + 1):
            x = -size + j * step
            vertices.append((x, y, 0.0))

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


__all__ = ["sphere_uv", "torus", "saddle_grid", "plane_grid"]
