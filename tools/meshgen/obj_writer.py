from __future__ import annotations

from dataclasses import dataclass
from typing import Iterable, List, Sequence, Tuple


Vector3 = Tuple[float, float, float]
Face = Tuple[int, int, int]  # 1-based indices


@dataclass
class ObjMesh:
    vertices: List[Vector3]
    faces: List[Face]


def write_obj(path: str, vertices: Sequence[Vector3], faces: Sequence[Face], header: Iterable[str] | None = None) -> None:
    with open(path, "w", encoding="utf-8") as f:
        if header:
            for line in header:
                f.write(f"# {line}\n")
        for v in vertices:
            f.write(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
        for a, b, c in faces:
            f.write(f"f {a} {b} {c}\n")


__all__ = ["ObjMesh", "write_obj", "Vector3", "Face"]
