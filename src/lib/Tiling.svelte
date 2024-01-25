<script lang="ts">
  import { writable, get, type Writable } from "svelte/store";
  import { onMount } from "svelte";
  import {
    BBox,
    type Edge,
    SitePoint,
    SiteSegment,
    type Vertex,
    Sites,
    type Cell,
  } from "./voronoiDataStructures";
  import instantiate_wasmVoronoi, {
    type VoronoiWasmModule,
    type EdgeResult,
    type DiagrammResult,
    type CellResult,
  } from "../lib/wasm/wasmVoronoi";

  export let siteStore: Writable<Sites>;

  let wasmVoronoi: VoronoiWasmModule;
  let bbox: BBox = { xl: 0, xh: 500, yl: 0, yh: 300 };

  let sitePoints: SitePoint[] = [];
  let siteSegments: SiteSegment[] = [];
  let voronoiVertices: Vertex[] = [];
  let voronoiEdges: Edge[] = [];
  let voronoiCells: Cell[] = [];

  let showSecondary: boolean = true;

  let lastError: any = "";

  function updateVoronoi() {
    try {
      const bboxVector = new wasmVoronoi.VectorDouble();
      bboxVector.push_back(bbox.xl);
      bboxVector.push_back(bbox.yl);
      bboxVector.push_back(bbox.xh);
      bboxVector.push_back(bbox.yh);

      const pointVector = new wasmVoronoi.VectorInt();
      sitePoints.forEach((sp) => {
        pointVector.push_back(sp.x);
        pointVector.push_back(sp.y);
      });
      const segmentVector = new wasmVoronoi.VectorInt();
      siteSegments.forEach((ss) => {
        segmentVector.push_back(ss.x1);
        segmentVector.push_back(ss.y1);
        segmentVector.push_back(ss.x2);
        segmentVector.push_back(ss.y2);
      });

      let result: DiagrammResult = wasmVoronoi.computevoronoi(
        bboxVector,
        pointVector,
        segmentVector,
      );
      let newVoronoiVertices: Vertex[] = [];
      for (var i = 0; i < result.vertices.size(); i += 2) {
        // console.log("Vert: ", result.vertices.get(i));
        newVoronoiVertices.push({
          x: result.vertices.get(i),
          y: result.vertices.get(i + 1),
        });
      }

      let newVoronoiEdges: Edge[] = [];
      for (var i = 0; i < result.edges.size(); i++) {
        let e: EdgeResult = result.edges.get(i);
        // let samples: Vertex[] = [];
        // for (var j = 0; j < e.samples.size(); j+=2) {
        //   samples.push({x: e.samples.get(j), y: e.samples.get(j+1)})
        // }
        let controlPoints: Vertex[] = [];
        let valid = true;
        for (var j = 0; j < e.controll_points.size(); j += 2) {
          const v: Vertex = {
            x: e.controll_points.get(j),
            y: e.controll_points.get(j + 1),
          };
          if (Number.isNaN(v.x) || Number.isNaN(v.y)) {
            // Sometimes Calculation failes for very short paths -> ignore
            valid = false;
            console.warn("Invalid Parabolar (NaN): " + e);
          }
          controlPoints.push(v);
        }
        if (!valid) continue;

        newVoronoiEdges.push({
          va: { x: e.x1, y: e.y1 },
          vb: { x: e.x2, y: e.y2 },
          // samples: samples,
          isCurved: e.isCurved,
          controlPoints: controlPoints,
          isPrimary: e.isPrimary,
        });
      }

      let newVoronoiCells: Cell[] = [];
      for (var i = 0; i < result.cells.size(); i++) {
        let c: CellResult = result.cells.get(i);
        let newCell: Cell = {
          sourceIndex: c.sourceIndex, 
          sourceCategory: c.sourceCategory, 
          isDegenerate: c.isDegenerate, 
          containsPoint: c.containsPoint, 
          containsSegment: c.containsSegment, 
          edgeIndices: [],           
        };
        newVoronoiCells.push(newCell);
        for (var j = 0; j < c.edgeIndices.size(); j++) {
          let newIndex: number = c.edgeIndices.get(j);
          if(newIndex >= newVoronoiEdges.length)
            console.warn("Index too high " + newIndex + " for cell " + c.sourceIndex);
          else
            newCell.edgeIndices.push(newIndex);
        }
        // console.log(newVoronoiCells[newVoronoiCells.length-1]);
      }

      // console.log(newVoronoiEdges)
      voronoiVertices = newVoronoiVertices;
      voronoiEdges = newVoronoiEdges;
      voronoiCells = newVoronoiCells;

      checks();

    } catch (e) {
      lastError = e;
      throw e;
    }
  }

  function checks(){
    for (var i = voronoiVertices.length-1; i >= 0; i--) {
      if(Number.isNaN(voronoiVertices[i].x) ||Number.isNaN(voronoiVertices[i].y)){
        console.log("V NaN " + i);
        console.log(voronoiVertices[i]);
        voronoiVertices.splice(i, 1);
      }
    }

    for (var i = voronoiEdges.length-1; i >= 0; i--) {
      if(Number.isNaN(voronoiEdges[i].va.x) ||Number.isNaN(voronoiEdges[i].va.y)){
        console.log("E NaN " + i);
        console.log(voronoiEdges[i]);
        voronoiEdges[i].va.x = 0;
        voronoiEdges[i].va.y = 0;

      }
      if(Number.isNaN(voronoiEdges[i].vb.x) ||Number.isNaN(voronoiEdges[i].vb.y)
      ){
        console.log("E NaN " + i);
        console.log(voronoiEdges[i]);
        voronoiEdges[i].vb.x = 0;
        voronoiEdges[i].vb.y = 0;
      }
    }

    
    
  }

  function addPoint(event: MouseEvent) {
    sitePoints = [...sitePoints, new SitePoint(event.offsetX, event.offsetY)]; // This syntax triggeres Sveltes reactive reload
    siteStore.set({ sitePoints: sitePoints, siteSegments: siteSegments });
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

  function isCellPathConsistant(c: Cell): boolean{
    if(c.edgeIndices.length <= 1)
      return false;

    for(let i: number = 1; i < c.edgeIndices.length - 1; i++){
      if((voronoiEdges[c.edgeIndices[i-1]].vb.x != voronoiEdges[c.edgeIndices[i]].va.x)
      || (voronoiEdges[c.edgeIndices[i-1]].vb.y != voronoiEdges[c.edgeIndices[i]].va.y))
        return false;
    }

    let i = c.edgeIndices.length-1;
    if((voronoiEdges[c.edgeIndices[i-1]].vb.x != voronoiEdges[c.edgeIndices[i]].va.x)
      || (voronoiEdges[c.edgeIndices[i-1]].vb.y != voronoiEdges[c.edgeIndices[i]].va.y))
      return false;

    // close loop
    if((voronoiEdges[c.edgeIndices[i]].vb.x != voronoiEdges[c.edgeIndices[0]].va.x)
      || (voronoiEdges[c.edgeIndices[i]].vb.y != voronoiEdges[c.edgeIndices[0]].va.y))
      return false;

      return true;
  }

  function getCellPath(c: Cell): string{
    // console.log("Cell " + c.sourceIndex);
    let e: Edge = voronoiEdges[c.edgeIndices[0]];

    let d: string;
    if(e.isCurved){
      d = "M " + e.controlPoints[0].x + " " + e.controlPoints[0].y 
        + "Q " + e.controlPoints[1].x + " " + e.controlPoints[1].y + " " + e.controlPoints[2].x + " " + e.controlPoints[2].y + " ";
    }else{
      d = "M " + voronoiEdges[c.edgeIndices[0]].va.x + ' ' + voronoiEdges[c.edgeIndices[0]].va.y + " ";
    }
    
    // console.log("(" + voronoiEdges[c.edgeIndices[0]].va.x + ' ' + voronoiEdges[c.edgeIndices[0]].va.y + ") (" 
    //                 + voronoiEdges[c.edgeIndices[0]].vb.x + ' ' + voronoiEdges[c.edgeIndices[0]].vb.y + ")");

    for(let i: number = 1; i < c.edgeIndices.length; i++){
      let e: Edge = voronoiEdges[c.edgeIndices[i]];
      let eprev: Edge = voronoiEdges[c.edgeIndices[i-1]];

      if(e.isCurved){
        d = d + "L " + e.controlPoints[0].x + " " + e.controlPoints[0].y 
              + "Q " + e.controlPoints[1].x + " " + e.controlPoints[1].y + " " + e.controlPoints[2].x + " " + e.controlPoints[2].y + " ";
      }else{
        d = d + "L " + e.va.x + " " + e.va.y + " ";
        // console.log("(" + e.va.x + ' ' + e.va.y + ") (" 
        //               + e.vb.x + ' ' + e.vb.y + ")");
      }
      

      if((eprev.vb.x != e.va.x)
      || (eprev.vb.y != e.va.y))
        console.log("Inconsistent: (" + eprev.vb.x + " " + eprev.vb.y + ") (" + e.va.x + " " + e.va.y + ")");
    }
    let i = c.edgeIndices.length-1;
    e = voronoiEdges[c.edgeIndices[i]];
    let eprev: Edge = voronoiEdges[c.edgeIndices[i-1]];

    if(!eprev.isCurved){
      d = d + "L " + e.vb.x + " " + e.vb.y + " ";
      // console.log("(" + e.va.x + ' ' + e.va.y + ") (" 
      //                 + e.vb.x + ' ' + e.vb.y + ")");

    }

    if((eprev.vb.x != e.va.x)
      || (eprev.vb.y != e.va.y))
        console.log("Inconsistent: (" + eprev.vb.x + " " + eprev.vb.y + ") (" + e.va.x + " " + e.va.y + ")");

    if((e.vb.x != voronoiEdges[c.edgeIndices[0]].va.x)
      || (e.vb.y != voronoiEdges[c.edgeIndices[0]].va.y))
        console.log("Inconsistent: (" + e.vb.x + " " + e.vb.y + ") (" + voronoiEdges[c.edgeIndices[0]].va.x + " " + voronoiEdges[c.edgeIndices[0]].va.y + ")");



    return d;
  }

  onMount(async () => {
    wasmVoronoi = await instantiate_wasmVoronoi();
    siteStore.subscribe((value: Sites) => {
      sitePoints = value.sitePoints;
      siteSegments = value.siteSegments;
      updateVoronoi();
    });
    sitePoints = [
      new SitePoint(100, 50),
      new SitePoint(300, 50), 
      new SitePoint(100, 250), 
      new SitePoint(300, 250), 
      new SitePoint(200, 150), 

    ]; 
    siteSegments = [
      // new SiteSegment(2, 2, 498, 2),
      // new SiteSegment(498, 2, 498, 298),
      // new SiteSegment(498, 298, 2, 298),
      // new SiteSegment(2, 298, 2, 2),
      // new SiteSegment(100, 100, 100, 200),
      // new SiteSegment(200, 100, 200, 200),
    ];
    siteStore.set({ sitePoints: sitePoints, siteSegments: siteSegments });
  });
</script>

<!-- svelte-ignore a11y-click-events-have-key-events -->
<!-- svelte-ignore a11y-no-static-element-interactions -->
<svg
  id="voronoiSvg"
  width="500"
  height="300"
  viewBox="{bbox.xl} {bbox.yl} {bbox.xh} {bbox.yh}"
  xmlns="http://www.w3.org/2000/svg"
  on:click={addPoint}
>
  <rect
    x="0"
    y="0"
    width="500"
    height="300"
    stroke="black"
    stroke-width="3"
    fill="rgb(240, 238, 231)"
  />
  {#each voronoiCells as c}
    {#if isCellPathConsistant(c)}
      <path
      id="cell {c.sourceIndex}"
      d={getCellPath(c)}
      stroke="black"
      stroke-width="0"
      fill="red"
    ></path>
    {/if}
  {/each}
  {#each sitePoints as siteP, idx}
    <circle 
      id="point {idx}"
      cx={siteP.x} 
      cy={siteP.y} 
      r="2" 
      fill="black"></circle
    >
  {/each}
  {#each siteSegments as siteS, idx}
    <path
      id="segment {idx + sitePoints.length}"
      d="M {siteS.x1} {siteS.y1} L {siteS.x2} {siteS.y2}"
      stroke="black"
      stroke-width="1"
      fill="none"
    ></path>
    <circle 
      id="segment {idx + sitePoints.length}"
      cx={siteS.x1} cy={siteS.y1} r="2" fill="black"></circle
    >
    <circle 
      id="segment {idx + sitePoints.length}"
      cx={siteS.x2} cy={siteS.y2} r="2" fill="black"></circle
    >
  {/each}
  {#each voronoiEdges as e, idx}
    <circle cx={e.va.x} cy={e.va.y} r="2" fill="green"></circle>
    <circle cx={e.vb.x} cy={e.vb.y} r="2" fill="green"></circle>
    {#if e.isCurved}
      <path
        d="M {e.controlPoints[0].x} {e.controlPoints[0].y} Q {e.controlPoints[1]
          .x} {e.controlPoints[1].y} {e.controlPoints[2].x} {e.controlPoints[2]
          .y}"
        stroke="blue"
        stroke-width="1"
        fill="none"
      ></path>
    {:else if e.isPrimary}
      <path
        d="M {e.va.x} {e.va.y} L {e.vb.x} {e.vb.y}"
        stroke="blue"
        stroke-width="1"
        fill="none"
      ></path>
    {:else if showSecondary}
      <path
        d="M {e.va.x} {e.va.y} L {e.vb.x} {e.vb.y}"
        stroke="green"
        stroke-width="1"
        fill="none"
      ></path>
    {/if}
  {/each}


</svg>
<button
  class="fileButton"
  on:click={() => {
    downloadSVG();
  }}>Download SVG</button
>
<div class="lastErrorContainer">
  <p class="lastError">{lastError}</p>
</div>

<style>
  .lastError {
    text-align: left;
    color: red;
    font-size: 10pt;
  }
</style>
