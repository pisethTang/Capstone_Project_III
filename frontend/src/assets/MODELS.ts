export const MODELS = [
    { name: "Icosahedron", file: "icosahedron.obj", up: "z" as const },
    { name: "Square", file: "zig_zag.obj", up: "z" as const },
    // Stanford bunny is commonly authored as Y-up.
    { name: "Bunny", file: "stanford-bunny.obj", up: "y" as const },
    { name: "Plane Grid", file: "plane.obj" },
    { name: "Sphere", file: "sphere.obj" },
    { name: "Sphere (Low Res)", file: "sphere_low.obj" },
    { name: "Torus", file: "donut.obj" },
    { name: "Saddle", file: "saddle.obj" }, // hyperbolic paraboloid (pringle-shaped)
];