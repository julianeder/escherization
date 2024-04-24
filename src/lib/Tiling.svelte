<script lang="ts">
  import Range from "./Range.svelte";
  import { onMount } from "svelte";
  import { BBox, type Edge, SitePoint, SiteSegment, type Vertex, Sites, type Cell, type Tile, Point } from "./voronoiDataStructures";
  import instantiate_wasmVoronoi, { type VoronoiWasmModule, type EdgeResult, type DiagrammResult, type CellResult } from "../lib/wasm/wasmVoronoi";
  import instantiate_wasmMorph, { type FeatureLine, type MorphWasmModule } from "../lib/wasm/wasmMorph";
  import { IsohedralTiling } from "./tactile/tactile";
  import ColorPicker from "svelte-awesome-color-picker";
  import { type Matrix, compose, scale, toSVG, translate, rotate, applyToPoint, flipX, flipY, identity } from "transformation-matrix";
  import { canvasSize, ImageStoreContent, imageStore, siteStore, originStore, SymGroupParams, dataBackStore } from "./state";
  import { checkIntersections } from "./collisionDetection";

  import p4m from "./images/p4m.png";
  import p1 from "./images/p1.png";
  import p2 from "./images/p2.png";
  import p3 from "./images/p3.png";
  import p6 from "./images/p6.png";
  import p6m from "./images/p6m.png";
  import p4 from "./images/p4.png";

  // wasm modules
  let wasmVoronoi: VoronoiWasmModule;
  let wasmMorph: MorphWasmModule;
  
  // state
  let bbox: BBox = new BBox(0, 500, 0, 500);
  let tiles: Tile[] = [];
  let tileSitePoints: SitePoint[] = [];
  let tileSiteSegments: SiteSegment[] = [];
  let outlines: FeatureLine[] = [];
  let mostCenterTile: Tile;

  let tilingSitePoints: SitePoint[] = [];
  let tilingSiteSegments: SiteSegment[] = [];
  let voronoiEdges: Edge[] = [];
  let voronoiCells: Cell[] = [];
  
  let lastError: any = "";
  let showSecondary: boolean = false;
  let isUptoDate: boolean = false;

  let tilingIdx = 0;
  let prevTilingIndex = -1;

  // parameters
  let tileWidth: number = 300;
  let tileHeight: number = 300;
  let tilingScaleFactor: number = 1;
  let tilingSize: number = 100;
  
  let tileSize: number = 1;
  let tileRotation: number = 0;
  let tileCenter: Point = new Point(150, 150);
  let imageOffset: Point;
  
  let autoUpdate: boolean = false;
  let showBorder: boolean = true;
  let showSkeleton: boolean = true;
  let showOrigins: boolean = false;
  let showBackground: boolean = true;
  let showBackgroundImage: boolean = true;
  let showDebugMorphLines: boolean = false;
  
  let tilingParams: number[] = [];

  let doMorph: boolean = true;
  let p: number = 0.6;
  let a: number = 1;
  let b: number = 2;
  let t: number = 1;

  let borderColor: string = "#0000ffff";
  let skelletonColor: string = "#ff0000ff";

  let color1: string = "#e06d4380";
  let color2: string = "#59da5980";
  let color3: string = "#c3c63580";
  
  //data
  let backgroundImage: HTMLImageElement | null;
  let imageData: ImageData | null;
  let imageDataProcessed: ImageData | null;
  let updatePromise: Promise<void> | null = null;
  
  // debug
  let morphedSiteSegments: SiteSegment[] = []; 
  let morphedBBox: number[] = [];
  let morphedBBox_transf: BBox = new BBox(0, 0, 0, 0);



  // tiling consts
  const p2ParameterNames = {
    0: { name: "Spacing Column 1-2", min: 0.01, max: 0.6, stepSize: 0.01, initialValue: 0.2 },
    1: { name: "Height Offset", min: 0.12, max: 0.25, stepSize: 0.01, initialValue: 0 },
  };

  const p1ParameterNames = {
    0: { name: "Column Spacing", min: 0.01, max: 0.3, stepSize: 0.01, initialValue: 0.12239750492 },
    1: { name: "Height Offset", min: 0.0, max: 0.2, stepSize: 0.01, initialValue: 0.1 },
  };

  const p3ParameterNames = {
    0: { name: "Horizontal Spacing", min: 0.5, max: 0.8, stepSize: 0.01, initialValue: 0.6 },
    1: { name: "Vertical Spacing", min: 0.1, max: 0.6, stepSize: 0.01, initialValue: 0.196416770201 },
  };

  const p6ParameterNames = {
    0: { name: "Vertical Spacing", min: 0.0, max: 0.5, stepSize: 0.01, initialValue: 0.104512294489 },
    1: { name: "Horizontal Spacing", min: 0.0, max: 1.3, stepSize: 0.01, initialValue: 0.65 },
  };

  const p4ParameterNames = {
    0: { name: "Horizontal Spacing", min: 0.0, max: 2.0, stepSize: 0.01, initialValue: 0.230769230769 },
    1: { name: "Vertical Spacing", min: 0.0, max: 2.0, stepSize: 0.01, initialValue: 0.230769230769 },
  };

  const symGroups: Array<SymGroupParams> = [
    { symGroup: "p1", IH: 1, origin: "center", name: "Grid Shifted", image: p1, tilingScaleFactor: 0.66, parameterNames: p1ParameterNames },
    { symGroup: "p4m", IH: 76, origin: "ul", name: "Grid", image: p4m, tilingScaleFactor: 0.5, parameterNames: {} },
    { symGroup: "p2", IH: 4, origin: "ul", name: "2 Rotations", image: p2, tilingScaleFactor: 0.66, parameterNames: p2ParameterNames },
    { symGroup: "p3", IH: 7, origin: "center", name: "3 Rotations", image: p3, tilingScaleFactor: 0.66, parameterNames: p3ParameterNames },
    { symGroup: "p4", IH: 28, origin: "center", name: "4 Rotations", image: p4, tilingScaleFactor: 0.5, parameterNames: p4ParameterNames },
    { symGroup: "p6", IH: 21, origin: "center", name: "6 Rotations", image: p6, tilingScaleFactor: 0.5, parameterNames: p6ParameterNames },
    { symGroup: "p6m", IH: 37, origin: "center", name: "6 Rotations Mirrored", image: p6m, tilingScaleFactor: 0.5, parameterNames: {} },
  ];



  function clean() {
    tiles = [];
    voronoiCells = [];
    voronoiEdges = [];
    tilingSitePoints = [];
    tilingSiteSegments = [];
  }

  async function update(): Promise<void> {
    isUptoDate = true;
    await new Promise<void>((resolve, reject) => {
      setTimeout(() => {
        try {
          let startTime = new Date().getTime();

          updateTilingParameters();
          updateTiling();

          if (checkIntersections(tilingSiteSegments)) {
            lastError = "Collision between tiles detected, please change the paremeters (e.g. decrease Tile Size)";
          } else {
            lastError = "";
            updateVoronoi();
            updateMorph();
          }
        } catch (e) {
          lastError = e;
          reject(e);
          throw e;
        }

        resolve();
      });
    });
  }

  function GetMostCenterTileIdx(tiles: Tile[]): Tile {
    let svgCenter: Point = new Point((bbox.xh - bbox.xl) / 2, (bbox.yh - bbox.yl) / 2);
    let sqrDist = Number.MAX_VALUE;
    let idx = -1;
    for (let i = 0; i < tiles.length; i++) {
      let sd = (tiles[i].origin.x - svgCenter.x) * (tiles[i].origin.x - svgCenter.x) + (tiles[i].origin.y - svgCenter.y) * (tiles[i].origin.y - svgCenter.y);
      if (sd < sqrDist) {
        idx = i;
        sqrDist = sd;
      }
    }
    return tiles[idx];
  }

  function updateMorph() {
    mostCenterTile = GetMostCenterTileIdx(tiles);

    outlines = [];
    let T2I: Matrix;
    let I2T: Matrix;

    for (let i = 0; i < tiles.length; i++) {
      if (tiles[i].tileIdx == mostCenterTile.tileIdx) {
        T2I = getInverseTransformation(tiles[i].M, tiles[i].origin, true);
        I2T = getTransformation(tiles[i].M, tiles[i].origin, false, true);

        voronoiCells
          .filter((c) => c.tileIdx == tiles[i].tileIdx)
          .forEach((c) => {
            c.edgeIndices.forEach((idxE) => {
              if (voronoiEdges[idxE].isPrimary && !voronoiEdges[idxE].isWithinCell) {
                let featureLine: FeatureLine = {
                  startPoint: {
                    x: voronoiEdges[idxE].va.x,
                    y: voronoiEdges[idxE].va.y,
                  },
                  endPoint: {
                    x: voronoiEdges[idxE].vb.x,
                    y: voronoiEdges[idxE].vb.y,
                  },
                };
                outlines.push(featureLine);
              }
            });
          });
      }
    }
    outlines = outlines; // Reactive Update

    // Morphing
    if (imageDataProcessed != null && imageData != null) {
      if (doMorph) {
        const imageDataVector = new wasmMorph.VectorByte();
        imageData.data.forEach((b) => imageDataVector.push_back(b));
        const imageDataProcessedVector = new wasmMorph.VectorByte();
        imageDataProcessed.data.forEach((b) => imageDataProcessedVector.push_back(b));

        const skelletonLinesVector = new wasmMorph.VectorFeatureLine();
        tileSiteSegments.forEach((ss) =>
          skelletonLinesVector.push_back({
            startPoint: { x: ss.x1 + tileCenter.x, y: ss.y1 + tileCenter.y },
            endPoint: { x: ss.x2 + tileCenter.x, y: ss.y2 + tileCenter.y },
          }),
        );

        const outlineLinesVector = new wasmMorph.VectorFeatureLine();
        outlines.forEach((s) =>
          outlineLinesVector.push_back({
            startPoint: { x: s.startPoint.x, y: s.startPoint.y },
            endPoint: { x: s.endPoint.x, y: s.endPoint.y },
          }),
        );


        let outlines_Img: FeatureLine[] = [];
        for (let i = 0; i < outlines.length; i++) {
          let s = applyToPoint(T2I!, new Point(outlines[i].startPoint.x, outlines[i].startPoint.y));
          let e = applyToPoint(T2I!, new Point(outlines[i].endPoint.x, outlines[i].endPoint.y));

          outlines_Img.push({ startPoint: s, endPoint: e });
        }

        if (showDebugMorphLines) {
          dataBackStore.set(outlines_Img);
        } else {
          dataBackStore.set([]);
        }

        const mInvVector = new wasmMorph.VectorDouble();
        mInvVector.push_back(T2I!.a);
        mInvVector.push_back(T2I!.b);
        mInvVector.push_back(T2I!.c);
        mInvVector.push_back(T2I!.d);
        mInvVector.push_back(T2I!.e);
        mInvVector.push_back(T2I!.f);

        let bbox = wasmMorph.getBBox(outlineLinesVector, mInvVector);

        morphedBBox = [];
        for (let i = 0; i < bbox.size(); i++) {
          morphedBBox.push(bbox.get(i)!);
        }
        morphedBBox = morphedBBox;

        let l = applyToPoint(I2T!, new Point(morphedBBox[0], morphedBBox[1]));
        let h = applyToPoint(I2T!, new Point(morphedBBox[2], morphedBBox[3]));
        morphedBBox_transf.yl = l.y;
        morphedBBox_transf.yh = h.y;

        // In case of fliping
        if (l.x < h.x) {
          morphedBBox_transf.xl = l.x;
          morphedBBox_transf.xh = h.x;
        } else {
          morphedBBox_transf.xl = h.x;
          morphedBBox_transf.xh = l.x;
        }
        if (l.y < h.y) {
          morphedBBox_transf.yl = l.y;
          morphedBBox_transf.yh = h.y;
        } else {
          morphedBBox_transf.yl = h.y;
          morphedBBox_transf.yh = l.y;
        }

        if (showDebugMorphLines) {
          let morphedOutline = wasmMorph.getMorphOutline(imageData.width, imageData.height, t, imageDataProcessedVector, skelletonLinesVector, outlineLinesVector, mInvVector);

          morphedSiteSegments = [];
          for (let i = 0; i < morphedOutline.size(); i++) {
            morphedSiteSegments.push(new SiteSegment(morphedOutline.get(i)!.startPoint.x, morphedOutline.get(i)!.startPoint.y, morphedOutline.get(i)!.endPoint.x, morphedOutline.get(i)!.endPoint.y));
          }
          morphedSiteSegments = morphedSiteSegments;
        }

        let result = wasmMorph.doMorph(imageDataProcessed.width, imageDataProcessed.height, p, a, b, t, imageDataVector, imageDataProcessedVector, skelletonLinesVector, outlineLinesVector, mInvVector);

        let morphedImageData: ImageData = new ImageData(morphedBBox[2] - morphedBBox[0], morphedBBox[3] - morphedBBox[1]);
        for (let i = 0; i < result.size(); i++) {
          morphedImageData.data[i] = result.get(i)!;
        }

        backgroundImage = imagedataToImage(morphedImageData);
      } else {
        backgroundImage = imagedataToImage(imageData);
        morphedBBox = [0, 0, tileWidth, tileHeight];
      }
    }
  }

  function imagedataToImage(imagedata: ImageData) {
    var canvas = document.createElement("canvas");
    var ctx = canvas.getContext("2d");
    canvas.width = imagedata.width;
    canvas.height = imagedata.height;
    ctx!.putImageData(imagedata, 0, 0);

    var image = new Image();
    image.src = canvas.toDataURL();
    return image;
  }

  function updateTilingParameters() {
    let tiling: IsohedralTiling = new IsohedralTiling(Number(symGroups[tilingIdx].IH));

    if (prevTilingIndex != tilingIdx) {
      if (tiling.numParameters() > 0) {
        tilingParams = tiling.getParameters();
        for (let i = 0; i < tilingParams.length; i++) {
          tilingParams[i] = symGroups[tilingIdx].parameterNames[i]?.initialValue ?? tilingParams[i]; // If we have other initial values, use them
        }
        tiling.setParameters(tilingParams);
      } else {
        tilingParams = [];
      }
    }
    prevTilingIndex = tilingIdx;
  }

  function updateTiling() {
    tiles = [];
    tilingSitePoints = [];
    tilingSiteSegments = [];
    let tiling: IsohedralTiling = new IsohedralTiling(Number(symGroups[tilingIdx].IH));
    tiling.setParameters(tilingParams);

    for (let i of tiling.fillRegionBounds(bbox.xl / (tilingSize * tilingScaleFactor), bbox.yl / (tilingSize * tilingScaleFactor), bbox.xh / (tilingSize * tilingScaleFactor), bbox.yh / (tilingSize * tilingScaleFactor))) {
      // Use a simple colouring algorithm to pick a colour for this tile
      // so that adjacent tiles aren't the same colour.  The resulting
      // value col will be 0, 1, or 2, which you should map to your
      // three favourite colours.
      const color = tiling.getColour(i.t1, i.t2, i.aspect);
      // Get the 3x3 matrix corresponding to one of the transformed
      // tiles in the filled region.
      const mat = i.T;
      let M: Matrix = {
        a: mat[0],
        b: mat[3],
        c: mat[1],
        d: mat[4],
        e: mat[2],
        f: mat[5],
      };

      let angle: number = tileRotation * (Math.PI / 180);

      let I2T = compose(scale(tilingSize * tilingScaleFactor, tilingSize * tilingScaleFactor), M, rotate(angle));

      let o = applyToPoint(I2T, { x: 0, y: 0 });
      let origin = new SitePoint(o.x, o.y);
      let tile: Tile = { origin: origin, M: M, tileIdx: tiles.length - 1 }; //tileIndex counting up

      tiles.push(tile);

      let I2T2C = compose(I2T, scale(tileSize / canvasSize.x, tileSize / canvasSize.y));

      for (let j = 0; j < tileSitePoints.length; j++) {
        let p = applyToPoint(I2T2C, { x: tileSitePoints[j].x, y: tileSitePoints[j].y });

        let newSitePoint: SitePoint = new SitePoint(p.x, p.y, color, tile.tileIdx);

        if (bbox.contains(newSitePoint.x, newSitePoint.y)) tilingSitePoints.push(newSitePoint);
      }

      for (let j = 0; j < tileSiteSegments.length; j++) {
        let s = applyToPoint(I2T2C, { x: tileSiteSegments[j].x1, y: tileSiteSegments[j].y1 });
        let e = applyToPoint(I2T2C, { x: tileSiteSegments[j].x2, y: tileSiteSegments[j].y2 });

        let newSiteSegment: SiteSegment = new SiteSegment(s.x, s.y, e.x, e.y, color, tile.tileIdx);

        if (bbox.contains(newSiteSegment.x1, newSiteSegment.y1) || bbox.contains(newSiteSegment.x2, newSiteSegment.y2)) tilingSiteSegments.push(newSiteSegment);
      }
    }
  }

  function updateVoronoi() {
    try {
      const bboxVector = new wasmVoronoi.VectorDouble();
      bboxVector.push_back(bbox.xl);
      bboxVector.push_back(bbox.yl);
      bboxVector.push_back(bbox.xh);
      bboxVector.push_back(bbox.yh);

      const pointVector = new wasmVoronoi.VectorInt();
      const pointColorVector = new wasmVoronoi.VectorInt();
      const pointTileIdxVector = new wasmVoronoi.VectorInt();
      tilingSitePoints.forEach((sp) => {
        pointVector.push_back(sp.x);
        pointVector.push_back(sp.y);
        pointColorVector.push_back(sp.color);
        pointTileIdxVector.push_back(sp.tileIdx);
      });

      const segmentVector = new wasmVoronoi.VectorInt();
      const segmentColorVector = new wasmVoronoi.VectorInt();
      const segmentTileIdxVector = new wasmVoronoi.VectorInt();
      tilingSiteSegments.forEach((ss) => {
        segmentVector.push_back(ss.x1);
        segmentVector.push_back(ss.y1);
        segmentVector.push_back(ss.x2);
        segmentVector.push_back(ss.y2);
        segmentColorVector.push_back(ss.color);
        segmentTileIdxVector.push_back(ss.tileIdx);
      });

      let result: DiagrammResult = wasmVoronoi.computevoronoi(bboxVector, pointVector, segmentVector, pointColorVector, segmentColorVector, pointTileIdxVector, segmentTileIdxVector);
      let newVoronoiVertices: Vertex[] = [];
      for (let i = 0; i < result.vertices.size(); i += 2) {
        let valid = true;
        if (Number.isNaN(result.vertices.get(i)) || Number.isNaN(result.vertices.get(i + 1))) {
          console.log("Vertex " + i + ": " + result.vertices.get(i) + " " + result.vertices.get(i + 1));
          valid = false;
        }
        newVoronoiVertices.push({
          x: result.vertices.get(i)!,
          y: result.vertices.get(i + 1)!,
          isValid: valid,
        });
      }

      let newVoronoiEdges: Edge[] = [];
      for (let i = 0; i < result.edges.size(); i++) {
        let e: EdgeResult = result.edges.get(i)!;
        let controlPoints: Vertex[] = [];
        let valid = true;
        for (let j = 0; j < e.controll_points.size(); j += 2) {
          if (Number.isNaN(e.controll_points.get(j)) || Number.isNaN(e.controll_points.get(j + 1))) {
            console.log("CP " + i + " " + j / 2 + ": " + e.controll_points.get(j) + " " + e.controll_points.get(j + 1));
            valid = false;
          }
          const v: Vertex = {
            x: e.controll_points.get(j)!,
            y: e.controll_points.get(j + 1)!,
            isValid: valid,
          };
          controlPoints.push(v);
        }

        if (Number.isNaN(e.x1) || Number.isNaN(e.y1) || Number.isNaN(e.x2) || Number.isNaN(e.y2)) {
          console.log("Edge " + i + ": " + e.x1 + " " + e.y1 + " " + e.x2 + " " + e.y2);
          valid = false;
        }

        newVoronoiEdges.push({
          va: { x: e.x1, y: e.y1, isValid: valid },
          vb: { x: e.x2, y: e.y2, isValid: valid },
          isCurved: e.isCurved,
          controlPoints: controlPoints,
          isPrimary: e.isPrimary,
          isValid: valid,
          isWithinCell: e.isWithinCell,
        });
      }

      let newVoronoiCells: Cell[] = [];
      for (let i = 0; i < result.cells.size(); i++) {
        let c: CellResult = result.cells.get(i)!;
        let newCell: Cell = {
          sourceIndex: c.sourceIndex,
          sourceCategory: c.sourceCategory,
          isDegenerate: c.isDegenerate,
          containsPoint: c.containsPoint,
          containsSegment: c.containsSegment,
          edgeIndices: [],
          color: c.color,
          tileIdx: c.tileIdx,
        };

        newVoronoiCells.push(newCell);
        for (let j = 0; j < c.edgeIndices.size(); j++) {
          let newIndex: number = c.edgeIndices.get(j)!;
          if (newIndex >= newVoronoiEdges.length) console.warn("Index too high " + newIndex + " for cell " + c.sourceIndex + " isDegenerate " + c.isDegenerate);
          else newCell.edgeIndices.push(newIndex);
        }
      }

      voronoiEdges = newVoronoiEdges;
      voronoiCells = newVoronoiCells;

    } catch (e) {
      lastError = e;
      throw e;
    }
  }

  function downloadSVG() {
    const svg = document.getElementById("voronoiSvg");
    if (svg != null) {
      const blob = new Blob([svg.outerHTML], { type: "image/svg+xml" });
      const link = document.createElement("a");
      link.href = URL.createObjectURL(blob);
      link.download = "voronoi.svg";
      link.click();
    }
  }

  function isCellPathConsistant(c: Cell): boolean {
    if (c.edgeIndices.length <= 1) return false;

    // Check if each endpoint matches the next start point
    for (let i: number = 1; i <= c.edgeIndices.length - 1; i++) {
      if (!voronoiEdges[c.edgeIndices[i - 1]].isValid) return false;

      if (voronoiEdges[c.edgeIndices[i - 1]].vb.x != voronoiEdges[c.edgeIndices[i]].va.x || voronoiEdges[c.edgeIndices[i - 1]].vb.y != voronoiEdges[c.edgeIndices[i]].va.y) return false;
      if (Number.isNaN(voronoiEdges[c.edgeIndices[i]].va.x) || Number.isNaN(voronoiEdges[c.edgeIndices[i]].va.y)) return false;
    }

    // close loop
    let i = c.edgeIndices.length - 1;
    if (voronoiEdges[c.edgeIndices[i]].vb.x != voronoiEdges[c.edgeIndices[0]].va.x || voronoiEdges[c.edgeIndices[i]].vb.y != voronoiEdges[c.edgeIndices[0]].va.y) return false;

    return true;
  }

  function getCellPath(c: Cell): string {
    let e: Edge = voronoiEdges[c.edgeIndices[0]];

    let d: string;
    if (e.isCurved) {
      d = "M " + e.controlPoints[0].x + " " + e.controlPoints[0].y + "Q " + e.controlPoints[1].x + " " + e.controlPoints[1].y + " " + e.controlPoints[2].x + " " + e.controlPoints[2].y + " ";
    } else {
      d = "M " + voronoiEdges[c.edgeIndices[0]].va.x + " " + voronoiEdges[c.edgeIndices[0]].va.y + " ";
    }

    for (let i: number = 1; i < c.edgeIndices.length; i++) {
      let e: Edge = voronoiEdges[c.edgeIndices[i]];
      let eprev: Edge = voronoiEdges[c.edgeIndices[i - 1]];

      if (e.isCurved) {
        d = d + "L " + e.controlPoints[0].x + " " + e.controlPoints[0].y + "Q " + e.controlPoints[1].x + " " + e.controlPoints[1].y + " " + e.controlPoints[2].x + " " + e.controlPoints[2].y + " ";
      } else {
        d = d + "L " + e.va.x + " " + e.va.y + " ";
      }

      if (eprev.vb.x != e.va.x || eprev.vb.y != e.va.y) console.log("Inconsistent: (" + eprev.vb.x + " " + eprev.vb.y + ") (" + e.va.x + " " + e.va.y + ")");
    }
    let i = c.edgeIndices.length - 1;
    e = voronoiEdges[c.edgeIndices[i]];
    let eprev: Edge = voronoiEdges[c.edgeIndices[i - 1]];

    if (!eprev.isCurved) {
      d = d + "L " + e.vb.x + " " + e.vb.y + " ";
    }

    if (eprev.vb.x != e.va.x || eprev.vb.y != e.va.y) console.log("Inconsistent: (" + eprev.vb.x + " " + eprev.vb.y + ") (" + e.va.x + " " + e.va.y + ")");

    if (e.vb.x != voronoiEdges[c.edgeIndices[0]].va.x || e.vb.y != voronoiEdges[c.edgeIndices[0]].va.y) console.log("Inconsistent: (" + e.vb.x + " " + e.vb.y + ") (" + voronoiEdges[c.edgeIndices[0]].va.x + " " + voronoiEdges[c.edgeIndices[0]].va.y + ")");

    return d;
  }

  function onTilingPlus() {
    tilingIdx = (tilingIdx + 1) % symGroups.length;

    tilingScaleFactor = symGroups[tilingIdx].tilingScaleFactor;

    originStore.set(symGroups[tilingIdx]);

    clean();
    updateTilingParameters();

    if (autoUpdate) updatePromise = update();
  }

  function onTilingMinus() {
    tilingIdx = tilingIdx - 1;
    if (tilingIdx < 0) tilingIdx = symGroups.length - 1;

    tilingScaleFactor = symGroups[tilingIdx].tilingScaleFactor;

    originStore.set(symGroups[tilingIdx]);

    clean();
    updateTilingParameters();

    if (autoUpdate) updatePromise = update();
  }

  function onParamChanged(newValue: number, idx: number) {
    if (idx < tilingParams.length) {
      tilingParams[idx] = newValue;
    }

    if (autoUpdate) updatePromise = update();
  }

  function onTilingSizeChanged(newValue: number) {
    tilingSize = newValue;
    if (autoUpdate) updatePromise = update();
  }

  function onTileSizeChanged(newValue: number) {
    tileSize = newValue / 100;
    if (autoUpdate) updatePromise = update();
  }

  function onTileRotationChanged(newValue: number) {
    tileRotation = newValue;
    if (autoUpdate) updatePromise = update();
  }

  function getTransformation(M: Matrix, origin: Point, includeMorphedBBox: boolean, scaleCanvasSize: boolean): Matrix {
    // Decompose M into Translation, Rotation, Scale Matrices
    M.e = 0;
    M.f = 0;
    let sx = Math.sqrt(M.a * M.a + M.b * M.b);
    let sy = Math.sqrt(M.c * M.c + M.d * M.d);

    let angle = Math.atan2(M.b, M.a) + tileRotation * (Math.PI / 180);

    let tx: number = -tileCenter.x + origin.x;
    let ty: number = -tileCenter.y + origin.y;

    if (includeMorphedBBox) {
      tx = tx + morphedBBox[0];
      ty = ty + morphedBBox[1];
    }
    if (scaleCanvasSize) {
      sx = (sx * tilingSize * tilingScaleFactor * tileSize) / canvasSize.x;
      sy = (sy * tilingSize * tilingScaleFactor * tileSize) / canvasSize.y;
    } else {
      sx = (sx * tilingSize * tilingScaleFactor * tileSize) / tileWidth;
      sy = (sy * tilingSize * tilingScaleFactor * tileSize) / tileHeight;
    }

    let Mtransform = identity();

    if (Math.sign(M.a) != Math.sign(M.d)) Mtransform = compose(Mtransform, flipX());

    if (Math.sign(M.c) != Math.sign(M.b)) Mtransform = compose(Mtransform, flipY());

    // and reassemble the matrix
    Mtransform = compose(
      rotate(angle, origin.x, origin.y),
      scale(sx, sy, origin.x, origin.y),
      translate(tx, ty),
    );

    return Mtransform;
  }

  function getInverseTransformation(M: Matrix, origin: Point, scaleCanvasSize: boolean): Matrix {
    // Decompose M into Translation, Rotation, Scale Matrices
    M.e = 0;
    M.f = 0;
    let sx = Math.sqrt(M.a * M.a + M.b * M.b);
    let sy = Math.sqrt(M.c * M.c + M.d * M.d);

    let angle = Math.atan2(M.b, M.a) + tileRotation * (Math.PI / 180);

    let tx: number = tileCenter.x - origin.x;
    let ty: number = tileCenter.y - origin.y;

    if (scaleCanvasSize) {
      sx = canvasSize.x / (sx * tilingSize * tilingScaleFactor * tileSize);
      sy = canvasSize.y / (sy * tilingSize * tilingScaleFactor * tileSize);
    } else {
      sx = tileWidth / (sx * tilingSize * tilingScaleFactor * tileSize);
      sy = tileHeight / (sy * tilingSize * tilingScaleFactor * tileSize);
    }

    let Mtransform = identity();

    if (Math.sign(M.a) != Math.sign(M.d)) Mtransform = compose(Mtransform, flipX());

    if (Math.sign(M.c) != Math.sign(M.b)) Mtransform = compose(Mtransform, flipY());

    // and reassemble the matrix
    Mtransform = compose(
      translate(tx, ty), 
      scale(sx, sy, origin.x, origin.y), 
      rotate(-angle, origin.x, origin.y)
    );

    return Mtransform;
  }

  function getPatternUrl(cell: Cell): string {
    return "url(#pattern_" + cell.tileIdx + ")";
  }

  onMount(async () => {
    wasmVoronoi = await instantiate_wasmVoronoi();
    wasmMorph = await instantiate_wasmMorph();

    siteStore.subscribe((value: Sites) => {
      tileWidth = value.tileWidth;
      tileHeight = value.tileHeight;
      morphedBBox = [0, 0, tileWidth, tileHeight];
      tileSitePoints = value.sitePoints;
      tileSiteSegments = value.siteSegments;
      tileCenter = value.tileCenter;
      imageOffset = value.imageOffset;

      if (autoUpdate) updatePromise = update();
    });

    imageStore.subscribe((value: ImageStoreContent) => {
      backgroundImage = value.image;
      imageData = value.imageData;
      imageDataProcessed = value.imageDataProcessed;
      isUptoDate = false;
    });

    originStore.set(symGroups[tilingIdx]);

    tileSitePoints = [];
    tilingSiteSegments = [];
    siteStore.set({
      sitePoints: tilingSitePoints,
      siteSegments: tilingSiteSegments,
      tileWidth: tileWidth,
      tileHeight: tileHeight,
      tileCenter: tileCenter,
      imageOffset: imageOffset,
    });

    tilingScaleFactor = symGroups[tilingIdx].tilingScaleFactor;
  });

</script>

<div class="flex flex-row flex-wrap flex-grow">
  <div class="flex-1 min-w-fit grid grid-cols-1 justify-items-center content-start gap-4 pl-10">
    <!-- svelte-ignore a11y-click-events-have-key-events -->
    <!-- svelte-ignore a11y-no-static-element-interactions -->
    <svg id="voronoiSvg" width={bbox.xh} height={bbox.yh} viewBox="{bbox.xl + 100} {bbox.yl + 100} {bbox.xh - 200} {bbox.yh - 200}" xmlns="http://www.w3.org/2000/svg">
      <rect x={bbox.xl + 100} y={bbox.yl + 100} width={bbox.xh - 200} height={bbox.yh - 200} stroke="rgb(2 132 199)" stroke-width="0.66" fill="white" />
      {#await updatePromise}
        <text x="250" y="220" text-anchor="middle" class="svgText">Loading...</text>
      {:then}
        {#if isUptoDate}
          <defs>
            {#each tiles as tile, idx}
              <pattern id={"pattern_" + tile.tileIdx} patternContentUnits="userSpaceOnUse" patternUnits="userSpaceOnUse" patternTransform={toSVG(getTransformation(tile.M, tile.origin, doMorph, true))} width={morphedBBox[2] - morphedBBox[0]} height={morphedBBox[3] - morphedBBox[1]}>
                <image href={backgroundImage?.src} x={0} y={0} width={morphedBBox[2] - morphedBBox[0]} height={morphedBBox[3] - morphedBBox[1]} />
              </pattern>
            {/each}
          </defs>

          {#each voronoiCells as c}
            {#if showBackground && isCellPathConsistant(c)}
              {#if showBackgroundImage}
                <path id="cell {c.sourceIndex}" d={getCellPath(c)} stroke="black" stroke-width="0" fill={getPatternUrl(c)}></path>
              {:else}
                <path id="cell {c.sourceIndex}" d={getCellPath(c)} stroke="black" stroke-width="0" fill={c.color == 0 ? color1 : c.color == 1 ? color2 : color3}></path>
              {/if}
            {/if}
          {/each}
          {#if showSkeleton}
            {#each tilingSitePoints as siteP, idx}
              <circle id="point {idx}" cx={siteP.x} cy={siteP.y} r="2" fill={skelletonColor}></circle>
            {/each}
            {#each tilingSiteSegments as siteS, idx}
              <path id="segment {idx + tilingSitePoints.length}" d="M {siteS.x1} {siteS.y1} L {siteS.x2} {siteS.y2}" stroke={skelletonColor} stroke-width="0.66" fill="none"></path>
              <circle id="segment {idx + tilingSitePoints.length}" cx={siteS.x1} cy={siteS.y1} r="1" fill={skelletonColor}></circle>
              <circle id="segment {idx + tilingSitePoints.length}" cx={siteS.x2} cy={siteS.y2} r="1" fill={skelletonColor}></circle>
            {/each}
          {/if}
          {#each voronoiEdges as e, idx}
            {#if e.isValid && !e.isWithinCell && showBorder}
              {#if e.isCurved && e.controlPoints.length == 3}
                <path id={"edge_" + idx} d="M {e.controlPoints[0].x} {e.controlPoints[0].y} Q {e.controlPoints[1].x} {e.controlPoints[1].y} {e.controlPoints[2].x} {e.controlPoints[2].y}" stroke={borderColor} stroke-width="0.66" fill="none"></path>
              {:else if e.isPrimary}
                <path id={"edge_" + idx} d="M {e.va.x} {e.va.y} L {e.vb.x} {e.vb.y}" stroke={borderColor} stroke-width="0.66" fill="none"></path>
              {:else if showSecondary}
                <path id={"edge_" + idx} d="M {e.va.x} {e.va.y} L {e.vb.x} {e.vb.y}" stroke="green" stroke-width="0.66" fill="none"></path>
              {/if}
            {:else if !e.isValid}
              <path id={"edge_" + idx} d="M {e.va.x} {e.va.y} L {e.vb.x} {e.vb.y}" stroke="red" stroke-width="0.66" fill="none"></path>
            {/if}
          {/each}
          {#each tiles as tile, idx}
            {#if showOrigins}
              <circle id="origin {idx}" cx={tile.origin.x} cy={tile.origin.y} r="2" fill="pink"></circle>
            {/if}
          {/each}
          {#if showDebugMorphLines}
            {#each outlines as fl, idx}
              <path id={"outline_" + idx} d="M {fl.startPoint.x} {fl.startPoint.y} L {fl.endPoint.x} {fl.endPoint.y}" stroke="yellow" stroke-width="0.66" fill="none"></path>
            {/each}
            {#each morphedSiteSegments as fl, idx}
              <g transform={toSVG(getTransformation(mostCenterTile.M, mostCenterTile.origin, false, true))}>
                <path id={"outlineMorphed_" + idx} d="M {fl.x1} {fl.y1} L {fl.x2} {fl.y2}" stroke="#c300ff" stroke-width="2" fill="none"></path>
              </g>
            {/each}
            <rect x={morphedBBox_transf.xl} y={morphedBBox_transf.yl} width={morphedBBox_transf.xh - morphedBBox_transf.xl} height={morphedBBox_transf.yh - morphedBBox_transf.yl} stroke="green" style="fill:none"> </rect>
          {/if}
        {/if}
      {:catch someError}
        <text x="250" y="250" text-anchor="middle" class="svgText">Error: {someError.message}.</text>
      {/await}
    </svg>
    <button class="bg-blue-500 hover:bg-blue-700 text-white font-bold py-2 px-4 rounded inline-flex items-center min-w-52" on:click={() => (updatePromise = update())}>
      <span class="material-symbols-outlined me-2"> refresh </span>
      Update
    </button>
    <button
      class="bg-blue-500 hover:bg-blue-700 text-white font-bold py-2 px-4 rounded inline-flex items-center min-w-52"
      on:click={() => {
        downloadSVG();
      }}
    >
      <span class="material-symbols-outlined me-2"> download </span>
      Download SVG
    </button>
    <div class="lastErrorContainer max-w-96 max-h-24">
      <p class="text-red-700 text-sm break-words">{lastError}</p>
    </div>
  </div>
  <div class="flex-1 min-w-fit grid grid-cols-1 justify-items-center gap-2 content-start">
    <p class="text-xl font-sans text-center text-sky-400 p-2">Tiling Settings</p>
    <div class="grid grid-cols-4">
      <p class="">Tile Size [%]</p>
      <div class="col-span-3 min-w-72">
        <Range min={50} max={300} stepSize={1} initialValue={tileSize * 100} decimalPlaces={0} on:change={(e) => onTileSizeChanged(e.detail.value)} />
      </div>
    </div>
    <div class="grid grid-cols-4">
      <p class="">Tile Rotation [°]</p>
      <div class="col-span-3 min-w-72">
        <Range min={0} max={360} stepSize={1} initialValue={tileRotation} decimalPlaces={0} on:change={(e) => onTileRotationChanged(e.detail.value)} />
      </div>
    </div>
    <div class="grid grid-cols-4">
      <p class="">Tiling Size [%]</p>
      <div class="col-span-3 min-w-72">
        <Range min={50} max={250} stepSize={1} initialValue={tilingSize} decimalPlaces={0} on:change={(e) => onTilingSizeChanged(e.detail.value)} />
      </div>
    </div>
    <div class="tilingCtrl flex items-center gap-4">
      <button
        class="bg-sky-300 hover:bg-sky-500 text-black font-bold rounded h-10 w-10"
        on:click={() => {
          onTilingMinus();
        }}
      >
        &lt;</button
      >
      <div class="bg-slate-100 flex items-center justify-center p-4 h-10 min-w-52">
        <p class="text-center">
          {symGroups[tilingIdx].name}
        </p>
      </div>
      <img class="max-w-20" src={symGroups[tilingIdx].image} alt="Preview..." />
      <button
        class="bg-sky-300 hover:bg-sky-500 text-black font-bold rounded w-10 h-10"
        on:click={() => {
          onTilingPlus();
        }}
      >
        &gt;
      </button>
    </div>
    <div class="tilingParams">
      {#each tilingParams as p, idx}
        {#if symGroups[tilingIdx].parameterNames[idx]}
          <div class="tilingParam flex flex-row gap-4">
            <p class="basis-1/3">
              {symGroups[tilingIdx].parameterNames[idx].name}
            </p>
            <div class="min-w-72">
              <Range
                id={idx}
                min={symGroups[tilingIdx].parameterNames[idx].min * 100}
                max={symGroups[tilingIdx].parameterNames[idx].max * 100}
                stepSize={symGroups[tilingIdx].parameterNames[idx].stepSize}
                initialValue={symGroups[tilingIdx].parameterNames[idx].initialValue ?? p}
                decimalPlaces={2}
                on:change={(e) => onParamChanged(e.detail.value, idx)}
              />
            </div>
          </div>
        {/if}
      {/each}
    </div>
    <p class="text-xl font-sans text-center text-sky-400 p-2">Morph Settings</p>
    <div class="bg-slate-100 flex items-center justify-center h-10 rounded">
      <label class="p-4">
        <input type="checkbox" bind:checked={doMorph} />
        Morph
      </label>
    </div>
    {#if doMorph}
      <div class="flex flex-col">
        <div class="flex flex-row">
          <p class="pr-5 text-right basis-1/4 min-w-40">no morph</p>
          <div class="min-w-72  basis-1/2">
            <Range min={0} max={100} stepSize={0.01} initialValue={t} decimalPlaces={2} on:change={(e) => (t = Number(e.detail.value))} />
          </div>
          <p class="pl-5 basis-1/4 min-w-40">full morph</p>
        </div>
        <div class="flex flex-row">
          <p class="pr-5 text-right basis-1/4 min-w-40">all lines same</p>
          <div class="min-w-72">
            <Range min={0} max={100} stepSize={0.01} initialValue={p} decimalPlaces={2} on:change={(e) => (p = Number(e.detail.value))} />
          </div>
          <p class="pl-5 basis-1/4 min-w-40">long lines stronger</p>
        </div>
        <div class="flex flex-row">
          <p class="pr-5 text-right basis-1/4 min-w-40">precision</p>
          <div class="min-w-72">
            <Range min={1} max={300} stepSize={0.01} initialValue={a} decimalPlaces={2} on:change={(e) => (a = Number(e.detail.value))} />
          </div>
          <p class="pl-5 basis-1/4">smoothness</p>
        </div>
        <div class="flex flex-row">
          <p class="pr-5 text-right basis-1/4 min-w-40">all lines same</p>
          <div class="min-w-72">
            <Range min={50} max={200} stepSize={0.01} initialValue={b} decimalPlaces={2} on:change={(e) => (b = Number(e.detail.value))} />
          </div>
          <p class="pl-5 basis-1/4 min-w-40">close lines stronger</p>
        </div>
      </div>
    {/if}
    <p class="text-xl font-sans text-center text-sky-400 p-2">Display Settings</p>
    <div class="displaySettings grid grid-cols-3 gap-4 mr-4 ml-4">
      <div class="bg-slate-100 flex items-center justify-left h-10 rounded">
        <label class="p-4">
          <input type="checkbox" bind:checked={showBorder} />
          Border
        </label>
      </div>
      <div class="bg-slate-100 flex items-center justify-left h-10 rounded">
        <label class="p-4">
          <input type="checkbox" bind:checked={showSkeleton} />
          Skeleton
        </label>
      </div>
      <div class="bg-slate-100 flex items-center justify-left h-10 rounded">
        <label class="p-4">
          <input type="checkbox" bind:checked={showOrigins} />
          Origins
        </label>
      </div>
      <div class="bg-slate-100 flex items-center justify-left h-10 rounded">
        <label class="p-4">
          <input type="checkbox" bind:checked={showBackground} />
          Background
        </label>
      </div>
      <div class="bg-slate-100 flex items-center justify-left h-10 rounded">
        <label class="p-4">
          <input type="checkbox" bind:checked={showBackgroundImage} />
          Background Image
        </label>
      </div>
      <div class="bg-slate-100 flex items-center justify-left h-10 rounded">
        <label class="p-4">
          <input type="checkbox" bind:checked={showDebugMorphLines} />
          Debug Morph Lines
        </label>
      </div>
      <div>
        <ColorPicker bind:hex={borderColor} label="Border" />
      </div>
      <div>
        <ColorPicker bind:hex={skelletonColor} label="Skelleton" />
      </div>
      <div class="col-start-1">
        <ColorPicker bind:hex={color1} label="Background 1" />
      </div>
      <div>
        <ColorPicker bind:hex={color2} label="Background 2" />
      </div>
      <div>
        <ColorPicker bind:hex={color3} label="Background 3" />
      </div>
    </div>
  </div>
</div>

<style>
  .svgText {
    font: 20px;
    fill: rgb(29 78 216);
  }
</style>
