// Third-party imports
import { OrbitControls } from "@react-three/drei";
import { Canvas } from "@react-three/fiber";
import { Suspense, useCallback, useRef, useState } from "react";
import * as THREE from "three";
import type { OrbitControls as OrbitControlsImpl } from "three-stdlib";



// Import defined components
import Axes from "../components/Axes";
import GeodesicMesh from "../components/GeodesicMesh";



// Import styles
import { uiContainerStyle } from "./assets/styles/uiContainerStyle";
import { resetViewStyle } from "./assets/styles/resetViewStyle";
import { inputStyle } from "./assets/styles/inputStyle";
import { buttonStyle } from "./assets/styles/buttonStyle";





// Model options (name + corresponding OBJ file in public/data)
import { MODELS } from "./assets/MODELS";




// Import types 
import type { DijkstraJson } from "./types/DijkstraJson";
import type { AnalyticsJson } from "./types/AnalyticsJson";


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
    const [dijkstraTime, setDijkstraTime] = useState<number | null>(null);
    const [analyticsTime, setAnalyticsTime] = useState<number | null>(null);
    // const [heatLength, setHeatLength] = useState<number | null>(null);
    const [showDijkstraPath, setShowDijkstraPath] = useState(true);
    const [showAnalyticsPath, setShowAnalyticsPath] = useState(true);
    const [showHeatPath, setShowHeatPath] = useState(true);
    const [showMesh, setShowMesh] = useState(true);
    const [showSmoothSurface, setShowSmoothSurface] = useState(true);

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
        setDijkstraTime(null);
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
                if (Number.isFinite(payload.elapsedMs)) {
                    setDijkstraTime(payload.elapsedMs ?? null);
                }
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
        setAnalyticsTime(null);
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
                if (Number.isFinite(payload.elapsedMs)) {
                    setAnalyticsTime(payload.elapsedMs ?? null);
                }
            } else {
                console.error("ERROR Response: ", response);
            }
        } catch (error) {
            console.error("Failed to send POST: ", error);
        } finally {
            setAnalyticsLoading(false);
        }
    };

   

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
                        setDijkstraTime(null);
                        setAnalyticsTime(null);
                        // setHeatLength(null);
                        setShowDijkstraPath(true);
                        setShowAnalyticsPath(true);
                        setShowHeatPath(true);
                        setShowMesh(true);
                        setShowSmoothSurface(true);
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
                    <div>
                        <span style={{ color: "#aaa" }}>Dijkstra Time:</span>{" "}
                        {dijkstraTime == null
                            ? "—"
                            : Number.isFinite(dijkstraTime)
                              ? `${dijkstraTime.toFixed(2)} ms`
                              : "—"}
                    </div>
                    <div>
                        <span style={{ color: "#aaa" }}>Analytics Time:</span>{" "}
                        {analyticsTime == null
                            ? "—"
                            : Number.isFinite(analyticsTime)
                              ? `${analyticsTime.toFixed(2)} ms`
                              : "—"}
                    </div>
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
                    <button
                        onClick={() => setShowMesh((v) => !v)}
                        style={{
                            ...buttonStyle,
                            marginTop: 0,
                            background: showMesh ? "#2aa1ff" : "#333",
                        }}
                    >
                        {showMesh ? "Hide Mesh" : "Show Mesh"}
                    </button>
                    <button
                        onClick={() => setShowSmoothSurface((v) => !v)}
                        style={{
                            ...buttonStyle,
                            marginTop: 0,
                            background: showSmoothSurface ? "#e0e0e0" : "#333",
                            color: showSmoothSurface ? "#111" : "#fff",
                        }}
                    >
                        {showSmoothSurface
                            ? "Hide Smooth Surface"
                            : "Show Smooth Surface"}
                    </button>
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
                        showMesh={showMesh}
                        showSmoothSurface={showSmoothSurface}
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
