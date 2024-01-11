<script lang="ts">
    
    import Range from "./Range.svelte"	
    import TraceSkeleton from './trace_skeleton.vanilla';
    import TraceSkeleton2  from './wasmSkelleton/index'
    import { onMount } from "svelte";

    let tracer: any;

    let inputImage: any, fileinput: any;

    let canvas: any;
    let ctx: any;
    let svgContainer: any;

    let width = 100;
    let height = 100;

    let resolution: number = 25;

    let svgStr: string = "";

    const onFileSelected = (e: any) => {
        Promise.resolve().then(() => handleFileUpload(e)); // Run Async
    };
    const handleFileUpload = (e: any) => {
        let image = e.target?.files[0];
        let reader = new FileReader();
        reader.readAsDataURL(image);
        reader.onload = (e) => {
            inputImage = e.target?.result;
            
            let image = new Image();
            image.onload = () => Promise.resolve().then(() => {
                width = image.width;
                height = image.height;
                ctx = canvas.getContext('2d', {
                    willReadFrequently: true,
                });                
                ctx.fillStyle = ctx.createPattern(image, 'repeat'); 
                ctx.fillRect(0, 0, width, height);

                updateSkelleton();

   	        });
		    image.src = inputImage;
            // let img = inputImage;

        };
    }

    const onResolutionChanged = (newRes: number) => {
        resolution = newRes;
        Promise.resolve().then(updateSkelleton); // Run Async
    
    };
    const updateSkelleton = () => {
        
        let imageData = ctx.getImageData(0,0,width,height);
        // let { polylines,rects } = tracer.fromImageData(imageData, resolution); // resolution is ignored in WASM for now
        let { polylines,rects } = TraceSkeleton.fromImageData(imageData, resolution);
        svgStr = TraceSkeleton.visualize({ polylines,rects, width, height }, {rects: true})
        svgContainer.innerHTML = svgStr;
    };

    onMount(async () => {
        tracer = await TraceSkeleton2.load();
    });

</script>

<div id="app">
    <div class="drawingContainer">

        <canvas class="uploadCanvas"
            bind:this={canvas}
            width=377 
            height=309
            style="width: 377px; height: 309px"
        ></canvas>
        <div
            class="overdrawSvg"
            bind:this={svgContainer}
        ></div>
    </div>
    <div class:purple-theme={false} class="resolution-slider">
        <label for="basic-range">Resolution</label>
        <Range min={5} max={100} initialValue={resolution} on:change={(e) => onResolutionChanged(e.detail.value)} />
    </div>
    <!-- {#if inputImage}
        <img 
            class="inputImage" 
            bind:this={htmlImg}
            src={inputImage} 
            alt="d" 
        />
    {:else}
        <div class="placehodlerImage">

        </div>
    {/if} -->

    <button
        class="upload"
        on:click={() => {
            fileinput.click();
        }}
    >Upload Image</button>
    <input
        style="display:none"
        type="file"
        accept=".jpg, .jpeg, .png"
        on:change={(e) => onFileSelected(e)}
        bind:this={fileinput}
    />
</div>

<style>
    #app {
        display: flex;
        align-items: center;
        justify-content: center;
        flex-flow: column;
    }
    .upload {
        height: 50px;
        width: 150px;
        cursor: pointer;
        background-color: #9af4fa;
        margin: 20px;
    }
    .inputImage {
        display: flex;
        max-width: 50%;
        max-height: 100%;
    }
    .placehodlerImage{
        border-color: #000;
        background-color: #f8f8f8;
        border-style:solid;
        border-width: 1px;
        border-radius: 10px;
        width: 100px;
        height: 100px;
    }
    .drawingContainer{
        position: relative;

    }
    .uploadCanvas{
        border-color: #000;
        background-color: #f8f8f8;
        border-style:solid;
        border-width: 1px;
    }
    .overdrawSvg{
        position: absolute;
        top: 0px;
        left: 0px;
        width: 377px; 
        height: 309px;
        z-index: 10;
    }
    .resolution-slider{
        width: 300px;
    }
</style>
