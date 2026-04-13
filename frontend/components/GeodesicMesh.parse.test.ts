import { describe, expect, it } from "vitest";

import { parseObj } from "./GeodesicMesh";

describe("parseObj", () => {
    it("parses vertices and a basic triangular face", () => {
        const parsed = parseObj(`
            v 0 0 0
            v 1 0 0
            v 0 1 0
            f 1 2 3
        `);

        expect(parsed.vertices).toHaveLength(3);
        expect(parsed.triangles).toEqual([0, 1, 2]);
        expect(parsed.vertices[1].x).toBe(1);
        expect(parsed.vertices[2].y).toBe(1);
    });

    it("triangulates quad faces using fan triangulation", () => {
        const parsed = parseObj(`
            v 0 0 0
            v 1 0 0
            v 1 1 0
            v 0 1 0
            f 1 2 3 4
        `);

        expect(parsed.triangles).toEqual([0, 1, 2, 0, 2, 3]);
    });

    it("supports slashed tokens and negative indices", () => {
        const parsed = parseObj(`
            v 0 0 0
            v 1 0 0
            v 1 1 0
            v 0 1 0
            f 1/9/10 2/9/10 3/9/10
            f -4 -3 -2
        `);

        expect(parsed.vertices).toHaveLength(4);
        expect(parsed.triangles).toEqual([0, 1, 2, 0, 1, 2]);
    });

    it("skips malformed faces with invalid indices", () => {
        const parsed = parseObj(`
            v 0 0 0
            v 1 0 0
            v 0 1 0
            f 1 2 nope
            f 1 2 99
        `);

        expect(parsed.vertices).toHaveLength(3);
        expect(parsed.triangles).toEqual([]);
    });
});
