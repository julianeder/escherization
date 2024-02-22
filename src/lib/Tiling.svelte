<script lang="ts">
  import { writable, get, type Writable } from "svelte/store";
  import Range from "./Range.svelte";
  import { onMount } from "svelte";
  import {
    BBox,
    type Edge,
    SitePoint,
    SiteSegment,
    type Vertex,
    Sites,
    type Cell,
    type Tile,
    Point,
  } from "./voronoiDataStructures";
  import instantiate_wasmVoronoi, {
    type VoronoiWasmModule,
    type EdgeResult,
    type DiagrammResult,
    type CellResult,
  } from "../lib/wasm/wasmVoronoi";
  import instantiate_wasmMorph, { type FeatureLine, type MorphWasmModule } from "../lib/wasm/wasmMorph";

  import { IsohedralTiling, mul, mulSegment } from "./tactile/tactile";
  import ColorPicker from "svelte-awesome-color-picker";
  import {
    type Matrix,
    compose,
    identity,
    scale,
    toSVG,
    inverse,
    translate,
    rotate,
  } from "transformation-matrix";
  import { ImageStoreContent, imageStore, siteStore } from "./state";
    import { checkIntersections } from "./collisionDetection";

  let wasmVoronoi: VoronoiWasmModule;
  let wasmMorph: MorphWasmModule;
  let bbox: BBox = new BBox(0, 500, 0, 500);

  let tiles: Tile[] = [];
  let tileSitePoints: SitePoint[] = [];
  let tileSiteSegments: SiteSegment[] = [];
  let outlines: FeatureLine[][] = []; 

  let tilingSitePoints: SitePoint[] = [];
  let tilingSiteSegments: SiteSegment[] = [];
  let tileWidth: number = 300;
  let tileHeight: number = 300;
  let voronoiVertices: Vertex[] = [];
  let voronoiEdges: Edge[] = [];
  let voronoiCells: Cell[] = [];
  let tilingSize: number = 100;
  let tileSize: number = 1;
  let tileCenter: Point;
  let imageOffset: Point;

  let backgroundImage: HTMLImageElement | null;
  let backgroundImageData: ImageData | null;

  let showSecondary: boolean = false;

  let lastError: any = "";

  let tilingParams: number[] = [];

  let autoUpdate: boolean = false;
  let showBorder: boolean = true;
  let showSkeleton: boolean = true;
  let showOrigins: boolean = true;
  let showBackground: boolean = true;
  let showBackgroundImage: boolean = true;

  const symGroups: Record<string, any> = {
    "1": "p1",
    "2": "pg",
    "4": "p2",
    "5": "pgg",
    "7": "p3",
    "11": "p6",
    "12": "cm",
    "13": "pmg",
    "16": "p31m",
    "26": "cmm",
    "28": "p4",
    "37": "p6m",
    "42": "pm",
    "56": "p4g",
    "72": "pmm",
    "76": "p4m",
  };

  const availableTilings: string[] = Object.keys(symGroups);
  let tilingIdx = 0;
  let prevTilingIndex = -1;

  let color1: string = "#e06d4380";
  let color2: string = "#59da5980";
  let color3: string = "#c3c63580";

  function update() {
    updateTiling();

    if(checkIntersections(tilingSiteSegments)){
      lastError = "Collision between tiles detected, please change the paremeters (e.g. decrease Tile Size)"
    }
    else{
      lastError = ""
      updateVoronoi();
      updateMorph();
    }
  }

  function updateMorph(){

    // Get Outline
    outlines = [];
    for (let i = 0; i < tiles.length; i++) {
      if(tiles[i].tileIdx == 14){
        let outline: FeatureLine[] = [];
        // console.log(tiles[i].tileIdx + " " + tiles[i].origin.x + " " + tiles[i].origin.y);
        voronoiCells.filter(c => c.tileIdx == tiles[i].tileIdx).forEach(c => {
          // console.log(c)
          c.edgeIndices.forEach(idxE => {
            if(voronoiEdges[idxE].isPrimary && !voronoiEdges[idxE].isBetweenSameColorCells){              
              // console.log(voronoiEdges[idxE])
              let featureLine: FeatureLine = {
                startPoint: {x: voronoiEdges[idxE].va.x, y: voronoiEdges[idxE].va.y},
                endPoint:{x: voronoiEdges[idxE].vb.x, y: voronoiEdges[idxE].vb.y}
              } 
              outline.push(featureLine);
            }
          })
        });
        outlines.push(outline);
      }
    }
    console.log(outlines);
    outlines = outlines; // Reactive Update

    // Morphing
    if(backgroundImageData != null){ 
        console.log("do Morph");
        
        const imageDataVector = new wasmMorph.VectorByte();
        backgroundImageData.data.forEach((b) => imageDataVector.push_back(b));
        
        
        const srcLinesVector = new wasmMorph.VectorFeatureLine();
        tileSiteSegments.forEach((ss) => srcLinesVector.push_back({
          startPoint: {x: ss.x1, y: ss.y1},
          endPoint: {x: ss.x2, y: ss.y2}
        }))
        
        const morphLinesVector = new wasmMorph.VectorFeatureLine();
        tileSiteSegments.forEach((ss) => morphLinesVector.push_back({
          startPoint: {x: ss.x1, y: ss.y1},
          endPoint: {x: ss.x2, y: ss.y2}
        }))

        
        let result = wasmMorph.doMorph(backgroundImageData.width, backgroundImageData.height, 
          imageDataVector, 
          srcLinesVector, 
          morphLinesVector);     
        
        console.log("result.size " + result.size());
        for (let i = 0; i < result.size(); i++) {
          // let e: number = result.get(i);
          backgroundImageData.data[i] = result.get(i);
        }
        console.log(backgroundImageData.data);
        backgroundImage = imagedataToImage(backgroundImageData);

      }
    }

  function imagedataToImage(imagedata: ImageData) {
      var canvas = document.createElement('canvas');
      var ctx = canvas.getContext('2d');
      canvas.width = imagedata.width;
      canvas.height = imagedata.height;
      ctx!.putImageData(imagedata, 0, 0);

      var image = new Image();
      image.src = canvas.toDataURL();
      return image;
  }

  function updateTiling() {
    // console.log(
    //   "updateTiling " +
    //     tileWidth +
    //     "x" +
    //     tileHeight +
    //     " l " +
    //     tileSitePoints.length +
    //     " / " +
    //     tileSiteSegments.length,
    // );

    tiles = [];
    tilingSitePoints = [];
    tilingSiteSegments = [];
    let tiling: IsohedralTiling = new IsohedralTiling(
      Number(availableTilings[tilingIdx]),
    );

    if (prevTilingIndex == tilingIdx) {
      tiling.setParameters(tilingParams);
    } else {
      if (tiling.numParameters() > 0) {
        tilingParams = tiling.getParameters();
      } else {
        tilingParams = [];
      }
    }
    // console.log(tilingParams);

    prevTilingIndex = tilingIdx;

    for (let i of tiling.fillRegionBounds(
      bbox.xl / tilingSize,
      bbox.yl / tilingSize,
      bbox.xh / tilingSize,
      bbox.yh / tilingSize,
    )) {
      // Use a simple colouring algorithm to pick a colour for this tile
      // so that adjacent tiles aren't the same colour.  The resulting
      // value col will be 0, 1, or 2, which you should map to your
      // three favourite colours.
      const color = tiling.getColour(i.t1, i.t2, i.aspect);
      // Get the 3x3 matrix corresponding to one of the transformed
      // tiles in the filled region.
      const M = i.T;
      // console.log(
      //   M[0] + " " + M[1] + " " + M[2] + "\n"
      // + M[3] + " " + M[4] + " " + M[5] + "\n"
      // + 0 + " " + 0 + " " + 0 + "\n"
      // );

      let origin = scalePoint(
        SitePoint.mulPoint(M, new SitePoint(0, 0)),
        tilingSize,
        tilingSize,
      );
      let tile: Tile = { origin: origin, M: M, tileIdx: tiles.length - 1 }; //tileIndex counting up
      tiles.push(tile);

      for (let j = 0; j < tileSitePoints.length; j++) {
        let newSitePoint: SitePoint = scalePoint(
          SitePoint.mulPoint(
            M,
            scalePoint(
              tileSitePoints[j],
              tileSize / tileWidth,
              tileSize / tileHeight,
            ),
          ),
          tilingSize,
          tilingSize,
        );

        newSitePoint.color = color;
        newSitePoint.tileIdx = tile.tileIdx;
        newSitePoint.M = M;
        if(bbox.contains(newSitePoint.x, newSitePoint.y))
          tilingSitePoints.push(newSitePoint);
      }

      for (let j = 0; j < tileSiteSegments.length; j++) {
        let newSiteSegment: SiteSegment = scaleSegment(
          mulSegment(
            M,
            scaleSegment(tileSiteSegments[j], tileSize / 300, tileSize / 300),
          ),
          tilingSize,
          tilingSize,
        );
        newSiteSegment.color = color;
        newSiteSegment.M = M;
        newSiteSegment.tileIdx = tile.tileIdx;

        if(bbox.contains(newSiteSegment.x1, newSiteSegment.y1) || bbox.contains(newSiteSegment.x2, newSiteSegment.y2))
          tilingSiteSegments.push(newSiteSegment);
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

      // console.log(tilingSiteSegments[72]);

      let result: DiagrammResult = wasmVoronoi.computevoronoi(
        bboxVector,
        pointVector,
        segmentVector,
        pointColorVector,
        segmentColorVector,
        pointTileIdxVector,
        segmentTileIdxVector,
      );
      let newVoronoiVertices: Vertex[] = [];
      for (let i = 0; i < result.vertices.size(); i += 2) {
        // console.log("Vert: ", result.vertices.get(i));
        let valid = true;
        if (
          Number.isNaN(result.vertices.get(i)) ||
          Number.isNaN(result.vertices.get(i + 1))
        ) {
          console.log(
            "Vertex " +
              i +
              ": " +
              result.vertices.get(i) +
              " " +
              result.vertices.get(i + 1),
          );
          valid = false;
        }
        newVoronoiVertices.push({
          x: result.vertices.get(i),
          y: result.vertices.get(i + 1),
          isValid: valid,
        });
      }

      let newVoronoiEdges: Edge[] = [];
      for (let i = 0; i < result.edges.size(); i++) {
        let e: EdgeResult = result.edges.get(i);
        let controlPoints: Vertex[] = [];
        let valid = true;
        for (let j = 0; j < e.controll_points.size(); j += 2) {
          if (
            Number.isNaN(e.controll_points.get(j)) ||
            Number.isNaN(e.controll_points.get(j + 1))
          ) {
            console.log(
              "CP " +
                i +
                ": " +
                e.controll_points.get(j) +
                " " +
                e.controll_points.get(j + 1),
            );
            valid = false;
          }
          const v: Vertex = {
            x: e.controll_points.get(j),
            y: e.controll_points.get(j + 1),
            isValid: valid,
          };
          controlPoints.push(v);
        }

        if (
          Number.isNaN(e.x1) ||
          Number.isNaN(e.y1) ||
          Number.isNaN(e.x2) ||
          Number.isNaN(e.y2)
        ) {
          console.log(
            "Edge " + i + ": " + e.x1 + " " + e.y1 + " " + e.x2 + " " + e.y2,
          );
          valid = false;
        }

        newVoronoiEdges.push({
          va: { x: e.x1, y: e.y1, isValid: valid },
          vb: { x: e.x2, y: e.y2, isValid: valid },
          // samples: samples,
          isCurved: e.isCurved,
          controlPoints: controlPoints,
          isPrimary: e.isPrimary,
          isValid: valid,
          isBetweenSameColorCells: e.isBetweenSameColorCells,
        });

        // if (newVoronoiEdges.length == 98) {
        //   console.log(newVoronoiEdges[97]);
        // }

        // console.log(e.isBetweenSameColorCells);
      }

      let newVoronoiCells: Cell[] = [];
      for (let i = 0; i < result.cells.size(); i++) {
        let c: CellResult = result.cells.get(i);
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
          let newIndex: number = c.edgeIndices.get(j);
          if (newIndex >= newVoronoiEdges.length)
            console.warn(
              "Index too high " +
                newIndex +
                " for cell " +
                c.sourceIndex +
                " isDegenerate " +
                c.isDegenerate,
            );
          else newCell.edgeIndices.push(newIndex);
        }
        // console.log(newVoronoiCells[newVoronoiCells.length-1]);
      }

      voronoiVertices = newVoronoiVertices;
      voronoiEdges = newVoronoiEdges;
      voronoiCells = newVoronoiCells;

      // checks();
    } catch (e) {
      lastError = e;
      throw e;
    }
  }

  function scalePoint(p: SitePoint, sX: number, sY: number): SitePoint {
    return new SitePoint(p.x * sX, p.y * sY, p.color, p.M, p.tileIdx);
  }

  function scaleSegment(s: SiteSegment, sX: number, sY: number) {
    return new SiteSegment(
      s.x1 * sX,
      s.y1 * sY,
      s.x2 * sX,
      s.y2 * sY,
      s.color,
      s.M,
      s.tileIdx,
    );
  }

  function translateSegment(s: SiteSegment, offset: Point): SiteSegment {
    return new SiteSegment(
      s.x1 + offset.x,
      s.y1 + offset.y,
      s.x2 + offset.x,
      s.y2 + offset.y,
      s.color,
      s.M,
      s.tileIdx,
    );
  }

  function translatePoint(p: SitePoint, offset: Point): SitePoint {
    return new SitePoint(
      p.x + offset.x,
      p.y + offset.y,
      p.color,
      p.M,
      p.tileIdx,
    );
  }



  function addPoint(event: MouseEvent) {
    tilingSitePoints = [
      ...tilingSitePoints,
      new SitePoint(event.offsetX, event.offsetY, undefined, [], -1),
    ]; // This syntax triggeres Sveltes reactive reload
    siteStore.set({
      sitePoints: tilingSitePoints,
      siteSegments: tilingSiteSegments,
      tileWidth: tileWidth,
      tileHeight: tileHeight,
      tileCenter: tileCenter,
      imageOffset: imageOffset,
    });
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

      if (
        voronoiEdges[c.edgeIndices[i - 1]].vb.x !=
          voronoiEdges[c.edgeIndices[i]].va.x ||
        voronoiEdges[c.edgeIndices[i - 1]].vb.y !=
          voronoiEdges[c.edgeIndices[i]].va.y
      )
        return false;
      if (
        Number.isNaN(voronoiEdges[c.edgeIndices[i]].va.x) ||
        Number.isNaN(voronoiEdges[c.edgeIndices[i]].va.y)
      )
        return false;
    }

    // if (
    //   voronoiEdges[c.edgeIndices[i - 1]].vb.x !=
    //   voronoiEdges[c.edgeIndices[i]].va.x ||
    //   voronoiEdges[c.edgeIndices[i - 1]].vb.y !=
    //   voronoiEdges[c.edgeIndices[i]].va.y
    //   )
    //   return false;

    // close loop
    let i = c.edgeIndices.length - 1;
    if (
      voronoiEdges[c.edgeIndices[i]].vb.x !=
        voronoiEdges[c.edgeIndices[0]].va.x ||
      voronoiEdges[c.edgeIndices[i]].vb.y != voronoiEdges[c.edgeIndices[0]].va.y
    )
      return false;

    return true;
  }

  function getCellPath(c: Cell): string {
    // console.log("Cell " + c.sourceIndex);
    let e: Edge = voronoiEdges[c.edgeIndices[0]];

    let d: string;
    if (e.isCurved) {
      d =
        "M " +
        e.controlPoints[0].x +
        " " +
        e.controlPoints[0].y +
        "Q " +
        e.controlPoints[1].x +
        " " +
        e.controlPoints[1].y +
        " " +
        e.controlPoints[2].x +
        " " +
        e.controlPoints[2].y +
        " ";
    } else {
      d =
        "M " +
        voronoiEdges[c.edgeIndices[0]].va.x +
        " " +
        voronoiEdges[c.edgeIndices[0]].va.y +
        " ";
    }

    // console.log("(" + voronoiEdges[c.edgeIndices[0]].va.x + ' ' + voronoiEdges[c.edgeIndices[0]].va.y + ") ("
    //                 + voronoiEdges[c.edgeIndices[0]].vb.x + ' ' + voronoiEdges[c.edgeIndices[0]].vb.y + ")");

    for (let i: number = 1; i < c.edgeIndices.length; i++) {
      let e: Edge = voronoiEdges[c.edgeIndices[i]];
      let eprev: Edge = voronoiEdges[c.edgeIndices[i - 1]];

      if (e.isCurved) {
        d =
          d +
          "L " +
          e.controlPoints[0].x +
          " " +
          e.controlPoints[0].y +
          "Q " +
          e.controlPoints[1].x +
          " " +
          e.controlPoints[1].y +
          " " +
          e.controlPoints[2].x +
          " " +
          e.controlPoints[2].y +
          " ";
      } else {
        d = d + "L " + e.va.x + " " + e.va.y + " ";
        // console.log("(" + e.va.x + ' ' + e.va.y + ") ("
        //               + e.vb.x + ' ' + e.vb.y + ")");
      }

      if (eprev.vb.x != e.va.x || eprev.vb.y != e.va.y)
        console.log(
          "Inconsistent: (" +
            eprev.vb.x +
            " " +
            eprev.vb.y +
            ") (" +
            e.va.x +
            " " +
            e.va.y +
            ")",
        );
    }
    let i = c.edgeIndices.length - 1;
    e = voronoiEdges[c.edgeIndices[i]];
    let eprev: Edge = voronoiEdges[c.edgeIndices[i - 1]];

    if (!eprev.isCurved) {
      d = d + "L " + e.vb.x + " " + e.vb.y + " ";
      // console.log("(" + e.va.x + ' ' + e.va.y + ") ("
      //                 + e.vb.x + ' ' + e.vb.y + ")");
    }

    if (eprev.vb.x != e.va.x || eprev.vb.y != e.va.y)
      console.log(
        "Inconsistent: (" +
          eprev.vb.x +
          " " +
          eprev.vb.y +
          ") (" +
          e.va.x +
          " " +
          e.va.y +
          ")",
      );

    if (
      e.vb.x != voronoiEdges[c.edgeIndices[0]].va.x ||
      e.vb.y != voronoiEdges[c.edgeIndices[0]].va.y
    )
      console.log(
        "Inconsistent: (" +
          e.vb.x +
          " " +
          e.vb.y +
          ") (" +
          voronoiEdges[c.edgeIndices[0]].va.x +
          " " +
          voronoiEdges[c.edgeIndices[0]].va.y +
          ")",
      );

    return d;
  }

  function onTilingPlus() {
    tilingIdx = (tilingIdx + 1) % availableTilings.length;

    if (autoUpdate) update();
  }

  function onTilingMinus() {
    tilingIdx = tilingIdx - 1;
    if (tilingIdx < 0) tilingIdx = availableTilings.length - 1;

    if (autoUpdate) update();
  }

  function onParamChanged(newValue: number, idx: number) {
    if (idx < tilingParams.length) {
      tilingParams[idx] = newValue;
    }

    if (autoUpdate) update();
  }

  function onResetParams() {
    let tiling: IsohedralTiling = new IsohedralTiling(
      Number(availableTilings[tilingIdx]),
    );
    if (tiling.numParameters() > 0) {
      tilingParams = tiling.getParameters();
    } else {
      tilingParams = [];
    }

    tilingSize = 100;
    tileSize = 1;

    if (autoUpdate) update();
  }

  onMount(async () => {
    wasmVoronoi = await instantiate_wasmVoronoi();
    wasmMorph = await instantiate_wasmMorph();


    siteStore.subscribe((value: Sites) => {
      tileWidth = value.tileWidth;
      tileHeight = value.tileHeight;
      tileSitePoints = value.sitePoints;
      tileSiteSegments = value.siteSegments;
      tileCenter = value.tileCenter;
      imageOffset = value.imageOffset;

      if (autoUpdate) update();
    });

    imageStore.subscribe((value: ImageStoreContent) => {
      backgroundImage = value.image;
      backgroundImageData = value.imageData;
      // console.log(backgroundImage);
      // if (autoUpdate) update();
    });

    tilingSitePoints = [
      new SitePoint(100, 50),
      new SitePoint(300, 50),
      new SitePoint(100, 250),
      new SitePoint(300, 250),
      new SitePoint(200, 150),
    ];
    tilingSiteSegments = [
      // new SiteSegment(2, 2, 498, 2),
      // new SiteSegment(498, 2, 498, 298),
      // new SiteSegment(498, 298, 2, 298),
      // new SiteSegment(2, 298, 2, 2),
      // new SiteSegment(100, 100, 100, 200),
      // new SiteSegment(200, 100, 200, 200),
    ];
    siteStore.set({
      sitePoints: tilingSitePoints,
      siteSegments: tilingSiteSegments,
      tileWidth: tileWidth,
      tileHeight: tileHeight,
      tileCenter: tileCenter,
      imageOffset: imageOffset,
    });
  });

  function color1Changed(rgba: any) {
    console.log(rgba);
  }

  function onTilingSizeChanged(newValue: number) {
    tilingSize = newValue;
    if (autoUpdate) update();
  }
  function onTileSizeChanged(newValue: number) {
    tileSize = newValue / 100;
    if (autoUpdate) update();
  }

  function getTransformation(mat: number[], origin: Point): string {
    let M: Matrix = {
      a: mat[0],
      b: mat[3],
      c: mat[1],
      d: mat[4],
      e: mat[2],
      f: mat[5],
    };

    // Decompose M into Translation, Rotation, Scale Matrices
    let T: Matrix = { a: 0, b: 0, c: 0, d: 0, e: M.e, f: M.f };
    M.e = 0; //MM.e*tilingSize;
    M.f = 0; //MM.f*tilingSize;
    let sx = Math.sqrt(M.a * M.a + M.b * M.b);
    let sy = Math.sqrt(M.c * M.c + M.d * M.d);
    let R: Matrix = {
      a: M.a / sx,
      b: M.b / sx,
      c: M.c / sy,
      d: M.d / sy,
      e: 0,
      f: 0,
    };
    let angle = Math.atan2(M.b, M.a);

    let Mtransform = compose(
      rotate(angle, origin.x, origin.y),
      scale(
        (sx * tilingSize * tileSize) / 300,
        (sy * tilingSize * tileSize) / 300,
        origin.x,
        origin.y,
      ),
      translate(-tileCenter.x + origin.x, -tileCenter.y + origin.y),
    );
    // console.log("tileCenter: " + tileCenter.x +" "+ tileCenter.y +" origin:  " + origin.x +" "+ origin.y + " imageOffset: "  + imageOffset.x +" "+ imageOffset.y +  " / " + toSVG(Mtransform))
    // console.log("e: " + MM.e + " f "+ MM.f)
    return toSVG(Mtransform);
  }

  function getInverseTransformation(cell: Cell): string {
    if (cell.containsPoint) {
      // let M: number[] = tilingSitePoints[c.sourceIndex].M;
      // M.join(", ")
      // return "matrix(" + M.join(", ") + ")";
      return "";
    } else if (cell.containsSegment) {
      let M: number[] = tilingSiteSegments[cell.sourceIndex].M;
      let MM: Matrix = { a: M[0], b: M[3], c: M[1], d: M[4], e: M[2], f: M[5] };

      let Mtransform = compose(
        scale(1 / tilingSize),
        inverse(MM),
        scale(tileWidth / tileSize, tileHeight / tileSize),
      );

      return toSVG(Mtransform);
    }
    return "";
  }

  function getPatternUrl(cell: Cell): string {
    return "url(#pattern_" + cell.tileIdx + ")";
  }
</script>

<div class="grid grid-cols-1 justify-items-center gap-4">
  <!-- svelte-ignore a11y-click-events-have-key-events -->
  <!-- svelte-ignore a11y-no-static-element-interactions -->
  <svg
    id="voronoiSvg"
    width={bbox.xh}
    height={bbox.yh}
    viewBox="{bbox.xl} {bbox.yl} {bbox.xh} {bbox.yh}"
    xmlns="http://www.w3.org/2000/svg"
    on:click={addPoint}
  >
    <defs>
      {#each tiles as tile, idx}
        <pattern
          id={"pattern_" + idx}
          patternContentUnits="userSpaceOnUse"
          patternUnits="userSpaceOnUse"
          patternTransform={getTransformation(tile.M, tile.origin)}
          width={tileWidth}
          height={tileHeight}
        >
          <image
            href={backgroundImage?.src}
            x={0}
            y={0}
            width={tileWidth}
            height={tileHeight}
          />
        </pattern>
      {/each}
    </defs>
    <rect
      x="0"
      y="0"
      width={bbox.xh}
      height={bbox.yh}
      stroke="rgb(2 132 199)"
      stroke-width="1"
      fill="rgb(248 250 252)"
    />
    {#each voronoiCells as c}
      {#if showBackground && isCellPathConsistant(c)}
        {#if showBackgroundImage}
          <path
            id="cell {c.sourceIndex}"
            d={getCellPath(c)}
            stroke="black"
            stroke-width="0"
            fill={getPatternUrl(c)}
          ></path>
        {:else}
          <path
            id="cell {c.sourceIndex}"
            d={getCellPath(c)}
            stroke="black"
            stroke-width="0"
            fill={c.color == 0 ? color1 : c.color == 1 ? color2 : color3}
          ></path>
        {/if}
      {/if}
    {/each}
    {#if showSkeleton}
      {#each tilingSitePoints as siteP, idx}
        <circle id="point {idx}" cx={siteP.x} cy={siteP.y} r="2" fill="red"
        ></circle>
      {/each}
      {#each tilingSiteSegments as siteS, idx}
        <path
          id="segment {idx + tilingSitePoints.length}"
          d="M {siteS.x1} {siteS.y1} L {siteS.x2} {siteS.y2}"
          stroke="red"
          stroke-width="1"
          fill="none"
        ></path>
        <circle
          id="segment {idx + tilingSitePoints.length}"
          cx={siteS.x1}
          cy={siteS.y1}
          r="2"
          fill="red"
        ></circle>
        <circle
          id="segment {idx + tilingSitePoints.length}"
          cx={siteS.x2}
          cy={siteS.y2}
          r="2"
          fill="red"
        ></circle>
      {/each}
    {/if}
    {#each voronoiEdges as e, idx}
      {#if e.isValid && !e.isBetweenSameColorCells && showBorder}
        <circle cx={e.va.x} cy={e.va.y} r="2" fill="green"></circle>
        <circle cx={e.vb.x} cy={e.vb.y} r="2" fill="green"></circle>
        {#if e.isCurved && e.controlPoints.length == 3}
          <path
            id={"edge_" + idx}
            d="M {e.controlPoints[0].x} {e.controlPoints[0].y} Q {e
              .controlPoints[1].x} {e.controlPoints[1].y} {e.controlPoints[2]
              .x} {e.controlPoints[2].y}"
            stroke="blue"
            stroke-width="1"
            fill="none"
          ></path>
        {:else if e.isPrimary}
          <path
            id={"edge_" + idx}
            d="M {e.va.x} {e.va.y} L {e.vb.x} {e.vb.y}"
            stroke="blue"
            stroke-width="1"
            fill="none"
          ></path>
        {:else if showSecondary}
          <path
            id={"edge_" + idx}
            d="M {e.va.x} {e.va.y} L {e.vb.x} {e.vb.y}"
            stroke="green"
            stroke-width="1"
            fill="none"
          ></path>
        {/if}
      {/if}
    {/each}
    {#each tiles as tile, idx}
      {#if showOrigins}
        <circle
          id="origin {idx}"
          cx={tile.origin.x}
          cy={tile.origin.y}
          r="5"
          fill="pink"
        ></circle>
      {/if}
    {/each}
    {#each outlines as outline}
      {#each outline as fl, idx}
        <path
          id={"outline_" + idx}
          d="M {fl.startPoint.x} {fl.startPoint.y} L {fl.endPoint.x} {fl.endPoint.y}"
          stroke="yellow"
          stroke-width="1"
          fill="none"
        ></path>
        {/each}
      {/each}
    </svg>
  <div class="lastErrorContainer max-w-96">
    <p class="text-red-700 text-sm break-words">{lastError}</p>
  </div>
  <!-- <div class="grid grid-rows-4 content-start"> -->
  <p class="text-xl font-sans text-center text-sky-400 p-4">Tiling Settings</p>
  <div class="grid grid-cols-4">
    <p class="">Tile Size [%]</p>
    <div class="col-span-3 min-w-72">
      <Range
        min={50}
        max={300}
        stepSize={1}
        initialValue={tileSize * 100}
        decimalPlaces={0}
        on:change={(e) => onTileSizeChanged(e.detail.value)}
      />
    </div>
  </div>
  <div class="grid grid-cols-4">
    <p class="">Tiling Size [%]</p>
    <div class="col-span-3 min-w-72">
      <Range
        min={50}
        max={300}
        stepSize={1}
        initialValue={tilingSize}
        decimalPlaces={0}
        on:change={(e) => onTilingSizeChanged(e.detail.value)}
      />
    </div>
  </div>
  <div class="tilingCtrl grid grid-cols-8 gap-4">
    <button
      class="bg-sky-300 hover:bg-sky-500 text-white font-bold rounded"
      on:click={() => {
        onTilingMinus();
      }}
    >
      &lt;</button
    >
    <div class="bg-slate-100 flex items-center justify-center col-span-2">
      <p class="text-center">
        IH {availableTilings[tilingIdx]} / {symGroups[
          availableTilings[tilingIdx]
        ]}
      </p>
    </div>
    <button
      class="bg-sky-300 hover:bg-sky-500 text-white font-bold rounded"
      on:click={() => {
        onTilingPlus();
      }}
    >
      &gt;</button
    >
    <button
      class="bg-blue-500 hover:bg-blue-700 text-white font-bold rounded col-span-2"
      on:click={() => {
        update();
      }}>Update</button
    >
    <div class="bg-slate-100 flex items-center justify-center col-span-2">
      <label class="p-2">
        <input type="checkbox" bind:checked={autoUpdate} />
        Auto Update
      </label>
    </div>
  </div>
  <div class="tilingParams">
    {#each tilingParams as p, idx}
      <div class="tilingParam flex flex-row gap-4">
        <p class="basis-1/12">p {idx}</p>
        <div class="min-w-72">
          <Range
            id={idx}
            min={1}
            max={200}
            stepSize={0.1}
            initialValue={p}
            decimalPlaces={2}
            on:change={(e) => onParamChanged(e.detail.value, idx)}
          />
        </div>
      </div>
    {/each}
  </div>
  <div class="actionButtons grid grid-cols-2 gap-4 max-h-10">
    <button
      class="bg-blue-500 hover:bg-blue-700 text-white font-bold py-2 px-4 rounded min-w-40"
      on:click={() => {
        onResetParams();
      }}
    >
      Reset Params
    </button>
    <button
      class="bg-blue-500 hover:bg-blue-700 text-white font-bold py-2 px-4 rounded min-w-40"
      on:click={() => {
        downloadSVG();
      }}>Download SVG</button
    >
  </div>
  <p class="text-xl font-sans text-center text-sky-400 p-4">Display Settings</p>
  <div class="displaySettings grid grid-cols-3 gap-4">
    <div class="bg-slate-100 flex items-center justify-center h-10">
      <label class="p-2">
        <input type="checkbox" bind:checked={showBorder} />
        Show Border
      </label>
    </div>
    <div class="bg-slate-100 flex items-center justify-center h-10">
      <label class="p-2">
        <input type="checkbox" bind:checked={showSkeleton} />
        Show Skeleton
      </label>
    </div>
    <div class="bg-slate-100 flex items-center justify-center h-10">
      <label class="p-2">
        <input type="checkbox" bind:checked={showOrigins} />
        Show Origins
      </label>
    </div>
    <div class="bg-slate-100 flex items-center justify-center h-10">
      <label class="p-2">
        <input type="checkbox" bind:checked={showBackground} />
        Show Background
      </label>
    </div>
    <div class="bg-slate-100 flex items-center justify-center h-10">
      <label class="p-2">
        <input type="checkbox" bind:checked={showBackgroundImage} />
        Show Background Image
      </label>
    </div>
  </div>
  <div class="colorSettings grid grid-cols-3 gap-4 min-h-80">
    <ColorPicker bind:hex={color1} label="Color 1" />
    <ColorPicker bind:hex={color2} label="Color 2" />
    <ColorPicker bind:hex={color3} label="Color 3" />
  </div>
</div>

<style>
</style>
