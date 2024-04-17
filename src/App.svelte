<script lang="ts">
  import UploadImage from "./lib/UploadImage.svelte";
  import Tiling from "./lib/Tiling.svelte";

  let helpActive: boolean = false;

  function showHelp() {
    helpActive = !helpActive;
  }
</script>

<link
  rel="stylesheet"
  href="https://fonts.googleapis.com/css2?family=Material+Symbols+Outlined:opsz,wght,FILL,GRAD@20..48,100..700,0..1,-50..200"
/>
<main class="flex flex-col min-h-screen">
  <link
    href="https://fonts.googleapis.com/icon?family=Material+Icons"
    rel="stylesheet"
  />
  <header class="sticky z-50 bg-sky-500 top-0 p-4">
    <h1 class="text-4xl font-sans text-center text-white">Eschherization</h1>
    <button
      class="absolute right-3 top-3 min-h-12 min-w-12 bg-sky-500 rounded"
      on:click={() => {
        showHelp();
      }}
    >
      <span class="material-symbols-outlined"> help </span>
    </button>
  </header>

  {#if helpActive}
    <div class="flex flex-col grow pb-10">

      <p class="text-2xl font-sans text-sky-400 p-4">
        Help
      </p>
      <p class="pl-10">This tool is meant to generate escher like tilings from any Image. To generate a tiling follow the steps below: </p>      
      <p class="text-xl font-sans text-sky-400 p-4">
        Step 1: Skeleton
      </p>
      <ul class="list-disc list-inside pl-10 pr-10">
        <li> Get an image to use for the tool in a common image format (png, jpg, ...). </li>
        <li> 
          To be able to generate a tiling the tool has to know what the content and what the backgound of the image is. 
          It assumes that all black (#000000) pixels are the background and everything else is the subject.
          Use a tool like Photoshop, GIMP or MS Paint to fill the background with black pixels.          
        </li>
        <li> 
          Now use the "Upload Image" button to upload the Image to the tool.
        </li>
        <li>
          The Tool will automaticaly calculate a "skelleton" of the image.
          Use the "Deviation Tolerance" slider to adjust the skelleton, so it closely assembles the subject but does not have to many red lines.
        </li>
        <li>
          The tools next to the image can be used to manualy adjust the skelleton.
        </li>        
        <li>
          The pink dot can be moved to adjust the center of the tiling. The skelloton will be drawn relative to this center in the next step.
        </li>
        <li>
          The steps above can be repeated anytime to readjust the skelleton. Changing the deviation tolerance will reset any manual changes.
        </li>

      </ul>  
      <p></p>      
      
      
      <p class="text-xl font-sans text-sky-400 p-4">
        Step 2: Tiling
      </p>  
      <ul class="list-disc list-inside pl-10">
        <li> Press the "Update" button to create the tiling. Update has to be pressed every time a setting is chaged. </li>  
        <li> Now the settings on the right can be chaged to modify the tiling.</li> 
        <li> The "Tile Size" changes the size of an individual tile</li>
        <li> The "Tiling Size" changes the scale of the whole tiling</li>
        <li> The "Base Type" of the tiling can be changed using the "&lt;" and "&gt;" buttons</li>
        <li> In the Morph Settings area image morphing (warping) can be disabled and the behaviour of the morph can be modified</li>
        <li> t - amount of warping (none vs. full) </li>
        <li> p - all lines influence the same vs. longer lines have greater influence </li>
        <li> a - precision vs. smoothness of the warping </li>
        <li> b - affection of all lines vs. closest line to a pixel </li>
        <li> The Display Settings area can be used to customize which parts of the output are displayed and in which colors</li>
        <li> Press the "Download SVG" button to save the generate tiling as an SVG file. SVGs can be displayed using most Web-Browsers and further Edited using Software like Incscape, Adobe Illustrator etc.</li>

      </ul>  
    </div>
  {/if}


    <div class="flex flex-row flex-wrap flex-grow">
      <div class="flex-grow flex-1 min-w-fit">
        <p class="text-2xl font-sans text-center text-sky-400 p-4">
          Skeleton
        </p>
        <UploadImage></UploadImage>
      </div>
  
      <div
        class="col-start-2 min-h-[1em] w-px self-stretch bg-gradient-to-tr from-transparent via-neutral-500 to-transparent opacity-20 dark:opacity-100"
      ></div>
  
      <div class="flex-grow flex-1 min-w-fit">
        <p class="text-2xl font-sans text-center text-sky-400 p-4">
          Tiling
        </p>
        <Tiling></Tiling>
      </div>
    </div>
  

  <div class="sticky z-50 bottom-0 w-screen bg-sky-900">
    <footer class="flex flex-col items-center text-center">
      <div class="w-full p-1 text-center">
        <a class="text-white" href="https://www.tum.de/en/">Julian Eder - TUM</a
        >
      </div>
    </footer>
  </div>
</main>

<style>
  .material-symbols-outlined {
    font-variation-settings:
      "FILL" 0,
      "wght" 400,
      "GRAD" 0,
      "opsz" 24;
  }
</style>
