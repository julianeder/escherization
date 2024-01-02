import type { RBTree } from "./rbtree";
import type { SiteEvent } from "./voronoi";

export class SitePoint {
    x: number = 0;
    y: number = 0;
    // voronoiId: number = -1;

    constructor(x: number, y: number) {
        this.x = x;
        this.y = y;
    }
}

export class SiteSegment {
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

export class Cell {
    site: SiteEvent;
    halfedges: Halfedge[];
    closeMe: boolean;

    constructor(site: SiteEvent) {
        this.site = site;
        this.halfedges = [];
        this.closeMe = false;
    }
    init(site: SiteEvent) {
        this.site = site;
        this.halfedges = [];
        this.closeMe = false;
        return this;
    };

    prepareHalfedges() {
        let halfedges = this.halfedges,
            iHalfedge = halfedges.length,
            edge;
        // get rid of unused halfedges
        // rhill 2011-05-27: Keep it simple, no point here in trying
        // to be fancy: dangling edges are a typically a minority.
        while (iHalfedge--) {
            edge = halfedges[iHalfedge].edge;
            if (!edge.vb || !edge.va) {
                halfedges.splice(iHalfedge, 1);
            }
        }

        // rhill 2011-05-26: I tried to use a binary search at insertion
        // time to keep the array sorted on-the-fly (in Cell.addHalfedge()).
        // There was no real benefits in doing so, performance on
        // Firefox 3.6 was improved marginally, while performance on
        // Opera 11 was penalized marginally.
        halfedges.sort(function (a, b) { return b.angle - a.angle; });
        return halfedges.length;
    };

    // Return a list of the neighbor Ids
    getNeighborIds() {
        let neighbors = [],
            iHalfedge = this.halfedges.length,
            edge;
        while (iHalfedge--) {
            edge = this.halfedges[iHalfedge].edge;
            if (edge.lSite !== null && edge.lSite.voronoiId != this.site.voronoiId) {
                neighbors.push(edge.lSite.voronoiId);
            }
            else if (edge.rSite !== null && edge.rSite.voronoiId != this.site.voronoiId) {
                neighbors.push(edge.rSite.voronoiId);
            }
        }
        return neighbors;
    };

    // Compute bounding box
    //
    getBbox() {
        let halfedges = this.halfedges,
            iHalfedge = halfedges.length,
            xmin = Infinity,
            ymin = Infinity,
            xmax = -Infinity,
            ymax = -Infinity,
            v, vx, vy;
        while (iHalfedge--) {
            v = halfedges[iHalfedge].getStartpoint()!;
            vx = v.x;
            vy = v.y;
            if (vx < xmin) { xmin = vx; }
            if (vy < ymin) { ymin = vy; }
            if (vx > xmax) { xmax = vx; }
            if (vy > ymax) { ymax = vy; }
            // we dont need to take into account end point,
            // since each end point matches a start point
        }
        return {
            x: xmin,
            y: ymin,
            width: xmax - xmin,
            height: ymax - ymin
        };
    };

    // Return whether a point is inside, on, or outside the cell:
    //   -1: point is outside the perimeter of the cell
    //    0: point is on the perimeter of the cell
    //    1: point is inside the perimeter of the cell
    //
    pointIntersection(x: number, y: number) {
        // Check if point in polygon. Since all polygons of a Voronoi
        // diagram are convex, then:
        // http://paulbourke.net/geometry/polygonmesh/
        // Solution 3 (2D):
        //   "If the polygon is convex then one can consider the polygon
        //   "as a 'path' from the first vertex. A point is on the interior
        //   "of this polygons if it is always on the same side of all the
        //   "line segments making up the path. ...
        //   "(y - y0) (x1 - x0) - (x - x0) (y1 - y0)
        //   "if it is less than 0 then P is to the right of the line segment,
        //   "if greater than 0 it is to the left, if equal to 0 then it lies
        //   "on the line segment"
        let halfedges = this.halfedges,
            iHalfedge = halfedges.length,
            halfedge,
            p0, p1, r;
        while (iHalfedge--) {
            halfedge = halfedges[iHalfedge];
            p0 = halfedge.getStartpoint()!;
            p1 = halfedge.getEndpoint()!;
            r = (y - p0.y) * (p1.x - p0.x) - (x - p0.x) * (p1.y - p0.y);
            if (!r) {
                return 0;
            }
            if (r > 0) {
                return -1;
            }
        }
        return 1;
    };
}

export class Diagram {
    site: SitePoint | null;
    vertices: Vertex[] = [];
    beachline: RBTree | null = null;
    sweepline: number = 0;
    edges: Edge[] = [];
    cells: Cell[] = [];

    execTime: number = 0;


    constructor(site: SitePoint | null) {
        this.site = site;
    };
}

export class Vertex {
    x: number;
    y: number;
    constructor(x: number, y: number) {
        this.x = x;
        this.y = y;
    }
}

export class Edge {
    lSite: SiteEvent | null;
    rSite: SiteEvent | null;
    va: Vertex | null;
    vb: Vertex | null;

    constructor(lSite: SiteEvent | null, rSite: SiteEvent | null) {
        this.lSite = lSite;
        this.rSite = rSite;
        this.va = null;
        this.vb = null;
    };

}

export class Halfedge {
    site: SiteEvent | null;
    edge: Edge;
    angle: number;

    constructor(edge: Edge, lSite: SiteEvent, rSite: SiteEvent | null) {
        this.site = lSite;
        this.edge = edge;
        // 'angle' is a value to be used for properly sorting the
        // halfsegments counterclockwise. By convention, we will
        // use the angle of the line defined by the 'site to the left'
        // to the 'site to the right'.
        // However, border edges have no 'site to the right': thus we
        // use the angle of line perpendicular to the halfsegment (the
        // edge should have both end points defined in such case.)
        if (rSite) {
            this.angle = Math.atan2(rSite.y() - lSite.y(), rSite.x() - lSite.x());
        }
        else {
            let va = edge.va,
                vb = edge.vb;
            // rhill 2011-05-31: used to call getStartpoint()/getEndpoint(),
            // but for performance purpose, these are expanded in place here.
            this.angle = edge.lSite === lSite ?
                Math.atan2(vb!.x - va!.x, va!.y - vb!.y) :
                Math.atan2(va!.x - vb!.x, vb!.y - va!.y);
        }
    };

    getStartpoint() {
        return this.edge.lSite === this.site ? this.edge.va : this.edge.vb;
    };

    getEndpoint() {
        return this.edge.lSite === this.site ? this.edge.vb : this.edge.va;
    };
}
