
<script lang="ts">
  import { onMount } from "svelte";
  import MainPage from "./lib/MainPage.svelte";
  import UploadImage from "./lib/UploadImage.svelte";
  import Tiling from "./lib/Tiling.svelte";
</script>

<main>
<MainPage>
    <div class="parent-container grid">
      <div class="col1">
        <p>Upload Image</p>
        <UploadImage></UploadImage>
      </div>
      <div class="col2">
        <p>Voronoi</p>
        <Tiling></Tiling>
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
