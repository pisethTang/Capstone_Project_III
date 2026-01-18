import { OrbitControls } from "@react-three/drei";
import { Canvas } from "@react-three/fiber";
import { Suspense, useState } from "react";
import GeodesicMesh from "../components/GeodesicMesh";

const MODELS = [
    { name: "Icosahedron", file: "icosahedron.obj" },
    { name: "Square", file: "zig_zag.obj" },
    { name: "Bunny", file: "stanford-bunny.obj" },
];

export default function App() {
    const [modelFile, setModelFile] = useState(MODELS[0].file);
    const [startId, setStartId] = useState(0);
    const [endId, setEndId] = useState(11);
    const [loading, setLoading] = useState(false);
    const [version, setVersion] = useState(0);

    // Make a call to the /compute endpoint
    const handleCompute = async () => {
        setLoading(true);
        const response = await fetch("http://localhost:8080/compute", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({
                start: startId,
                end: endId,
                model: modelFile,
            }),
        });

        if (response.ok) {
            // trigger a re-render in GeodesicMesh by updating a key or ref
            setVersion(v => v + 1); // triggers the useEffect in GeoedesicMesh
            setLoading(false);
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
                    onChange={(e) => setModelFile(e.target.value)}
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
                    <label>Start ID: </label>
                    <input
                        type="number"
                        value={startId}
                        onChange={(e) => setStartId(Number(e.target.value))}
                        style={inputStyle}
                    ></input>
                </div>

                <div style={{ marginTop: "10px" }}>
                    <label>End ID: </label>
                    <input
                        type="number"
                        value={endId}
                        onChange={(e) => setEndId(Number(e.target.value))}
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
            <Canvas camera={{ position: [2, 2, 2] }}>
                <ambientLight intensity={0.7} />
                <pointLight position={[10, 10, 10]} />
                <Suspense fallback={null}>
                    <GeodesicMesh modelPath={`/data/${modelFile}`} version={version} />
                </Suspense>
                <OrbitControls makeDefault />
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
