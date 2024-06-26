import { identity, type Matrix } from "transformation-matrix";

export class BBox {
    contains(x: number, y: number): any {
        return (this.xl < x && x < this.xh) && (this.yl < y && y < this.yh);
    }

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
    tileCenter: Point = new Point(0,0);
    imageOffset: Point = new Point(0,0);
}

export class Point {
	x: number = 0;
	y: number = 0;

	constructor(x: number, y: number) {
		this.x = x;
		this.y = y;
	}
}

export class SitePoint extends Point {
    minus(rhs: SitePoint): SitePoint {
        return new SitePoint(this.x - rhs.x, this.y - rhs.y, this.color, this.tileIdx);
    }

    static mulPoint(A: Matrix, B: SitePoint): SitePoint {
        // Matrix * Point
        return new SitePoint(
            A.a * B.x + A.c * B.y + A.e,
            A.b * B.x + A.d * B.y + A.f,
            B.color,
            B.tileIdx
        );
    
    };

    color: number = 0;
    tileIdx: number;

    constructor(x: number, y: number, color: number = 0, tileIdx: number = -1) {
        super(x, y);
        this.color = color;
		this.tileIdx = tileIdx;
    }
}



export class SiteSegment {
    minus(rhs: Point): any {
        return new SiteSegment(this.x1-rhs.x, this.y1-rhs.y, this.x2-rhs.x, this.y2-rhs.y, this.color, this.tileIdx);
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
    tileIdx: number;
    
    constructor(x1: number, y1: number, x2: number, y2: number, color: number = 0, tileIdx: number = -1) {
        this.x1 = x1;
        this.y1 = y1;
        this.x2 = x2;
        this.y2 = y2;
        this.color = color;
        this.connected_11 = [];
        this.connected_12 = [];
        this.connected_21 = [];
        this.connected_22 = [];
        this.tileIdx = tileIdx;
    }

    toString(): string{
        return "(" + this.x1 + " " + this.y1 + ") (" + this.x2 + " " + this.y2 + ")";
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
    controlPoints: Vertex[];
    isPrimary: boolean;
    isCurved: boolean;
    isValid: boolean;
    isWithinCell: boolean;
}

export interface Cell {
    sourceIndex: number;
    sourceCategory: number;
    isDegenerate: boolean;
    containsPoint: boolean;
    containsSegment: boolean;
    edgeIndices: number[];
    color: number;
    tileIdx: number;
}

export interface Tile {
    tileIdx: number;
    origin: Point;
    M: Matrix;
}

