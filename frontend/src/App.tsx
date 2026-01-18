import { OrbitControls } from "@react-three/drei";
import { Canvas } from "@react-three/fiber";
import { Suspense, useCallback, useState } from "react";
import Axes from "../components/Axes";
import GeodesicMesh from "../components/GeodesicMesh";

const MODELS = [
    { name: "Icosahedron", file: "icosahedron.obj", up: "z" as const },
    { name: "Square", file: "zig_zag.obj", up: "z" as const },
    // Stanford bunny is commonly authored as Y-up.
    { name: "Bunny", file: "stanford-bunny.obj", up: "y" as const },
];

export default function App() {
    const [modelFile, setModelFile] = useState(MODELS[0].file);
    const [startId, setStartId] = useState(0);
    const [endId, setEndId] = useState(11);
    const [loading, setLoading] = useState(false);
    const [version, setVersion] = useState(0);
    const [vertexCount, setVertexCount] = useState<number | null>(null);
    const [showAxes, setShowAxes] = useState(true);

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

    const selectedModel = MODELS.find((m) => m.file === modelFile) ?? MODELS[0];

    // Make a call to the /compute endpoint
    const handleCompute = async () => {
        setLoading(true);
        const data = {
            start: startId,
            end: endId,
            model: modelFile,
        };
        try {
            const response = await fetch("http://localhost:8080/compute", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify(data),
            });

            if (response.ok) {
                // trigger a re-render in GeodesicMesh by updating a key or ref
                setVersion((v) => v + 1); // triggers the useEffect in GeoedesicMesh
                setLoading(false);
            } else {
                console.error("ERROR Response: ", response);
            }
        } catch (error) {
            console.error("Failed to send POST: ", error);
        }
    };

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
                <h3 style={{ margin: "0 0 10px 0" }}>Geodesic Lab</h3>
                <label>Select Model: </label>
                <select
                    value={modelFile}
                    onChange={(e) => {
                        setModelFile(e.target.value);
                        setVersion(0); // hides the red line
                        setStartId(0); // reset inputs
                        setEndId(1);
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

                <div style={{ marginTop: "10px" }}>
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

                <div style={{ marginTop: "10px" }}>
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

                <div style={{ marginTop: "10px" }}>
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
            </div>

            {/* 3D Canvas */}
            {/* Use Z-up so OBJ files with z=0 lie on the ground (XY plane). */}
            <Canvas camera={{ position: [2, 2, 1.2], up: [0, 0, 1] }}>
                <ambientLight intensity={0.7} />
                <pointLight position={[10, 10, 10]} />
                {showAxes && <Axes size={2} />}
                <Suspense fallback={null}>
                    <GeodesicMesh
                        modelPath={`/data/${modelFile}`}
                        version={version}
                        startId={startId}
                        endId={endId}
                        modelUp={selectedModel.up}
                        onVertexCountChange={handleVertexCountChange}
                    />
                </Suspense>
                <OrbitControls makeDefault target={[0, 0, 0]} />
            </Canvas>
        </div>
    );
}

// https://github.com/alecjacobson/common-3d-test-models/tree/master

// Simple styles (move to CSS later)
const uiContainerStyle: React.CSSProperties = {
    position: "absolute",
    top: 20,
    left: 20,
    zIndex: 10,
    background: "rgba(0,0,0,0.85)",
    padding: 20,
    borderRadius: 8,
    color: "white",
};
const inputStyle: React.CSSProperties = {
    background: "#333",
    color: "white",
    border: "1px solid #555",
    padding: "5px",
    width: "100%",
};
const buttonStyle: React.CSSProperties = {
    marginTop: "15px",
    width: "100%",
    padding: "10px",
    background: "#f00",
    color: "white",
    border: "none",
    cursor: "pointer",
    fontWeight: "bold",
};
