<script lang="ts">
    import { onMount } from "svelte";
  import MainPage from "./lib/MainPage.svelte";
  import UploadImage from "./lib/UploadImage.svelte";
  import Voronoi, { Diagram } from "./lib/voronoi";
    import type { SitePoint, SiteSegment, VoronoiBBox } from "./lib/voronoi/myVoronoi";
  // import Voronoi, { Voronoi } from "./lib/myVoronoi";

  let voronoi:Voronoi;
  let diagram: Diagram;

  let sitePoints: Array<SitePoint> = [];
  let siteSegments: Array<SiteSegment> = [];
  let voronoiVertices: any = [];
  let voronoiEdges: any = [];


  let bbox: VoronoiBBox = {xl:0, xr:500, yt:0, yb:300};
  

  onMount(() => {
    sitePoints = [{x:100,y:100}, {x:400,y:150}];
    siteSegments = [{x1: 10, y1:10, x2:200, y2: 200}];
    
    voronoi = new Voronoi();
    // voronoi = new Voronoi(sitePoints, siteSegments , bbox);
    
    diagram = voronoi.compute(sitePoints,bbox);
    voronoiVertices = diagram.vertices;
    voronoiEdges = diagram.edges;

    console.log('mounted')
  });


  function addPoint(event: MouseEvent){
    sitePoints = [...sitePoints, {x:event.offsetX, y:event.offsetY}];
    diagram = voronoi.compute(sitePoints,bbox);
    // voronoi.construct_voronoi(sitePoints, siteSegments, bbox);
    voronoiVertices = diagram.vertices;
    voronoiEdges = diagram.edges;
    console.log(voronoiVertices)

  }

</script>

<main>
  <MainPage>
    <div class="parent-container grid">
      <div class="col1">
        <p>Upload Image</p>
        <UploadImage></UploadImage>
      </div>
      <div class="col2">
        <!-- svelte-ignore a11y-click-events-have-key-events -->
        <!-- svelte-ignore a11y-no-static-element-interactions -->
        <svg
          width="500"
          height="300"
          viewBox="{bbox.xl} {bbox.yt} {bbox.xr} {bbox.yb}"
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
          <!-- <path
            d="M 10 10 q 200 200 300 0"
            stroke="blue"
            stroke-width="1"
            fill="none"
          ></path> -->
          {#each sitePoints as site, idx}
            <circle cx={site.x} cy={site.y} r="3" fill="black"></circle>
          {/each}
          {#each voronoiVertices as v, idx}
            <circle cx={v.x} cy={v.y} r="2" fill="red"></circle>
          {/each}
          {#each voronoiEdges as e, idx}
          <path
          d="M {e.va.x} {e.va.y} L {e.vb.x} {e.vb.y}"
          stroke="blue"
          stroke-width="1"
          fill="none"
        ></path>
          {/each}
        </svg>
      </div>
    </div>
  </MainPage>
</main>

<style>
  .col1 {
    padding: 20px;
  }
  .col2 {
    padding: 20px;
  }
  .parent-container {
    border: 2px black solid;
    /* width: 100%; */
  }
  .grid {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
</style>
