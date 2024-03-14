import TraceSkeleton from "./trace_skeleton.vanilla";
import { Point, SiteSegment } from "./voronoiDataStructures";

class Node {
    constructor(sitePoint: Point, parent: Node | null, pixel: Array<Point>) {
        this.point = sitePoint;
        this.parent = parent;
        if (parent != null) parent.children.push(this);
        this.pixel = pixel;
    }
    point: Point | null;
    parent: Node | null = null;
    children: Array<Node> = [];
    pixel: Array<Point> = [];
    loops: Array<Node> = [];
    segmentRef: SiteSegment | null = null;
}

export class Vectorization {

    static ctx: CanvasRenderingContext2D;
    static imgX: number;
    static imgY: number;
    static tileWidth: number;
    static tileHeight: number;
    static deviation: number;

    static updateSkelleton(ctx: CanvasRenderingContext2D, imgX: number, imgY: number, tileWidth: number, tileHeight: number, deviation: number): Array<SiteSegment> {
        if (ctx == null) return [];

        Vectorization.ctx = ctx;
        Vectorization.imgX = imgX;
        Vectorization.imgY = imgY;
        Vectorization.tileWidth = tileWidth;
        Vectorization.tileHeight = tileHeight;
        Vectorization.deviation = deviation;

        let imageData: ImageData = ctx.getImageData(
            imgX,
            imgY,
            tileWidth,
            tileHeight,
        );
        // console.log('w ' + width + ' h ' +  height + ' imgX ' + imgX + ' imgY ' + imgY);
        // console.log(imageData);
        // let { polylines, rects, thinImag } = TraceSkeleton.fromImageData(imageData, resolution);
        let binaryImg: number[] = TraceSkeleton.imageDataToBinary(imageData);

        // let borderImg: number[] = Vectorization.getBorderImg(binaryImg);

        let thinImag = TraceSkeleton.thinningZS(
            binaryImg,
            tileWidth,
            tileHeight,
        );

        // Erode to 4-Neighbourhood, but keep potential corners
        Vectorization.erodeKeepCrossings(thinImag);


        let { thinImagRGB, crossings, ends } = Vectorization.detectEndsAndCrossings(thinImag, imageData);


        let n: Node = Vectorization.vectorize(thinImag, crossings, ends);
        
        Vectorization.visualizeTreeRec(n, thinImagRGB);
        Vectorization.visualizePoints(crossings, thinImagRGB, [0,255,255]);
        Vectorization.visualizePoints(ends, thinImagRGB, [255,0,0]);
        
        // for (let x = 0; x < Vectorization.tileWidth; x++) {
        //     for (let y = 0; y < Vectorization.tileHeight; y++) {
        //         if(borderImg[this.fromXY(x,y)] == 1){
        //             thinImagRGB[this.fromXY(x,y) * 4] = 255;
        //         }
        //     }
        // }

        var thinImgageData = new ImageData(
            new Uint8ClampedArray(thinImagRGB),
            tileWidth,
            tileHeight,
        );
        ctx.putImageData(thinImgageData, imgX, imgY);

        Vectorization.subdivideTreeRec(n);

        let ss: Array<SiteSegment> = [];
        Vectorization.segemntsFromTreeRec(n, ss);

        return ss;
    }
    static visualizePoints(crossings: Point[], thinImagRGB: number[], col: number[]) {
        crossings.forEach((p) => {
            thinImagRGB[Vectorization.fromXY(p.x, p.y) * 4] = col[0];
            thinImagRGB[Vectorization.fromXY(p.x, p.y) * 4 + 1] = col[1];
            thinImagRGB[Vectorization.fromXY(p.x, p.y) * 4 + 2] = col[2];
        });   
    }

    static detectEndsAndCrossings(thinImag: any, imageData: ImageData) {
        let i = 0;
        let j = 0;
        let thinImagRGB: number[] = [];
        let crossings: Array<Point> = [];
        let ends: Array<Point> = [];

        // Corner Detection
        thinImag.forEach((e: number) => {
            let { x, y } = Vectorization.fromI(i);

            // Neighbours
            let A = thinImag[Vectorization.fromXY(x - 1, y - 1)];
            let B = thinImag[Vectorization.fromXY(x, y - 1)];
            let C = thinImag[Vectorization.fromXY(x + 1, y - 1)];
            let D = thinImag[Vectorization.fromXY(x - 1, y)];
            let E = thinImag[Vectorization.fromXY(x + 1, y)];
            let F = thinImag[Vectorization.fromXY(x - 1, y + 1)];
            let G = thinImag[Vectorization.fromXY(x, y + 1)];
            let H = thinImag[Vectorization.fromXY(x + 1, y + 1)];

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
                crossings.push(new Point(x, y));
            } else if (e && END) {
                //End
                thinImagRGB.push(255);
                thinImagRGB.push(255);
                thinImagRGB.push(0);
                thinImagRGB.push(255);
                thinImag[i] = 3;
                ends.push(new Point(x, y));
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

        return { thinImagRGB, crossings, ends };

    }

    static erodeKeepCrossings(thinImag: any) {
        let i = 0;
        thinImag.forEach((e: number) => {
            let { x, y } = Vectorization.fromI(i);

            let A = thinImag[Vectorization.fromXY(x - 1, y - 1)];
            let B = thinImag[Vectorization.fromXY(x, y - 1)];
            let C = thinImag[Vectorization.fromXY(x + 1, y - 1)];
            let D = thinImag[Vectorization.fromXY(x - 1, y)];
            let E = thinImag[Vectorization.fromXY(x + 1, y)];
            let F = thinImag[Vectorization.fromXY(x - 1, y + 1)];
            let G = thinImag[Vectorization.fromXY(x, y + 1)];
            let H = thinImag[Vectorization.fromXY(x + 1, y + 1)];
            
            let E1 =
                e && E && G;
            let E2 =
                e && E && B;
            let E3 =
                e && D && B;
            let E4 =
                e && D && G;


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
    }

    static getBorderImg(binaryImg: number[]): number[] {

        let borderImg: number[] = Array<number>(binaryImg.length);
        for (let x = 0; x < Vectorization.tileWidth; x++) {
            for (let y = 0; y < Vectorization.tileHeight; y++) {
                if (binaryImg[this.fromXY(x, y)] == 0 &&
                    (
                        binaryImg[this.fromXY(x - 1, y)] == 1
                        || binaryImg[this.fromXY(x + 1, y)] == 1
                        || binaryImg[this.fromXY(x, y - 1)] == 1
                        || binaryImg[this.fromXY(x, y + 1)] == 1
                    )
                ) {
                    borderImg[this.fromXY(x, y)] = 1;
                }
                else {
                    borderImg[this.fromXY(x, y)] = 0;
                }

            }
        }
        return borderImg;
    }

    static calcNeighbourCnt16(thinImag: number[], x: number, y: number) {
        let neighbourCnt: number = 0;
        if (thinImag[Vectorization.fromXY(x - 2, y - 2)]) neighbourCnt++;
        if (thinImag[Vectorization.fromXY(x - 1, y - 2)]) neighbourCnt++;
        if (thinImag[Vectorization.fromXY(x, y - 2)]) neighbourCnt++;
        if (thinImag[Vectorization.fromXY(x + 1, y - 2)]) neighbourCnt++;
        if (thinImag[Vectorization.fromXY(x + 2, y - 2)]) neighbourCnt++;

        if (thinImag[Vectorization.fromXY(x + 2, y - 1)]) neighbourCnt++;
        if (thinImag[Vectorization.fromXY(x + 2, y)]) neighbourCnt++;
        if (thinImag[Vectorization.fromXY(x + 2, y + 1)]) neighbourCnt++;

        if (thinImag[Vectorization.fromXY(x + 2, y + 2)]) neighbourCnt++;
        if (thinImag[Vectorization.fromXY(x + 1, y + 2)]) neighbourCnt++;
        if (thinImag[Vectorization.fromXY(x, y + 2)]) neighbourCnt++;
        if (thinImag[Vectorization.fromXY(x - 1, y + 2)]) neighbourCnt++;
        if (thinImag[Vectorization.fromXY(x - 2, y + 2)]) neighbourCnt++;

        if (thinImag[Vectorization.fromXY(x - 2, y + 1)]) neighbourCnt++;
        if (thinImag[Vectorization.fromXY(x - 2, y)]) neighbourCnt++;
        if (thinImag[Vectorization.fromXY(x - 2, y - 1)]) neighbourCnt++;
    }

    static calcNeighbourCnt8(
        thinImag: number[],
        x: number,
        y: number,
    ): number {
        let neighbourCnt: number = 0;
        if (thinImag[Vectorization.fromXY(x - 1, y - 1)]) neighbourCnt++;
        if (thinImag[Vectorization.fromXY(x, y - 1)]) neighbourCnt++;
        if (thinImag[Vectorization.fromXY(x + 1, y - 1)]) neighbourCnt++;
        if (thinImag[Vectorization.fromXY(x + 1, y)]) neighbourCnt++;
        if (thinImag[Vectorization.fromXY(x + 1, y + 1)]) neighbourCnt++;
        if (thinImag[Vectorization.fromXY(x, y + 1)]) neighbourCnt++;
        if (thinImag[Vectorization.fromXY(x - 1, y + 1)]) neighbourCnt++;
        if (thinImag[Vectorization.fromXY(x - 1, y)]) neighbourCnt++;
        return neighbourCnt;
    }
    static calcNeighbourCnt4(
        thinImag: number[],
        x: number,
        y: number,
    ): number {
        let neighbourCnt: number = 0;
        if (thinImag[Vectorization.fromXY(x, y - 1)]) neighbourCnt++;
        if (thinImag[Vectorization.fromXY(x + 1, y)]) neighbourCnt++;
        if (thinImag[Vectorization.fromXY(x, y + 1)]) neighbourCnt++;
        if (thinImag[Vectorization.fromXY(x - 1, y)]) neighbourCnt++;
        return neighbourCnt;
    }
    static fromI(i: number) {
        return { x: i % Vectorization.tileWidth, y: Math.floor(i / Vectorization.tileWidth) };
    }

    static fromXY(x: number, y: number) {
        return y * Vectorization.tileWidth + x;
    }

    static vectorize(
        thinImag: number[],
        crossings: Array<Point>,
        ends: Array<Point>,
    ): Node {
        let curNode = new Node(ends[0], null, []);
        thinImag[Vectorization.fromXY(curNode.point!.x, curNode.point!.y)] = 4; // Set Visited
        let neigh: Array<Point> = Vectorization.get1Neighbours(
            thinImag,
            curNode.point!.x,
            curNode.point!.y,
        );
        neigh.forEach((n: Point) => {
            if (thinImag[Vectorization.fromXY(n.x, n.y)] < 4) {
                // Skip already Visited
                Vectorization.vectorizeRec(
                    curNode,
                    n,
                    [curNode.point!],
                    thinImag,
                    crossings,
                    ends,
                );
            }
        });
        return curNode;
    }

    // thinImag --- Legend
    // 0 - Nothing
    // 1 - Line
    // 2 - Crossing
    // 3 - End
    // 4 - Visited
    // 5 - Visited Crossing
    static vectorizeRec(
        curNode: Node,
        curPos: Point,
        pixel: Array<Point>,
        thinImag: number[],
        crossings: Array<Point>,
        ends: Array<Point>,
    ) {
        pixel.push(curPos);
        if (thinImag[Vectorization.fromXY(curPos.x, curPos.y)] == 2) { // Crossing
            thinImag[Vectorization.fromXY(curPos.x, curPos.y)] = 5; // Set Visited

            let squareDist =
                (curPos.x - curNode.point!.x) *
                (curPos.x - curNode.point!.x) +
                (curPos.y - curNode.point!.y) *
                (curPos.y - curNode.point!.y);
            if (squareDist > 4) {
                let node = new Node(curPos, curNode, pixel); //here wrong px cnt
                let neigh: Array<Point> = Vectorization.get1Neighbours(
                    thinImag,
                    curPos.x,
                    curPos.y,
                );
                neigh.forEach((n: Point) => {
                    if (thinImag[Vectorization.fromXY(n.x, n.y)] < 4) { // Skip already Visited
                        Vectorization.vectorizeRec(node, n, [], thinImag, crossings, ends);
                    }
                });
            } else {
                curNode.pixel.push(curPos);
                thinImag[Vectorization.fromXY(curNode.point!.x, curNode.point!.y)] = 4; // Old Point is not a Node anymore
                curNode.point = curPos;
                let neigh: Array<Point> = Vectorization.get1Neighbours(thinImag, curPos.x, curPos.y );
                neigh.forEach((n: Point) => {
                    if (thinImag[Vectorization.fromXY(n.x, n.y)] < 4) { // Skip already Visited
                        Vectorization.vectorizeRec(curNode, n, [], thinImag, crossings, ends,);
                    }
                });
            }
        } else if (thinImag[Vectorization.fromXY(curPos.x, curPos.y)] == 3) { // End
            thinImag[Vectorization.fromXY(curPos.x, curPos.y)] = 4; // Set Visited
            let node = new Node(curPos, curNode, pixel);
        } else if (thinImag[Vectorization.fromXY(curPos.x, curPos.y)] == 1) { // Line
            thinImag[Vectorization.fromXY(curPos.x, curPos.y)] = 4; // Set Visited
            let neigh: Array<Point> = Vectorization.get1Neighbours(
                thinImag,
                curPos.x,
                curPos.y,
            );
            if(neigh.length > 2){ // Edge Case where we currently are next to a crossing and another pixel is our neigbour, but it actually is the crossings neighbour 
                
                neigh = neigh.filter(n => {
                    let keep = true;
                    Vectorization.get1Neighbours(thinImag, curNode.point!.x,curNode.point!.y).forEach(p =>{
                        if(p.x == n.x && p.y == n.y){
                            keep = false;
                            return;
                        }
                    });
                    return keep;
                });
            }
            neigh.forEach((n: Point) => {
                if (thinImag[Vectorization.fromXY(n.x, n.y)] < 4) {
                    // Skip already Visited
                    Vectorization.vectorizeRec(curNode, n, pixel, thinImag, crossings, ends);
                } 
                else if (thinImag[Vectorization.fromXY(n.x, n.y)] == 5 && pixel.length > 1) { // detect loops but not if we just started the run and are still next to our parent node                    
                    let node = new Node(n, curNode, pixel);
                    curNode.loops.push(node);
                }
            });
        }
    }

    static get1Neighbours(
        thinImag: number[],
        x: number,
        y: number,
    ): Array<Point> {
        let n: Array<Point> = [];
        if (thinImag[Vectorization.fromXY(x - 1, y - 1)]) n.push(new Point(x - 1, y - 1));
        if (thinImag[Vectorization.fromXY(x - 1, y)]) n.push(new Point(x - 1, y));
        if (thinImag[Vectorization.fromXY(x - 1, y + 1)]) n.push(new Point(x - 1, y + 1));
        if (thinImag[Vectorization.fromXY(x, y - 1)]) n.push(new Point(x, y - 1));
        if (thinImag[Vectorization.fromXY(x, y + 1)]) n.push(new Point(x, y + 1));
        if (thinImag[Vectorization.fromXY(x + 1, y - 1)]) n.push(new Point(x + 1, y - 1));
        if (thinImag[Vectorization.fromXY(x + 1, y)]) n.push(new Point(x + 1, y));
        if (thinImag[Vectorization.fromXY(x + 1, y + 1)]) n.push(new Point(x + 1, y + 1));
        return n.sort(
            (a, b) => thinImag[Vectorization.fromXY(b.x, b.y)] - thinImag[Vectorization.fromXY(a.x, a.y)],
        ); // Sort to process Ends and Crossings first;
    }

    static segemntsFromTreeRec(
        n: Node,
        segments: Array<SiteSegment>,
    ): Array<SiteSegment> {
        n.children.forEach((c) => {
            let s = new SiteSegment(
                n.point!.x,
                n.point!.y,
                c.point!.x,
                c.point!.y,
            );

            // Setup connections
            c.segmentRef = s;
            if (n.parent != null) {
                s.connected_12.push(n.segmentRef!)
                n.segmentRef!.connected_21.push(s)
            }

            segments.push(s);
            Vectorization.segemntsFromTreeRec(c, segments);
        });

        // Setup connections to peers
        for (let i = 0; i < n.children.length; i++) {
            for (let j = 0; j < n.children.length; j++) {
                if (i != j) {
                    n.children[i].segmentRef?.connected_11.push(n.children[j].segmentRef!);
                    n.children[j].segmentRef?.connected_11.push(n.children[i].segmentRef!);
                }
            }
        }
        // n.loops.forEach(l => {
        //     segments.push(new SiteSegment(n.sitePoint!.x, n.sitePoint!.y, l.sitePoint!.x, l.sitePoint!.y));
        //     segemntsFromTreeRec(l, segments);
        // });

        return segments;
    }

    static visualizeTreeRec(n: Node, thinImagRGB: number[]) {
        let col = Vectorization.getRandomColor();
        n.pixel.forEach((p) => {
            thinImagRGB[Vectorization.fromXY(p.x, p.y) * 4] = col[0];
            thinImagRGB[Vectorization.fromXY(p.x, p.y) * 4 + 1] = col[1];
            thinImagRGB[Vectorization.fromXY(p.x, p.y) * 4 + 2] = col[2];
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
            Vectorization.visualizeTreeRec(c, thinImagRGB);
        });
    }

    static subdivideTreeRec(n: Node) {
        n.children.forEach((c) => {
            // if(c.sitePoint!.x == 186 && c.sitePoint!.y == 104)
            //     console.log(n.children);
            if (c.pixel.length > 10) {
                let idx = Math.floor(c.pixel.length / 2);
                let halfPoint = c.pixel[idx];
                let x = halfPoint.x;
                let y = halfPoint.y;
                let x1 = n.point!.x;
                let y1 = n.point!.y;
                let x2 = c.point!.x;
                let y2 = c.point!.y;
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
                if (dist > Vectorization.deviation) {
                    // console.log('dividing (' + n.sitePoint!.x + ',' + n.sitePoint!.y + ') (' + c.sitePoint!.x + ',' + c.sitePoint!.y + ') -> (' + halfPoint.x + ' ' + halfPoint.y + ')');
                    let center = new Node(halfPoint, null, []);

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

                    Vectorization.subdivideTreeRec(n);
                }
            }
            Vectorization.subdivideTreeRec(c);
        });
        n.loops.forEach((l) => {
            Vectorization.subdivideTreeRec(l);
        });
    }

    static getRandomColor() {
        var color: number[] = [];
        for (var i = 0; i < 3; i++) {
            color.push(Math.floor(Math.random() * 255));
        }
        return color;
    }

}