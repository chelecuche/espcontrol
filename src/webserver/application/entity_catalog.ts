import type { HomeAssistantEntityPage, HomeAssistantEntityRecord } from "../model/entity_catalog";

export interface EntityCatalogClient {
    /** Search the HA catalog through the display's native ESPHome connection. */
    search(query: string, domains?: string[]): Promise<HomeAssistantEntityRecord[]>;
}

function fieldForDomains(domains: string[]): string {
    const fields: Record<string, string> = {
        alarm_control_panel: "alarm",
        automation: "automation",
        binary_sensor: "binary_sensor",
        button: "button",
        camera: "camera",
        climate: "climate",
        cover: "cover",
        fan: "fan",
        image: "camera",
        input_boolean: "switch",
        input_button: "button",
        input_number: "number",
        input_select: "select",
        lawn_mower: "lawn_mower",
        light: "light",
        lock: "lock",
        media_player: "media_player",
        number: "number",
        person: "person",
        scene: "scene",
        select: "select",
        script: "script",
        sensor: "sensor",
        text_sensor: "sensor",
        switch: "switch",
        vacuum: "vacuum",
        weather: "weather",
        device_tracker: "device_tracker",
    };
    const first = domains[0];
    if (domains.length === 1 && first && fields[first]) return fields[first];
    return "entity";
}

const SEARCH_PATH = "/api/v1/ha/entities/search";
const POLL_DELAY_MS = 100;
const MAX_POLLS = 150;

function wait(milliseconds: number): Promise<void> {
    return new Promise((resolve) => setTimeout(resolve, milliseconds));
}

export function createEntityCatalogClient(
    _storage?: Storage,
    fetchImpl: typeof fetch = fetch,
): EntityCatalogClient {
    async function search(query: string, domains: string[] = []): Promise<HomeAssistantEntityRecord[]> {
        const entities: HomeAssistantEntityRecord[] = [];
        let cursor = 0;
        for (let pageNumber = 0; pageNumber < 200; pageNumber += 1) {
            const params = new URLSearchParams({
                query,
                field: fieldForDomains(domains),
                limit: "50",
                cursor: String(cursor),
            });
            const start = await fetchImpl(`${SEARCH_PATH}?${params}`, {
                credentials: "same-origin",
                cache: "no-store",
            });
            if (!start.ok && start.status !== 202) {
                throw new Error(`Entity catalog request failed (${start.status})`);
            }
            const pending = await start.json() as { request_id?: number };
            if (typeof pending.request_id !== "number") {
                throw new Error("Entity catalog did not return a request ID");
            }
            let page: HomeAssistantEntityPage | null = null;
            for (let poll = 0; poll < MAX_POLLS; poll += 1) {
                await wait(POLL_DELAY_MS);
                const response = await fetchImpl(`${SEARCH_PATH}?request_id=${pending.request_id}`, {
                    credentials: "same-origin",
                    cache: "no-store",
                });
                if (response.status === 202) continue;
                if (!response.ok) {
                    const error = await response.json().catch(() => ({})) as { error?: string };
                    throw new Error(error.error || `Entity catalog request failed (${response.status})`);
                }
                page = await response.json() as HomeAssistantEntityPage;
                break;
            }
            if (!page || !Array.isArray(page.entities)) throw new Error("Home Assistant entity catalog timed out");
            entities.push(...page.entities);
            if (page.next_cursor === null || typeof page.next_cursor !== "number" || page.next_cursor <= cursor) break;
            cursor = page.next_cursor;
        }
        return entities;
    }
    return { search };
}
