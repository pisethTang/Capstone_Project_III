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
                "cmake -S . -B build-e2e -DBUILD_TESTING=OFF && cmake --build build-e2e --target geodesic_engine -j && cd backend && go build -o app . && ./app",
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
