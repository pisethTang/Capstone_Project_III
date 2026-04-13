import { defineConfig, devices } from "@playwright/test";

export default defineConfig({
    testDir: "./e2e",
    timeout: 90_000,
    expect: {
        timeout: 15_000,
    },
    fullyParallel: false,
    reporter: [["list"], ["html", { open: "never" }]],
    use: {
        baseURL: "http://127.0.0.1:4173",
        trace: "on-first-retry",
    },
    webServer: [
        {
            command:
                "[ -x main ] || g++ -O2 -std=c++17 -o main main.cpp src/algorithms/dijkstra_solver.cpp src/algorithms/heat_method.cpp src/analytics/analytic_service.cpp src/analytics/analytic_surface_curves.cpp src/analytics/analytic_normalization.cpp src/analytics/analytic_string_utils.cpp src/io/analytics_json_writer.cpp src/io/obj_loader.cpp src/io/result_json_writer.cpp src/mesh/adjacency_builder.cpp; cd backend && go build -o app . && ./app",
            cwd: "..",
            url: "http://127.0.0.1:8080/health",
            reuseExistingServer: true,
            timeout: 180_000,
        },
        {
            command:
                "VITE_API_BASE=http://127.0.0.1:8080 npm run dev -- --host 127.0.0.1 --port 4173",
            url: "http://127.0.0.1:4173",
            reuseExistingServer: true,
            timeout: 120_000,
        },
    ],
    projects: [
        {
            name: "chromium",
            use: {
                ...devices["Desktop Chrome"],
            },
        },
    ],
});
