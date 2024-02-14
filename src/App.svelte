<script lang="ts">
  import UploadImage from "./lib/UploadImage.svelte";
  import Tiling from "./lib/Tiling.svelte";
  import { writable, type Writable } from "svelte/store";
  import { Sites } from "./lib/voronoiDataStructures";
    import { Point } from "./lib/tactile/tactile";

  let siteStore: Writable<Sites> = writable({
    sitePoints: [],
    siteSegments: [],
    tileWidth: 300,
    tileHeight: 300,
    tileCenter: new Point(0,0),
    imageOffset: new Point(0,0),
  });

  export let imageStore: Writable<HTMLImageElement | null> = writable(null);

</script>

<main class="flex flex-col min-h-screen">
  <link href="https://fonts.googleapis.com/icon?family=Material+Icons"
      rel="stylesheet">
  <header class="sticky z-50 bg-white top-0 p-4">
    <h1 class="text-4xl font-sans text-center text-sky-400">Eschherization</h1>
  </header>
  <div class="flex flex-row flex-wrap flex-grow">
      <div class="flex-grow flex-1" 
      style="min-width: 600px">
        <p class="text-2xl font-sans text-center text-sky-400 p-4">Upload Image</p>
        <UploadImage {siteStore}, {imageStore}></UploadImage>
      </div>

      <div
        class="col-start-2 min-h-[1em] w-px self-stretch bg-gradient-to-tr from-transparent via-neutral-500 to-transparent opacity-20 dark:opacity-100"
      ></div>

      <div class="flex-grow flex-1" 
      style="min-width: 600px">
        <p class="text-2xl font-sans text-center text-sky-400 p-4">Tiling</p>
        <Tiling {siteStore}, {imageStore}></Tiling>
      </div>
  </div>

  <div class="sticky z-50 bottom-0 w-screen bg-sky-900"
  >
    <footer
      class="flex flex-col items-center text-center"
    >
      <div
        class="w-full p-1 text-center"
      >
        <a class="text-white" href="https://www.tum.de/en/">Julian Eder - TUM</a
        >
      </div>
    </footer>
  </div>
</main>

<style>
</style>
