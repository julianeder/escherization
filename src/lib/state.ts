import { writable, type Writable } from "svelte/store";
import { Sites, Point } from "./voronoiDataStructures";
import type { FeatureLine } from "./wasm/wasmMorph";

export const canvasSize: Point = new Point(300, 300);

export let siteStore: Writable<Sites> = writable({
  sitePoints: [],
  siteSegments: [],
  tileWidth: canvasSize.x,
  tileHeight: canvasSize.y,
  tileCenter: new Point(0, 0),
  imageOffset: new Point(0, 0),
});

export class ImageStoreContent {
  image: HTMLImageElement | null = null;
  imageData: ImageData | null = null;
}

export let imageStore: Writable<ImageStoreContent> = writable({ image: null, imageData: null });

export class SymGroupParams {
  symGroup: string = "";
  IH: number = -1;
  origin: string = "";
  name: string = "";
  image: string = "";
  tilingScaleFactor: number = 1;
  parameterNames: any;
}

export let originStore: Writable<SymGroupParams> = writable({
  symGroup: "",
  IH: -1,
  origin: "",
  name: "",
  image: "",
  tilingScaleFactor: 1,
  parameterNames: null
});


export let dataBackStore: Writable<FeatureLine[]> = writable([]);
