from __future__ import annotations

from dataclasses import dataclass
from typing import Iterable, List, Sequence, Tuple


Vector3 = Tuple[float, float, float]
Face = Tuple[int, int, int]  # 1-based indices


@dataclass
class ObjMesh:
    vertices: List[Vector3]
    faces: List[Face]


def compact_mesh(vertices: Sequence[Vector3], faces: Sequence[Face]) -> ObjMesh:
    """Remove unreferenced vertices and reindex faces.

    Faces are expected to use 1-based indexing (OBJ convention).
    """
    if not vertices:
        return ObjMesh([], list(faces))

    used: set[int] = set()
    vcount = len(vertices)
    for a, b, c in faces:
        for idx in (a, b, c):
            if idx < 1 or idx > vcount:
                raise ValueError(
                    f"Face index {idx} out of range for {vcount} vertices",
                )
            used.add(idx)

    if not used:
        # No faces => keep vertices as-is.
        return ObjMesh(list(vertices), list(faces))

    used_sorted = sorted(used)
    remap: dict[int, int] = {old: new for new, old in enumerate(used_sorted, start=1)}
    new_vertices = [vertices[i - 1] for i in used_sorted]
    new_faces: List[Face] = [
        (remap[a], remap[b], remap[c])
        for (a, b, c) in faces
    ]

    return ObjMesh(new_vertices, new_faces)


def write_obj(
    path: str,
    vertices: Sequence[Vector3],
    faces: Sequence[Face],
    header: Iterable[str] | None = None,
    *,
    compact: bool = True,
) -> None:
    if compact:
        mesh = compact_mesh(vertices, faces)
        vertices = mesh.vertices
        faces = mesh.faces
    with open(path, "w", encoding="utf-8") as f:
        if header:
            for line in header:
                f.write(f"# {line}\n")
        for v in vertices:
            f.write(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
        for a, b, c in faces:
            f.write(f"f {a} {b} {c}\n")


__all__ = ["ObjMesh", "write_obj", "compact_mesh", "Vector3", "Face"]
