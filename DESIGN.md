# Geodesic Lab — Design Document

## 1. Problem Statement

Geodesic Lab is a teaching and research tool for computing and visualizing geodesics on well-known parametric surfaces in 3D Euclidean space.

Given:
- A parametric surface `S(u, v)`.
- Two points `p, q` on that surface.

Compute and render the geodesic curve `γ(t)` on `S` that connects `p` and `q`.

The project demonstrates both:
- **Analytic geometry**: closed-form geodesics for simple surfaces.
- **Numerical differential geometry**: solving the geodesic equation via ODE integration and shooting methods for surfaces without closed-form solutions.

This is intentionally narrower than the previous scope, which conflated graph shortest paths (Dijkstra), mesh-based approximate geodesics (Heat Method), and analytic surface geodesics. Those are different problems; this project focuses only on the last one.

## 2. Scope

### In Scope

- Parametric surfaces:
  - **Plane** `z = 0`
  - **Sphere** of radius `R`
  - **Cylinder** of radius `R`
  - **Torus** with major radius `R` and minor radius `r`
  - **Saddle** (hyperbolic paraboloid) `z = a(x² − y²)`

- Geodesic methods:
  - **Analytic**: closed-form formulas where available.
  - **RK4 IVP**: integrate the geodesic equation forward from a point and initial velocity.
  - **Shooting BVP**: search for the initial velocity that connects `p` to `q`.

- Output:
  - A sampled geodesic curve as a list of 3D points.
  - Curve length.
  - Computation time.
  - Error estimate when analytic ground truth is available.

### Out of Scope

- Geodesics on arbitrary triangle meshes.
- Dijkstra shortest paths on mesh edge graphs.
- Heat Method / Fast Marching on meshes.
- General Riemannian manifolds not described by an explicit parametric map `S(u,v)`.

## 3. System Architecture

```
┌─────────────────┐
│  React + Three  │  UI: surface/method selection, 3D rendering
│    .js Frontend │
└────────┬────────┘
         │ POST /geodesic
┌────────▼────────┐
│   Go API Server │  HTTP layer, validation, dispatch
└────────┬────────┘
         │ exec C++ engine or call shared library
┌────────▼────────┐
│   C++ Engine    │  Surface classes, Christoffel symbols,
│                 │  RK4 integrator, shooting solver
└─────────────────┘
```

The C++ engine returns JSON. The Go server may shell out to the engine (current design) or link against it as a shared library in a future refactor. For this design we keep the existing CLI/exec model but make the engine stateless: it reads parameters from CLI arguments or stdin and writes JSON to stdout.

## 4. Parametric Surface Abstraction

Each surface implements a common interface:

```cpp
struct ParametricSurface {
    // 3D position at parameter (u, v)
    virtual Vec3 eval(double u, double v) const = 0;

    // First fundamental form coefficients E, F, G
    virtual Metric2 firstFundamentalForm(double u, double v) const = 0;

    // Christoffel symbols Γ^k_ij in (u, v) coordinates
    virtual Christoffel2 christoffel(double u, double v) const = 0;

    // Parameter domain: (u_min, u_max, v_min, v_max)
    virtual Domain domain() const = 0;

    // Human-readable name
    virtual const char* name() const = 0;
};
```

Surfaces live in `src/surfaces/`. Christoffel symbols are derived analytically for each surface, not computed by finite differences.

### Surface Parameterizations

| Surface | Parametric form | Domain |
|---|---|---|
| Plane | `(u, v, 0)` | `[-1, 1] × [-1, 1]` |
| Sphere | `(R cos u cos v, R sin u cos v, R sin v)` | `[0, 2π) × [-π/2, π/2]` |
| Cylinder | `(R cos u, R sin u, v)` | `[0, 2π) × [-H, H]` |
| Torus | `((R + r cos v) cos u, (R + r cos v) sin u, r sin v)` | `[0, 2π) × [0, 2π)` |
| Saddle | `(u, v, a(u² − v²))` | `[-1, 1] × [-1, 1]` |

## 5. Geodesic Methods

### 5.1 Analytic Solutions

Used for verification and for surfaces where closed-form geodesics exist.

| Surface | Analytic geodesic | Notes |
|---|---|---|
| Plane | Straight line segment | Trivial ground truth |
| Sphere | Great-circle arc | Unique shortest unless antipodal |
| Cylinder | Helix or straight line | Found by unfolding to a plane |

### 5.2 RK4 Integration of the Geodesic Equation

State vector: `s = (u, v, du/dt, dv/dt)`.

ODE:
```
du/dt  = u'
dv/dt  = v'
du'/dt = -Γ^u_uu (u')² - 2 Γ^u_uv u' v' - Γ^u_vv (v')²
dv'/dt = -Γ^v_uu (u')² - 2 Γ^v_uv u' v' - Γ^v_vv (v')²
```

Integrate with RK4 from `t = 0` to `t = 1` (or arc-length parameter `s`).

### 5.3 Shooting Method for Two-Point Boundary Value Problems

Given `p = S(u0, v0)` and `q = S(u1, v1)`:

1. Guess initial velocity `(u'_0, v'_0)`.
2. RK4-integrate forward to obtain endpoint `(u(T), v(T))`.
3. Compute error `e = (u(T) - u1, v(T) - v1)`.
4. Use finite-difference Jacobian or Broyden update to adjust `(u'_0, v'_0)`.
5. Repeat until `||e|| < tolerance` or iteration limit reached.

Special cases:
- Sphere antipodal points: infinitely many geodesics; report non-uniqueness.
- Torus: multiple geodesics may exist; shooting returns one local solution.

## 6. C++ Engine CLI

```bash
./geodesic_engine \
    --surface sphere \
    --u0 0.0 --v0 0.0 \
    --u1 1.5708 --v1 0.0 \
    --method analytic \
    --samples 128
```

Output (stdout, JSON):

```json
{
  "surface": "sphere",
  "method": "analytic",
  "start": { "u": 0.0, "v": 0.0, "xyz": [1.0, 0.0, 0.0] },
  "end": { "u": 1.5708, "v": 0.0, "xyz": [0.0, 1.0, 0.0] },
  "samples": 128,
  "length": 1.5708,
  "elapsedMs": 0.012,
  "error": null,
  "curve": [
    [1.0, 0.0, 0.0],
    [0.9997, 0.0245, 0.0],
    ...
  ]
}
```

Error is non-null only when the method fails to converge or the input is invalid.

## 7. Go API

### Endpoint

`POST /geodesic`

Request body:

```json
{
  "surface": "sphere",
  "params": { "radius": 1.0 },
  "start": { "u": 0.0, "v": 0.0 },
  "end": { "u": 1.5708, "v": 0.0 },
  "method": "analytic",
  "samples": 128
}
```

Response: the JSON produced by the C++ engine, forwarded directly.

### Validation

- `surface` must be one of the supported names.
- `start` and `end` must lie inside the surface domain.
- `method` must be one of `analytic`, `rk4`, `shooting`.
- `analytic` is rejected for surfaces where no closed form is implemented.

## 8. Frontend

### UI Elements

- Surface selector (dropdown).
- Surface parameter editor (e.g., radius slider for sphere/torus, `a` slider for saddle).
- Start/end point editor:
  - Either `(u, v)` coordinate inputs, or
  - 3D click-to-pick on the rendered surface (stretch goal).
- Method selector (analytic / RK4 / shooting).
- Sample count slider.
- Render toggles: surface, geodesic curve, start/end markers, analytic overlay.
- Metrics panel: length, elapsed time, error vs. analytic (if applicable).

### 3D Rendering

- Surface: `THREE.ParametricGeometry` or custom `BufferGeometry` built from the parametric equation.
- Geodesic: `THREE.Line` or `THREE.LineLoop` through computed points.
- Start/end markers: small spheres.
- Optional: render the analytic geodesic alongside the numerical one for comparison.

## 9. Testing Strategy

### Unit Tests

- For each surface, verify that `firstFundamentalForm` and `christoffel` are consistent (e.g., Gauss equation for Gaussian curvature when applicable).
- Verify that analytic geodesics have the expected length on plane and sphere.

### Integration Tests

- RK4 integration from a known point and tangent direction should reproduce the analytic sphere great-circle up to a small tolerance.
- Shooting method on the sphere should converge to the analytic great-circle.
- RK4 step-size convergence test: halve `dt` and confirm error drops by the expected RK4 order.

### Regression Tests

- CLI produces valid JSON for every supported surface/method combination.
- Go API returns 400 for invalid parameters and 500 with a clean error message on engine failure.

## 10. Milestones

1. **M1 — Surface abstraction and sphere**
   - Implement `ParametricSurface` interface.
   - Implement sphere with analytic great-circle geodesic.
   - C++ engine CLI for sphere.

2. **M2 — RK4 integrator**
   - Implement generic RK4 for the geodesic equation.
   - Verify against sphere analytic solution.

3. **M3 — Shooting method**
   - Implement two-point BVP solver.
   - Test on sphere and cylinder.

4. **M4 — Additional surfaces**
   - Add plane, cylinder, torus, saddle.
   - Add analytic solutions for plane and cylinder.

5. **M5 — API and frontend**
   - Wire Go endpoints to the new engine.
   - Build React/Three.js UI for surface selection and rendering.

6. **M6 — Verification and documentation**
   - Add convergence and error tests.
   - Rewrite README and user guide.

## 11. Open Questions

1. Should the engine be a shared library linked into the Go server, or keep the CLI/exec model?
2. Should the frontend allow direct `(u,v)` editing, 3D point picking, or both?
3. Do we want to animate geodesic construction (e.g., show RK4 integration step by step)?
4. Should we keep any mesh-related code as a deprecated or comparison branch?
