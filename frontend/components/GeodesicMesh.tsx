import { useLoader } from "@react-three/fiber";
import { useEffect, useMemo, useRef, useState } from "react";
import * as THREE from "three";
import * as BufferGeometryUtils from "three-stdlib";
import { OBJLoader } from "three-stdlib";
import VertexLabels from "../src/VertexLabels";

function parseObjVertices(objText: string): THREE.Vector3[] {
    const vertices: THREE.Vector3[] = [];
    const lines = objText.split(/\r?\n/);
    for (const line of lines) {
        if (!line.startsWith("v ")) continue;
        const parts = line.trim().split(/\s+/);
        if (parts.length < 4) continue;
        const x = Number(parts[1]);
        const y = Number(parts[2]);
        const z = Number(parts[3]);
        if (Number.isFinite(x) && Number.isFinite(y) && Number.isFinite(z)) {
            vertices.push(new THREE.Vector3(x, y, z));
        }
    }
    return vertices;
}

function positionKey(
    x: number,
    y: number,
    z: number,
    tolerance: number,
): string {
    const qx = Math.round(x / tolerance);
    const qy = Math.round(y / tolerance);
    const qz = Math.round(z / tolerance);
    return `${qx}|${qy}|${qz}`;
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
    const originalObj = useLoader(OBJLoader, modelPath);

    const pathInstancedRef = useRef<THREE.InstancedMesh>(null);

    const modelRotation = useMemo(() => {
        // Our scene is Z-up. If a model was authored Y-up (common for some assets like the bunny),
        // rotate it so its "up" axis aligns with world +Z.
        return modelUp === "y"
            ? (new THREE.Euler(Math.PI / 2, 0, 0) as THREE.Euler)
            : (new THREE.Euler(0, 0, 0) as THREE.Euler);
    }, [modelUp]);

    const [objVertices, setObjVertices] = useState<THREE.Vector3[] | null>(
        null,
    );

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
                setObjVertices(parseObjVertices(text));
            })
            .catch(() => {
                if (cancelled) return;
                setObjVertices(null);
            });
        return () => {
            cancelled = true;
        };
    }, [modelPath]);

    const processedMesh = useMemo(() => {
        if (!originalObj) return null;
        const mesh = originalObj.children.find(
            (c) => c instanceof THREE.Mesh,
        ) as THREE.Mesh;
        if (!mesh) return null;

        // 1) Start from a clean clone.
        // OBJLoader often produces a geometry where the same logical OBJ vertex is duplicated
        // across faces (and may differ in normals/uvs). That creates "overlapping vertices"
        // and breaks vertex indexing vs your C++ engine.
        let geometry = mesh.geometry.clone();

        // 2) Weld by POSITION ONLY.
        // BufferGeometryUtils.mergeVertices considers *all* attributes when deciding uniqueness.
        // If normals/uvs differ, identical positions won't merge, leaving duplicates.
        geometry = geometry.toNonIndexed();
        for (const name of Object.keys(geometry.attributes)) {
            if (name !== "position") geometry.deleteAttribute(name);
        }
        const mergeTolerance = 1e-6;
        geometry = BufferGeometryUtils.mergeVertices(geometry, mergeTolerance);

        // 3) Standard scaling logic (applied to both the mesh geometry and the raw OBJ vertices).
        geometry.computeBoundingBox();
        const box = geometry.boundingBox!;
        const size = new THREE.Vector3();
        box.getSize(size);
        const center = new THREE.Vector3();
        box.getCenter(center);
        geometry.translate(-center.x, -center.y, -center.z);
        const scale = 2 / Math.max(size.x, size.y, size.z);
        geometry.scale(scale, scale, scale);

        // 4) Reorder vertices to match the OBJ file's original `v` order.
        // This makes vertex IDs 0..N-1 line up with the C++ engine that uses OBJ vertex indices.
        if (objVertices && objVertices.length > 0 && geometry.getIndex()) {
            const expectedCount = objVertices.length;
            const posAttr = geometry.getAttribute(
                "position",
            ) as THREE.BufferAttribute;
            const posArray = posAttr.array as ArrayLike<number>;
            const actualCount = posAttr.count;

            if (actualCount === expectedCount) {
                // Transform raw OBJ vertices with the same translate/scale applied to geometry.
                const objTransformed = objVertices.map((v) =>
                    v.clone().sub(center).multiplyScalar(scale),
                );

                const keyToObjIndices = new Map<string, number[]>();
                const reorderTol = 1e-5;
                for (let i = 0; i < objTransformed.length; i++) {
                    const v = objTransformed[i];
                    const key = positionKey(v.x, v.y, v.z, reorderTol);
                    const bucket = keyToObjIndices.get(key);
                    if (bucket) bucket.push(i);
                    else keyToObjIndices.set(key, [i]);
                }

                const oldToNew = new Int32Array(actualCount);
                oldToNew.fill(-1);
                for (let oldIndex = 0; oldIndex < actualCount; oldIndex++) {
                    const x = Number(posArray[oldIndex * 3]);
                    const y = Number(posArray[oldIndex * 3 + 1]);
                    const z = Number(posArray[oldIndex * 3 + 2]);
                    const key = positionKey(x, y, z, reorderTol);
                    const bucket = keyToObjIndices.get(key);
                    if (bucket && bucket.length > 0) {
                        oldToNew[oldIndex] = bucket.pop()!;
                    }
                }

                let mappingOk = true;
                const seen = new Uint8Array(actualCount);
                for (let i = 0; i < oldToNew.length; i++) {
                    const newIndex = oldToNew[i];
                    if (newIndex < 0 || newIndex >= actualCount) {
                        mappingOk = false;
                        break;
                    }
                    if (seen[newIndex]) {
                        mappingOk = false;
                        break;
                    }
                    seen[newIndex] = 1;
                }

                if (mappingOk) {
                    const newPositions = new Float32Array(actualCount * 3);
                    for (let oldIndex = 0; oldIndex < actualCount; oldIndex++) {
                        const newIndex = oldToNew[oldIndex];
                        newPositions[newIndex * 3] = Number(
                            posArray[oldIndex * 3],
                        );
                        newPositions[newIndex * 3 + 1] = Number(
                            posArray[oldIndex * 3 + 1],
                        );
                        newPositions[newIndex * 3 + 2] = Number(
                            posArray[oldIndex * 3 + 2],
                        );
                    }

                    const oldIndexAttr = geometry.getIndex()!;
                    const oldIndexArray = oldIndexAttr.array as
                        | Uint16Array
                        | Uint32Array;
                    const NewIndexArrayCtor = oldIndexArray.constructor as
                        | Uint16ArrayConstructor
                        | Uint32ArrayConstructor;
                    const newIndexArray = new NewIndexArrayCtor(
                        oldIndexArray.length,
                    );
                    for (let i = 0; i < oldIndexArray.length; i++) {
                        newIndexArray[i] = oldToNew[oldIndexArray[i]];
                    }

                    geometry.setAttribute(
                        "position",
                        new THREE.BufferAttribute(newPositions, 3),
                    );
                    geometry.setIndex(
                        new THREE.BufferAttribute(newIndexArray, 1),
                    );
                }
            }
        }

        geometry.computeVertexNormals();

        return {
            geometry,
            material: new THREE.MeshStandardMaterial({
                color: "#2aa1ff",
                wireframe: true,
                transparent: true,
                opacity: 0.35,
            }),
        };
    }, [originalObj, objVertices]);

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
