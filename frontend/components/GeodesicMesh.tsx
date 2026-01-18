import { useEffect, useMemo, useRef, useState } from "react";
import * as THREE from "three";
import VertexLabels from "../src/VertexLabels";

type ParsedObj = {
    vertices: THREE.Vector3[];
    triangles: number[]; // flat array of vertex indices [a,b,c,a,b,c,...]
};

function parseObj(objText: string): ParsedObj {
    const vertices: THREE.Vector3[] = [];
    const triangles: number[] = [];

    const lines = objText.split(/\r?\n/);
    for (const rawLine of lines) {
        const line = rawLine.trim();
        if (line.length === 0 || line.startsWith("#")) continue;

        if (line.startsWith("v ")) {
            const parts = line.split(/\s+/);
            if (parts.length < 4) continue;
            const x = Number(parts[1]);
            const y = Number(parts[2]);
            const z = Number(parts[3]);
            if (
                Number.isFinite(x) &&
                Number.isFinite(y) &&
                Number.isFinite(z)
            ) {
                vertices.push(new THREE.Vector3(x, y, z));
            }
            continue;
        }

        if (line.startsWith("f ")) {
            const parts = line.split(/\s+/).slice(1);
            if (parts.length < 3) continue;

            // Support tokens like "12", "12/3/7", "12//7".
            const toIndex = (token: string) => {
                const head = token.split("/")[0];
                const idx = Number(head);
                if (!Number.isFinite(idx) || idx === 0) return null;
                // OBJ indices are 1-based; negative indices are relative to the end.
                const resolved = idx > 0 ? idx - 1 : vertices.length + idx;
                return resolved >= 0 && resolved < vertices.length
                    ? resolved
                    : null;
            };

            const faceIndices: number[] = [];
            for (const t of parts) {
                const resolved = toIndex(t);
                if (resolved == null) {
                    faceIndices.length = 0;
                    break;
                }
                faceIndices.push(resolved);
            }
            if (faceIndices.length < 3) continue;

            // Triangulate fan for quads/ngons: (0,i,i+1)
            for (let i = 1; i + 1 < faceIndices.length; i++) {
                triangles.push(
                    faceIndices[0],
                    faceIndices[i],
                    faceIndices[i + 1],
                );
            }
        }
    }

    return { vertices, triangles };
}

export default function GeodesicMesh({
    modelPath,
    version,
    startId,
    endId,
    modelUp = "z",
    onVertexCountChange,
}: {
    modelPath: string;
    version: number;
    startId: number;
    endId: number;
    modelUp?: "y" | "z";
    onVertexCountChange?: (count: number) => void;
}) {
    const pathInstancedRef = useRef<THREE.InstancedMesh>(null);

    const modelRotation = useMemo(() => {
        // Our scene is Z-up. If a model was authored Y-up (common for some assets like the bunny),
        // rotate it so its "up" axis aligns with world +Z.
        return modelUp === "y"
            ? (new THREE.Euler(Math.PI / 2, 0, 0) as THREE.Euler)
            : (new THREE.Euler(0, 0, 0) as THREE.Euler);
    }, [modelUp]);

    const [objData, setObjData] = useState<ParsedObj | null>(null);

    const [pathData, setPathData] = useState<{
        path: number[];
        allDistances: number[];
    } | null>(null);

    const activePathData = version === 0 ? null : pathData;

    useEffect(() => {
        if (version === 0) return;

        let cancelled = false;
        // We assume the result.json updates whenever you run the C++ engine
        fetch(`/result.json?v=${Date.now()}`)
            .then((res) => res.json())
            .then((data) => {
                if (cancelled) return;
                setPathData(data);
            })
            .catch(() => {
                // Leave existing pathData as-is; activePathData gates rendering.
            });

        return () => {
            cancelled = true;
        };
    }, [modelPath, version]); // Re-fetch data if the model changes

    useEffect(() => {
        let cancelled = false;
        fetch(modelPath)
            .then((res) => res.text())
            .then((text) => {
                if (cancelled) return;
                setObjData(parseObj(text));
            })
            .catch(() => {
                if (cancelled) return;
                setObjData(null);
            });
        return () => {
            cancelled = true;
        };
    }, [modelPath]);

    const processedMesh = useMemo(() => {
        if (!objData || objData.vertices.length === 0) return null;

        // Build geometry directly from OBJ vertex order + face indices.
        // This guarantees the same indexing as your C++ engine.
        const positions = new Float32Array(objData.vertices.length * 3);
        for (let i = 0; i < objData.vertices.length; i++) {
            const v = objData.vertices[i];
            positions[i * 3] = v.x;
            positions[i * 3 + 1] = v.y;
            positions[i * 3 + 2] = v.z;
        }

        const geometry = new THREE.BufferGeometry();
        geometry.setAttribute(
            "position",
            new THREE.BufferAttribute(positions, 3),
        );
        if (objData.triangles.length > 0) {
            const IndexArrayCtor =
                objData.vertices.length > 65535 ? Uint32Array : Uint16Array;
            geometry.setIndex(
                new THREE.BufferAttribute(
                    new IndexArrayCtor(objData.triangles),
                    1,
                ),
            );
        }

        // Center + scale to a consistent size.
        geometry.computeBoundingBox();
        const box = geometry.boundingBox!;
        const size = new THREE.Vector3();
        box.getSize(size);
        const center = new THREE.Vector3();
        box.getCenter(center);
        geometry.translate(-center.x, -center.y, -center.z);
        const scale = 2 / Math.max(size.x, size.y, size.z);
        geometry.scale(scale, scale, scale);

        // Use explicit wireframe geometry so what you see is exactly the OBJ edges.
        const wireframe = new THREE.WireframeGeometry(geometry);

        return {
            geometry,
            wireframe,
            scale,
            center,
        };
    }, [objData]);

    const meshVertexCount =
        processedMesh?.geometry.attributes.position.count ?? null;

    useEffect(() => {
        if (meshVertexCount == null) return;
        onVertexCountChange?.(meshVertexCount);
    }, [meshVertexCount, onVertexCountChange]);

    // Dijkstra path rendering:
    // Use straight cylinder segments between vertices (no spline smoothing), so the path
    // follows the graph edges exactly and doesn't look wavy/curved.
    const pathSegmentMatrices = useMemo(() => {
        if (!activePathData || !processedMesh) return [] as THREE.Matrix4[];

        const posAttr = processedMesh.geometry.attributes
            .position as THREE.BufferAttribute;
        const pos = posAttr.array as ArrayLike<number>;

        const ids = activePathData.path;
        if (!ids || ids.length < 2) return [] as THREE.Matrix4[];

        const up = new THREE.Vector3(0, 1, 0);
        const p1 = new THREE.Vector3();
        const p2 = new THREE.Vector3();
        const dir = new THREE.Vector3();
        const mid = new THREE.Vector3();
        const quat = new THREE.Quaternion();
        const scale = new THREE.Vector3();
        const matrix = new THREE.Matrix4();

        const out: THREE.Matrix4[] = [];
        for (let i = 0; i < ids.length - 1; i++) {
            const a = ids[i];
            const b = ids[i + 1];

            p1.set(
                Number(pos[a * 3]),
                Number(pos[a * 3 + 1]),
                Number(pos[a * 3 + 2]),
            );
            p2.set(
                Number(pos[b * 3]),
                Number(pos[b * 3 + 1]),
                Number(pos[b * 3 + 2]),
            );

            dir.subVectors(p2, p1);
            const length = dir.length();
            if (length <= 1e-9) continue;

            mid.addVectors(p1, p2).multiplyScalar(0.5);
            quat.setFromUnitVectors(up, dir.normalize());

            // CylinderGeometry is built along +Y with height=1; scale Y to the segment length.
            scale.set(1, length, 1);
            matrix.compose(mid, quat, scale);
            out.push(matrix.clone());
        }
        return out;
    }, [activePathData, processedMesh]);

    useEffect(() => {
        const mesh = pathInstancedRef.current;
        if (!mesh) return;
        for (let i = 0; i < pathSegmentMatrices.length; i++) {
            mesh.setMatrixAt(i, pathSegmentMatrices[i]);
        }
        mesh.count = pathSegmentMatrices.length;
        mesh.instanceMatrix.needsUpdate = true;
    }, [pathSegmentMatrices]);
    // This calculates the 3D position of the start and end vertices
    // and updates in real-time as startId/endId change.
    const markers = useMemo(() => {
        if (!processedMesh) return null;

        const posAttr = processedMesh.geometry.attributes
            .position as THREE.BufferAttribute;
        const positions = posAttr.array as ArrayLike<number>;
        const count = posAttr.count;
        if (count <= 0) return null;

        const clampIndex = (value: number) => {
            const asInt = Number.isFinite(value) ? Math.trunc(value) : 0;
            return Math.max(0, Math.min(count - 1, asInt));
        };

        // Helper to grab x,y,z for a specific vertex index
        const getPos = (id: number) =>
            new THREE.Vector3(
                Number(positions[id * 3]),
                Number(positions[id * 3 + 1]),
                Number(positions[id * 3 + 2]),
            );

        return {
            start: getPos(clampIndex(startId)),
            end: getPos(clampIndex(endId)),
        };
    }, [processedMesh, startId, endId]);

    return (
        <group rotation={modelRotation}>
            {processedMesh && (
                <>
                    <lineSegments>
                        <primitive object={processedMesh.wireframe} />
                        <lineBasicMaterial
                            color="#2aa1ff"
                            transparent
                            opacity={0.6}
                        />
                    </lineSegments>
                    <VertexLabels
                        positions={
                            processedMesh.geometry.attributes.position
                                .array as Float32Array
                        }
                    />
                </>
            )}
            {/* Start Point - RED */}
            {markers && (
                <mesh position={markers.start}>
                    <sphereGeometry args={[0.07, 16, 16]} />
                    <meshBasicMaterial color="#ea1313" />
                </mesh>
            )}

            {/* End Point - GREEN */}
            {markers && (
                <mesh position={markers.end}>
                    <sphereGeometry args={[0.07, 16, 16]} />
                    <meshBasicMaterial color="#32CD32" />
                </mesh>
            )}

            {/* The dijkstra path (straight segments, slightly thicker than edges) */}
            {pathSegmentMatrices.length > 0 && (
                <instancedMesh
                    ref={pathInstancedRef}
                    args={[undefined, undefined, pathSegmentMatrices.length]}
                    renderOrder={10}
                    frustumCulled={false}
                >
                    {/* height=1; scaled per-instance to the actual segment length */}
                    <cylinderGeometry args={[0.01, 0.01, 1, 10]} />
                    <meshBasicMaterial
                        color="#ff2d2d"
                        depthWrite={false}
                        polygonOffset
                        polygonOffsetFactor={-2}
                        polygonOffsetUnits={-2}
                    />
                </instancedMesh>
            )}
        </group>
    );
}
