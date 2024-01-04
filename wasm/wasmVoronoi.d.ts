/// <reference types="emscripten" />

export interface EdgeResult{
    x1: number;
    y1: number;
    x2: number;
    y2: number;
    isFinite: boolean;
    isCurved: boolean;
    samples: any;
}

export interface VoronoiWasmModule extends EmscriptenModule {
    VectorInt: any;
    VectorDouble: any;
    computevoronoi(bbox: any, points: any, segments: any): any;
}

export default function instantiate_wasmVoronoi(mod?: any): Promise<VoronoiWasmModule>;