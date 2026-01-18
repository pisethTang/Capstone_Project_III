import { useLoader } from "@react-three/fiber";
import { useEffect, useMemo, useState } from "react";
import * as THREE from "three";
import * as BufferGeometryUtils from "three-stdlib";
import { OBJLoader } from "three-stdlib";
import VertexLabels from "../src/VertexLabels";

export default function GeodesicMesh({ modelPath, version }: { modelPath: string, version: number}) {
    const originalObj = useLoader(OBJLoader, modelPath);

    const [pathData, setPathData] = useState<{
        path: number[];
        allDistances: number[];
    } | null>(null);

    useEffect(() => {
        // We assume the result.json updates whenever you run the C++ engine
        fetch(`/result.json?v=${Date.now()}`)
            .then((res) => res.json())
            .then(setPathData);
    }, [modelPath, version]); // Re-fetch data if the model changes

    const processedMesh = useMemo(() => {
        if (!originalObj || !pathData) return null;
        const mesh = originalObj.children.find(
            (c) => c instanceof THREE.Mesh,
        ) as THREE.Mesh;
        if (!mesh) return null;

        // 1. Force the geometry to be indexed correctly
        let geometry = mesh.geometry.clone();

        // Use this specific utility to ensure vertex 0 in C++ is index 0 in React
        geometry = BufferGeometryUtils.mergeVertices(geometry, 0.0001);

        // 2. Standard Scaling logic
        geometry.computeBoundingBox();
        const box = geometry.boundingBox!;
        const size = new THREE.Vector3();
        box.getSize(size);
        const center = new THREE.Vector3();
        box.getCenter(center);
        geometry.translate(-center.x, -center.y, -center.z);
        const scale = 2 / Math.max(size.x, size.y, size.z);
        geometry.scale(scale, scale, scale);

        // 3. Coloring Logic
        const count = geometry.attributes.position.count;
        const colors = new Float32Array(count * 3);
        const color = new THREE.Color();
        for (let i = 0; i < count; i++) {
            // We use the index 'i' directly because mergeVertices aligned it
            const dist = pathData.allDistances[i] || 0;
            color.setHSL(0.6 * (1 - Math.min(dist / 3, 1)), 1, 0.5);
            color.toArray(colors, i * 3);
        }
        geometry.setAttribute("color", new THREE.BufferAttribute(colors, 3));

        return {
            geometry,
            material: new THREE.MeshStandardMaterial({
                vertexColors: true,
                wireframe: true,
                transparent: true,
                opacity: 0.4,
            }),
        };
    }, [originalObj, pathData]);

    // Dijkstra Path Logic
    const lineGeometry = useMemo(() => {
        if (!pathData || !processedMesh) return null;
        const pos = processedMesh.geometry.attributes.position.array;
        const points = pathData.path.map(
            (id: number) =>
                new THREE.Vector3(
                    pos[id * 3],
                    pos[id * 3 + 1],
                    pos[id * 3 + 2],
                ),
        );
        return new THREE.BufferGeometry().setFromPoints(points);
    }, [pathData, processedMesh]);
    // This calculates the 3D position of the start and end vertices
    const markers = useMemo(() => {
        if (!pathData || !processedMesh) return null;

        const positions = processedMesh.geometry.attributes.position.array;

        // Helper to grab x,y,z for a specific vertex index
        const getPos = (id: number) =>
            new THREE.Vector3(
                positions[id * 3],
                positions[id * 3 + 1],
                positions[id * 3 + 2],
            );

        // Vertex IDs from your C++ engine's path
        const startIdx = pathData.path[0];
        const endIdx = pathData.path[pathData.path.length - 1];

        return {
            start: getPos(startIdx),
            end: getPos(endIdx),
        };
    }, [pathData, processedMesh]);

    return (
        <group>
            {processedMesh && (
                <>
                    <mesh
                        geometry={processedMesh.geometry}
                        material={processedMesh.material}
                    />
                    <VertexLabels
                        positions={
                            processedMesh.geometry.attributes.position
                                .array as Float32Array
                        }
                    />
                </>
            )}
            {/* Start Point - LIME GREEN */}
            {markers && (
                <mesh position={markers.start}>
                    <sphereGeometry args={[0.07, 16, 16]} />
                    <meshBasicMaterial color="#32CD32" />
                </mesh>
            )}

            {/* End Point - HOT PINK */}
            {markers && (
                <mesh position={markers.end}>
                    <sphereGeometry args={[0.07, 16, 16]} />
                    <meshBasicMaterial color="#FF69B4" />
                </mesh>
            )}

            {/* The dijsktra path */}
            {lineGeometry && (
                <primitive
                    object={
                        new THREE.Line(
                            lineGeometry,
                            new THREE.LineBasicMaterial({ color: "red" }),
                        )
                    }
                />
            )}
        </group>
    );
}
