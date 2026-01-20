import { useEffect, useMemo, useRef, useState } from "react";
import * as THREE from "three";
import VertexLabels from "../src/VertexLabels";

type ParsedObj = {
    vertices: THREE.Vector3[];
    triangles: number[]; // flat array of vertex indices [a,b,c,a,b,c,...]
};

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
        points: number[][]; // [[x,y,z],...]
    }>;
};

type HeatJson = AnalyticsJson;

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
    analyticsVersion,
    heatVersion,
    startId,
    endId,
    modelUp = "z",
    showDijkstraPath = true,
    showAnalyticsPath = true,
    showHeatPath = true,
    onVertexCountChange,
    onMeshStatsChange,
    highlightFaceIndex,
    onDijkstraResultChange,
    onAnalyticsResultChange,
    onHeatResultChange,
}: {
    modelPath: string;
    version: number;
    analyticsVersion: number;
    heatVersion: number;
    startId: number;
    endId: number;
    modelUp?: "y" | "z";
    showDijkstraPath?: boolean;
    showAnalyticsPath?: boolean;
    showHeatPath?: boolean;
    onVertexCountChange?: (count: number) => void;
    onMeshStatsChange?: (stats: {
        vertexCount: number;
        faceCount: number;
    }) => void;
    highlightFaceIndex?: number | null;
    onDijkstraResultChange?: (
        result: {
            totalDistance: number;
            inputFileName?: string;
            pathLength: number;
        } | null,
    ) => void;
    onAnalyticsResultChange?: (
        result: {
            surfaceType: string;
            curveCount: number;
            totalLength: number;
            error?: string;
        } | null,
    ) => void;
    onHeatResultChange?: (
        result: {
            surfaceType: string;
            curveCount: number;
            totalLength: number;
            error?: string;
        } | null,
    ) => void;
}) {
    const pathInstancedRef = useRef<THREE.InstancedMesh>(null);
    const analyticsInstancedRef = useRef<THREE.InstancedMesh>(null);
    const heatInstancedRef = useRef<THREE.InstancedMesh>(null);

    const modelRotation = useMemo(() => {
        // Our scene is Z-up. If a model was authored Y-up (common for some assets like the bunny),
        // rotate it so its "up" axis aligns with world +Z.
        return modelUp === "y"
            ? (new THREE.Euler(Math.PI / 2, 0, 0) as THREE.Euler)
            : (new THREE.Euler(0, 0, 0) as THREE.Euler);
    }, [modelUp]);

    const [objData, setObjData] = useState<ParsedObj | null>(null);

    const [pathData, setPathData] = useState<DijkstraJson | null>(null);
    const [analyticsData, setAnalyticsData] = useState<AnalyticsJson | null>(
        null,
    );
    const [heatData, setHeatData] = useState<HeatJson | null>(null);

    const activePathData = version === 0 ? null : pathData;
    const activeAnalyticsData = analyticsVersion === 0 ? null : analyticsData;
    const activeHeatData = heatVersion === 0 ? null : heatData;

    useEffect(() => {
        if (version === 0) return;

        let cancelled = false;
        // We assume the result.json updates whenever you run the C++ engine
        fetch(`/result.json?v=${Date.now()}`)
            .then((res) => res.json())
            .then((data: DijkstraJson) => {
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
        if (analyticsVersion === 0) return;

        let cancelled = false;
        fetch(`/analytics.json?v=${Date.now()}`)
            .then((res) => res.json())
            .then((data: AnalyticsJson) => {
                if (cancelled) return;
                setAnalyticsData(data);
            })
            .catch(() => {
                // Leave existing analyticsData as-is; activeAnalyticsData gates rendering.
            });

        return () => {
            cancelled = true;
        };
    }, [modelPath, analyticsVersion]);

    useEffect(() => {
        if (heatVersion === 0) return;

        let cancelled = false;
        fetch(`/heat_result.json?v=${Date.now()}`)
            .then((res) => res.json())
            .then((data: HeatJson) => {
                if (cancelled) return;
                setHeatData(data);
            })
            .catch(() => {
                // Leave existing heatData as-is; activeHeatData gates rendering.
            });

        return () => {
            cancelled = true;
        };
    }, [modelPath, heatVersion]);

    useEffect(() => {
        if (!onDijkstraResultChange) return;
        if (!activePathData) {
            onDijkstraResultChange(null);
            return;
        }

        if (
            activePathData.reachable === false ||
            activePathData.totalDistance == null
        ) {
            onDijkstraResultChange(null);
            return;
        }
        onDijkstraResultChange({
            totalDistance: activePathData.totalDistance,
            inputFileName: activePathData.inputFileName,
            pathLength: activePathData.path?.length ?? 0,
        });
    }, [activePathData, onDijkstraResultChange]);

    useEffect(() => {
        if (!onAnalyticsResultChange) return;
        if (!activeAnalyticsData) {
            onAnalyticsResultChange(null);
            return;
        }

        const curves = activeAnalyticsData.curves ?? [];
        const totalLength = curves.reduce((acc, c) => {
            const v = Number(c.length);
            return Number.isFinite(v) ? acc + v : acc;
        }, 0);
        onAnalyticsResultChange({
            surfaceType: activeAnalyticsData.surfaceType,
            curveCount: activeAnalyticsData.curves?.length ?? 0,
            totalLength,
            error: activeAnalyticsData.error,
        });
    }, [activeAnalyticsData, onAnalyticsResultChange]);

    useEffect(() => {
        if (!onHeatResultChange) return;
        if (!activeHeatData) {
            onHeatResultChange(null);
            return;
        }

        const curves = activeHeatData.curves ?? [];
        const totalLength = curves.reduce((acc, c) => {
            const v = Number(c.length);
            return Number.isFinite(v) ? acc + v : acc;
        }, 0);
        onHeatResultChange({
            surfaceType: activeHeatData.surfaceType,
            curveCount: activeHeatData.curves?.length ?? 0,
            totalLength,
            error: activeHeatData.error,
        });
    }, [activeHeatData, onHeatResultChange]);

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

    const meshFaceCount = useMemo(() => {
        if (!processedMesh?.geometry.index) return null;
        const indexCount = processedMesh.geometry.index.count;
        if (!Number.isFinite(indexCount) || indexCount < 0) return null;
        return Math.floor(indexCount / 3);
    }, [processedMesh]);

    useEffect(() => {
        if (meshVertexCount == null) return;
        onVertexCountChange?.(meshVertexCount);
    }, [meshVertexCount, onVertexCountChange]);

    useEffect(() => {
        if (meshVertexCount == null || meshFaceCount == null) return;
        onMeshStatsChange?.({
            vertexCount: meshVertexCount,
            faceCount: meshFaceCount,
        });
    }, [meshVertexCount, meshFaceCount, onMeshStatsChange]);

    const highlightedFaceGeometry = useMemo(() => {
        if (!processedMesh) return null;
        if (highlightFaceIndex == null) return null;
        const geom = processedMesh.geometry;
        const indexAttr = geom.index;
        const posAttr = geom.attributes.position as THREE.BufferAttribute;
        if (!indexAttr || !posAttr) return null;

        const faceCount = Math.floor(indexAttr.count / 3);
        if (faceCount <= 0) return null;

        const face = Math.max(
            0,
            Math.min(faceCount - 1, Math.trunc(highlightFaceIndex)),
        );
        const i0 = Number(indexAttr.getX(face * 3));
        const i1 = Number(indexAttr.getX(face * 3 + 1));
        const i2 = Number(indexAttr.getX(face * 3 + 2));
        if (
            ![i0, i1, i2].every(
                (v) => Number.isFinite(v) && v >= 0 && v < posAttr.count,
            )
        ) {
            return null;
        }

        const out = new THREE.BufferGeometry();
        const tri = new Float32Array(9);
        tri[0] = Number(posAttr.getX(i0));
        tri[1] = Number(posAttr.getY(i0));
        tri[2] = Number(posAttr.getZ(i0));
        tri[3] = Number(posAttr.getX(i1));
        tri[4] = Number(posAttr.getY(i1));
        tri[5] = Number(posAttr.getZ(i1));
        tri[6] = Number(posAttr.getX(i2));
        tri[7] = Number(posAttr.getY(i2));
        tri[8] = Number(posAttr.getZ(i2));
        out.setAttribute("position", new THREE.BufferAttribute(tri, 3));
        out.computeVertexNormals();
        return out;
    }, [processedMesh, highlightFaceIndex]);

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

        const sphereLike = /sphere/i.test(modelPath);

        const up = new THREE.Vector3(0, 1, 0);
        const p1 = new THREE.Vector3();
        const p2 = new THREE.Vector3();
        const dir = new THREE.Vector3();
        const mid = new THREE.Vector3();
        const quat = new THREE.Quaternion();
        const scale = new THREE.Vector3();
        const matrix = new THREE.Matrix4();

        const out: THREE.Matrix4[] = [];

        // Estimate a radius for sphere-like models (in normalized/world units).
        // This helps keep interpolated segments on the surface.
        let sphereRadius = 1.0;
        if (sphereLike) {
            let sum = 0;
            let count = 0;
            for (
                let i = 0;
                i < posAttr.count;
                i += Math.max(1, Math.floor(posAttr.count / 5000))
            ) {
                const x = Number(pos[i * 3]);
                const y = Number(pos[i * 3 + 1]);
                const z = Number(pos[i * 3 + 2]);
                const r = Math.sqrt(x * x + y * y + z * z);
                if (Number.isFinite(r) && r > 1e-6) {
                    sum += r;
                    count++;
                }
            }
            if (count > 0) sphereRadius = sum / count;
        }

        const slerpOnSphere = (
            a: THREE.Vector3,
            b: THREE.Vector3,
            t: number,
        ) => {
            const an = a.clone().normalize();
            const bn = b.clone().normalize();
            const dot = THREE.MathUtils.clamp(an.dot(bn), -1, 1);
            const theta = Math.acos(dot);
            const sinTheta = Math.sin(theta);
            if (sinTheta < 1e-6 || !Number.isFinite(sinTheta)) {
                return an.lerp(bn, t).normalize().multiplyScalar(sphereRadius);
            }
            const w1 = Math.sin((1 - t) * theta) / sinTheta;
            const w2 = Math.sin(t * theta) / sinTheta;
            return an
                .multiplyScalar(w1)
                .add(bn.multiplyScalar(w2))
                .multiplyScalar(sphereRadius);
        };

        const addSegment = (a: THREE.Vector3, b: THREE.Vector3) => {
            dir.subVectors(b, a);
            const length = dir.length();
            if (length <= 1e-9) return;
            mid.addVectors(a, b).multiplyScalar(0.5);
            quat.setFromUnitVectors(up, dir.normalize());
            scale.set(1, length, 1);
            matrix.compose(mid, quat, scale);
            out.push(matrix.clone());
        };

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

            if (!sphereLike) {
                addSegment(p1, p2);
                continue;
            }

            // Subdivide each hop so it visually follows the sphere surface.
            const an = p1.clone().normalize();
            const bn = p2.clone().normalize();
            const dot = THREE.MathUtils.clamp(an.dot(bn), -1, 1);
            const theta = Math.acos(dot);
            const steps = Math.min(
                16,
                Math.max(2, Math.ceil(theta / (Math.PI / 18))),
            ); // ~10° per segment

            let prev = p1.clone().normalize().multiplyScalar(sphereRadius);
            for (let s = 1; s <= steps; s++) {
                const t = s / steps;
                const next = slerpOnSphere(p1, p2, t);
                addSegment(prev, next);
                prev = next;
            }
        }

        return out;
    }, [activePathData, processedMesh, modelPath]);

    useEffect(() => {
        const mesh = pathInstancedRef.current;
        if (!mesh) return;
        if (!showDijkstraPath) return;
        for (let i = 0; i < pathSegmentMatrices.length; i++) {
            mesh.setMatrixAt(i, pathSegmentMatrices[i]);
        }
        mesh.count = pathSegmentMatrices.length;
        mesh.instanceMatrix.needsUpdate = true;
    }, [pathSegmentMatrices, showDijkstraPath]);

    const analyticsSegmentMatrices = useMemo(() => {
        if (!activeAnalyticsData) return [] as THREE.Matrix4[];
        const curves = activeAnalyticsData.curves ?? [];
        if (curves.length === 0) return [] as THREE.Matrix4[];

        const up = new THREE.Vector3(0, 1, 0);
        const p1 = new THREE.Vector3();
        const p2 = new THREE.Vector3();
        const dir = new THREE.Vector3();
        const mid = new THREE.Vector3();
        const quat = new THREE.Quaternion();
        const scale = new THREE.Vector3();
        const matrix = new THREE.Matrix4();

        const out: THREE.Matrix4[] = [];
        for (const curve of curves) {
            const pts = curve.points ?? [];
            if (pts.length < 2) continue;
            for (let i = 0; i < pts.length - 1; i++) {
                const a = pts[i];
                const b = pts[i + 1];
                if (!a || !b || a.length < 3 || b.length < 3) continue;

                p1.set(Number(a[0]), Number(a[1]), Number(a[2]));
                p2.set(Number(b[0]), Number(b[1]), Number(b[2]));

                dir.subVectors(p2, p1);
                const length = dir.length();
                if (length <= 1e-9) continue;

                mid.addVectors(p1, p2).multiplyScalar(0.5);
                quat.setFromUnitVectors(up, dir.normalize());
                scale.set(1, length, 1);
                matrix.compose(mid, quat, scale);
                out.push(matrix.clone());
            }
        }
        return out;
    }, [activeAnalyticsData]);

    const heatSegmentMatrices = useMemo(() => {
        if (!activeHeatData) return [] as THREE.Matrix4[];
        const curves = activeHeatData.curves ?? [];
        if (curves.length === 0) return [] as THREE.Matrix4[];

        const up = new THREE.Vector3(0, 1, 0);
        const p1 = new THREE.Vector3();
        const p2 = new THREE.Vector3();
        const dir = new THREE.Vector3();
        const mid = new THREE.Vector3();
        const quat = new THREE.Quaternion();
        const scale = new THREE.Vector3();
        const matrix = new THREE.Matrix4();

        const out: THREE.Matrix4[] = [];
        for (const curve of curves) {
            const pts = curve.points ?? [];
            if (pts.length < 2) continue;
            for (let i = 0; i < pts.length - 1; i++) {
                const a = pts[i];
                const b = pts[i + 1];
                if (!a || !b || a.length < 3 || b.length < 3) continue;

                p1.set(Number(a[0]), Number(a[1]), Number(a[2]));
                p2.set(Number(b[0]), Number(b[1]), Number(b[2]));

                dir.subVectors(p2, p1);
                const length = dir.length();
                if (length <= 1e-9) continue;

                mid.addVectors(p1, p2).multiplyScalar(0.5);
                quat.setFromUnitVectors(up, dir.normalize());
                scale.set(1, length, 1);
                matrix.compose(mid, quat, scale);
                out.push(matrix.clone());
            }
        }
        return out;
    }, [activeHeatData]);

    useEffect(() => {
        const mesh = analyticsInstancedRef.current;
        if (!mesh) return;
        if (!showAnalyticsPath) return;
        for (let i = 0; i < analyticsSegmentMatrices.length; i++) {
            mesh.setMatrixAt(i, analyticsSegmentMatrices[i]);
        }
        mesh.count = analyticsSegmentMatrices.length;
        mesh.instanceMatrix.needsUpdate = true;
    }, [analyticsSegmentMatrices, showAnalyticsPath]);

    useEffect(() => {
        const mesh = heatInstancedRef.current;
        if (!mesh) return;
        if (!showHeatPath) return;
        for (let i = 0; i < heatSegmentMatrices.length; i++) {
            mesh.setMatrixAt(i, heatSegmentMatrices[i]);
        }
        mesh.count = heatSegmentMatrices.length;
        mesh.instanceMatrix.needsUpdate = true;
    }, [heatSegmentMatrices, showHeatPath]);

    const meshEdgeScale = useMemo(() => {
        if (!processedMesh) {
            return { avgEdgeLen: 0.02, markerRadius: 0.02, pathRadius: 0.005 };
        }
        const geom = processedMesh.geometry;
        const indexAttr = geom.index;
        const posAttr = geom.attributes.position as THREE.BufferAttribute;
        if (!indexAttr || !posAttr || posAttr.count <= 0) {
            return { avgEdgeLen: 0.02, markerRadius: 0.02, pathRadius: 0.005 };
        }

        const positions = posAttr.array as ArrayLike<number>;
        const indices = indexAttr.array as ArrayLike<number>;
        const faceCount = Math.floor(indexAttr.count / 3);
        if (faceCount <= 0) {
            return { avgEdgeLen: 0.02, markerRadius: 0.02, pathRadius: 0.005 };
        }

        const maxTriangleSamples = 5000;
        const triStep = Math.max(1, Math.floor(faceCount / maxTriangleSamples));

        const dist = (a: number, b: number) => {
            const ax = Number(positions[a * 3]);
            const ay = Number(positions[a * 3 + 1]);
            const az = Number(positions[a * 3 + 2]);
            const bx = Number(positions[b * 3]);
            const by = Number(positions[b * 3 + 1]);
            const bz = Number(positions[b * 3 + 2]);
            const dx = bx - ax;
            const dy = by - ay;
            const dz = bz - az;
            return Math.sqrt(dx * dx + dy * dy + dz * dz);
        };

        let sum = 0;
        let samples = 0;
        for (let t = 0; t < faceCount; t += triStep) {
            const base = t * 3;
            const i0 = Number(indices[base]);
            const i1 = Number(indices[base + 1]);
            const i2 = Number(indices[base + 2]);
            if (
                !Number.isFinite(i0) ||
                !Number.isFinite(i1) ||
                !Number.isFinite(i2) ||
                i0 < 0 ||
                i1 < 0 ||
                i2 < 0 ||
                i0 >= posAttr.count ||
                i1 >= posAttr.count ||
                i2 >= posAttr.count
            ) {
                continue;
            }
            sum += dist(i0, i1) + dist(i1, i2) + dist(i2, i0);
            samples += 3;
        }

        const avgEdgeLen = samples > 0 ? sum / samples : 0.02;

        // Heuristics (in world units after our normalize-to-size transform):
        // - Markers: a bit bigger than before so they're easier to see on dense meshes.
        // - Path: just slightly thicker than the wireframe, so individual edges are still readable.
        const markerRadius = Math.max(0.006, Math.min(0.08, avgEdgeLen * 0.12));
        const pathRadius = Math.max(0.0015, Math.min(0.01, avgEdgeLen * 0.02));

        return { avgEdgeLen, markerRadius, pathRadius };
    }, [processedMesh]);

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
                    {highlightedFaceGeometry && (
                        <mesh
                            geometry={highlightedFaceGeometry}
                            renderOrder={5}
                        >
                            <meshBasicMaterial
                                color="#ffd54a"
                                transparent
                                opacity={0.45}
                                side={THREE.DoubleSide}
                                depthWrite={false}
                                polygonOffset
                                polygonOffsetFactor={-2}
                                polygonOffsetUnits={-2}
                            />
                        </mesh>
                    )}
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
                <mesh position={markers.start} renderOrder={20}>
                    <sphereGeometry
                        args={[meshEdgeScale.markerRadius, 12, 12]}
                    />
                    <meshBasicMaterial color="#ea1313" depthTest={false} />
                </mesh>
            )}

            {/* End Point - GREEN */}
            {markers && (
                <mesh position={markers.end} renderOrder={20}>
                    <sphereGeometry
                        args={[meshEdgeScale.markerRadius, 12, 12]}
                    />
                    <meshBasicMaterial color="#32CD32" depthTest={false} />
                </mesh>
            )}

            {/* The dijkstra path (straight segments, slightly thicker than edges) */}
            {showDijkstraPath && pathSegmentMatrices.length > 0 && (
                <instancedMesh
                    ref={pathInstancedRef}
                    args={[undefined, undefined, pathSegmentMatrices.length]}
                    renderOrder={10}
                    frustumCulled={false}
                >
                    {/* height=1; scaled per-instance to the actual segment length */}
                    <cylinderGeometry
                        args={[
                            meshEdgeScale.pathRadius,
                            meshEdgeScale.pathRadius,
                            1,
                            10,
                        ]}
                    />
                    <meshBasicMaterial
                        color="#ff2d2d"
                        depthWrite={false}
                        depthTest
                        polygonOffset
                        polygonOffsetFactor={-2}
                        polygonOffsetUnits={-2}
                    />
                </instancedMesh>
            )}

            {/* Analytics geodesic(s) (yellow) */}
            {showAnalyticsPath && analyticsSegmentMatrices.length > 0 && (
                <instancedMesh
                    ref={analyticsInstancedRef}
                    args={[
                        undefined,
                        undefined,
                        analyticsSegmentMatrices.length,
                    ]}
                    renderOrder={11}
                    frustumCulled={false}
                >
                    <cylinderGeometry
                        args={[
                            meshEdgeScale.pathRadius * 0.9,
                            meshEdgeScale.pathRadius * 0.9,
                            1,
                            10,
                        ]}
                    />
                    <meshBasicMaterial
                        color="#ffe100"
                        depthWrite={false}
                        depthTest
                        polygonOffset
                        polygonOffsetFactor={-3}
                        polygonOffsetUnits={-3}
                    />
                </instancedMesh>
            )}

            {/* Heat method geodesic (cyan) */}
            {showHeatPath && heatSegmentMatrices.length > 0 && (
                <instancedMesh
                    ref={heatInstancedRef}
                    args={[undefined, undefined, heatSegmentMatrices.length]}
                    renderOrder={12}
                    frustumCulled={false}
                >
                    <cylinderGeometry
                        args={[
                            meshEdgeScale.pathRadius * 0.85,
                            meshEdgeScale.pathRadius * 0.85,
                            1,
                            10,
                        ]}
                    />
                    <meshBasicMaterial
                        color="#00e5ff"
                        depthWrite={false}
                        depthTest
                        polygonOffset
                        polygonOffsetFactor={-4}
                        polygonOffsetUnits={-4}
                    />
                </instancedMesh>
            )}
        </group>
    );
}
