<script lang="ts">
    let inputImage: any, fileinput: any;

    const onFileSelected = (e: Event) => {
        let image = e.target?.files[0];
        let reader = new FileReader();
        reader.readAsDataURL(image);
        reader.onload = (e) => {
            inputImage = e.target?.result;
        };
    };
</script>

<div id="app">
    {#if inputImage}
        <img 
            class="inputImage" 
            src={inputImage} 
            alt="d" 
        />
    {:else}
        <div class="placehodlerImage">

        </div>
    {/if}

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
</style>
