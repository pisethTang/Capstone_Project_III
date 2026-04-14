import { expect, test } from "@playwright/test";

test.describe("Geodesic Lab UI to backend flows", () => {
    test("backend health endpoint is available", async ({ request }) => {
        const response = await request.get("http://127.0.0.1:8080/health");
        expect(response.ok()).toBeTruthy();
        const payload = (await response.json()) as { status?: string };
        expect(payload.status).toBe("ok");
    });

    test("run Dijkstra from UI and render computed distance", async ({ page }) => {
        await page.goto("/");
        await expect(page.getByRole("heading", { name: "Geodesic Lab" })).toBeVisible();

        const dijkstraRow = page
            .locator("span", { hasText: "Dijkstra Distance:" })
            .locator("xpath=..");
        await expect(dijkstraRow).toContainText("—");

        const responsePromise = page.waitForResponse((response) => {
            return (
                response.url().endsWith("/compute") &&
                response.request().method() === "POST"
            );
        });

        await page.getByRole("button", { name: "Run Dijkstra" }).click();
        const response = await responsePromise;
        expect(response.status()).toBe(200);
        const payload = (await response.json()) as {
            path?: number[];
            totalDistance?: number | null;
        };

        expect(Array.isArray(payload.path)).toBeTruthy();
        expect((payload.path ?? []).length).toBeGreaterThan(0);
        expect(payload.totalDistance).not.toBeNull();

        await expect
            .poll(async () => await dijkstraRow.innerText(), {
                timeout: 30_000,
            })
            .toMatch(/[0-9]+\.[0-9]{6}/);
    });

    test("run Analytics from UI and render analytics length", async ({ page }) => {
        await page.goto("/");

        const analyticsRow = page
            .locator("span", { hasText: "Analytics Length:" })
            .locator("xpath=..");
        await expect(analyticsRow).toContainText("—");

        const responsePromise = page.waitForResponse((response) => {
            return (
                response.url().endsWith("/analytics") &&
                response.request().method() === "POST"
            );
        });

        await page.getByRole("button", { name: "Run Analytics" }).click();
        const response = await responsePromise;
        expect(response.status()).toBe(200);
        const payload = (await response.json()) as {
            curves?: Array<{ length: number }>;
        };

        expect(Array.isArray(payload.curves)).toBeTruthy();
        expect((payload.curves ?? []).length).toBeGreaterThan(0);

        await expect
            .poll(async () => await analyticsRow.innerText(), {
                timeout: 30_000,
            })
            .toMatch(/[0-9]+\.[0-9]{6}/);
    });
});
