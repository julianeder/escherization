import { writable, type Writable } from "svelte/store";
import { Sites, Point } from "./voronoiDataStructures";

export const canvasSize: Point = new Point(300, 300);

export let siteStore: Writable<Sites> = writable({
  sitePoints: [],
  siteSegments: [],
  tileWidth: canvasSize.x,
  tileHeight: canvasSize.y,
  tileCenter: new Point(0,0),
  imageOffset: new Point(0,0),
});

export let imageStore: Writable<HTMLImageElement | null> = writable(null);