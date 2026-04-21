# Geodesic Engine CLI Playbook

<p style="font-family: 'Segoe UI', 'Inter', sans-serif; font-size: 15px; color: #334155; margin-top: -4px;">
Copy/paste guide for running Dijkstra, Heat, and Analytics directly from the C++ engine.
</p>

<p style="font-family: 'Segoe UI', 'Inter', sans-serif; font-size: 13px; color: #64748b;">
Project root: <code>/home/sething2002/Capstone_Project/Capstone_Project_III</code>
</p>

---

## <span style="color:#0f766e; font-family: 'Segoe UI', 'Inter', sans-serif;">1) Build The Engine</span>

```bash
cd /home/sething2002/Capstone_Project/Capstone_Project_III
cmake -S . -B build-engine -DBUILD_TESTING=OFF
cmake --build build-engine --target geodesic_engine -j
ls -l ./main
```

---

## <span style="color:#1d4ed8; font-family: 'Segoe UI', 'Inter', sans-serif;">2) Modes At A Glance</span>

| Mode | CLI Form | Output JSON | Use Case |
|---|---|---|---|
| <span style="color:#0f766e;"><strong>Dijkstra</strong></span> | `./main START END MODEL_PATH` | `frontend/public/result.json` | Edge-constrained shortest path |
| <span style="color:#b45309;"><strong>Heat</strong></span> | `./main START END MODEL_PATH heat` | `frontend/public/heat_result.json` | Mesh geodesic approximation |
| <span style="color:#7c3aed;"><strong>Analytics</strong></span> | `./main START END MODEL_PATH analytics` | `frontend/public/analytics.json` | Surface-specific analytic solver |

---

## <span style="color:#be123c; font-family: 'Segoe UI', 'Inter', sans-serif;">3) Find Models + Valid Vertex IDs</span>

```bash
# Show available OBJ files
ls -1 ./frontend/public/data/*.obj

# Count vertices (valid IDs are 0 to count-1)
grep -c '^v ' ./frontend/public/data/stanford-bunny.obj
grep -c '^v ' ./frontend/public/data/sphere.obj
```

---

## <span style="color:#0369a1; font-family: 'Segoe UI', 'Inter', sans-serif;">4) Single-Run Template</span>

<p style="font-family: 'Segoe UI', 'Inter', sans-serif; font-size: 14px; color: #334155;">
The first 3 lines create shell variables in your current terminal session.<br/>
In <strong>zsh/bash</strong>, <code>NAME=value</code> means "store this value under this name".<br/>
Then <code>$NAME</code> injects that value into later commands.
</p>

<p style="font-family: 'Segoe UI', 'Inter', sans-serif; font-size: 13px; color: #64748b;">
Important: no spaces around <code>=</code> (good: <code>START=0</code>, bad: <code>START = 0</code>).
</p>

```bash
START=0
END=100
MODEL=./frontend/public/data/stanford-bunny.obj

# Dijkstra
./main "$START" "$END" "$MODEL"

# Heat
./main "$START" "$END" "$MODEL" heat

# Analytics
./main "$START" "$END" "$MODEL" analytics
```

No-variable equivalent (literal values):

```bash
./main 0 100 ./frontend/public/data/stanford-bunny.obj
./main 0 100 ./frontend/public/data/stanford-bunny.obj heat
./main 0 100 ./frontend/public/data/stanford-bunny.obj analytics
```

---

## <span style="color:#374151; font-family: 'Segoe UI', 'Inter', sans-serif;">5) Ready Examples</span>

### Stanford Bunny

```bash
./main 0 100 ./frontend/public/data/stanford-bunny.obj
./main 0 100 ./frontend/public/data/stanford-bunny.obj heat
./main 0 100 ./frontend/public/data/stanford-bunny.obj analytics
```

### Sphere

```bash
./main 0 42 ./frontend/public/data/sphere.obj
./main 0 42 ./frontend/public/data/sphere.obj heat
./main 0 42 ./frontend/public/data/sphere.obj analytics
```

---

## <span style="color:#b45309; font-family: 'Segoe UI', 'Inter', sans-serif;">6) Quickest Detailed Method Comparison (One Model)</span>

```bash
MODEL=./frontend/public/data/sphere.obj
START=0
END=42

echo "=== Dijkstra ==="
./main "$START" "$END" "$MODEL"

echo "=== Heat ==="
./main "$START" "$END" "$MODEL" heat

echo "=== Analytics ==="
./main "$START" "$END" "$MODEL" analytics
```

---

## <span style="color:#065f46; font-family: 'Segoe UI', 'Inter', sans-serif;">7) Nitty-Gritty Batch Run (All OBJ Files, All Methods)</span>

```bash
for MODEL_PATH in ./frontend/public/data/*.obj; do
  VCOUNT=$(grep -c '^v ' "$MODEL_PATH")
  if [ "$VCOUNT" -lt 2 ]; then
    echo "Skipping $MODEL_PATH (less than 2 vertices)."
    continue
  fi

  START=0
  END=$((VCOUNT - 1))

  echo "=============================================="
  echo "Model: $MODEL_PATH"
  echo "Vertices: $VCOUNT | START=$START END=$END"

  ./main "$START" "$END" "$MODEL_PATH"
  ./main "$START" "$END" "$MODEL_PATH" heat
  ./main "$START" "$END" "$MODEL_PATH" analytics
done
```

---

## <span style="color:#6d28d9; font-family: 'Segoe UI', 'Inter', sans-serif;">8) Inspect JSON Outputs</span>

```bash
cat ./frontend/public/result.json
cat ./frontend/public/heat_result.json
cat ./frontend/public/analytics.json
```

Optional pretty-print with jq:

```bash
jq . ./frontend/public/result.json
jq . ./frontend/public/heat_result.json
jq . ./frontend/public/analytics.json
```

---

> <span style="color:#b45309;"><strong>Important:</strong></span>
> Analytics is surface-specific and may return an error on arbitrary meshes (for example, `stanford-bunny.obj`).
> Heat is the general-purpose method for arbitrary triangle meshes.
