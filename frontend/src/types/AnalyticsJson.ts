export type AnalyticsJson = {
    inputFileName?: string;
    startId: number;
    endId: number;
    surfaceType: string;
    error?: string;
    curves: Array<{
        name: string;
        length: number;
        points: number[][];
    }>;
};