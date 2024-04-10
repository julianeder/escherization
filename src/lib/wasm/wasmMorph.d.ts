// TypeScript bindings for emscripten-generated code.  Automatically generated at compile time.
interface WasmModule {
  __Z15getMorphOutlineiifNSt3__26vectorIhNS_9allocatorIhEEEENS0_I11FeatureLineNS1_IS4_EEEES6_NS0_IdNS1_IdEEEE(_0: number, _1: number, _2: number, _3: number, _4: number, _5: number, _6: number, _7: number): void;
  __Z7getBBoxNSt3__26vectorI11FeatureLineNS_9allocatorIS1_EEEENS0_IdNS2_IdEEEE(_0: number, _1: number, _2: number): void;
  __Z7doMorphiiffffNSt3__26vectorIhNS_9allocatorIhEEEES3_NS0_I11FeatureLineNS1_IS4_EEEES6_NS0_IdNS1_IdEEEE(_0: number, _1: number, _2: number, _3: number, _4: number, _5: number, _6: number, _7: number, _8: number, _9: number, _10: number, _11: number): void;
}

export interface VectorByte {
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

export interface VectorInt {
  push_back(_0: number): void;
  resize(_0: number, _1: number): void;
  size(): number;
  get(_0: number): number | undefined;
  set(_0: number, _1: number): boolean;
  delete(): void;
}

export interface VectorFeatureLine {
  size(): number;
  get(_0: number): FeatureLine | undefined;
  push_back(_0: FeatureLine): void;
  resize(_0: number, _1: FeatureLine): void;
  set(_0: number, _1: FeatureLine): boolean;
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

interface EmbindModule {
  VectorByte: {new(): VectorByte};
  VectorDouble: {new(): VectorDouble};
  VectorInt: {new(): VectorInt};
  VectorFeatureLine: {new(): VectorFeatureLine};
  getBBox(_0: VectorFeatureLine, _1: VectorDouble): VectorInt;
  doMorph(_0: number, _1: number, _2: number, _3: number, _4: number, _5: number, _6: VectorByte, _7: VectorByte, _8: VectorFeatureLine, _9: VectorFeatureLine, _10: VectorDouble): VectorByte;
  getMorphOutline(_0: number, _1: number, _2: number, _3: VectorByte, _4: VectorFeatureLine, _5: VectorFeatureLine, _6: VectorDouble): VectorFeatureLine;
}
export type MorphWasmModule = WasmModule & EmbindModule;
export default function MorphWasmModuleFactory (options?: unknown): Promise<MorphWasmModule>;
export default function instantiate_wasmMorph(mod?: any): Promise<MorphWasmModule>; 
