import { OrbitControls } from "@react-three/drei";
import { Canvas } from "@react-three/fiber";
import { Suspense, useCallback, useRef, useState } from "react";
import * as THREE from "three";
import type { OrbitControls as OrbitControlsImpl } from "three-stdlib";
import Axes from "../components/Axes";
import GeodesicMesh from "../components/GeodesicMesh";

const MODELS = [
    { name: "Icosahedron", file: "icosahedron.obj", up: "z" as const },
    { name: "Square", file: "zig_zag.obj", up: "z" as const },
    // Stanford bunny is commonly authored as Y-up.
    { name: "Bunny", file: "stanford-bunny.obj", up: "y" as const },
    { name: "Plane Grid", file: "plane.obj" },
    { name: "Sphere", file: "sphere.obj" },
    { name: "Sphere (Low Res)", file: "sphere_low.obj" },
    { name: "Torus", file: "donut.obj" },
    { name: "Saddle", file: "saddle.obj" }, // hyperbolic paraboloid (pringle-shaped)
];

type DijkstraJson = {
    inputFileName?: string;
    reachable?: boolean;
    totalDistance: number | null;
    path: number[];
    allDistances: number[];
};

type AnalyticsJson = {
    inputFileName?: string;
    startId: number;
    endId: number;
    surfaceType: string;
    error?: string;
    curves: Array<{
        name: string;
        length: number;
        points: number[][];
    }>;
};

type HeatJson = AnalyticsJson;

export default function App() {
    const [modelFile, setModelFile] = useState(MODELS[0].file);
    const [startId, setStartId] = useState(0);
    const [endId, setEndId] = useState(11);
    const [loading, setLoading] = useState(false);
    const [analyticsLoading, setAnalyticsLoading] = useState(false);
    // const [heatLoading, setHeatLoading] = useState(false);
    const [dijkstraData, setDijkstraData] = useState<DijkstraJson | null>(
        null,
    );
    const [analyticsData, setAnalyticsData] = useState<AnalyticsJson | null>(
        null,
    );
    const [heatData, setHeatData] = useState<HeatJson | null>(null);
    const [vertexCount, setVertexCount] = useState<number | null>(null);
    const [faceCount, setFaceCount] = useState<number | null>(null);
    const [showAxes, setShowAxes] = useState(true);

    const [highlightEnabled, setHighlightEnabled] = useState(false);
    const [highlightFace, setHighlightFace] = useState(0);

    const [totalDistance, setTotalDistance] = useState<number | null>(null);
    const [analyticsLength, setAnalyticsLength] = useState<number | null>(null);
    // const [heatLength, setHeatLength] = useState<number | null>(null);
    const [showDijkstraPath, setShowDijkstraPath] = useState(true);
    const [showAnalyticsPath, setShowAnalyticsPath] = useState(true);
    const [showHeatPath, setShowHeatPath] = useState(true);

    const apiBase =
        import.meta.env.VITE_API_BASE?.replace(/\/$/, "") ??
        "http://localhost:8080";

    const controlsRef = useRef<OrbitControlsImpl | null>(null);
    const cameraRef = useRef<THREE.PerspectiveCamera | null>(null);
    const defaultCameraPos = useRef(new THREE.Vector3(2, 2, 1.2));
    const defaultTarget = useRef(new THREE.Vector3(0, 0, 0));
    const resetAnimRef = useRef<number | null>(null);

    const clampId = (value: number) => {
        const asInt = Number.isFinite(value) ? Math.trunc(value) : 0;
        if (!vertexCount || vertexCount <= 0) return Math.max(0, asInt);
        return Math.max(0, Math.min(vertexCount - 1, asInt));
    };

    const handleVertexCountChange = useCallback((count: number) => {
        setVertexCount((prev) => (prev === count ? prev : count));

        // Clamp existing values if a different model loads.
        setStartId((prev) => {
            const asInt = Number.isFinite(prev) ? Math.trunc(prev) : 0;
            const next = Math.max(0, Math.min(count - 1, asInt));
            return next === prev ? prev : next;
        });
        setEndId((prev) => {
            const asInt = Number.isFinite(prev) ? Math.trunc(prev) : 0;
            const next = Math.max(0, Math.min(count - 1, asInt));
            return next === prev ? prev : next;
        });
    }, []);

    const handleMeshStatsChange = useCallback(
        (stats: { vertexCount: number; faceCount: number }) => {
            setVertexCount((prev) =>
                prev === stats.vertexCount ? prev : stats.vertexCount,
            );
            setFaceCount((prev) =>
                prev === stats.faceCount ? prev : stats.faceCount,
            );

            setHighlightFace((prev) => {
                const asInt = Number.isFinite(prev) ? Math.trunc(prev) : 0;
                if (stats.faceCount <= 0) return 0;
                const next = Math.max(0, Math.min(stats.faceCount - 1, asInt));
                return next === prev ? prev : next;
            });

            // Clamp existing start/end too (in case a model changes).
            setStartId((prev) => {
                const asInt = Number.isFinite(prev) ? Math.trunc(prev) : 0;
                const next = Math.max(
                    0,
                    Math.min(stats.vertexCount - 1, asInt),
                );
                return next === prev ? prev : next;
            });
            setEndId((prev) => {
                const asInt = Number.isFinite(prev) ? Math.trunc(prev) : 0;
                const next = Math.max(
                    0,
                    Math.min(stats.vertexCount - 1, asInt),
                );
                return next === prev ? prev : next;
            });
        },
        [],
    );

    const selectedModel = MODELS.find((m) => m.file === modelFile) ?? MODELS[0];

    // Make a call to the /compute endpoint
    const handleCompute = async () => {
        setLoading(true);
        setTotalDistance(null);
        setDijkstraData(null);
        const data = {
            start: startId,
            end: endId,
            model: modelFile,
        };
        try {
            const response = await fetch(`${apiBase}/compute`, {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify(data),
            });

            if (response.ok) {
                const payload = (await response.json()) as DijkstraJson;
                setDijkstraData(payload);
            } else {
                console.error("ERROR Response: ", response);
            }
        } catch (error) {
            console.error("Failed to send POST: ", error);
        } finally {
            setLoading(false);
        }
    };

    const handleAnalytics = async () => {
        setAnalyticsLoading(true);
        setAnalyticsLength(null);
        setAnalyticsData(null);
        const data = {
            start: startId,
            end: endId,
            model: modelFile,
        };
        try {
            const response = await fetch(`${apiBase}/analytics`, {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify(data),
            });

            if (response.ok) {
                const payload = (await response.json()) as AnalyticsJson;
                setAnalyticsData(payload);
            } else {
                console.error("ERROR Response: ", response);
            }
        } catch (error) {
            console.error("Failed to send POST: ", error);
        } finally {
            setAnalyticsLoading(false);
        }
    };

    // const handleHeat = async () => {
    //     setHeatLoading(true);
    //     setHeatLength(null);
    //     setHeatData(null);
    //     const data = {
    //         start: startId,
    //         end: endId,
    //         model: modelFile,
    //     };
    //     try {
    //         const response = await fetch(`${apiBase}/heat`, {
    //             method: "POST",
    //             headers: { "Content-Type": "application/json" },
    //             body: JSON.stringify(data),
    //         });

    //         if (response.ok) {
    //             const payload = (await response.json()) as HeatJson;
    //             setHeatData(payload);
    //         } else {
    //             console.error("ERROR Response: ", response);
    //         }
    //     } catch (error) {
    //         console.error("Failed to send POST: ", error);
    //     } finally {
    //         setHeatLoading(false);
    //     }
    // };

    const handleResetView = useCallback(() => {
        const camera = cameraRef.current;
        const controls = controlsRef.current;
        if (!camera || !controls) return;

        if (resetAnimRef.current != null) {
            cancelAnimationFrame(resetAnimRef.current);
            resetAnimRef.current = null;
        }

        const startPos = camera.position.clone();
        const startTarget = controls.target.clone();
        const endPos = defaultCameraPos.current.clone();
        const endTarget = defaultTarget.current.clone();
        const duration = 900;
        const startTime = performance.now();

        const easeOutCubic = (t: number) => 1 - Math.pow(1 - t, 3);

        const tick = (now: number) => {
            const elapsed = now - startTime;
            const t = Math.min(1, elapsed / duration);
            const k = easeOutCubic(t);

            camera.position.lerpVectors(startPos, endPos, k);
            camera.up.set(0, 0, 1);
            controls.target.lerpVectors(startTarget, endTarget, k);
            controls.update();

            if (t < 1) {
                resetAnimRef.current = requestAnimationFrame(tick);
            } else {
                resetAnimRef.current = null;
            }
        };

        resetAnimRef.current = requestAnimationFrame(tick);
    }, []);

    return (
        <div
            style={{
                width: "100vw",
                height: "100vh",
                background: "#111",
                position: "relative",
            }}
        >
            {/* UI Overlay */}
            <div style={uiContainerStyle}>
                <h3 style={{ margin: "0 0 6px 0" }}>Geodesic Lab</h3>
                <label>Select Model: </label>
                <select
                    value={modelFile}
                    onChange={(e) => {
                        setModelFile(e.target.value);
                        setDijkstraData(null); // hides the red line
                        setAnalyticsData(null); // hides the yellow line
                        setHeatData(null); // hides the heat line
                        setStartId(0); // reset inputs
                        setEndId(1);
                        setHighlightEnabled(false);
                        setHighlightFace(0);
                        setTotalDistance(null);
                        setAnalyticsLength(null);
                        // setHeatLength(null);
                        setShowDijkstraPath(true);
                        setShowAnalyticsPath(true);
                        setShowHeatPath(true);
                    }}
                    style={{
                        padding: "5px",
                        background: "#333",
                        color: "white",
                        border: "1px solid #555",
                    }}
                >
                    {MODELS.map((m) => (
                        <option key={m.file} value={m.file}>
                            {m.name}
                        </option>
                    ))}
                </select>

                <div style={{ marginTop: "8px" }}>
                    <button
                        onClick={() => setShowAxes((v) => !v)}
                        style={{
                            ...buttonStyle,
                            marginTop: 0,
                            background: showAxes ? "#333" : "#555",
                            padding: "8px",
                            fontWeight: 600,
                        }}
                    >
                        {showAxes ? "Hide Axes" : "Show Axes"}
                    </button>
                </div>

                <div style={{ marginTop: "8px", fontSize: 12, color: "#ddd" }}>
                    <div>
                        <span style={{ color: "#aaa" }}>Vertices:</span>{" "}
                        {vertexCount ?? "—"}
                    </div>
                    <div>
                        <span style={{ color: "#aaa" }}>Faces:</span>{" "}
                        {faceCount ?? "—"}
                    </div>
                    <div>
                        <span style={{ color: "#aaa" }}>
                            Dijkstra Distance:
                        </span>{" "}
                        {totalDistance == null
                            ? "—"
                            : Number.isFinite(totalDistance)
                              ? totalDistance.toFixed(6)
                              : "—"}
                    </div>
                    <div>
                        <span style={{ color: "#aaa" }}>Analytics Length:</span>{" "}
                        {analyticsLength == null
                            ? "—"
                            : Number.isFinite(analyticsLength)
                              ? analyticsLength.toFixed(6)
                              : "—"}
                    </div>
                    {/* <div>
                        <span style={{ color: "#aaa" }}>Heat Length:</span>{" "}
                        {heatLength == null
                            ? "—"
                            : Number.isFinite(heatLength)
                              ? heatLength.toFixed(6)
                              : "—"}
                    </div> */}
                </div>

                <div style={{ marginTop: "8px" }}>
                    <label>Start ID: </label>
                    <input
                        type="number"
                        value={startId}
                        min={0}
                        max={vertexCount ? vertexCount - 1 : undefined}
                        step={1}
                        onChange={(e) =>
                            setStartId(clampId(Number(e.target.value)))
                        }
                        style={inputStyle}
                    ></input>
                </div>

                <div style={{ marginTop: "8px" }}>
                    <label>End ID: </label>
                    <input
                        type="number"
                        value={endId}
                        min={0}
                        max={vertexCount ? vertexCount - 1 : undefined}
                        step={1}
                        onChange={(e) =>
                            setEndId(clampId(Number(e.target.value)))
                        }
                        style={inputStyle}
                    ></input>
                </div>

                <button
                    onClick={handleCompute}
                    disabled={loading}
                    style={buttonStyle}
                >
                    {loading ? "Computing ..." : "Run Dijkstra"}
                </button>

                <button
                    onClick={handleAnalytics}
                    disabled={analyticsLoading}
                    style={{
                        ...buttonStyle,
                        marginTop: 8,
                        background: "#c8b400",
                        color: "#111",
                    }}
                >
                    {analyticsLoading ? "Computing ..." : "Run Analytics"}
                </button>

                {/* <button
                    onClick={handleHeat}
                    disabled={heatLoading}
                    style={{
                        ...buttonStyle,
                        marginTop: 8,
                        background: "#00bcd4",
                        color: "#0b1114",
                    }}
                >
                    {heatLoading ? "Computing ..." : "Run Heat Method"}
                </button> */}

                <div
                    style={{
                        marginTop: 10,
                        paddingTop: 10,
                        borderTop: "1px solid rgba(255,255,255,0.12)",
                        display: "grid",
                        gap: 8,
                    }}
                >
                    <button
                        onClick={() => setShowDijkstraPath((v) => !v)}
                        style={{
                            ...buttonStyle,
                            marginTop: 0,
                            background: showDijkstraPath ? "#d22525" : "#333",
                        }}
                    >
                        {showDijkstraPath
                            ? "Hide Dijkstra Path"
                            : "Show Dijkstra Path"}
                    </button>
                    <button
                        onClick={() => setShowAnalyticsPath((v) => !v)}
                        style={{
                            ...buttonStyle,
                            marginTop: 0,
                            background: showAnalyticsPath ? "#c8b400" : "#333",
                            color: showAnalyticsPath ? "#111" : "#fff",
                        }}
                    >
                        {showAnalyticsPath
                            ? "Hide Analytics Path"
                            : "Show Analytics Path"}
                    </button>
                    {/* <button
                        onClick={() => setShowHeatPath((v) => !v)}
                        style={{
                            ...buttonStyle,
                            marginTop: 0,
                            background: showHeatPath ? "#00bcd4" : "#333",
                            color: showHeatPath ? "#0b1114" : "#fff",
                        }}
                    >
                        {showHeatPath ? "Hide Heat Path" : "Show Heat Path"}
                    </button> */}
                </div>

                <div
                    style={{
                        marginTop: 10,
                        paddingTop: 10,
                        borderTop: "1px solid rgba(255,255,255,0.12)",
                    }}
                >
                    <div
                        style={{
                            display: "flex",
                            alignItems: "center",
                            gap: 8,
                        }}
                    >
                        <input
                            type="checkbox"
                            checked={highlightEnabled}
                            onChange={(e) =>
                                setHighlightEnabled(e.target.checked)
                            }
                        />
                        <label style={{ fontWeight: 600 }}>
                            Highlight face
                        </label>
                    </div>

                    <div style={{ marginTop: 8 }}>
                        <label>Face ID: </label>
                        <input
                            type="number"
                            value={highlightFace}
                            min={0}
                            max={faceCount ? faceCount - 1 : undefined}
                            step={1}
                            onChange={(e) => {
                                const raw = Number(e.target.value);
                                const asInt = Number.isFinite(raw)
                                    ? Math.trunc(raw)
                                    : 0;
                                if (!faceCount || faceCount <= 0) {
                                    setHighlightFace(Math.max(0, asInt));
                                } else {
                                    setHighlightFace(
                                        Math.max(
                                            0,
                                            Math.min(faceCount - 1, asInt),
                                        ),
                                    );
                                }
                            }}
                            style={inputStyle}
                            disabled={!faceCount || faceCount <= 0}
                        ></input>
                    </div>

                    <div style={{ marginTop: 8, display: "flex", gap: 8 }}>
                        <button
                            onClick={() => setHighlightEnabled(true)}
                            style={{
                                ...buttonStyle,
                                marginTop: 0,
                                background: "#d4a100",
                            }}
                            disabled={!faceCount || faceCount <= 0}
                        >
                            Show
                        </button>
                        <button
                            onClick={() => setHighlightEnabled(false)}
                            style={{
                                ...buttonStyle,
                                marginTop: 0,
                                background: "#333",
                            }}
                        >
                            Clear
                        </button>
                    </div>
                </div>
            </div>

            <button onClick={handleResetView} style={resetViewStyle}>
                Reset View
            </button>

            {/* 3D Canvas */}
            {/* Use Z-up so OBJ files with z=0 lie on the ground (XY plane). */}
            <Canvas
                camera={{ position: [2, 2, 1.2], up: [0, 0, 1] }}
                onCreated={({ camera }) => {
                    cameraRef.current = camera as THREE.PerspectiveCamera;
                }}
            >
                <ambientLight intensity={0.7} />
                <pointLight position={[10, 10, 10]} />
                {showAxes && <Axes size={2} />}
                <Suspense fallback={null}>
                    <GeodesicMesh
                        modelPath={`/data/${modelFile}`}
                        dijkstraData={dijkstraData}
                        analyticsData={analyticsData}
                        heatData={heatData}
                        startId={startId}
                        endId={endId}
                        modelUp={selectedModel.up}
                        showDijkstraPath={showDijkstraPath}
                        showAnalyticsPath={showAnalyticsPath}
                        showHeatPath={showHeatPath}
                        onVertexCountChange={handleVertexCountChange}
                        onMeshStatsChange={handleMeshStatsChange}
                        highlightFaceIndex={
                            highlightEnabled ? highlightFace : null
                        }
                        onDijkstraResultChange={(res) =>
                            setTotalDistance(res?.totalDistance ?? null)
                        }
                        onAnalyticsResultChange={(res) =>
                            setAnalyticsLength(res?.totalLength ?? null)
                        }
                        // onHeatResultChange={(res) =>
                        //     setHeatLength(res?.totalLength ?? null)
                        // }
                    />
                </Suspense>
                <OrbitControls
                    makeDefault
                    target={[0, 0, 0]}
                    ref={controlsRef}
                />
            </Canvas>
        </div>
    );
}

// https://github.com/alecjacobson/common-3d-test-models/tree/master

// Simple styles (move to CSS later)
const uiContainerStyle: React.CSSProperties = {
    position: "absolute",
    top: 12,
    left: 12,
    zIndex: 10,
    background: "rgba(0,0,0,0.85)",
    padding: 14,
    borderRadius: 8,
    color: "white",
    fontSize: 13,
    lineHeight: 1.2,
    maxHeight: "calc(100vh - 24px)",
};
const inputStyle: React.CSSProperties = {
    background: "#333",
    color: "white",
    border: "1px solid #555",
    padding: "4px",
    width: "100%",
};
const buttonStyle: React.CSSProperties = {
    marginTop: "10px",
    width: "100%",
    padding: "8px",
    background: "#f00",
    color: "white",
    border: "none",
    cursor: "pointer",
    fontWeight: "bold",
};

const resetViewStyle: React.CSSProperties = {
    position: "absolute",
    top: 12,
    right: 12,
    zIndex: 11,
    padding: "8px 12px",
    background: "rgba(47, 92, 255, 0.92)",
    color: "#fff",
    border: "none",
    borderRadius: 6,
    cursor: "pointer",
    fontWeight: 700,
    boxShadow: "0 6px 16px rgba(0,0,0,0.35)",
};
