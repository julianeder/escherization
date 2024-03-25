// TypeScript bindings for emscripten-generated code.  Automatically generated at compile time.
interface WasmModule {
  __Z7computeNSt3__26vectorIdNS_9allocatorIdEEEENS0_IiNS1_IiEEEES5_S5_S5_S5_S5_(_0: number, _1: number, _2: number, _3: number, _4: number, _5: number, _6: number, _7: number): void;
}

export interface VectorInt {
  push_back(_0: number): void;
  resize(_0: number, _1: number): void;
  size(): number;
  get(_0: number): number | undefined;
  set(_0: number, _1: number): boolean;
  delete(): void;
}

export interface VectorDouble {
  size(): number;
  get(_0: number): number | undefined;
  push_back(_0: number): void;
  resize(_0: number, _1: number): void;
  set(_0: number, _1: number): boolean;
  delete(): void;
}

export interface VectorEdgeResult {
  size(): number;
  get(_0: number): EdgeResult | undefined;
  push_back(_0: EdgeResult): void;
  resize(_0: number, _1: EdgeResult): void;
  set(_0: number, _1: EdgeResult): boolean;
  delete(): void;
}

export interface VectorCellResult {
  size(): number;
  get(_0: number): CellResult | undefined;
  push_back(_0: CellResult): void;
  resize(_0: number, _1: CellResult): void;
  set(_0: number, _1: CellResult): boolean;
  delete(): void;
}

export type DiagrammResult = {
  vertices: VectorDouble,
  edges: VectorEdgeResult,
  cells: VectorCellResult,
  numVerticies: number
};

export type CellResult = {
  sourceIndex: number,
  sourceCategory: number,
  isDegenerate: boolean,
  containsPoint: boolean,
  containsSegment: boolean,
  edgeIndices: VectorInt,
  color: number,
  tileIdx: number
};

export type EdgeResult = {
  x1: number,
  y1: number,
  x2: number,
  y2: number,
  isFinite: boolean,
  isCurved: boolean,
  isPrimary: boolean,
  controll_points: VectorDouble,
  isWithinCell: boolean
};

interface EmbindModule {
  VectorInt: {new(): VectorInt};
  VectorDouble: {new(): VectorDouble};
  VectorEdgeResult: {new(): VectorEdgeResult};
  VectorCellResult: {new(): VectorCellResult};
  computevoronoi(_0: VectorDouble, _1: VectorInt, _2: VectorInt, _3: VectorInt, _4: VectorInt, _5: VectorInt, _6: VectorInt): DiagrammResult;
}
export type VoronoiWasmModule = WasmModule & EmbindModule;
export default function VoronoiWasmModuleFactory (options?: unknown): Promise<VoronoiWasmModule>;
export default function instantiate_wasmVoronoi(mod?: any): Promise<VoronoiWasmModule>; 
