<script lang="ts">
  import { writable, get, type Writable } from 'svelte/store';
  import { onMount } from "svelte";
  import { BBox, type Edge, SitePoint, SiteSegment, type Vertex, Sites } from "./voronoiDataStructures";
  import instantiate_wasmVoronoi, { type VoronoiWasmModule, type EdgeResult } from "../lib/wasm/wasmVoronoi"

  export let siteStore: Writable<Sites>;

  let wasmVoronoi: VoronoiWasmModule;
  let bbox: BBox = { xl: 0, xh: 500, yl: 0, yh: 300 };

  let sitePoints: SitePoint[] = [];
  let siteSegments: SiteSegment[] = [];
  let voronoiVertices: Vertex[] = [];
  let voronoiEdges: Edge[] = [];

  let showSecondary: boolean = false;

  let lastError: any = "";

  function updateVoronoi(){
    
    try{

      
      const bboxVector = new wasmVoronoi.VectorDouble();
      bboxVector.push_back(bbox.xl)
      bboxVector.push_back(bbox.yl)
      bboxVector.push_back(bbox.xh)
      bboxVector.push_back(bbox.yh)

      const pointVector = new wasmVoronoi.VectorInt();
      sitePoints.forEach(sp => {
        pointVector.push_back(sp.x)
        pointVector.push_back(sp.y)
      });
      const segmentVector = new wasmVoronoi.VectorInt();
      siteSegments.forEach(ss => {
        segmentVector.push_back(ss.x1)
        segmentVector.push_back(ss.y1)
        segmentVector.push_back(ss.x2)
        segmentVector.push_back(ss.y2)
      });
      
      let result = wasmVoronoi.computevoronoi(bboxVector, pointVector, segmentVector);
      let newVoronoiVertices: Vertex[] = [];
      for (var i = 0; i < result.vertices.size(); i+=2) {
        // console.log("Vert: ", result.vertices.get(i));
        newVoronoiVertices.push({x: result.vertices.get(i), y: result.vertices.get(i+1)})
      }
      
      let newVoronoiEdges: Edge[] = [];
      for (var i = 0; i < result.edges.size(); i++) {
        let e: EdgeResult = result.edges.get(i);
        let samples: Vertex[] = [];
        for (var j = 0; j < e.samples.size(); j+=2) {
          samples.push({x: e.samples.get(j), y: e.samples.get(j+1)})
        }
        let controlPoints: Vertex[] = [];
        for (var j = 0; j < e.controll_points.size(); j+=2) {
          controlPoints.push({x: e.controll_points.get(j), y: e.controll_points.get(j+1)})
        }

        newVoronoiEdges.push({
          va: {x: e.x1, y: e.y1},
          vb: {x: e.x2, y: e.y2},
          samples: samples,
          controlPoints: controlPoints,
          isPrimary: e.isPrimary
        })
      }

      // console.log(newVoronoiEdges)
      voronoiVertices = newVoronoiVertices;
      voronoiEdges = newVoronoiEdges;
    }catch(e){
      lastError = e;
      throw e;
    }
  }

  onMount(async () => {
    
    wasmVoronoi = await instantiate_wasmVoronoi();

    siteStore.subscribe((value: Sites) => {
      sitePoints = value.sitePoints;
      siteSegments = value.siteSegments;
      updateVoronoi();
    });
    sitePoints = []; //new SitePoint(200, 200), new SitePoint(300, 100), new SitePoint(100, 100)];
    siteSegments = [
      new SiteSegment(10, 10, 490, 10), 
      new SiteSegment(490, 10, 490, 290),
      new SiteSegment(490, 290, 10, 290),
      new SiteSegment(10, 290, 10, 10),
    ];
    siteStore.set({sitePoints: sitePoints, siteSegments: siteSegments})

  });

  function addPoint(event: MouseEvent) {
    sitePoints = [...sitePoints, new SitePoint(event.offsetX, event.offsetY)]; // This syntax triggeres Sveltes reactive reload
    siteStore.set({sitePoints: sitePoints, siteSegments: siteSegments})
  }
</script>

<!-- svelte-ignore a11y-click-events-have-key-events -->
        <!-- svelte-ignore a11y-no-static-element-interactions -->
        <svg
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
          {#each sitePoints as siteP, idx}
            <circle cx={siteP.x} cy={siteP.y} r="2" fill="black"></circle>
          {/each}
          {#each siteSegments as siteS, idx}
            <path
              d="M {siteS.x1} {siteS.y1} L {siteS.x2} {siteS.y2}"
              stroke="black"
              stroke-width="1"
              fill="none"
            ></path>
            <circle cx={siteS.x1} cy={siteS.y1} r="2" fill="black"></circle>
            <circle cx={siteS.x2} cy={siteS.y2} r="2" fill="black"></circle>
          {/each}
          <!-- {#each voronoiVertices as v, idx}
            <circle cx={v.x} cy={v.y} r="5" fill="red"></circle>
          {/each} -->
          {#each voronoiEdges as e, idx}
            <circle cx={e.va.x} cy={e.va.y} r="2" fill="green"></circle>
            <circle cx={e.vb.x} cy={e.vb.y} r="2" fill="green"></circle>
            {#if e.samples.length == 0}
              {#if e.isPrimary}
                <path
                d="M {e.va.x} {e.va.y} L {e.vb.x} {e.vb.y}"
                stroke="blue"
                stroke-width="1"
                fill="none"
                ></path>
              {:else}
                {#if showSecondary}
                  <path
                  d="M {e.va.x} {e.va.y} L {e.vb.x} {e.vb.y}"
                  stroke="green"
                  stroke-width="1"
                  fill="none"
                  ></path>
                {/if}
              {/if}
            {:else}
            <!-- <path d="M {e.va.x} {e.va.y} {e.samples.map((s) => 'L ' + s.x + ' ' + s.y).reduce((a,b) => a + ' ' + b)} "
              stroke="red"
              stroke-width="1"
              fill="none"
              ></path> -->
              <path d="M {e.controlPoints[0].x} {e.controlPoints[0].y} Q {e.controlPoints[1].x} {e.controlPoints[1].y} {e.controlPoints[2].x} {e.controlPoints[2].y}"
                stroke="blue"
                stroke-width="1"
                fill="none"
                ></path>
              {/if}
          {/each}
        </svg>
        <div class="lastErrorContainer">
          <p class="lastError">{lastError}</p>
        </div>

<style>
    .lastError{
      text-align: left;
      color: red;
      font-size: 10pt;
    }
</style>
