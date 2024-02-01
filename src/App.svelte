<script lang="ts">
  import { onMount } from "svelte";
  import MainPage from "./lib/MainPage.svelte";
  import UploadImage from "./lib/UploadImage.svelte";
  import Tiling from "./lib/Tiling.svelte";
  import { writable, type Writable } from "svelte/store";
  import { Sites } from "./lib/voronoiDataStructures";

  let siteStore: Writable<Sites> = writable({
    sitePoints: [],
    siteSegments: [],
    tileWidth: 300,
    tileHeight: 300,
  });
</script>

<main class="flex flex-col min-h-screen">
  <header class="sticky z-50 bg-white top-0 p-4">
    <h1 class="text-4xl font-sans text-center text-sky-400">Eschherization</h1>
  </header>
  <div class="flex-grow">
    <div class="flex flex-row">
      <div class="basis-1/2">
        <p class="text-2xl font-sans text-center text-sky-400 p-4">Upload Image</p>
        <UploadImage {siteStore}></UploadImage>
      </div>
      <div
        class=" min-h-[1em] w-px self-stretch bg-gradient-to-tr from-transparent via-neutral-500 to-transparent opacity-20 dark:opacity-100"
      ></div>

      <div class="basis-1/2">
        <p class="text-2xl font-sans text-center text-sky-400 p-4">Tiling</p>
        <Tiling {siteStore}></Tiling>
      </div>
    </div>
  </div>

  <div class="sticky z-50 bottom-0"
  style="background-color: #0a4275"
  >
    <footer
      class="flex flex-col items-center text-center"
    >
      <!--Copyright section-->
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
