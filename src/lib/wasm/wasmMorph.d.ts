export interface VectorByte {
  push_back(_0: number): void;
  resize(_0: number, _1: number): void;
  size(): number;
  set(_0: number, _1: number): boolean;
  get(_0: number): any;
  delete(): void;
}

export interface MorphWasmModule {
  VectorByte: {new(): VectorByte};
  doMorph(_0: number, _1: number, _2: VectorByte): VectorByte;
}
export default function instantiate_wasmMorph(mod?: any): Promise<MorphWasmModule>; 
