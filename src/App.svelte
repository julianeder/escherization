<script lang="ts">
  import { onMount } from "svelte";
  import MainPage from "./lib/MainPage.svelte";
  import UploadImage from "./lib/UploadImage.svelte";
  import Voronoi, {
    Diagram,
    SitePoint,
    Edge,
    BBox,
    SiteSegment,
  } from "./lib/voronoi";
    import Beachline from "./lib/Beachline.svelte";

  let voronoi: Voronoi;
  let diagram: Diagram;

  let sitePoints: SitePoint[] = [];
  let siteSegments: SiteSegment[] = [];
  let voronoiVertices: any = [];
  let voronoiEdges: any = [];

  let bbox: BBox = { xl: 0, xr: 500, yt: 0, yb: 300 };
  let steps = 0;

  onMount(() => {
    sitePoints = [new SitePoint(350, 150), new SitePoint(150, 251), new SitePoint(100, 100)
        // ,new SitePoint(10, 200),
        // new SitePoint(400, 250)
    ];
    siteSegments = []; //[new SiteSegment(10, 200, 400, 250)];

    voronoi = new Voronoi();
    // voronoi = new Voronoi(sitePoints, siteSegments , bbox);

    diagram = voronoi.compute(steps, sitePoints, siteSegments, bbox);
    voronoiVertices = diagram.vertices;
    voronoiEdges = diagram.edges;

    console.log(diagram.beachline?.root);
    // console.log("mounted");

  });

  function addPoint(event: MouseEvent) {
    sitePoints = [...sitePoints, new SitePoint(event.offsetX, event.offsetY)]; // This syntax triggeres Sveltes reactive reload
    diagram = voronoi.compute(steps, sitePoints, siteSegments, bbox);
    // voronoi.construct_voronoi(sitePoints, siteSegments, bbox);
    voronoiVertices = diagram.vertices;
    voronoiEdges = diagram.edges;
    console.log(voronoiVertices);

  }

  function inc(){
    steps++;

    diagram = voronoi.compute(steps, sitePoints, siteSegments, bbox);
    // voronoi.construct_voronoi(sitePoints, siteSegments, bbox);
    voronoiVertices = diagram.vertices;
    voronoiEdges = diagram.edges;
  }

  function dec(){
    steps--;

    diagram = voronoi.compute(steps, sitePoints, siteSegments, bbox);
    // voronoi.construct_voronoi(sitePoints, siteSegments, bbox);
    voronoiVertices = diagram.vertices;
    voronoiEdges = diagram.edges;
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
          {#each sitePoints as siteP, idx}
            <circle cx={siteP.x} cy={siteP.y} r="4" fill="black"></circle>
          {/each}
          {#each siteSegments as siteS, idx}
            <path
              d="M {siteS.x1} {siteS.y1} L {siteS.x2} {siteS.y2}"
              stroke="black"
              stroke-width="2"
              fill="none"
            ></path>
            <circle cx={siteS.x1} cy={siteS.y1} r="4" fill="black"></circle>
            <circle cx={siteS.x2} cy={siteS.y2} r="4" fill="black"></circle>
          {/each}
          {#each voronoiVertices as v, idx}
            <circle cx={v.x} cy={v.y} r="3" fill="green"></circle>
          {/each}
          {#each voronoiEdges as e, idx}
            <path
              d="M {e.va.x} {e.va.y} L {e.vb.x} {e.vb.y}"
              stroke="blue"
              stroke-width="1"
              fill="none"
            ></path>
          {/each}
          <Beachline node={diagram?.beachline?.getFirst(diagram?.beachline?.root)}></Beachline>
          {#if diagram}
            <path
              d="M 0 {diagram?.sweepline} L 500 {diagram?.sweepline}"
              stroke="red"
              stroke-width="1"
              fill="none"
            ></path>
          {/if}
        </svg>
        <button on:click={inc}> + </button>
        <button on:click={dec}> - </button>
        <p>{steps}</p>
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
