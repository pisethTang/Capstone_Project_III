export type DijkstraJson = {
    inputFileName?: string;
    reachable?: boolean;
    totalDistance: number | null;
    path: number[];
    allDistances: number[];
}; 