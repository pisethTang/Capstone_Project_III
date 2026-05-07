# Mesh Generator

Generates higher-resolution OBJ meshes for the `assets/models` folder.

## Usage

```sh
python -m tools.meshgen.generate --out ./assets/models
```

## Parameters

You can tune resolution via CLI flags:

- `--sphere-slices` / `--sphere-stacks`
- `--torus-seg-major` / `--torus-seg-minor`
- `--saddle-divisions`

All output meshes use only `v` and `f` lines (triangles), so they work with the C++ loader.
