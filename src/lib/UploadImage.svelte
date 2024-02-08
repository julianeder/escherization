<script lang="ts">
    import { type Writable } from "svelte/store";
    import Range from "./Range.svelte";
    import TraceSkeleton from "./trace_skeleton.vanilla";
    import TraceSkeleton2 from "./wasmSkelleton/index";
    import { onMount } from "svelte";
    import { SitePoint, SiteSegment, Sites } from "./voronoiDataStructures";
    import { Point } from "./tactile/tactile";
    import { Vectorization } from "./vectorization";

    export let siteStore: Writable<Sites>;

    let tracer: any;

    let inputImage: any, fileinput: any;

    let canvas: HTMLCanvasElement;
    let ctx: CanvasRenderingContext2D | null = null;
    let svgContainer: any;

    const canvasWidth: number = 300;
    const canvasHeight: number = 300;
    let tileWidth: number = 300;
    let tileHeight: number = 300;
    let imgX: number = 0;
    let imgY: number = 0;

    let deviation: number = 40;

    let activeTool: string = "move";
    let styles = {
		'cursorDraggable': 'move',
		'cursorBackground': 'default',
	};
    $: cssVarStyles = Object.entries(styles)
		.map(([key, value]) => `--${key}:${value}`)
		.join(';');

    let tileCenter = new Point(tileWidth / 2, tileHeight / 2);

    const initialSitePoints: SitePoint[] = [
        new SitePoint(50, 50),
        new SitePoint(250, 50),
        new SitePoint(50, 250),
        new SitePoint(250, 250),
        new SitePoint(150, 150),
    ];
    const initialSiteSegments: SiteSegment[] = [
        //   new SiteSegment(2, 2, 498, 2),
        //   new SiteSegment(498, 2, 498, 298),
        //   new SiteSegment(498, 298, 2, 298),
        //   new SiteSegment(2, 298, 2, 2),
    ];

    let sitePoints: Array<SitePoint> = initialSitePoints; // local copy of Segments for visualization as svg
    let siteSegments: Array<SiteSegment> = initialSiteSegments; // local copy of Segments for visualization as svg

    let creatingSegmet: SiteSegment | null = null;

    function updateStore() {
        siteStore.set({
            sitePoints: sitePoints.map((p) => translatePoint(p, tileCenter)),
            siteSegments: siteSegments.map((s) =>
                translateSegment(s, tileCenter),
            ),
            tileWidth: tileWidth,
            tileHeight: tileHeight,
        });
    }

    function onFileSelected(e: any) {
        Promise.resolve().then(() => handleFileUpload(e)); // Run Async
    }

    function handleFileUpload(e: any) {
        sitePoints = [];
        let imageFile = e.target?.files[0];
        let reader = new FileReader();
        reader.readAsDataURL(imageFile);
        reader.onload = (e) => {
            inputImage = e.target?.result;

            let image: HTMLImageElement = new Image();
            image.onload = () =>
                Promise.resolve().then(() => {
                    ctx = canvas.getContext("2d", {
                        willReadFrequently: true,
                    });
                    ctx?.clearRect(0, 0, canvasWidth, canvasHeight);
                    drawImageScaled(image);
                    // ctx.fillStyle = ctx.createPattern(image, 'repeat');
                    // ctx.fillRect(0, 0, width, height);

                    //Trigger Reactive Update?
                    siteSegments = Vectorization.updateSkelleton(ctx!, imgX, imgY, tileWidth, tileHeight, deviation);
                    updateStore();

                });
            image.src = inputImage;
            // let img = inputImage;
        };
    }

    function drawImageScaled(img: HTMLImageElement) {
        if (ctx == null) return;
        let canvas = ctx.canvas;
        let hRatio = canvas.width / img.width;
        let vRatio = canvas.height / img.height;
        let ratio = Math.min(hRatio, vRatio);
        let centerShift_x = (canvas.width - img.width * ratio) / 2;
        let centerShift_y = (canvas.height - img.height * ratio) / 2;

        tileWidth = Math.floor(img.width * ratio);
        tileHeight = Math.floor(img.height * ratio);
        imgX = Math.floor(centerShift_x);
        imgY = Math.floor(centerShift_y);

        // ctx.clearRect(0,0,canvas.width, canvas.height);
        // ctx.drawImage(img, 0,0, img.width, img.height,
        //                     centerShift_x,centerShift_y,width, height);

        let offScreenCanvas = document.createElement("canvas");
        let context = offScreenCanvas.getContext("2d");
        offScreenCanvas.width = img.width;
        offScreenCanvas.height = img.height;
        context!.drawImage(img, 0, 0);
        let imgageData = context!.getImageData(0, 0, img.width, img.height);
        // Nearest Neighbour Resampling
        let imagRGBA: number[] = [];
        for (let y = 0; y < tileHeight; y++) {
            for (let x = 0; x < tileWidth; x++) {
                let i =
                    (Math.round(y / ratio) * img.width +
                        Math.round(x / ratio)) *
                    4;
                imagRGBA.push(imgageData.data[i]);
                imagRGBA.push(imgageData.data[i + 1]);
                imagRGBA.push(imgageData.data[i + 2]);
                imagRGBA.push(imgageData.data[i + 3]);
            }
        }
        let newImgageData = new ImageData(
            new Uint8ClampedArray(imagRGBA),
            tileWidth,
            tileHeight,
        );
        ctx.putImageData(newImgageData, imgX, imgY);
    }

    function onResolutionChanged(newRes: number) {
        deviation = newRes;
        Promise.resolve().then(updateSkelleton); // Run Async
    }

    function translatePoint(p: SitePoint, tileCenter: Point): SitePoint {
        return new SitePoint(p.x - tileCenter.x, p.y - tileCenter.y);
    }

    function translateSegment(s: SiteSegment, tileCenter: Point): SiteSegment {
        return new SiteSegment(
            s.x1 - tileCenter.x,
            s.y1 - tileCenter.y,
            s.x2 - tileCenter.x,
            s.y2 - tileCenter.y,
        );
    }

    const downloadCanvasImage = () => {
        var link = document.createElement("a");
        link.download = "canvas.png";
        link.href = canvas.toDataURL();
        link.click();
    };

    function downloadSVG() {
        const svg = document.getElementById("previewSvg");
        if (svg != null) {
            const blob = new Blob([svg.outerHTML], { type: "image/svg+xml" });
            const link = document.createElement("a");
            link.href = URL.createObjectURL(blob);
            link.download = "preview.svg";
            link.click();
        }
    }

    onMount(async () => {
        tracer = await TraceSkeleton2.load();
        updateStore();
    });

    var selectedElement: any = null;
    var offset: any = null;
    function makeDraggable(evt: any) {
        var svg = evt.target;
        svg!.addEventListener("mousedown", startDrag);
        svg!.addEventListener("mousemove", drag);
        svg!.addEventListener("mouseup", endDrag);
        svg!.addEventListener("mouseleave", endDrag);
        function startDrag(evt: any) {
            if (activeTool == "move") {
                if (evt.target.classList.contains("draggable")) {
                    selectedElement = evt.target;
                    offset = getMousePosition(evt);
                    // console.log(selectedElement.getAttributeNS(null, "cx"));
                    // console.log(offset);
                    offset.x -= parseFloat(
                        selectedElement.getAttributeNS(null, "cx"),
                    );
                    offset.y -= parseFloat(
                        selectedElement.getAttributeNS(null, "cy"),
                    );
                }
            }
        }
        function drag(evt: any) {
            if (selectedElement != null) {
                evt.preventDefault();
                var coord = getMousePosition(evt);
                if (selectedElement.classList.contains("tileCenter")){
                    selectedElement.setAttributeNS(null, "cx", coord.x - offset.x);
                    selectedElement.setAttributeNS(null, "cy", coord.y - offset.y);
                    tileCenter.x = coord.x - offset.x;
                    tileCenter.y = coord.y - offset.y;
                }
                else if (selectedElement.classList.contains("sitePoint")){
                    selectedElement.setAttributeNS(null, "cx", coord.x - offset.x);
                    selectedElement.setAttributeNS(null, "cy", coord.y - offset.y);
                    let idx: number = Number((selectedElement.id as string).substring(10)); //Requires specific id numbering scheme
                    sitePoints[idx].x = coord.x - offset.x;
                    sitePoints[idx].y = coord.y - offset.y;
                }
                else if (selectedElement.classList.contains("siteSegmentPoint")){
                    selectedElement.setAttributeNS(null, "cx", coord.x - offset.x);
                    selectedElement.setAttributeNS(null, "cy", coord.y - offset.y);
                    let idx: number = Number((selectedElement.id as string).substring(17)); //Requires specific id numbering scheme
                    let ss: SiteSegment;
                    if(idx % 2 == 0){
                        ss = siteSegments[idx / 2];
                        ss.x1 = coord.x - offset.x;
                        ss.y1 = coord.y - offset.y;
                        ss.connected_12.forEach(s => {
                            s.x2 = ss.x1;
                            s.y2 = ss.y1;
                        });
                        ss.connected_11.forEach(s => {
                            s.x1 = ss.x1;
                            s.y1 = ss.y1;
                        });
                    }
                    else{
                        ss = siteSegments[(idx-1)/2];
                        ss.x2 = coord.x - offset.x;
                        ss.y2 = coord.y - offset.y;
                        ss.connected_21.forEach(s => {
                            s.x1 = ss.x2;
                            s.y1 = ss.y2;
                        });
                        ss.connected_22.forEach(s => {
                            s.x2 = ss.x2;
                            s.y2 = ss.y2;
                        });
                    
                    }
                    siteSegments = [...siteSegments];


                }
            }

            if(creatingSegmet != null){
                var coord = getMousePosition(evt);
                creatingSegmet.x2 = coord.x- imgX;
                creatingSegmet.y2 = coord.y - imgY;
                creatingSegmet = creatingSegmet;
            }
        }
        function endDrag(evt: any) {
            if (activeTool == "move" && selectedElement != null) {
                selectedElement = null;
                updateStore();
            }
        }
        function getMousePosition(evt: any) {
            var CTM = svg.getScreenCTM();
            return {
                x: (evt.clientX - CTM.e) / CTM.a,
                y: (evt.clientY - CTM.f) / CTM.d,
            };
        }
    }

    function setActiveTool(newTool: string){
        activeTool = newTool;
        if(activeTool == "move"){
            styles.cursorDraggable = "move";
            styles.cursorBackground = "default";
        }else if(activeTool == "add" || activeTool=="addSegment"){
            styles.cursorDraggable = "default";
            styles.cursorBackground = "crosshair";
        }else if(activeTool == "delete"){ 
            styles.cursorDraggable = "default";
            styles.cursorBackground = "default";
        }

        if(activeTool != "addSegment")
            creatingSegmet = null;
    }

    function backgroundClick(evt: any){
        if(activeTool == "add"){
            sitePoints = [...sitePoints, new SitePoint(evt.offsetX - imgX, evt.offsetY - imgY)];
            updateStore();
        }
        else if(activeTool=="addSegment"){
            if(creatingSegmet == null){
                creatingSegmet = new SiteSegment(evt.offsetX - imgX, evt.offsetY - imgY, evt.offsetX - imgX, evt.offsetY - imgY);
            }else{
                creatingSegmet.x2 = evt.offsetX - imgX;
                creatingSegmet.y2 = evt.offsetY - imgY;
                siteSegments = [...siteSegments, creatingSegmet];
                creatingSegmet = null;
                updateStore();
            }
        }
    }

    function siteSegmentClick(evt: any){
        if(activeTool == "delete"){
            if(evt.target.classList.contains("siteSegmentPoint")){
                let idx: number = Number((evt.target.id as string).substring(17));
                console.log(idx)
                if(idx % 2 == 0)
                    idx = idx / 2;
                else
                    idx = (idx-1) / 2;
                console.log(idx)
                siteSegments.splice(idx, 1);
                siteSegments = [...siteSegments];
                updateStore();
            }
        }
    }

    function sitePointClick(evt: any){
        if(activeTool == "delete"){
            if(evt.target.classList.contains("sitePoint")){
                let idx: number = Number((evt.target.id as string).substring(17));
                console.log(idx)
                sitePoints.splice(idx, 1);
                sitePoints = sitePoints; // Trigger Reactive Update
                updateStore();
            }
        }
    }

</script>

<div>
    <div class="grid grid-cols-1 justify-items-center gap-4">
        <div class="flex flex-cols">
            <div class="tools flex flex-col">
                {#if activeTool == "move"}
                    <button
                        class="bg-sky-500 hover:bg-sky-300 border border-sky-600 p-2 rounded w-10 h-10 m-1"
                        on:click={() => { setActiveTool("move"); }}
                    >
                        <span class="material-icons md-36">open_with</span>
                    </button>
                {:else}
                    <button
                        class="bg-slate-50 hover:bg-slate-200 border border-sky-600 p-2 rounded w-10 h-10 m-1"
                        on:click={() => { setActiveTool("move"); }}
                    >
                        <span class="material-icons md-36">open_with</span>
                    </button>
                {/if}
                {#if activeTool == "add"}
                    <button
                        class="bg-sky-500 hover:bg-sky-300 border border-sky-600 p-2 rounded w-10 h-10 m-1"
                        on:click={() => { setActiveTool("add"); }}
                    >
                        <span class="material-icons md-36">add</span>
                        <div class="relative -top-4 left-3 text-xs">P</div>
                    </button>
                {:else}
                    <button
                        class="bg-slate-50 hover:bg-slate-200 border border-sky-600 p-2 rounded w-10 h-10 m-1"
                        on:click={() => { setActiveTool("add"); }}
                    >
                        <span class="material-icons md-36">add</span>
                        <div class="relative -top-4 left-3 text-xs">P</div>
                    </button>
                {/if}
                {#if activeTool == "addSegment"}
                    <button
                        class="bg-sky-500 hover:bg-sky-300 border border-sky-600 p-2 rounded w-10 h-10 m-1"
                        on:click={() => { setActiveTool("addSegment"); }}
                    >
                        <span class="material-icons md-36">add</span>
                        <div class="relative -top-4 left-3 text-xs">S</div>
                    </button>
                {:else}
                    <button
                        class="bg-slate-50 hover:bg-slate-200 border border-sky-600 p-2 rounded w-10 h-10 m-1"
                        on:click={() => { setActiveTool("addSegment"); }}
                    >
                        <span class="material-icons md-36">add</span>
                        <div class="relative -top-4 left-3 text-xs">S</div>
                    </button>
                {/if}
                {#if activeTool == "delete"}
                    <button
                        class="bg-sky-500 hover:bg-sky-300 border border-sky-600 p-2 rounded w-10 h-10 m-1"
                        on:click={() => { setActiveTool("delete"); }}
                    >
                        <span class="material-icons md-36">delete</span>
                    </button>
                {:else}
                    <button
                        class="bg-slate-50 hover:bg-slate-200 border border-sky-600 p-2 rounded w-10 h-10 m-1"
                        on:click={() => { setActiveTool("delete"); }}
                    >
                        <span class="material-icons md-36">delete</span>
                    </button>
                {/if}
            </div>
            <div class="grid grid-cols-1">
                <canvas
                    class="uploadCanvas bg-slate-50 border border-sky-600 col-start-1 row-start-1"
                    bind:this={canvas}
                    width={canvasWidth}
                    height={canvasHeight}
                    style="width: {canvasWidth}px; height: {canvasHeight}px"
                ></canvas>
                <div
                    class="overdrawSvg col-start-1 row-start-1"
                    bind:this={svgContainer}
                >
                    <svg
                        id="previewSvg"
                        on:load={makeDraggable}
                        width={canvasWidth}
                        height={canvasHeight}
                        viewBox="0 0 {canvasWidth} {canvasHeight}"
                        xmlns="http://www.w3.org/2000/svg"
                    >
                        <g transform="translate({imgX} {imgY})"
                        >
                            <!-- svelte-ignore a11y-click-events-have-key-events -->
                            <!-- svelte-ignore a11y-no-static-element-interactions -->
                            <rect
                                width={tileWidth}
                                height={tileHeight}
                                class="svgBackground"
                                style="{cssVarStyles}"
                                fill="transparent"
                                on:click={(evt) => backgroundClick(evt)}
                                >
                            </rect>
                            <circle
                                class="tileCenter draggable"
                                style="{cssVarStyles}"
                                id="origin"
                                cx={tileCenter.x}
                                cy={tileCenter.y}
                                r="5"
                                fill="pink"
                            ></circle>
                            <!-- svelte-ignore a11y-click-events-have-key-events -->
                            {#each sitePoints as siteP, idx}
                                <!-- svelte-ignore a11y-no-static-element-interactions -->
                                <circle
                                    id={"sitePoint_"+ (idx) }
                                    class="sitePoint draggable"
                                    style="{cssVarStyles}"
                                    cx={siteP.x}
                                    cy={siteP.y}
                                    r="2"
                                    fill="black"
                                    on:click={ (evt) => sitePointClick(evt) }

                                ></circle>
                            {/each}
                            {#each siteSegments as siteS, idx}
                                <path
                                    d="M {siteS.x1} {siteS.y1} L {siteS.x2} {siteS.y2}"
                                    stroke="red"
                                    stroke-width="1"
                                    fill="none"
                                ></path>
                                <!-- svelte-ignore a11y-click-events-have-key-events -->
                                <!-- svelte-ignore a11y-no-static-element-interactions -->
                                <circle
                                    id={"siteSegmentPoint_"+ (idx*2) }
                                    class="siteSegmentPoint draggable"
                                    style="{cssVarStyles}"
                                    cx={siteS.x1}
                                    cy={siteS.y1}
                                    r="2"
                                    fill="red"
                                    on:click={ (evt) => siteSegmentClick(evt) }
                                ></circle>
                                <!-- svelte-ignore a11y-click-events-have-key-events -->
                                <!-- svelte-ignore a11y-no-static-element-interactions -->
                                <circle
                                    id={"siteSegmentPoint_"+ (idx * 2 + 1) }
                                    class="siteSegmentPoint draggable"
                                    style="{cssVarStyles}"
                                    cx={siteS.x2}
                                    cy={siteS.y2}
                                    r="2"
                                    fill="red"
                                    on:click={ (evt) => siteSegmentClick(evt) }
                                ></circle>
                            {/each}
                            {#if creatingSegmet != null}
                                <path
                                    d="M {creatingSegmet.x1} {creatingSegmet.y1} L {creatingSegmet.x2} {creatingSegmet.y2}"
                                    stroke="red"
                                    stroke-width="1"
                                    fill="none"
                                ></path>
                                <!-- svelte-ignore a11y-click-events-have-key-events -->
                                <!-- svelte-ignore a11y-no-static-element-interactions -->
                                <circle
                                    class="siteSegmentPoint draggable"
                                    cx={creatingSegmet.x1}
                                    cy={creatingSegmet.y1}
                                    r="2"
                                    fill="red"
                                ></circle>
                                <!-- svelte-ignore a11y-click-events-have-key-events -->
                                <!-- svelte-ignore a11y-no-static-element-interactions -->
                                <circle
                                    class="siteSegmentPoint draggable"
                                    cx={creatingSegmet.x2}
                                    cy={creatingSegmet.y2}
                                    r="2"
                                    fill="red"
                                    on:click={ (evt) => backgroundClick(evt) }
                                ></circle>
                            {/if}
                        </g>
                    </svg>
                </div>
            </div>
        </div>
        <div
            class="resolution-slider min-w-72 font-sans text-center text-sky-400"
            class:purple-theme={false}
        >
            <label for="basic-range">Deviation Tolerance</label>
            <Range
                min={1}
                max={50}
                initialValue={deviation}
                on:change={(e) => onResolutionChanged(e.detail.value)}
            />
        </div>
        <button
            class="bg-blue-500 hover:bg-blue-700 text-white font-bold py-2 px-4 rounded min-w-40"
            on:click={() => {
                fileinput.click();
            }}>Upload Image</button
        >
        <button
            class="bg-blue-500 hover:bg-blue-700 text-white font-bold py-2 px-4 rounded min-w-40"
            on:click={() => {
                downloadCanvasImage();
            }}>Download Image</button
        >
        <button
            class="bg-blue-500 hover:bg-blue-700 text-white font-bold py-2 px-4 rounded min-w-40"
            on:click={() => {
                downloadSVG();
            }}>Download SVG</button
        >
        <input
            type="file"
            style="display:none"
            accept=".jpg, .jpeg, .png"
            on:change={(e) => onFileSelected(e)}
            bind:this={fileinput}
        />
    </div>
</div>

<style>
    .draggable {
        cursor: var(--cursorDraggable, default);
    }
    .svgBackground{
        cursor: var(--cursorBackground, default);
    }
</style>
