import type { SiteEvent } from "./voronoi";
import type { SitePoint } from "./voronoiDataStructures";

export function point_comparison(lhs: SitePoint, rhs: SitePoint): boolean {
    if (lhs.x == rhs.x)
        return lhs.y < rhs.y;
    return lhs.x < rhs.x;
}

export function event_comparison(lhs: SiteEvent, rhs: SiteEvent){
    if (lhs.y0() != rhs.y0())
        return lhs.y0() < rhs.y0();
    if (!lhs.is_segment()) {
        if (!rhs.is_segment())
            return lhs.x0() < rhs.x0();
        if (is_vertical_Segment(rhs))
            return lhs.x0() <= rhs.x0();
        return true;
    } else {
        if (is_vertical_Segment(rhs)) {
            if (is_vertical_Segment(lhs))
                return lhs.x0() < rhs.x0();
            return false;
        }
        if (is_vertical_Segment(lhs))
            return true;
        if (lhs.x0() != rhs.x0())
            return lhs.x0() < rhs.x0();
        // edj - this might require swaping x and y
        return orientation_test.eval(lhs.point1!, lhs.point0!, rhs.point1!) == Orientation.LEFT;
    }
}

export function is_vertical_Points(point1: SitePoint, point2: SitePoint): boolean {
    return point1.x == point2.x;
}

export function is_vertical_Segment(site: SiteEvent): boolean {
    return is_vertical_Points(site.point0!, site.point1!);
}

export enum Orientation {
    RIGHT = -1,
    COLLINEAR = 0,
    LEFT = 1
};

export class orientation_test {
    // Represents orientation test result.

    // Value is a determinant of two vectors (e.g. x1 * y2 - x2 * y1).
    // Return orientation based on the sign of the determinant.
    static evalV(value: number): Orientation {
        if (value == 0) return Orientation.COLLINEAR;
        return (value < 0) ? Orientation.RIGHT : Orientation.LEFT;
    }

    static evalN(dif_x1_: number,
        dif_y1_: number,
        dif_x2_: number,
        dif_y2_: number): Orientation {
        return this.evalV(orientation_test.robust_cross_product(dif_x1_, dif_y1_, dif_x2_, dif_y2_));
    }

    static eval(point1: SitePoint,
        point2: SitePoint,
        point3: SitePoint): Orientation {
        let dx1 = (point1.x) - (point2.x);
        let dx2 = (point2.x) - (point3.x);
        let dy1 = (point1.y) - (point2.y);
        let dy2 = (point2.y) - (point3.y);
        return this.evalV(orientation_test.robust_cross_product(dx1, dy1, dx2, dy2));
    }

    static robust_cross_product(
        a1_: number,
        b1_: number,
        a2_: number,
        b2_: number
    ): number {
        let a1: number = (a1_ < 0 ? -a1_ : a1_);
        let b1: number = (b1_ < 0 ? -b1_ : b1_);
        let a2: number = (a2_ < 0 ? -a2_ : a2_);
        let b2: number = (b2_ < 0 ? -b2_ : b2_);
    
        let l = a1 * b2;
        let r = b1 * a2;
    
        if ((a1_ < 0) !== (b2_ < 0)) {
            if ((a2_ < 0) !== (b1_ < 0))
                return (l > r) ? -(l - r) : (r - l);
            else
                return -(l + r);
        }
        else {
            if ((a2_ < 0) !== (b1_ < 0))
                return (l + r);
            else
                return (l < r) ? -(r - l) : (l - r);
        }
    }
};