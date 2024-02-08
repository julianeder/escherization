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

    constructor(x: number, y: number, color: number = 0) {
        super(x, y, color);
        // this.x = x;
        // this.y = y;
    }
}

export class SiteSegment {
    minus(rhs: Point): any {
        return new SiteSegment(this.x1-rhs.x, this.y1-rhs.y, this.x2-rhs.x, this.y2-rhs.y, this.color);
    }
    x1: number;
    y1: number;
    x2: number;
    y2: number;
    
    connected_11: Array<SiteSegment>;
    connected_12: Array<SiteSegment>;
    connected_21: Array<SiteSegment>;
    connected_22: Array<SiteSegment>;

    color: number = 0;
    
    constructor(x1: number, y1: number, x2: number, y2: number, color: number = 0) {
        this.x1 = x1;
        this.y1 = y1;
        this.x2 = x2;
        this.y2 = y2;
        this.color = color;
        this.connected_11 = [];
        this.connected_12 = [];
        this.connected_21 = [];
        this.connected_22 = [];
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
    isBetweenSameColorCells: boolean;
}

export interface Cell {
    sourceIndex: number;
    sourceCategory: number;
    isDegenerate: boolean;
    containsPoint: boolean;
    containsSegment: boolean;
    edgeIndices: number[];
    color: number;
}

