import { Html } from "@react-three/drei";

export default function Axes({ size = 2 }: { size?: number }) {
    const labelOffset = size * 0.08;
    const fontSize = Math.max(10, Math.round(size * 8));

    return (
        <group>
            <axesHelper args={[size]} />
            <Html position={[size + labelOffset, 0, 0]} center>
                <div style={{ color: "#ff4d4d", fontSize, userSelect: "none" }}>
                    x
                </div>
            </Html>
            <Html position={[0, size + labelOffset, 0]} center>
                <div style={{ color: "#4dff4d", fontSize, userSelect: "none" }}>
                    y
                </div>
            </Html>
            <Html position={[0, 0, size + labelOffset]} center>
                <div style={{ color: "#4d7dff", fontSize, userSelect: "none" }}>
                    z
                </div>
            </Html>
        </group>
    );
}
