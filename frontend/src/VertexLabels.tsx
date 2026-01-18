import { Html } from "@react-three/drei";

interface Props {
    positions: Float32Array;
}

export default function VertexLabels({ positions }: Props) {
    // We only show labels if there are a small number of vertices (e.g., < 100)
    // to avoid crashing the browser with the Bunny.
    console.log(positions);
    if (positions.length / 3 > 100) return null;

    const labels = [];
    for (let i = 0; i < positions.length / 3; i++) {
        labels.push(
            <Html
                key={i}
                position={[
                    positions[i * 3],
                    positions[i * 3 + 1],
                    positions[i * 3 + 2],
                ]}
            >
                <div
                    style={{
                        color: "white",
                        fontSize: "10px",
                        pointerEvents: "none",
                        background: "rgba(0,0,0,0.5)",
                        padding: "2px",
                    }}
                >
                    {i}
                </div>
            </Html>,
        );
    }
    return <>{labels}</>;
}
