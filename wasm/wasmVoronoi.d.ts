/// <reference types="emscripten" />

export interface EdgeResult{
    x1: number;
    y1: number;
    x2: number;
    y2: number;
    isFinite: boolean;
}

export interface VoronoiWasmModule extends EmscriptenModule {
    computevoronoi(points: any): any;
}

export default function instantiate_wasmVoronoi(mod?: any): Promise<VoronoiWasmModule>;