/// <reference types="emscripten" />

export interface VoronoiWasmModule extends EmscriptenModule {
    _testNumCells(): number;
    _compute(numPoints: number, numSegments: number, points: Array<number>): number;
}

export default function instantiate_wasmVoronoi(mod?: any): Promise<VoronoiWasmModule>;