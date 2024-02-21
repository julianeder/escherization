import type { SiteSegment } from "./voronoiDataStructures";

export function checkIntersections(siteSegments: SiteSegment[]): boolean {
    let intersect: boolean = false;
    for (let i = 0; i < siteSegments.length; i++) {
        for (let j = 0; j < siteSegments.length; j++) {
            if (i != j) {
                if (intersects(siteSegments[i], siteSegments[j])) {
                    // console.log("intersect " + i + " " + j);
                    intersects(siteSegments[i], siteSegments[j]);
                    intersect = true;
                    break;
                }
            }
        }
        if (intersect) break;
    }
    return intersect;
}

// Source: https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection
function intersects(a: SiteSegment, b: SiteSegment): boolean {
    let t: number =
        ((a.x1 - b.x1) * (b.y1 - b.y2) - (a.y1 - b.y1) * (b.x1 - b.x2)) /
        ((a.x1 - a.x2) * (b.y1 - b.y2) - (a.y1 - a.y2) * (b.x1 - b.x2));
    let u: number =
        -((a.x1 - a.x2) * (a.y1 - b.y1) - (a.y1 - a.y2) * (a.x1 - b.x1)) /
        ((a.x1 - a.x2) * (b.y1 - b.y2) - (a.y1 - a.y2) * (b.x1 - b.x2));
    return 0 < t && t < 1 && 0 < u && u < 1;
}