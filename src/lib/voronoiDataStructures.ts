import { Point } from "./tactile/tactile";

export class BBox {
    xl: number;
    xh: number;
    yl: number;
    yh: number;
    constructor(
        xl: number,
        xh: number,
        yl: number,
        yh: number) {
        this.xl = xl;
        this.xh = xh;
        this.yl = yl;
        this.yh = yh;
    }
};

export class Sites{
    sitePoints: SitePoint[] = [];
    siteSegments: SiteSegment[] = [];
    tileWidth: number = 300;
    tileHeight: number = 300;
}

export class SitePoint extends Point {

    // voronoiId: number = -1;

    constructor(x: number, y: number) {
        super(x, y);
        // this.x = x;
        // this.y = y;
    }
}

export class SiteSegment {
    minus(rhs: Point): any {
        return new SiteSegment(this.x1-rhs.x, this.y1-rhs.y, this.x2-rhs.x, this.y2-rhs.y);
    }
    x1: number;
    y1: number;
    x2: number;
    y2: number;
    constructor(x1: number, y1: number, x2: number, y2: number) {
        this.x1 = x1;
        this.y1 = y1;
        this.x2 = x2;
        this.y2 = y2;
    }
}


export interface Vertex {
    x: number;
    y: number;
    isValid: boolean;
}

export interface Edge {
    va: Vertex;
    vb: Vertex;
    // samples: Vertex[];
    controlPoints: Vertex[];
    isPrimary: boolean;
    isCurved: boolean;
    isValid: boolean;
}

export interface Cell {
    sourceIndex: number;
    sourceCategory: number;
    isDegenerate: boolean;
    containsPoint: boolean;
    containsSegment: boolean;
    edgeIndices: number[];
}

