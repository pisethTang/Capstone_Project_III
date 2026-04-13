import React from "react";
import { fireEvent, render, screen, waitFor } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";

const mockState = {
    latestModelPath: "",
};

vi.mock("@react-three/fiber", () => ({
    Canvas: ({ children }: { children: React.ReactNode }) => (
        <div data-testid="mock-canvas">{children}</div>
    ),
}));

vi.mock("@react-three/drei", () => {
    const OrbitControls = React.forwardRef<unknown, Record<string, never>>(
        function MockOrbitControls(_props, _ref) {
            return <div data-testid="mock-orbit-controls" />;
        },
    );

    return { OrbitControls };
});

vi.mock("../components/Axes", () => ({
    default: () => <div data-testid="mock-axes" />,
}));

vi.mock("../components/GeodesicMesh", () => {
    type MockDijkstra = {
        totalDistance: number | null;
        path: number[];
        inputFileName?: string;
    };

    type MockAnalytics = {
        surfaceType: string;
        curves: Array<{ length: number }>;
        error?: string;
    };

    type MockProps = {
        modelPath: string;
        dijkstraData: MockDijkstra | null;
        analyticsData: MockAnalytics | null;
        onVertexCountChange?: (count: number) => void;
        onMeshStatsChange?: (stats: {
            vertexCount: number;
            faceCount: number;
        }) => void;
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
    };

    function MockGeodesicMesh(props: MockProps) {
        mockState.latestModelPath = props.modelPath;

        React.useEffect(() => {
            props.onVertexCountChange?.(5);
            props.onMeshStatsChange?.({ vertexCount: 5, faceCount: 3 });
        }, [props]);

        React.useEffect(() => {
            if (!props.dijkstraData || props.dijkstraData.totalDistance == null) {
                props.onDijkstraResultChange?.(null);
                return;
            }
            props.onDijkstraResultChange?.({
                totalDistance: props.dijkstraData.totalDistance,
                inputFileName: props.dijkstraData.inputFileName,
                pathLength: props.dijkstraData.path.length,
            });
        }, [props.dijkstraData, props.onDijkstraResultChange]);

        React.useEffect(() => {
            if (!props.analyticsData) {
                props.onAnalyticsResultChange?.(null);
                return;
            }

            const totalLength = props.analyticsData.curves.reduce((acc, curve) => {
                const v = Number(curve.length);
                return Number.isFinite(v) ? acc + v : acc;
            }, 0);

            props.onAnalyticsResultChange?.({
                surfaceType: props.analyticsData.surfaceType,
                curveCount: props.analyticsData.curves.length,
                totalLength,
                error: props.analyticsData.error,
            });
        }, [props.analyticsData, props.onAnalyticsResultChange]);

        return (
            <div data-testid="mock-geodesic-mesh" data-model-path={props.modelPath}>
                mesh
            </div>
        );
    }

    return {
        default: MockGeodesicMesh,
    };
});

import App from "./App";

function getStartAndEndInputs() {
    const spinboxes = screen.getAllByRole("spinbutton") as HTMLInputElement[];
    return {
        start: spinboxes[0],
        end: spinboxes[1],
    };
}

describe("App", () => {
    const fetchMock = vi.fn();

    beforeEach(() => {
        vi.stubGlobal("fetch", fetchMock);
        fetchMock.mockReset();
        mockState.latestModelPath = "";
    });

    afterEach(() => {
        vi.unstubAllGlobals();
    });

    it("clamps start/end IDs to mesh vertex bounds", async () => {
        render(<App />);

        const { start, end } = getStartAndEndInputs();

        fireEvent.change(start, { target: { value: "999" } });
        expect(start.value).toBe("4");

        fireEvent.change(end, { target: { value: "-10" } });
        expect(end.value).toBe("0");
    });

    it("runs Dijkstra request and updates displayed distance", async () => {
        fetchMock.mockResolvedValueOnce({
            ok: true,
            json: async () => ({
                inputFileName: "icosahedron.obj",
                reachable: true,
                totalDistance: 12.34567,
                path: [0, 1, 2],
                allDistances: [0, 1, 2],
            }),
        });

        const user = userEvent.setup();
        render(<App />);

        const { start, end } = getStartAndEndInputs();
        fireEvent.change(start, { target: { value: "2" } });
        fireEvent.change(end, { target: { value: "3" } });

        await user.click(screen.getByRole("button", { name: "Run Dijkstra" }));

        await waitFor(() => {
            expect(fetchMock).toHaveBeenCalledTimes(1);
        });

        const [url, init] = fetchMock.mock.calls[0] as [string, RequestInit];
        expect(url).toContain("/compute");
        expect(init.method).toBe("POST");

        const body = JSON.parse(String(init.body)) as {
            start: number;
            end: number;
            model: string;
        };
        expect(body).toEqual({ start: 2, end: 3, model: "icosahedron.obj" });

        await waitFor(() => {
            expect(screen.getByText("12.345670")).toBeTruthy();
        });

        await waitFor(() => {
            expect(
                screen.getByRole("button", { name: "Run Dijkstra" }),
            ).toBeTruthy();
        });
    });

    it("runs analytics request and resets view state on model change", async () => {
        fetchMock.mockResolvedValueOnce({
            ok: true,
            json: async () => ({
                startId: 0,
                endId: 1,
                surfaceType: "sphere",
                curves: [
                    { name: "c1", length: 1.25, points: [] },
                    { name: "c2", length: 2.25, points: [] },
                ],
            }),
        });

        const user = userEvent.setup();
        render(<App />);

        await user.click(
            screen.getByRole("button", { name: "Hide Dijkstra Path" }),
        );
        await user.click(
            screen.getByRole("button", { name: "Hide Analytics Path" }),
        );

        await user.click(screen.getByRole("button", { name: "Run Analytics" }));

        await waitFor(() => {
            expect(fetchMock).toHaveBeenCalledTimes(1);
        });

        await waitFor(() => {
            expect(screen.getByText("3.500000")).toBeTruthy();
        });

        const modelSelect = screen.getByRole("combobox") as HTMLSelectElement;
        await user.selectOptions(modelSelect, "sphere.obj");

        expect(modelSelect.value).toBe("sphere.obj");

        const { start, end } = getStartAndEndInputs();
        expect(start.value).toBe("0");
        expect(end.value).toBe("1");

        expect(
            screen.getByRole("button", { name: "Hide Dijkstra Path" }),
        ).toBeTruthy();
        expect(
            screen.getByRole("button", { name: "Hide Analytics Path" }),
        ).toBeTruthy();

        expect(mockState.latestModelPath).toBe("/data/sphere.obj");
    });

    it("clears loading state when compute request fails", async () => {
        fetchMock.mockRejectedValueOnce(new Error("network down"));

        const user = userEvent.setup();
        render(<App />);

        await user.click(screen.getByRole("button", { name: "Run Dijkstra" }));

        await waitFor(() => {
            expect(
                screen.getByRole("button", { name: "Run Dijkstra" }),
            ).toBeTruthy();
        });
    });
});
