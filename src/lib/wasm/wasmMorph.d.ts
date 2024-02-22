export interface VectorByte {
  push_back(_0: number): void;
  resize(_0: number, _1: number): void;
  size(): number;
  set(_0: number, _1: number): boolean;
  get(_0: number): any;
  delete(): void;
}

export interface VectorFeatureLine {
  size(): number;
  push_back(_0: FeatureLine): void;
  resize(_0: number, _1: FeatureLine): void;
  set(_0: number, _1: FeatureLine): boolean;
  get(_0: number): any;
  delete(): void;
}

export type Point = {
  x: number,
  y: number
};

export type FeatureLine = {
  startPoint: Point,
  endPoint: Point
};

export interface MorphWasmModule {
  VectorByte: {new(): VectorByte};
  VectorFeatureLine: {new(): VectorFeatureLine};
  doMorph(_0: number, _1: number, _2: VectorByte, _3: VectorFeatureLine, _4: VectorFeatureLine): VectorByte;
}
export default function instantiate_wasmMorph(mod?: any): Promise<MorphWasmModule>; 
