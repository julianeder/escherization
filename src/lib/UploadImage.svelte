<script lang="ts">
    import { type Writable } from "svelte/store";
    import Range from "./Range.svelte";
    import TraceSkeleton from "./trace_skeleton.vanilla";
    import TraceSkeleton2 from "./wasmSkelleton/index";
    import { onMount } from "svelte";
    import { SitePoint, SiteSegment, Sites } from "./voronoiDataStructures";
    import { Point } from "./tactile/tactile";

    export let siteStore: Writable<Sites>;

    let tracer: any;

    let inputImage: any, fileinput: any;

    let canvas: HTMLCanvasElement;
    let ctx: CanvasRenderingContext2D | null = null;
    let svgContainer: any;

    const canvasWidth = 300;
    const canvasHeight = 300;
    let tileWidth = 300;
    let tileHeight = 300;
    let imgX = 0;
    let imgY = 0;

    let deviation: number = 2;

    let svgStr: string = "";

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

                    updateSkelleton();
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

    function calcNeighbourCnt16(thinImag: number[], x: number, y: number) {
        let neighbourCnt: number = 0;
        if (thinImag[fromXY(x - 2, y - 2)]) neighbourCnt++;
        if (thinImag[fromXY(x - 1, y - 2)]) neighbourCnt++;
        if (thinImag[fromXY(x, y - 2)]) neighbourCnt++;
        if (thinImag[fromXY(x + 1, y - 2)]) neighbourCnt++;
        if (thinImag[fromXY(x + 2, y - 2)]) neighbourCnt++;

        if (thinImag[fromXY(x + 2, y - 1)]) neighbourCnt++;
        if (thinImag[fromXY(x + 2, y)]) neighbourCnt++;
        if (thinImag[fromXY(x + 2, y + 1)]) neighbourCnt++;

        if (thinImag[fromXY(x + 2, y + 2)]) neighbourCnt++;
        if (thinImag[fromXY(x + 1, y + 2)]) neighbourCnt++;
        if (thinImag[fromXY(x, y + 2)]) neighbourCnt++;
        if (thinImag[fromXY(x - 1, y + 2)]) neighbourCnt++;
        if (thinImag[fromXY(x - 2, y + 2)]) neighbourCnt++;

        if (thinImag[fromXY(x - 2, y + 1)]) neighbourCnt++;
        if (thinImag[fromXY(x - 2, y)]) neighbourCnt++;
        if (thinImag[fromXY(x - 2, y - 1)]) neighbourCnt++;
    }

    function calcNeighbourCnt8(
        thinImag: number[],
        x: number,
        y: number,
    ): number {
        let neighbourCnt: number = 0;
        if (thinImag[fromXY(x - 1, y - 1)]) neighbourCnt++;
        if (thinImag[fromXY(x, y - 1)]) neighbourCnt++;
        if (thinImag[fromXY(x + 1, y - 1)]) neighbourCnt++;
        if (thinImag[fromXY(x + 1, y)]) neighbourCnt++;
        if (thinImag[fromXY(x + 1, y + 1)]) neighbourCnt++;
        if (thinImag[fromXY(x, y + 1)]) neighbourCnt++;
        if (thinImag[fromXY(x - 1, y + 1)]) neighbourCnt++;
        if (thinImag[fromXY(x - 1, y)]) neighbourCnt++;
        return neighbourCnt;
    }
    function calcNeighbourCnt4(
        thinImag: number[],
        x: number,
        y: number,
    ): number {
        let neighbourCnt: number = 0;
        if (thinImag[fromXY(x, y - 1)]) neighbourCnt++;
        if (thinImag[fromXY(x + 1, y)]) neighbourCnt++;
        if (thinImag[fromXY(x, y + 1)]) neighbourCnt++;
        if (thinImag[fromXY(x - 1, y)]) neighbourCnt++;
        return neighbourCnt;
    }
    function fromI(i: number) {
        return { x: i % tileWidth, y: Math.floor(i / tileWidth) };
    }

    function fromXY(x: number, y: number) {
        return y * tileWidth + x;
    }

    function updateSkelleton() {
        if (ctx == null) return;

        let imageData: ImageData = ctx.getImageData(
            imgX,
            imgY,
            tileWidth,
            tileHeight,
        );
        // console.log('w ' + width + ' h ' +  height + ' imgX ' + imgX + ' imgY ' + imgY);
        // console.log(imageData);
        // let { polylines, rects, thinImag } = TraceSkeleton.fromImageData(imageData, resolution);
        let thinImag = TraceSkeleton.thinningZS(
            TraceSkeleton.imageDataToBinary(imageData),
            tileWidth,
            tileHeight,
        );
        let thinImagRGB: number[] = [];
        let i = 0;
        let j = 0;

        // Erode to 4-Neighbourhood, but keep potential corners
        thinImag.forEach((e: number) => {
            let { x, y } = fromI(i);

            let E1 =
                e && thinImag[fromXY(x + 1, y)] && thinImag[fromXY(x, y + 1)];
            let E2 =
                e && thinImag[fromXY(x + 1, y)] && thinImag[fromXY(x, y - 1)];
            let E3 =
                e && thinImag[fromXY(x - 1, y)] && thinImag[fromXY(x, y - 1)];
            let E4 =
                e && thinImag[fromXY(x - 1, y)] && thinImag[fromXY(x, y + 1)];

            let A = thinImag[fromXY(x - 1, y - 1)];
            let B = thinImag[fromXY(x, y - 1)];
            let C = thinImag[fromXY(x + 1, y - 1)];
            let D = thinImag[fromXY(x - 1, y)];
            let E = thinImag[fromXY(x + 1, y)];
            let F = thinImag[fromXY(x - 1, y + 1)];
            let G = thinImag[fromXY(x, y + 1)];
            let H = thinImag[fromXY(x + 1, y + 1)];

            let C1 =
                (D && E && G) ||
                (B && D && G) ||
                (B && D && E) ||
                (B && E && G);
            let C2 =
                (C && D && G) ||
                (B && D && H) ||
                (B && E && F) ||
                (A && E && G);
            let C3 =
                (A && C && G) ||
                (C && D && H) ||
                (B && F && H) ||
                (A && E && F);
            let C4 =
                (A && F && H) ||
                (A && C && F) ||
                (A && C && H) ||
                (C && F && H);

            if ((E1 || E2 || E3 || E4) && !(C1 || C2 || C3 || C4)) {
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
            let { x, y } = fromI(i);

            // Neighbours
            let A = thinImag[fromXY(x - 1, y - 1)];
            let B = thinImag[fromXY(x, y - 1)];
            let C = thinImag[fromXY(x + 1, y - 1)];
            let D = thinImag[fromXY(x - 1, y)];
            let E = thinImag[fromXY(x + 1, y)];
            let F = thinImag[fromXY(x - 1, y + 1)];
            let G = thinImag[fromXY(x, y + 1)];
            let H = thinImag[fromXY(x + 1, y + 1)];

            // Crossings
            let C1 =
                (D && E && G && !A && !B && !C && !F && !H) ||
                (B && D && G && !A && !C && !E && !F && !H) ||
                (B && D && E && !A && !C && !F && !G && !H) ||
                (B && E && G && !A && !C && !D && !F && !H);
            let C2 =
                (C && D && G && !A && !B && !E && !F && !H) ||
                (B && D && H && !A && !C && !E && !F && !G) ||
                (B && E && F && !A && !C && !D && !G && !H) ||
                (A && E && G && !B && !C && !D && !F && !H);
            let C3 =
                (A && C && G && !B && !D && !E && !F && !H) ||
                (C && D && H && !A && !B && !E && !F && !G) ||
                (B && F && H && !A && !C && !D && !E && !G) ||
                (A && E && F && !B && !C && !D && !G && !H);
            let C4 =
                (A && F && H && !B && !C && !D && !E && !G) ||
                (A && C && F && !B && !D && !E && !G && !H) ||
                (A && C && H && !B && !D && !E && !F && !G) ||
                (C && F && H && !A && !B && !D && !E && !G);

            // Ends
            let END =
                (A && !B && !C && !D && !E && !F && !G && !H) ||
                (B && !A && !C && !D && !E && !F && !G && !H) ||
                (C && !A && !B && !D && !E && !F && !G && !H) ||
                (D && !A && !B && !C && !E && !F && !G && !H) ||
                (E && !A && !B && !C && !D && !F && !G && !H) ||
                (F && !A && !B && !C && !D && !E && !G && !H) ||
                (G && !A && !B && !C && !D && !E && !F && !H) ||
                (H && !A && !B && !C && !D && !E && !F && !G);

            // if(x == 205 - imgX && y == 113 - imgY)
            //     console.log('neighbourCnt ' + neighbourCnt8 + ' '
            //     + 'e ' + thinImag[fromXY(x,y)] + ' '
            //     + 'A ' + A + ' '
            //     + 'B ' + B + ' '
            //     + 'C ' + C + ' '
            //     + 'D ' + D + ' '
            //     + 'E ' + E + ' '
            //     + 'F ' + F + ' '
            //     + 'G ' + G + ' '
            //     + 'H ' + H + ' '
            //     + thinImag[fromXY(x-1, y-1)] + ' ' //
            //     + thinImag[fromXY(x  , y-1)] + ' ' //
            //     + thinImag[fromXY(x+1, y-1)] + ' ' //
            //     + thinImag[fromXY(x+1, y  )] + ' ' //
            //     + thinImag[fromXY(x+1, y+1)] + ' ' //
            //     + thinImag[fromXY(x  , y+1)] + ' ' //
            //     + thinImag[fromXY(x-1, y+1)] + ' ' //
            //     + thinImag[fromXY(x-1, y  )] + ' ' //
            // );

            if (C1 || C2 || C3 || C4) {
                //Crossing
                thinImagRGB.push(255);
                thinImagRGB.push(0);
                thinImagRGB.push(255);
                thinImagRGB.push(255);
                thinImag[i] = 2;
                crossings.push(new SitePoint(x, y));
            } else if (e && END) {
                //End
                thinImagRGB.push(255);
                thinImagRGB.push(255);
                thinImagRGB.push(0);
                thinImagRGB.push(255);
                thinImag[i] = 3;
                ends.push(new SitePoint(x, y));
            } else if (e) {
                //Line
                thinImagRGB.push(0);
                thinImagRGB.push(0);
                thinImagRGB.push(255);
                thinImagRGB.push(255);
            } else {
                //Background
                thinImagRGB.push(imageData.data[j]);
                thinImagRGB.push(imageData.data[j + 1]);
                thinImagRGB.push(imageData.data[j + 2]);
                thinImagRGB.push(255);
            }
            i++;
            j += 4;
        });

        let n: Node = vectorize(thinImag, crossings, ends);

        visualizeTreeRec(n, thinImagRGB);
        var thinImgageData = new ImageData(
            new Uint8ClampedArray(thinImagRGB),
            tileWidth,
            tileHeight,
        );
        ctx.putImageData(thinImgageData, imgX, imgY);

        subdivideTreeRec(n);

        let ss: Array<SiteSegment> = [];
        segemntsFromTreeRec(n, ss);
        siteSegments = ss; // Trigger Reactive Update
        siteStore.set({
            sitePoints: sitePoints.map((p) => translatePoint(p, tileCenter)),
            siteSegments: ss.map((s) => translateSegment(s, tileCenter)),
            tileWidth: tileWidth,
            tileHeight: tileHeight,
        });
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

    class Node {
        constructor(sitePoint: SitePoint, parent: Node | null) {
            this.sitePoint = sitePoint;
            this.parent = parent;
            if (parent != null) parent.children.push(this);
        }
        sitePoint: SitePoint | null;
        parent: Node | null = null;
        children: Array<Node> = [];
        pixel: Array<SitePoint> = [];
        loops: Array<Node> = [];
    }

    function vectorize(
        thinImag: number[],
        crossings: Array<SitePoint>,
        ends: Array<SitePoint>,
    ): Node {
        let currNode = new Node(ends[0], null);
        thinImag[fromXY(currNode.sitePoint!.x, currNode.sitePoint!.y)] = 4; // Set Visited
        let neigh: Array<SitePoint> = neighbourIdxs(
            thinImag,
            currNode.sitePoint!.x,
            currNode.sitePoint!.y,
        );
        neigh.forEach((n: SitePoint) => {
            if (thinImag[fromXY(n.x, n.y)] < 4) {
                // Skip already Visited
                vectorizeRec(
                    currNode,
                    n,
                    [currNode.sitePoint!],
                    thinImag,
                    crossings,
                    ends,
                );
            }
        });
        return currNode;
    }

    // thinImag --- Legend
    // 0 - Nothing
    // 1 - Line
    // 2 - Crossing
    // 3 - End
    // 4 - Visited
    // 5 - Visited Crossing
    function vectorizeRec(
        currNode: Node,
        currPos: SitePoint,
        pixel: Array<SitePoint>,
        thinImag: number[],
        crossings: Array<SitePoint>,
        ends: Array<SitePoint>,
    ) {
        let node: Node;
        pixel.push(currPos);
        if (thinImag[fromXY(currPos.x, currPos.y)] == 2) {
            // Crossing
            thinImag[fromXY(currPos.x, currPos.y)] = 5; // Set Visited

            let squareDist =
                (currPos.x - currNode.sitePoint!.x) *
                    (currPos.x - currNode.sitePoint!.x) +
                (currPos.y - currNode.sitePoint!.y) *
                    (currPos.y - currNode.sitePoint!.y);
            if (squareDist > 4) {
                node = new Node(currPos, currNode);
                node.pixel = pixel;
                let neigh: Array<SitePoint> = neighbourIdxs(
                    thinImag,
                    currPos.x,
                    currPos.y,
                );
                neigh.forEach((n: SitePoint) => {
                    if (thinImag[fromXY(n.x, n.y)] < 4) {
                        // Skip already Visited
                        vectorizeRec(node, n, [], thinImag, crossings, ends);
                    }
                });
            } else {
                currNode.pixel.push(currPos);
                thinImag[fromXY(currNode.sitePoint!.x, currNode.sitePoint!.y)] =
                    4; // Old Point is not a Node anymore
                currNode.sitePoint = currPos;
                let neigh: Array<SitePoint> = neighbourIdxs(
                    thinImag,
                    currPos.x,
                    currPos.y,
                );
                neigh.forEach((n: SitePoint) => {
                    if (thinImag[fromXY(n.x, n.y)] < 4) {
                        // Skip already Visited
                        vectorizeRec(
                            currNode,
                            n,
                            [],
                            thinImag,
                            crossings,
                            ends,
                        );
                    }
                });
            }
        } else if (thinImag[fromXY(currPos.x, currPos.y)] == 3) {
            // End
            thinImag[fromXY(currPos.x, currPos.y)] = 4; // Set Visited
            node = new Node(currPos, currNode);
            node.pixel = pixel;
        } else if (thinImag[fromXY(currPos.x, currPos.y)] == 1) {
            // Line
            thinImag[fromXY(currPos.x, currPos.y)] = 4; // Set Visited
            node = currNode;
            let neigh: Array<SitePoint> = neighbourIdxs(
                thinImag,
                currPos.x,
                currPos.y,
            );
            neigh.forEach((n: SitePoint) => {
                if (thinImag[fromXY(n.x, n.y)] < 4) {
                    // Skip already Visited
                    vectorizeRec(node, n, pixel, thinImag, crossings, ends);
                } else if (
                    thinImag[fromXY(n.x, n.y)] == 5 &&
                    pixel.length > 1
                ) {
                    // detect loops but not if we just started the run and are still next to our parent node
                    node = new Node(n, currNode);
                    node.pixel = pixel;
                    currNode.loops.push(node);
                }
            });
        }
    }

    function neighbourIdxs(
        thinImag: number[],
        x: number,
        y: number,
    ): Array<SitePoint> {
        let n: Array<SitePoint> = [];
        if (thinImag[fromXY(x - 1, y - 1)]) n.push({ x: x - 1, y: y - 1 });
        if (thinImag[fromXY(x - 1, y)]) n.push({ x: x - 1, y: y });
        if (thinImag[fromXY(x - 1, y + 1)]) n.push({ x: x - 1, y: y + 1 });
        if (thinImag[fromXY(x, y - 1)]) n.push({ x: x, y: y - 1 });
        if (thinImag[fromXY(x, y + 1)]) n.push({ x: x, y: y + 1 });
        if (thinImag[fromXY(x + 1, y - 1)]) n.push({ x: x + 1, y: y - 1 });
        if (thinImag[fromXY(x + 1, y)]) n.push({ x: x + 1, y: y });
        if (thinImag[fromXY(x + 1, y + 1)]) n.push({ x: x + 1, y: y + 1 });
        return n.sort(
            (a, b) => thinImag[fromXY(b.x, b.y)] - thinImag[fromXY(a.x, a.y)],
        ); // Sort to process Ends and Crossings first;
    }

    function segemntsFromTreeRec(
        n: Node,
        segments: Array<SiteSegment>,
    ): Array<SiteSegment> {
        n.children.forEach((c) => {
            segments.push(
                new SiteSegment(
                    n.sitePoint!.x,
                    n.sitePoint!.y,
                    c.sitePoint!.x,
                    c.sitePoint!.y,
                ),
            );
            segemntsFromTreeRec(c, segments);
        });
        // n.loops.forEach(l => {
        //     segments.push(new SiteSegment(n.sitePoint!.x, n.sitePoint!.y, l.sitePoint!.x, l.sitePoint!.y));
        //     segemntsFromTreeRec(l, segments);
        // });

        return segments;
    }

    function visualizeTreeRec(n: Node, thinImagRGB: number[]) {
        let col = getRandomColor();
        n.pixel.forEach((p) => {
            thinImagRGB[fromXY(p.x, p.y) * 4] = col[0];
            thinImagRGB[fromXY(p.x, p.y) * 4 + 1] = col[1];
            thinImagRGB[fromXY(p.x, p.y) * 4 + 2] = col[2];
        });

        // thinImagRGB[fromXY(n.sitePoint!.x+1, n.sitePoint!.y) * 4    ] = col[0];
        // thinImagRGB[fromXY(n.sitePoint!.x+1, n.sitePoint!.y) * 4 + 1] = col[1];
        // thinImagRGB[fromXY(n.sitePoint!.x+1, n.sitePoint!.y) * 4 + 2] = col[2];

        // thinImagRGB[fromXY(n.sitePoint!.x-1, n.sitePoint!.y) * 4    ] = col[0];
        // thinImagRGB[fromXY(n.sitePoint!.x-1, n.sitePoint!.y) * 4 + 1] = col[1];
        // thinImagRGB[fromXY(n.sitePoint!.x-1, n.sitePoint!.y) * 4 + 2] = col[2];

        // thinImagRGB[fromXY(n.sitePoint!.x, n.sitePoint!.y+1) * 4    ] = col[0];
        // thinImagRGB[fromXY(n.sitePoint!.x, n.sitePoint!.y+1) * 4 + 1] = col[1];
        // thinImagRGB[fromXY(n.sitePoint!.x, n.sitePoint!.y+1) * 4 + 2] = col[2];

        // thinImagRGB[fromXY(n.sitePoint!.x, n.sitePoint!.y-1) * 4    ] = col[0];
        // thinImagRGB[fromXY(n.sitePoint!.x, n.sitePoint!.y-1) * 4 + 1] = col[1];
        // thinImagRGB[fromXY(n.sitePoint!.x, n.sitePoint!.y-1) * 4 + 2] = col[2];

        n.children.forEach((c) => {
            visualizeTreeRec(c, thinImagRGB);
        });
    }

    function subdivideTreeRec(n: Node) {
        n.children.forEach((c) => {
            // if(c.sitePoint!.x == 186 && c.sitePoint!.y == 104)
            //     console.log(n.children);
            if (c.pixel.length > 10) {
                let idx = Math.floor(c.pixel.length / 2);
                let halfPoint = c.pixel[idx];
                let x = halfPoint.x;
                let y = halfPoint.y;
                let x1 = n.sitePoint!.x;
                let y1 = n.sitePoint!.y;
                let x2 = c.sitePoint!.x;
                let y2 = c.sitePoint!.y;
                let dist;

                // Cross-Track-Error
                let t =
                    ((x - x1) * (x2 - x1) + (y - y1) * (y2 - y1)) /
                    ((y2 - y1) * (y2 - y1) + (x2 - x1) * (x2 - x1));
                if (t < 0) {
                    dist = Math.sqrt((x1 - x) * (x1 - x) + (y1 - y) * (y1 - y));
                } else if (t > 1) {
                    dist = Math.sqrt((x2 - x) * (x2 - x) + (y2 - y) * (y2 - y));
                } else {
                    dist =
                        ((y2 - y1) * x - (x2 - x1) * y + x2 * y1 - y2 * x1) /
                        Math.sqrt(
                            (y2 - y1) * (y2 - y1) + (x2 - x1) * (x2 - x1),
                        );
                }
                dist = Math.abs(dist);
                if (dist > deviation) {
                    // console.log('dividing (' + n.sitePoint!.x + ',' + n.sitePoint!.y + ') (' + c.sitePoint!.x + ',' + c.sitePoint!.y + ') -> (' + halfPoint.x + ' ' + halfPoint.y + ')');
                    let center = new Node(halfPoint, null);

                    center.parent = n;
                    center.children = [c];

                    center.pixel = c.pixel.slice(0, idx + 1);
                    c.pixel = c.pixel.slice(idx + 1, c.pixel.length);

                    // Replace Child with new one
                    for (let index = 0; index < n.children.length; index++) {
                        // use for instead of other syntax to not confuse the outer foreach loop
                        if (n.children[index] == c) n.children[index] = center;
                    }
                    c.parent = center;

                    subdivideTreeRec(n);
                }
            }
            subdivideTreeRec(c);
        });
        n.loops.forEach((l) => {
            subdivideTreeRec(l);
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

        siteStore.set({
            sitePoints: [...sitePoints].map((p) =>
                translatePoint(p, tileCenter),
            ),
            siteSegments: [...initialSiteSegments].map((s) =>
                translateSegment(s, tileCenter),
            ),
            tileWidth: tileWidth,
            tileHeight: tileHeight,
        });
    });
</script>

<div>
    <div class="grid grid-cols-1 justify-items-center gap-4">
        <div class="flex flex-cols">
            <div class="tools flex flex-col">
                <button class="bg-slate-50 hover:bg-slate-200 border border-sky-600 p-2 rounded w-10 h-10 m-1"
                > M </button>
                <button class="bg-slate-50 hover:bg-slate-200 border border-sky-600 p-2 rounded w-10 h-10 m-1"
                > A </button>
                <button class="bg-slate-50 hover:bg-slate-200 border border-sky-600 p-2 rounded w-10 h-10 m-1"
                > D </button>
            </div>
            <div class="grid grid-cols-1">
                <canvas class="uploadCanvas bg-slate-50 border border-sky-600 col-start-1 row-start-1"
                    bind:this={canvas}
                    width={canvasWidth}
                    height={canvasHeight}
                    style="width: {canvasWidth}px; height: {canvasHeight}px"
                ></canvas>
                <div class="overdrawSvg col-start-1 row-start-1"
                bind:this={svgContainer}
            >
                <svg
                    id="previewSvg"
                    width={tileWidth}
                    height={tileHeight}
                    viewBox="0 0 {tileWidth} {tileHeight}"
                    xmlns="http://www.w3.org/2000/svg"
                >
                    <circle
                        id="origin"
                        cx={tileCenter.x}
                        cy={tileCenter.y}
                        r="5"
                        fill="pink"
                    ></circle>
                    {#each sitePoints as siteP}
                        <circle cx={siteP.x} cy={siteP.y} r="2" fill="black"
                        ></circle>
                    {/each}
                    {#each siteSegments as siteS}
                        <path
                            d="M {siteS.x1} {siteS.y1} L {siteS.x2} {siteS.y2}"
                            stroke="red"
                            stroke-width="1"
                            fill="none"
                        ></path>
                        <circle cx={siteS.x1} cy={siteS.y1} r="2" fill="red"
                        ></circle>
                        <circle cx={siteS.x2} cy={siteS.y2} r="2" fill="red"
                        ></circle>
                    {/each}
                </svg>
            </div>
            </div>
        </div>
        <div class="resolution-slider min-w-72 font-sans text-center text-sky-400"
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
        <button class="bg-blue-500 hover:bg-blue-700 text-white font-bold py-2 px-4 rounded min-w-40"
            on:click={() => {
                fileinput.click();
            }}>Upload Image</button
        >
        <button class="bg-blue-500 hover:bg-blue-700 text-white font-bold py-2 px-4 rounded min-w-40"
            on:click={() => {
                downloadCanvasImage();
            }}>Download Image</button
        >
        <button class="bg-blue-500 hover:bg-blue-700 text-white font-bold py-2 px-4 rounded min-w-40"
            on:click={() => {
                downloadSVG();
            }}>Download SVG</button
        >
        <input type="file"
            style="display:none"
            accept=".jpg, .jpeg, .png"
            on:change={(e) => onFileSelected(e)}
            bind:this={fileinput}
        />
    </div>
</div>

<style>
</style>
