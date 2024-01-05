export interface VectorInt {
  push_back(_0: number): void;
  resize(_0: number, _1: number): void;
  size(): number;
  set(_0: number, _1: number): boolean;
  get(_0: number): any;
  delete(): void;
}

export interface VectorDouble {
  size(): number;
  push_back(_0: number): void;
  resize(_0: number, _1: number): void;
  set(_0: number, _1: number): boolean;
  get(_0: number): any;
  delete(): void;
}

export interface VectorEdgeResult {
  size(): number;
  push_back(_0: EdgeResult): void;
  resize(_0: number, _1: EdgeResult): void;
  set(_0: number, _1: EdgeResult): boolean;
  get(_0: number): any;
  delete(): void;
}

export type DiagrammResult = {
  vertices: VectorDouble,
  edges: VectorEdgeResult,
  numVerticies: number
};

export type EdgeResult = {
  x1: number,
  y1: number,
  x2: number,
  y2: number,
  isFinite: boolean,
  isCurved: boolean,
  isPrimary: boolean,
  samples: VectorDouble,
  controll_points: VectorDouble
};

export interface VoronoiWasmModule {
  VectorInt: {new(): VectorInt};
  VectorDouble: {new(): VectorDouble};
  VectorEdgeResult: {new(): VectorEdgeResult};
  computevoronoi(_0: VectorDouble, _1: VectorInt, _2: VectorInt): DiagrammResult;
}
export default function instantiate_wasmVoronoi(mod?: any): Promise<VoronoiWasmModule>; 
