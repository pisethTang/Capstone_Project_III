# Geodesic Engine

C++ engine that computes shortest-path approximations on 3D meshes using Dijkstra, the Heat Method, and analytic geodesic solvers for parametric surfaces.

## Build

```bash
cmake -S . -B build -DBUILD_TESTING=OFF
cmake --build build --target geodesic_engine -j
```

The binary is written to `./main`.

## Run

```bash
# Dijkstra (shortest path along mesh edges)
./main <start_id> <end_id> <model_path>

# Heat Method (mesh geodesic approximation)
./main <start_id> <end_id> <model_path> heat

# Analytics (surface-specific analytic solver)
./main <start_id> <end_id> <model_path> analytics
```

Example:

```bash
./main 0 100 ./assets/models/stanford-bunny.obj
```

Results are written as JSON to the current working directory (`result.json`, `heat_result.json`, `analytics.json`).

## Test

```bash
cmake -S . -B build-tests
cmake --build build-tests -j$(nproc)
ctest --test-dir build-tests --output-on-failure
```

## Models

Sample OBJ files live in `assets/models/`. You can generate higher-resolution variants with:

```bash
python -m tools.meshgen.generate --out ./assets/models
```

## Algorithms

### 1) Dijkstra on the Mesh Graph

Treats mesh vertices as nodes and edges as graph edges. Edge weights are Euclidean distances between adjacent vertices.

### 2) Heat Method

For general triangle meshes, solves:
1. Heat equation for a short time $t$
2. Normalizes the temperature gradient to get vector field $X$
3. Solves Poisson equation $\Delta \phi = \nabla\cdot X$
4. $\phi$ approximates geodesic distance; follow its gradient to extract a path

### 3) Analytic Geodesics

Closed-form or ODE solutions for special parametric surfaces (plane, sphere, torus, saddle).

## References

[1] [Crane, K., Weischedel, C. and Wardetzky, M. (2013). _Geodesics in heat: A new approach to computing distance based on heat flow_. ACM Transactions on Graphics, 32(5), pp.1–11.](https://www.cs.cmu.edu/~kmcrane/Projects/HeatMethod/paperCACM.pdf)
