from __future__ import annotations

import argparse
from pathlib import Path

from .obj_writer import write_obj
from .primitives import plane_grid, saddle_grid, sphere_uv, torus


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate OBJ mesh primitives.")
    parser.add_argument("--out", type=Path, required=True, help="Output directory for OBJ files")

    parser.add_argument("--sphere-slices", type=int, default=64)
    parser.add_argument("--sphere-stacks", type=int, default=32)
    parser.add_argument("--sphere-radius", type=float, default=1.0)

    parser.add_argument("--torus-major", type=float, default=1.4)
    parser.add_argument("--torus-minor", type=float, default=0.45)
    parser.add_argument("--torus-seg-major", type=int, default=80)
    parser.add_argument("--torus-seg-minor", type=int, default=36)

    parser.add_argument("--saddle-size", type=float, default=1.2)
    parser.add_argument("--saddle-divisions", type=int, default=60)
    parser.add_argument("--saddle-height", type=float, default=0.6)

    parser.add_argument("--plane-size", type=float, default=1.4)
    parser.add_argument("--plane-divisions", type=int, default=64)

    return parser.parse_args()


def main() -> None:
    args = parse_args()
    out_dir: Path = args.out
    out_dir.mkdir(parents=True, exist_ok=True)

    sphere = sphere_uv(args.sphere_radius, args.sphere_slices, args.sphere_stacks)
    write_obj(
        str(out_dir / "sphere.obj"),
        sphere.vertices,
        sphere.faces,
        header=[
            "Generated sphere (UV)",
            f"slices={args.sphere_slices}",
            f"stacks={args.sphere_stacks}",
            f"radius={args.sphere_radius}",
        ],
    )

    # A deliberately low-res sphere to highlight differences between:
    # - Dijkstra on mesh edges (polyhedral approximation)
    # - Analytic great-circle on an ideal sphere
    sphere_low = sphere_uv(args.sphere_radius, 12, 6)
    write_obj(
        str(out_dir / "sphere_low.obj"),
        sphere_low.vertices,
        sphere_low.faces,
        header=[
            "Generated sphere (UV) - low resolution",
            "slices=12",
            "stacks=6",
            f"radius={args.sphere_radius}",
        ],
    )

    plane = plane_grid(args.plane_size, args.plane_divisions)
    write_obj(
        str(out_dir / "plane.obj"),
        plane.vertices,
        plane.faces,
        header=[
            "Generated plane grid (Z=0)",
            f"size={args.plane_size}",
            f"divisions={args.plane_divisions}",
        ],
    )

    torus_mesh = torus(
        args.torus_major,
        args.torus_minor,
        args.torus_seg_major,
        args.torus_seg_minor,
    )
    write_obj(
        str(out_dir / "donut.obj"),
        torus_mesh.vertices,
        torus_mesh.faces,
        header=[
            "Generated torus",
            f"major_radius={args.torus_major}",
            f"minor_radius={args.torus_minor}",
            f"segments_major={args.torus_seg_major}",
            f"segments_minor={args.torus_seg_minor}",
        ],
    )

    saddle = saddle_grid(args.saddle_size, args.saddle_divisions, args.saddle_height)
    write_obj(
        str(out_dir / "saddle.obj"),
        saddle.vertices,
        saddle.faces,
        header=[
            "Generated saddle z = h*(x^2 - y^2)",
            f"size={args.saddle_size}",
            f"divisions={args.saddle_divisions}",
            f"height={args.saddle_height}",
        ],
    )


if __name__ == "__main__":
    main()
