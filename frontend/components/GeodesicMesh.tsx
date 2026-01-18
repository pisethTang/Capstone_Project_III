import { useLoader } from "@react-three/fiber";
import { useEffect, useMemo, useState } from "react";
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
    onVertexCountChange,
}: {
    modelPath: string;
    version: number;
    startId: number;
    endId: number;
    onVertexCountChange?: (count: number) => void;
}) {
    const originalObj = useLoader(OBJLoader, modelPath);

    const [objVertices, setObjVertices] = useState<THREE.Vector3[] | null>(
        null,
    );

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

        // 5) Coloring Logic
        const count = geometry.attributes.position.count;
        const colors = new Float32Array(count * 3);
        const color = new THREE.Color();
        for (let i = 0; i < count; i++) {
            // Vertex index i should match the OBJ/C++ vertex index after reordering.
            const dist = pathData?.allDistances?.[i] || 0;
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
    }, [originalObj, pathData, objVertices]);

    useEffect(() => {
        if (!processedMesh || !onVertexCountChange) return;
        onVertexCountChange(processedMesh.geometry.attributes.position.count);
    }, [processedMesh, onVertexCountChange]);

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
