<script lang="ts">
  import Counter from "./lib/Counter.svelte";
  import MainPage from "./lib/MainPage.svelte";
  import UploadImage from "./lib/UploadImage.svelte";
</script>

<main>
  <MainPage>
    <div class="parent-container grid">
      <div class="col1">
        <p>Upload Image</p>
        <UploadImage></UploadImage>
      </div>
      <div class="col2">
        <svg
          width="500"
          height="300"
          viewBox="0 0 500 300"
          xmlns="http://www.w3.org/2000/svg"
        >
          <rect
            x="0"
            y="0"
            width="500"
            height="300"
            stroke="black"
            stroke-width="3"
            fill="red"
          />
          <path 
            d="M 10 10 q 200 200 300 0"
            stroke="blue"
            stroke-width="1" fill="none"
          ></path>
        </svg>
      </div>
    </div>
  </MainPage>
</main>

<style>

  .col1{
    padding: 20px;
  }
  .col2{
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
