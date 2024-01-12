<script lang="ts">
    import { type Writable } from 'svelte/store';
    import Range from "./Range.svelte"	
    import TraceSkeleton from './trace_skeleton.vanilla';
    import TraceSkeleton2  from './wasmSkelleton/index'
    import { onMount } from "svelte";
    import { SitePoint, SiteSegment, Sites } from "./voronoiDataStructures";

    export let siteStore: Writable<Sites>;

        
        let tracer: any;

    let inputImage: any, fileinput: any;

    let canvas: HTMLCanvasElement;
    let ctx: CanvasRenderingContext2D | null = null;
    let svgContainer: any;

    const canvasWidth=300;
    const canvasHeight=300;
    let width = 300;
    let height = 300;
    let imgX = 0;
    let imgY = 0;
    
    let resolution: number = 25;
    
    let svgStr: string = "";
    
    let siteSegments: Array<SiteSegment> = []; // local copy of Segments for visualization as svg
    const baseSiteSegments = [
      new SiteSegment(10, 10, 490, 10), 
      new SiteSegment(490, 10, 490, 290),
      new SiteSegment(490, 290, 10, 290),
      new SiteSegment(10, 290, 10, 10),
    ];

    const onFileSelected = (e: any) => {
        Promise.resolve().then(() => handleFileUpload(e)); // Run Async
    };
    const handleFileUpload = (e: any) => {
        let imageFile = e.target?.files[0];
        let reader = new FileReader();
        reader.readAsDataURL(imageFile);
        reader.onload = (e) => {
            inputImage = e.target?.result;
            
            let image: HTMLImageElement = new Image();
            image.onload = () => Promise.resolve().then(() => {
                ctx = canvas.getContext('2d', {
                    willReadFrequently: true,
                });                
                drawImageScaled(image);
                // ctx.fillStyle = ctx.createPattern(image, 'repeat'); 
                // ctx.fillRect(0, 0, width, height);

                updateSkelleton();

   	        });
		    image.src = inputImage;
            // let img = inputImage;

        };
    }

    const drawImageScaled = (img: HTMLImageElement) => {
        if(ctx == null) return;
        let canvas = ctx.canvas ;
        let hRatio = canvas.width  / img.width;
        let vRatio =  canvas.height / img.height;
        let ratio  = Math.min ( hRatio, vRatio );
        let centerShift_x = ( canvas.width - img.width*ratio ) / 2;
        let centerShift_y = ( canvas.height - img.height*ratio ) / 2;  
        
        width = Math.floor(img.width*ratio);
        height = Math.floor(img.height*ratio);
        imgX = Math.floor(centerShift_x);
        imgY = Math.floor(centerShift_y);

        // ctx.clearRect(0,0,canvas.width, canvas.height);
        // ctx.drawImage(img, 0,0, img.width, img.height,
        //                     centerShift_x,centerShift_y,width, height);  
        
        let offScreenCanvas = document.createElement('canvas');
        let context = offScreenCanvas.getContext('2d');
        offScreenCanvas.width = img.width;
        offScreenCanvas.height = img.height;
        context!.drawImage(img, 0, 0);
        let imgageData = context!.getImageData(0, 0, img.width, img.height);
        // Nearest Neighbour Resampling
        let imagRGBA: number[] = [];
        for (let y = 0; y < height; y++) {
            for (let x = 0; x < width; x++) {
                let i = (Math.round(y / ratio) * img.width + Math.round(x / ratio)) * 4;
                imagRGBA.push(imgageData.data[i]);
                imagRGBA.push(imgageData.data[i+1]);
                imagRGBA.push(imgageData.data[i+2]);
                imagRGBA.push(imgageData.data[i+3]);
            }            
        }
        let newImgageData = new ImageData(new Uint8ClampedArray(imagRGBA), width, height);
        ctx.putImageData(newImgageData, imgX,imgY);

    }

    const onResolutionChanged = (newRes: number) => {
        resolution = newRes;
        Promise.resolve().then(updateSkelleton); // Run Async
    
    };

    const calcNeighbourCnt16 = (thinImag: number[], x: number ,y: number) => {
        let neighbourCnt: number = 0;
        if(thinImag[fromXY(x-2, y-2)]) neighbourCnt ++;
        if(thinImag[fromXY(x-1, y-2)]) neighbourCnt ++;
        if(thinImag[fromXY(x  , y-2)]) neighbourCnt ++;
        if(thinImag[fromXY(x+1, y-2)]) neighbourCnt ++;
        if(thinImag[fromXY(x+2, y-2)]) neighbourCnt ++;

        if(thinImag[fromXY(x+2, y-1)]) neighbourCnt ++;
        if(thinImag[fromXY(x+2, y  )]) neighbourCnt ++;
        if(thinImag[fromXY(x+2, y+1)]) neighbourCnt ++;

        if(thinImag[fromXY(x+2, y+2)]) neighbourCnt ++;
        if(thinImag[fromXY(x+1, y+2)]) neighbourCnt ++;
        if(thinImag[fromXY(x  , y+2)]) neighbourCnt ++;
        if(thinImag[fromXY(x-1, y+2)]) neighbourCnt ++;
        if(thinImag[fromXY(x-2, y+2)]) neighbourCnt ++;

        if(thinImag[fromXY(x-2, y+1)]) neighbourCnt ++;
        if(thinImag[fromXY(x-2, y  )]) neighbourCnt ++;
        if(thinImag[fromXY(x-2, y-1)]) neighbourCnt ++;
    }

    function calcNeighbourCnt8(thinImag: number[], x: number ,y: number): number{
        let neighbourCnt: number = 0;
        if(thinImag[fromXY(x-1, y-1)]) neighbourCnt++;
        if(thinImag[fromXY(x  , y-1)]) neighbourCnt++;
        if(thinImag[fromXY(x+1, y-1)]) neighbourCnt++;
        if(thinImag[fromXY(x+1, y  )]) neighbourCnt++;
        if(thinImag[fromXY(x+1, y+1)]) neighbourCnt++;
        if(thinImag[fromXY(x  , y+1)]) neighbourCnt++;
        if(thinImag[fromXY(x-1, y+1)]) neighbourCnt++;
        if(thinImag[fromXY(x-1, y  )]) neighbourCnt++;
        return neighbourCnt;
    }
    function calcNeighbourCnt4(thinImag: number[], x: number ,y: number): number{
        let neighbourCnt: number = 0;
        if(thinImag[fromXY(x  , y-1)]) neighbourCnt ++;
        if(thinImag[fromXY(x+1, y  )]) neighbourCnt ++;
        if(thinImag[fromXY(x  , y+1)]) neighbourCnt ++;
        if(thinImag[fromXY(x-1, y  )]) neighbourCnt ++;
        return neighbourCnt;
    }
    const fromI = (i: number) => { return {x: i % width, y: Math.floor(i / width)}} 
    
    const fromXY = (x: number, y: number) => { return y * width + x; }
    
    const updateSkelleton = () => {
        if(ctx == null) return;

        let imageData: ImageData = ctx.getImageData(imgX, imgY, width, height);
        // console.log("w " + width + " h " +  height + " imgX " + imgX + " imgY " + imgY);
        // console.log(imageData);
        // let { polylines, rects, thinImag } = TraceSkeleton.fromImageData(imageData, resolution);
        let thinImag = TraceSkeleton.thinningZS(TraceSkeleton.imageDataToBinary(imageData), width, height);
        let thinImagRGB: number[] = [];
        let i = 0;
        let j = 0;


        // Erode to 4-Neighbourhood, but keep potential corners
        thinImag.forEach((e: number) => {
            let {x, y} = fromI(i);
            
            let E1 = e && thinImag[fromXY(x+1,y)] && thinImag[fromXY(x,y+1)];
            let E2 = e && thinImag[fromXY(x+1,y)] && thinImag[fromXY(x,y-1)];
            let E3 = e && thinImag[fromXY(x-1,y)] && thinImag[fromXY(x,y-1)];
            let E4 = e && thinImag[fromXY(x-1,y)] && thinImag[fromXY(x,y+1)];

            let A = thinImag[fromXY(x-1, y-1)];
            let B = thinImag[fromXY(x  , y-1)];
            let C = thinImag[fromXY(x+1, y-1)];
            let D = thinImag[fromXY(x-1, y  )];
            let E = thinImag[fromXY(x+1, y  )];
            let F = thinImag[fromXY(x-1, y+1)];
            let G = thinImag[fromXY(x  , y+1)];
            let H = thinImag[fromXY(x+1, y+1)];

            let C1 = (D && E && G) || (B && D && G) || (B && D && E) || (B && E && G);
            let C2 = (C && D && G) || (B && D && H) || (B && E && F) || (A && E && G);
            let C3 = (A && C && G) || (C && D && H) || (B && F && H) || (A && E && F);
            let C4 = (A && F && H) || (A && C && F) || (A && C && H) || (C && F && H);

            
            if((E1 || E2 || E3 || E4) && !(C1 || C2 || C3 || C4) ){
                thinImag[i] = 0;
            }
            i++;
        });

        i = 0;
        j = 0;

        let crossings: Array<SitePoint> = [];
        let ends: Array<SitePoint> = [];

        // Corner Detection
        thinImag.forEach((e: number) => {
            let {x, y} = fromI(i);
            
            // Neighbours
            let A = thinImag[fromXY(x-1, y-1)];
            let B = thinImag[fromXY(x  , y-1)];
            let C = thinImag[fromXY(x+1, y-1)];
            let D = thinImag[fromXY(x-1, y  )];
            let E = thinImag[fromXY(x+1, y  )];
            let F = thinImag[fromXY(x-1, y+1)];
            let G = thinImag[fromXY(x  , y+1)];
            let H = thinImag[fromXY(x+1, y+1)];

            // Crossings
            let C1 = ((D && E && G) && (!A && !B && !C && !F && !H)) || ((B && D && G) && (!A && !C && !E && !F && !H)) || ((B && D && E) && (!A && !C && !F && !G && !H)) || ((B && E && G) && (!A && !C && !D && !F && !H));
            let C2 = ((C && D && G) && (!A && !B && !E && !F && !H)) || ((B && D && H) && (!A && !C && !E && !F && !G)) || ((B && E && F) && (!A && !C && !D && !G && !H)) || ((A && E && G) && (!B && !C && !D && !F && !H));
            let C3 = ((A && C && G) && (!B && !D && !E && !F && !H)) || ((C && D && H) && (!A && !B && !E && !F && !G)) || ((B && F && H) && (!A && !C && !D && !E && !G)) || ((A && E && F) && (!B && !C && !D && !G && !H));
            let C4 = ((A && F && H) && (!B && !C && !D && !E && !G)) || ((A && C && F) && (!B && !D && !E && !G && !H)) || ((A && C && H) && (!B && !D && !E && !F && !G)) || ((C && F && H) && (!A && !B && !D && !E && !G));

            // Ends
            let END =  ((A) && (!B && !C && !D && !E && !F && !G && !H)) 
                    || ((B) && (!A && !C && !D && !E && !F && !G && !H))
                    || ((C) && (!A && !B && !D && !E && !F && !G && !H))
                    || ((D) && (!A && !B && !C && !E && !F && !G && !H))
                    || ((E) && (!A && !B && !C && !D && !F && !G && !H))
                    || ((F) && (!A && !B && !C && !D && !E && !G && !H))
                    || ((G) && (!A && !B && !C && !D && !E && !F && !H))
                    || ((H) && (!A && !B && !C && !D && !E && !F && !G));
            
            // if(x == 205 - imgX && y == 113 - imgY)
            //     console.log("neighbourCnt " + neighbourCnt8 + " "
            //     + "e " + thinImag[fromXY(x,y)] + " "
            //     + "A " + A + " "
            //     + "B " + B + " "
            //     + "C " + C + " "
            //     + "D " + D + " "
            //     + "E " + E + " "
            //     + "F " + F + " "
            //     + "G " + G + " "
            //     + "H " + H + " "
            //     + thinImag[fromXY(x-1, y-1)] + " " //
            //     + thinImag[fromXY(x  , y-1)] + " " // 
            //     + thinImag[fromXY(x+1, y-1)] + " " // 
            //     + thinImag[fromXY(x+1, y  )] + " " // 
            //     + thinImag[fromXY(x+1, y+1)] + " " // 
            //     + thinImag[fromXY(x  , y+1)] + " " // 
            //     + thinImag[fromXY(x-1, y+1)] + " " // 
            //     + thinImag[fromXY(x-1, y  )] + " " // 
            // );

            if(C1 || C2 || C3 || C4){
                thinImagRGB.push(255);
                thinImagRGB.push(0);
                thinImagRGB.push(255);
                thinImagRGB.push(255);
                thinImag[i] = 2;
                crossings.push(new SitePoint(x,y));
            }
            else if(e && END){
                thinImagRGB.push(255);
                thinImagRGB.push(255);
                thinImagRGB.push(0);
                thinImagRGB.push(255);
                thinImag[i] = 3;
                ends.push(new SitePoint(x,y));
            }
            else if(e){
                thinImagRGB.push(0);
                thinImagRGB.push(0);
                thinImagRGB.push(255);
                thinImagRGB.push(255);
            }
            else{
                thinImagRGB.push(imageData.data[j]);
                thinImagRGB.push(imageData.data[j+1]);
                thinImagRGB.push(imageData.data[j+2]);
                thinImagRGB.push(255);
            }
            i++;
            j+=4;
        });
        
        let n: Node = vectorize(thinImag, crossings, ends);

        visualizeTreeRec(n, thinImagRGB);
        var thinImgageData = new ImageData(new Uint8ClampedArray(thinImagRGB), width, height);
        ctx.putImageData(thinImgageData, imgX,imgY);  


        subdivideTreeRec(n, thinImag);
        console.log(n);

        let ss: Array<SiteSegment> = [];
        segemntsFromTreeRec(n, ss);
        siteSegments = ss; // Trigger Reactive Update 
        siteStore.set({sitePoints: [], siteSegments: [...baseSiteSegments, ...ss]})
        
        
    };

    class Node{
        constructor(sitePoint: SitePoint, parent: Node | null ){
            this.sitePoint = sitePoint;
            this.parent = parent;
            if(parent != null)
                parent.children.push(this);
            this.pixel.push(sitePoint);
        }
        sitePoint: SitePoint | null;
        parent: Node | null = null;
        children: Array<Node> = [];
        pixel: Array<SitePoint> = [];
    }

    function vectorize(thinImag:number[], crossings: Array<SitePoint>, ends: Array<SitePoint>): Node{
        let n = new Node(ends[0], null);
        thinImag[fromXY(n.sitePoint!.x, n.sitePoint!.y)] = 4; // Set Visited
        vectorizeRec(n, n.sitePoint!, [n.sitePoint!], thinImag, crossings, ends);
        return n;
    }

    // thinImag --- Legend
    // 0 - Nothing
    // 1 - Line
    // 2 - Crossing
    // 3 - End
    // 4 - Visited
    function vectorizeRec(currNode: Node, currPos: SitePoint, pixel: Array<SitePoint>, thinImag:number[], crossings: Array<SitePoint>, ends: Array<SitePoint>){
        let neigh: Array<SitePoint> = neighbourIdxs(thinImag, currPos.x, currPos.y);
        neigh
        .forEach((n: SitePoint) => {
            if(thinImag[fromXY(n.x, n.y)] != 4){ // Skip already Visited
                pixel.push(n);
                if(thinImag[fromXY(n.x, n.y)] == 2){ // Crossing
                    thinImag[fromXY(n.x, n.y)] = 4; // Set Visited
                    let node = new Node(n, currNode);
                    currNode.pixel = pixel;
                    vectorizeRec(node, n, [], thinImag, crossings, ends);
                }else if(thinImag[fromXY(n.x, n.y)] == 3){ // End
                    thinImag[fromXY(n.x, n.y)] = 4; // Set Visited
                    let node = new Node(n, currNode);
                    currNode.pixel = pixel;
                }else if(thinImag[fromXY(n.x, n.y)] == 1){ // Line
                    thinImag[fromXY(n.x, n.y)] = 4; // Set Visited
                    vectorizeRec(currNode, n, pixel, thinImag, crossings, ends);
                }
            }
        });
    }
    
    function neighbourIdxs(thinImag:number[], x: number, y: number): Array<SitePoint>{
        let n:Array<SitePoint> = [];
        if(thinImag[fromXY(x-1, y-1)]) n.push({x:x-1, y: y-1});
        if(thinImag[fromXY(x-1, y  )]) n.push({x:x-1, y: y  });
        if(thinImag[fromXY(x-1, y+1)]) n.push({x:x-1, y: y+1});
        if(thinImag[fromXY(x  , y-1)]) n.push({x:x  , y: y-1});
        if(thinImag[fromXY(x  , y+1)]) n.push({x:x  , y: y+1});
        if(thinImag[fromXY(x+1, y-1)]) n.push({x:x+1, y: y-1});
        if(thinImag[fromXY(x+1, y  )]) n.push({x:x+1, y: y  });
        if(thinImag[fromXY(x+1, y+1)]) n.push({x:x+1, y: y+1});
        return n.sort((a,b) => thinImag[fromXY(b.x, b.y)] - thinImag[fromXY(a.x, a.y)]) // Sort to process Ends and Crossings first;
    }

    function segemntsFromTreeRec(n: Node, segments: Array<SiteSegment>): Array<SiteSegment>{
        n.children.forEach(c => {
            segments.push(new SiteSegment(n.sitePoint!.x, n.sitePoint!.y, c.sitePoint!.x, c.sitePoint!.y));
            segemntsFromTreeRec(c, segments);
        });

        return segments;
    }

    function visualizeTreeRec(n: Node, thinImagRGB: number[]){
        let col = getRandomColor();
        n.pixel.forEach(p => {            
            thinImagRGB[fromXY(p.x, p.y) * 4    ] = col[0];
            thinImagRGB[fromXY(p.x, p.y) * 4 + 1] = col[1];
            thinImagRGB[fromXY(p.x, p.y) * 4 + 2] = col[2];
        });
        n.children.forEach(c => {
            visualizeTreeRec(c, thinImagRGB);
        });
    }

    function subdivideTreeRec(n: Node, thinImag:number[]){
        n.pixel.forEach(p => {            
        });
        n.children.forEach(c => {
            subdivideTreeRec(c, thinImag);
        });
    }

    function getRandomColor() {
        var color: number[] = [];
        for (var i = 0; i < 3; i++) {
            color.push(Math.floor(Math.random() * 255));
        }
        return color;
    }

    const downloadCanvasImage = () => {
        var link = document.createElement('a');
        link.download = 'canvas.png';
        link.href = canvas.toDataURL()
        link.click();
    }

    onMount(async () => {
        tracer = await TraceSkeleton2.load();
    });

</script>

<div id="app">
    <div class="drawingContainer">

        <canvas class="uploadCanvas"
            bind:this={canvas}
            width={canvasWidth} 
            height={canvasHeight}
            style="width: {canvasWidth}px; height: {canvasHeight}px"
        ></canvas>
        <div
            class="overdrawSvg"
            bind:this={svgContainer}
            style="left: {imgX}px; top: {imgY}px; width: {width}px; height: {height}px;"
        >
        <svg 
        width={width}
        height={height}
        viewBox="0 0 {width} {height}"
        xmlns="http://www.w3.org/2000/svg">
            {#each siteSegments as siteS}
                <path
                d="M {siteS.x1} {siteS.y1} L {siteS.x2} {siteS.y2}"
                stroke="red"
                stroke-width="1"
                fill="none"
                ></path>
                <circle cx={siteS.x1} cy={siteS.y1} r="2" fill="red"></circle>
                <circle cx={siteS.x2} cy={siteS.y2} r="2" fill="red"></circle>
            {/each}
        </svg>
    </div>
    </div>
    <div class:purple-theme={false} class="resolution-slider">
        <label for="basic-range">Resolution</label>
        <Range min={5} max={100} initialValue={resolution} on:change={(e) => onResolutionChanged(e.detail.value)} />
    </div>
    <button
        class="upload"
        on:click={() => {
            fileinput.click();
        }}
    >Upload Image</button>
    <button
        class="upload"
        on:click={() => {
            downloadCanvasImage();
        }}
    >Download</button>
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
        z-index: 10;
    }
    .resolution-slider{
        width: 300px;
    }
</style>
