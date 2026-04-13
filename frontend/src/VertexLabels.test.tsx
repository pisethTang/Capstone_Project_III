import React from "react";
import { render, screen } from "@testing-library/react";
import { describe, expect, it, vi } from "vitest";

vi.mock("@react-three/drei", () => ({
    Html: ({ children }: { children: React.ReactNode }) => (
        <div data-testid="mock-html">{children}</div>
    ),
}));

import VertexLabels from "./VertexLabels";

describe("VertexLabels", () => {
    it("renders labels for small meshes", () => {
        const positions = new Float32Array([
            0, 0, 0,
            1, 0, 0,
        ]);

        render(<VertexLabels positions={positions} />);

        expect(screen.getByText("0")).toBeTruthy();
        expect(screen.getByText("1")).toBeTruthy();
        expect(screen.getAllByTestId("mock-html")).toHaveLength(2);
    });

    it("does not render labels when vertex count exceeds threshold", () => {
        const positions = new Float32Array(101 * 3);

        render(<VertexLabels positions={positions} />);

        expect(screen.queryByTestId("mock-html")).toBeNull();
    });
});
