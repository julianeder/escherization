export default class Voronoi {
    vertices: Vertex[];
    edges: Edge[];
    cells: Cell[];
    toRecycle: Diagram | null = null;
    beachsectionJunkyard: Beachsection[] = [];
    circleEventJunkyard: Beachsection[] = [];
    vertexJunkyard: Vertex[] = [];
    edgeJunkyard: Edge[] = [];
    cellJunkyard: Cell[] = [];

    beachline: RBTree = new RBTree();
    circleEvents: RBTree = new RBTree();
    firstCircleEvent: Beachsection | null = null;

    constructor() {
        this.vertices = [];
        this.edges = [];
        this.cells = [];
        this.toRecycle = null;
        this.beachsectionJunkyard = [];
        this.circleEventJunkyard = [];
        this.vertexJunkyard = [];
        this.edgeJunkyard = [];
        this.cellJunkyard = [];
    }

    reset() {
        if (!this.beachline) {
            this.beachline = new RBTree();
        }
        // Move leftover beachsections to the beachsection junkyard.
        if (this.beachline.root) {
            let beachsection: Beachsection = this.beachline.getFirst(this.beachline.root);
            while (beachsection) {
                this.beachsectionJunkyard.push(beachsection); // mark for reuse
                beachsection = beachsection.rbNext!;
            }
        }
        this.beachline.root = null;

        this.circleEvents.root = null;
        this.firstCircleEvent = null;
        this.vertices = [];
        this.edges = [];
        this.cells = [];
    };

    static sqrt = Math.sqrt;
    static abs = Math.abs;
    static ε = 1e-9;
    static invε = 1.0 / Voronoi.ε;
    static equalWithEpsilon = function (a: number, b: number) { return Voronoi.abs(a - b) < 1e-9; };
    static greaterThanWithEpsilon = function (a: number, b: number) { return a - b > 1e-9; };
    static greaterThanOrEqualWithEpsilon = function (a: number, b: number) { return b - a < 1e-9; };
    static lessThanWithEpsilon = function (a: number, b: number) { return b - a > 1e-9; };
    static lessThanOrEqualWithEpsilon = function (a: number, b: number) { return a - b < 1e-9; };

    createHalfedge(edge: Edge, lSite: Site, rSite: Site | null): Halfedge {
        return new Halfedge(edge, lSite, rSite);
    };

    // this create and add a vertex to the internal collection
    createVertex(x: number, y: number) {
        let v: Vertex | undefined = this.vertexJunkyard.pop();
        if (!v) {
            v = new Vertex(x, y);
        }
        else {
            v.x = x;
            v.y = y;
        }
        this.vertices.push(v!);
        return v;
    }

    // this create and add an edge to internal collection, and also create
    // two halfedges which are added to each site's counterclockwise array
    // of halfedges.

    createEdge(lSite: Site, rSite: Site, va: Vertex | undefined, vb: Vertex | undefined) {
        let edge = this.edgeJunkyard.pop();
        if (!edge) {
            edge = new Edge(lSite, rSite);
        }
        else {
            edge.lSite = lSite;
            edge.rSite = rSite;
            edge.va = edge.vb = null;
        }

        this.edges.push(edge);
        if (va) {
            this.setEdgeStartpoint(edge, lSite, rSite, va);
        }
        if (vb) {
            this.setEdgeEndpoint(edge, lSite, rSite, vb);
        }
        this.cells[lSite.voronoiId].halfedges.push(this.createHalfedge(edge, lSite, rSite));
        this.cells[rSite.voronoiId].halfedges.push(this.createHalfedge(edge, rSite, lSite));
        return edge;
    };

    createCell(site: Site) {
        let cell = this.cellJunkyard.pop();
        if (cell) {
            return cell.init(site);
        }
        return new Cell(site);
    };

    createBorderEdge(lSite: Site, va: Vertex, vb: Vertex) {
        let edge = this.edgeJunkyard.pop();
        if (!edge) {
            edge = new Edge(lSite, null);
        }
        else {
            edge.lSite = lSite;
            edge.rSite = null;
        }
        edge.va = va;
        edge.vb = vb;
        this.edges.push(edge);
        return edge;
    };

    setEdgeStartpoint(edge: Edge, lSite: Site, rSite: Site, vertex: Vertex) {
        if (!edge.va && !edge.vb) {
            edge.va = vertex;
            edge.lSite = lSite;
            edge.rSite = rSite;
        }
        else if (edge.lSite === rSite) {
            edge.vb = vertex;
        }
        else {
            edge.va = vertex;
        }
    };

    setEdgeEndpoint(edge: Edge, lSite: Site, rSite: Site, vertex: Vertex) {
        this.setEdgeStartpoint(edge, rSite, lSite, vertex);
    };

    // rhill 2011-06-02: A lot of Beachsection instanciations
    // occur during the computation of the Voronoi diagram,
    // somewhere between the number of sites and twice the
    // number of sites, while the number of Beachsections on the
    // beachline at any given time is comparatively low. For this
    // reason, we reuse already created Beachsections, in order
    // to avoid new memory allocation. This resulted in a measurable
    // performance gain.

    createBeachsection(site: Site) {
        let beachsection: Beachsection | undefined = this.beachsectionJunkyard.pop();
        if (!beachsection) {
            beachsection = new Beachsection();
        }
        beachsection.site = site;
        return beachsection;
    };

    // calculate the left break point of a particular beach section,
    // given a particular sweep line
    leftBreakPoint(arc: Beachsection, directrix: number) {
        // http://en.wikipedia.org/wiki/Parabola
        // http://en.wikipedia.org/wiki/Quadratic_equation
        // h1 = x1,
        // k1 = (y1+directrix)/2,
        // h2 = x2,
        // k2 = (y2+directrix)/2,
        // p1 = k1-directrix,
        // a1 = 1/(4*p1),
        // b1 = -h1/(2*p1),
        // c1 = h1*h1/(4*p1)+k1,
        // p2 = k2-directrix,
        // a2 = 1/(4*p2),
        // b2 = -h2/(2*p2),
        // c2 = h2*h2/(4*p2)+k2,
        // x = (-(b2-b1) + Math.sqrt((b2-b1)*(b2-b1) - 4*(a2-a1)*(c2-c1))) / (2*(a2-a1))
        // When x1 become the x-origin:
        // h1 = 0,
        // k1 = (y1+directrix)/2,
        // h2 = x2-x1,
        // k2 = (y2+directrix)/2,
        // p1 = k1-directrix,
        // a1 = 1/(4*p1),
        // b1 = 0,
        // c1 = k1,
        // p2 = k2-directrix,
        // a2 = 1/(4*p2),
        // b2 = -h2/(2*p2),
        // c2 = h2*h2/(4*p2)+k2,
        // x = (-b2 + Math.sqrt(b2*b2 - 4*(a2-a1)*(c2-k1))) / (2*(a2-a1)) + x1

        // change code below at your own risk: care has been taken to
        // reduce errors due to computers' finite arithmetic precision.
        // Maybe can still be improved, will see if any more of this
        // kind of errors pop up again.
        let site = arc.site!,
            rfocx = site.x,
            rfocy = site.y,
            pby2 = rfocy - directrix;
        // parabola in degenerate case where focus is on directrix
        if (!pby2) {
            return rfocx;
        }
        let lArc = arc.rbPrevious;
        if (!lArc) {
            return -Infinity;
        }
        site = lArc.site!;
        let lfocx = site.x,
            lfocy = site.y,
            plby2 = lfocy - directrix;
        // parabola in degenerate case where focus is on directrix
        if (!plby2) {
            return lfocx;
        }
        let hl = lfocx - rfocx,
            aby2 = 1 / pby2 - 1 / plby2,
            b = hl / plby2;
        if (aby2) {
            return (-b + Voronoi.sqrt(b * b - 2 * aby2 * (hl * hl / (-2 * plby2) - lfocy + plby2 / 2 + rfocy - pby2 / 2))) / aby2 + rfocx;
        }
        // both parabolas have same distance to directrix, thus break point is midway
        return (rfocx + lfocx) / 2;
    };

    // calculate the right break point of a particular beach section,
    // given a particular directrix
    rightBreakPoint(arc: Beachsection, directrix: number) {
        let rArc = arc.rbNext;
        if (rArc) {
            return this.leftBreakPoint(rArc, directrix);
        }
        let site = arc.site!;
        return site.y === directrix ? site.x : Infinity;
    };

    detachBeachsection(beachsection: Beachsection) {
        this.detachCircleEvent(beachsection); // detach potentially attached circle event
        this.beachline.rbRemoveNode(beachsection); // remove from RB-tree
        this.beachsectionJunkyard.push(beachsection); // mark for reuse
    };

    removeBeachsection(beachsection: Beachsection) {
        let circle = beachsection.circleEvent!,
            x = circle.x,
            y = circle.ycenter,
            vertex = this.createVertex(x, y),
            previous = beachsection.rbPrevious,
            next = beachsection.rbNext,
            disappearingTransitions = [beachsection],
            abs_fn = Math.abs;

        // remove collapsed beachsection from beachline
        this.detachBeachsection(beachsection);

        // there could be more than one empty arc at the deletion point, this
        // happens when more than two edges are linked by the same vertex,
        // so we will collect all those edges by looking up both sides of
        // the deletion point.
        // by the way, there is *always* a predecessor/successor to any collapsed
        // beach section, it's just impossible to have a collapsing first/last
        // beach sections on the beachline, since they obviously are unconstrained
        // on their left/right side.

        // look left
        let lArc = previous!;
        while (lArc.circleEvent && abs_fn(x - lArc.circleEvent.x) < 1e-9 && abs_fn(y - lArc.circleEvent.ycenter) < 1e-9) {
            previous = lArc.rbPrevious;
            disappearingTransitions.unshift(lArc);
            this.detachBeachsection(lArc); // mark for reuse
            lArc = previous!;
        }
        // even though it is not disappearing, I will also add the beach section
        // immediately to the left of the left-most collapsed beach section, for
        // convenience, since we need to refer to it later as this beach section
        // is the 'left' site of an edge for which a start point is set.
        disappearingTransitions.unshift(lArc);
        this.detachCircleEvent(lArc);

        // look right
        let rArc = next!;
        while (rArc.circleEvent && abs_fn(x - rArc.circleEvent.x) < 1e-9 && abs_fn(y - rArc.circleEvent.ycenter) < 1e-9) {
            next = rArc.rbNext;
            disappearingTransitions.push(rArc);
            this.detachBeachsection(rArc); // mark for reuse
            rArc = next!;
        }
        // we also have to add the beach section immediately to the right of the
        // right-most collapsed beach section, since there is also a disappearing
        // transition representing an edge's start point on its left.
        disappearingTransitions.push(rArc);
        this.detachCircleEvent(rArc);

        // walk through all the disappearing transitions between beach sections and
        // set the start point of their (implied) edge.
        let nArcs = disappearingTransitions.length,
            iArc;
        for (iArc = 1; iArc < nArcs; iArc++) {
            rArc = disappearingTransitions[iArc];
            lArc = disappearingTransitions[iArc - 1];
            this.setEdgeStartpoint(rArc.edge!, lArc.site!, rArc.site!, vertex);
        }

        // create a new edge as we have now a new transition between
        // two beach sections which were previously not adjacent.
        // since this edge appears as a new vertex is defined, the vertex
        // actually define an end point of the edge (relative to the site
        // on the left)
        lArc = disappearingTransitions[0];
        rArc = disappearingTransitions[nArcs - 1];
        rArc.edge = this.createEdge(lArc.site!, rArc.site!, undefined, vertex);

        // create circle events if any for beach sections left in the beachline
        // adjacent to collapsed sections
        this.attachCircleEvent(lArc);
        this.attachCircleEvent(rArc);
    };

    addBeachsection(site: Site) {
        let x = site.x,
            directrix = site.y;

        // find the left and right beach sections which will surround the newly
        // created beach section.
        // rhill 2011-06-01: This loop is one of the most often executed,
        // hence we expand in-place the comparison-against-epsilon calls.
        let lArc, rArc,
            dxl, dxr,
            node = this.beachline.root;

        while (node) {
            dxl = this.leftBreakPoint(node, directrix) - x;
            // x lessThanWithEpsilon xl => falls somewhere before the left edge of the beachsection
            if (dxl > 1e-9) {
                // this case should never happen
                // if (!node.rbLeft) {
                //    rArc = node.rbLeft;
                //    break;
                //    }
                node = node.rbLeft;
            }
            else {
                dxr = x - this.rightBreakPoint(node, directrix);
                // x greaterThanWithEpsilon xr => falls somewhere after the right edge of the beachsection
                if (dxr > 1e-9) {
                    if (!node.rbRight) {
                        lArc = node;
                        break;
                    }
                    node = node.rbRight;
                }
                else {
                    // x equalWithEpsilon xl => falls exactly on the left edge of the beachsection
                    if (dxl > -1e-9) {
                        lArc = node.rbPrevious;
                        rArc = node;
                    }
                    // x equalWithEpsilon xr => falls exactly on the right edge of the beachsection
                    else if (dxr > -1e-9) {
                        lArc = node;
                        rArc = node.rbNext;
                    }
                    // falls exactly somewhere in the middle of the beachsection
                    else {
                        lArc = rArc = node;
                    }
                    break;
                }
            }
        }
        // at this point, keep in mind that lArc and/or rArc could be
        // undefined or null.

        // create a new beach section object for the site and add it to RB-tree
        let newArc = this.createBeachsection(site);
        this.beachline.rbInsertSuccessor(lArc, newArc);

        // cases:
        //

        // [null,null]
        // least likely case: new beach section is the first beach section on the
        // beachline.
        // This case means:
        //   no new transition appears
        //   no collapsing beach section
        //   new beachsection become root of the RB-tree
        if (!lArc && !rArc) {
            return;
        }

        // [lArc,rArc] where lArc == rArc
        // most likely case: new beach section split an existing beach
        // section.
        // This case means:
        //   one new transition appears
        //   the left and right beach section might be collapsing as a result
        //   two new nodes added to the RB-tree
        if (lArc === rArc) {
            // invalidate circle event of split beach section
            this.detachCircleEvent(lArc);

            // split the beach section into two separate beach sections
            rArc = this.createBeachsection(lArc!.site!);
            this.beachline.rbInsertSuccessor(newArc, rArc);

            // since we have a new transition between two beach sections,
            // a new edge is born
            newArc.edge = rArc.edge = this.createEdge(lArc!.site!, newArc.site!, undefined, undefined);

            // check whether the left and right beach sections are collapsing
            // and if so create circle events, to be notified when the point of
            // collapse is reached.
            this.attachCircleEvent(lArc);
            this.attachCircleEvent(rArc);
            return;
        }

        // [lArc,null]
        // even less likely case: new beach section is the *last* beach section
        // on the beachline -- this can happen *only* if *all* the previous beach
        // sections currently on the beachline share the same y value as
        // the new beach section.
        // This case means:
        //   one new transition appears
        //   no collapsing beach section as a result
        //   new beach section become right-most node of the RB-tree
        if (lArc && !rArc) {
            newArc.edge = this.createEdge(lArc.site!, newArc.site!, undefined, undefined);
            return;
        }

        // [null,rArc]
        // impossible case: because sites are strictly processed from top to bottom,
        // and left to right, which guarantees that there will always be a beach section
        // on the left -- except of course when there are no beach section at all on
        // the beach line, which case was handled above.
        // rhill 2011-06-02: No point testing in non-debug version
        //if (!lArc && rArc) {
        //    throw "Voronoi.addBeachsection(): What is this I don't even";
        //    }

        // [lArc,rArc] where lArc != rArc
        // somewhat less likely case: new beach section falls *exactly* in between two
        // existing beach sections
        // This case means:
        //   one transition disappears
        //   two new transitions appear
        //   the left and right beach section might be collapsing as a result
        //   only one new node added to the RB-tree
        if (lArc !== rArc) {
            // invalidate circle events of left and right sites
            this.detachCircleEvent(lArc);
            this.detachCircleEvent(rArc);

            // an existing transition disappears, meaning a vertex is defined at
            // the disappearance point.
            // since the disappearance is caused by the new beachsection, the
            // vertex is at the center of the circumscribed circle of the left,
            // new and right beachsections.
            // http://mathforum.org/library/drmath/view/55002.html
            // Except that I bring the origin at A to simplify
            // calculation
            let lSite = lArc!.site!,
                ax = lSite.x,
                ay = lSite.y,
                bx = site.x - ax,
                by = site.y - ay,
                rSite = rArc!.site!,
                cx = rSite.x - ax,
                cy = rSite.y - ay,
                d = 2 * (bx * cy - by * cx),
                hb = bx * bx + by * by,
                hc = cx * cx + cy * cy,
                vertex = this.createVertex((cy * hb - by * hc) / d + ax, (bx * hc - cx * hb) / d + ay);

            // one transition disappear
            this.setEdgeStartpoint(rArc!.edge!, lSite, rSite, vertex);

            // two new transitions appear at the new vertex location
            newArc.edge = this.createEdge(lSite, site, undefined, vertex);
            rArc!.edge = this.createEdge(site, rSite, undefined, vertex);

            // check whether the left and right beach sections are collapsing
            // and if so create circle events, to handle the point of collapse.
            this.attachCircleEvent(lArc);
            this.attachCircleEvent(rArc);
            return;
        }
    };

    attachCircleEvent(arc: Beachsection | null | undefined) {
        let lArc = arc!.rbPrevious,
            rArc = arc!.rbNext;
        if (!lArc || !rArc) { return; } // does that ever happen?
        let lSite = lArc.site,
            cSite = arc!.site,
            rSite = rArc.site;

        // If site of left beachsection is same as site of
        // right beachsection, there can't be convergence
        if (lSite === rSite) { return; }

        // Find the circumscribed circle for the three sites associated
        // with the beachsection triplet.
        // rhill 2011-05-26: It is more efficient to calculate in-place
        // rather than getting the resulting circumscribed circle from an
        // object returned by calling Voronoi.circumcircle()
        // http://mathforum.org/library/drmath/view/55002.html
        // Except that I bring the origin at cSite to simplify calculations.
        // The bottom-most part of the circumcircle is our Fortune 'circle
        // event', and its center is a vertex potentially part of the final
        // Voronoi diagram.
        let bx = cSite!.x,
            by = cSite!.y,
            ax = lSite!.x - bx,
            ay = lSite!.y - by,
            cx = rSite!.x - bx,
            cy = rSite!.y - by;

        // If points l->c->r are clockwise, then center beach section does not
        // collapse, hence it can't end up as a vertex (we reuse 'd' here, which
        // sign is reverse of the orientation, hence we reverse the test.
        // http://en.wikipedia.org/wiki/Curve_orientation#Orientation_of_a_simple_polygon
        // rhill 2011-05-21: Nasty finite precision error which caused circumcircle() to
        // return infinites: 1e-12 seems to fix the problem.
        let d = 2 * (ax * cy - ay * cx);
        if (d >= -2e-12) { return; }

        let ha = ax * ax + ay * ay,
            hc = cx * cx + cy * cy,
            x = (cy * ha - ay * hc) / d,
            y = (ax * hc - cx * ha) / d,
            ycenter = y + by;

        // Important: ybottom should always be under or at sweep, so no need
        // to waste CPU cycles by checking

        // recycle circle event object if possible
        let circleEvent: Beachsection | undefined = this.circleEventJunkyard.pop();
        if (!circleEvent) {
            circleEvent = new Beachsection();
        }
        circleEvent!.arc = arc;
        circleEvent.site = cSite;
        circleEvent.x = x + bx;
        circleEvent.y = ycenter + Voronoi.sqrt(x * x + y * y); // y bottom
        circleEvent.ycenter = ycenter;
        arc!.circleEvent = circleEvent;

        // find insertion point in RB-tree: circle events are ordered from
        // smallest to largest
        let predecessor = null,
            node = this.circleEvents.root;
        while (node) {
            if (circleEvent.y < node.y || (circleEvent.y === node.y && circleEvent.x <= node.x)) {
                if (node.rbLeft) {
                    node = node.rbLeft;
                }
                else {
                    predecessor = node.rbPrevious;
                    break;
                }
            }
            else {
                if (node.rbRight) {
                    node = node.rbRight;
                }
                else {
                    predecessor = node;
                    break;
                }
            }
        }
        this.circleEvents.rbInsertSuccessor(predecessor, circleEvent);
        if (!predecessor) {
            this.firstCircleEvent = circleEvent;
        }
    };

    detachCircleEvent(arc: Beachsection | null | undefined) {
        let circleEvent = arc?.circleEvent;
        if (circleEvent) {
            if (!circleEvent.rbPrevious) {
                this.firstCircleEvent = circleEvent.rbNext;
            }
            this.circleEvents.rbRemoveNode(circleEvent); // remove from RB-tree
            this.circleEventJunkyard.push(circleEvent);
            arc!.circleEvent = null;
        }
    };

    // ---------------------------------------------------------------------------
    // Diagram completion methods

    // connect dangling edges (not if a cursory test tells us
    // it is not going to be visible.
    // return value:
    //   false: the dangling endpoint couldn't be connected
    //   true: the dangling endpoint could be connected
    connectEdge(edge: Edge, bbox: BBox) {
        // skip if end point already connected
        let vb = edge.vb;
        if (!!vb) { return true; }

        // make local copy for performance purpose
        let va = edge.va,
            xl = bbox.xl,
            xr = bbox.xr,
            yt = bbox.yt,
            yb = bbox.yb,
            lSite = edge.lSite!,
            rSite = edge.rSite!,
            lx = lSite.x,
            ly = lSite.y,
            rx = rSite.x,
            ry = rSite.y,
            fx = (lx + rx) / 2,
            fy = (ly + ry) / 2,
            fm, fb;

        // if we reach here, this means cells which use this edge will need
        // to be closed, whether because the edge was removed, or because it
        // was connected to the bounding box.
        this.cells[lSite.voronoiId].closeMe = true;
        this.cells[rSite.voronoiId].closeMe = true;

        // get the line equation of the bisector if line is not vertical
        if (ry !== ly) {
            fm = (lx - rx) / (ry - ly);
            fb = fy - fm * fx;
        }

        // remember, direction of line (relative to left site):
        // upward: left.x < right.x
        // downward: left.x > right.x
        // horizontal: left.x == right.x
        // upward: left.x < right.x
        // rightward: left.y < right.y
        // leftward: left.y > right.y
        // vertical: left.y == right.y

        // depending on the direction, find the best side of the
        // bounding box to use to determine a reasonable start point

        // rhill 2013-12-02:
        // While at it, since we have the values which define the line,
        // clip the end of va if it is outside the bbox.
        // https://github.com/gorhill/Javascript-Voronoi/issues/15
        // TODO: Do all the clipping here rather than rely on Liang-Barsky
        // which does not do well sometimes due to loss of arithmetic
        // precision. The code here doesn't degrade if one of the vertex is
        // at a huge distance.

        // special case: vertical line
        if (fm === undefined) {
            // doesn't intersect with viewport
            if (fx < xl || fx >= xr) { return false; }
            // downward
            if (lx > rx) {
                if (!va || va.y < yt) {
                    va = this.createVertex(fx, yt);
                }
                else if (va.y >= yb) {
                    return false;
                }
                vb = this.createVertex(fx, yb);
            }
            // upward
            else {
                if (!va || va.y > yb) {
                    va = this.createVertex(fx, yb);
                }
                else if (va.y < yt) {
                    return false;
                }
                vb = this.createVertex(fx, yt);
            }
        }
        // closer to vertical than horizontal, connect start point to the
        // top or bottom side of the bounding box
        else if (fm < -1 || fm > 1) {
            // downward
            if (lx > rx) {
                if (!va || va.y < yt) {
                    va = this.createVertex((yt - fb!) / fm, yt);
                }
                else if (va.y >= yb) {
                    return false;
                }
                vb = this.createVertex((yb - fb!) / fm, yb);
            }
            // upward
            else {
                if (!va || va.y > yb) {
                    va = this.createVertex((yb - fb!) / fm, yb);
                }
                else if (va.y < yt) {
                    return false;
                }
                vb = this.createVertex((yt - fb!) / fm, yt);
            }
        }
        // closer to horizontal than vertical, connect start point to the
        // left or right side of the bounding box
        else {
            // rightward
            if (ly < ry) {
                if (!va || va.x < xl) {
                    va = this.createVertex(xl, fm * xl + fb!);
                }
                else if (va.x >= xr) {
                    return false;
                }
                vb = this.createVertex(xr, fm * xr + fb!);
            }
            // leftward
            else {
                if (!va || va.x > xr) {
                    va = this.createVertex(xr, fm * xr + fb!);
                }
                else if (va.x < xl) {
                    return false;
                }
                vb = this.createVertex(xl, fm * xl + fb!);
            }
        }
        edge.va = va;
        edge.vb = vb;

        return true;
    };

    // line-clipping code taken from:
    //   Liang-Barsky function by Daniel White
    //   http://www.skytopia.com/project/articles/compsci/clipping.html
    // Thanks!
    // A bit modified to minimize code paths
    clipEdge(edge: Edge, bbox: BBox) {
        let ax = edge.va!.x,
            ay = edge.va!.y,
            bx = edge.vb!.x,
            by = edge.vb!.y,
            t0 = 0,
            t1 = 1,
            dx = bx - ax,
            dy = by - ay;
        // left
        let q = ax - bbox.xl;
        if (dx === 0 && q < 0) { return false; }
        let r = -q / dx;
        if (dx < 0) {
            if (r < t0) { return false; }
            if (r < t1) { t1 = r; }
        }
        else if (dx > 0) {
            if (r > t1) { return false; }
            if (r > t0) { t0 = r; }
        }
        // right
        q = bbox.xr - ax;
        if (dx === 0 && q < 0) { return false; }
        r = q / dx;
        if (dx < 0) {
            if (r > t1) { return false; }
            if (r > t0) { t0 = r; }
        }
        else if (dx > 0) {
            if (r < t0) { return false; }
            if (r < t1) { t1 = r; }
        }
        // top
        q = ay - bbox.yt;
        if (dy === 0 && q < 0) { return false; }
        r = -q / dy;
        if (dy < 0) {
            if (r < t0) { return false; }
            if (r < t1) { t1 = r; }
        }
        else if (dy > 0) {
            if (r > t1) { return false; }
            if (r > t0) { t0 = r; }
        }
        // bottom        
        q = bbox.yb - ay;
        if (dy === 0 && q < 0) { return false; }
        r = q / dy;
        if (dy < 0) {
            if (r > t1) { return false; }
            if (r > t0) { t0 = r; }
        }
        else if (dy > 0) {
            if (r < t0) { return false; }
            if (r < t1) { t1 = r; }
        }

        // if we reach this point, Voronoi edge is within bbox

        // if t0 > 0, va needs to change
        // rhill 2011-06-03: we need to create a new vertex rather
        // than modifying the existing one, since the existing
        // one is likely shared with at least another edge
        if (t0 > 0) {
            edge.va = this.createVertex(ax + t0 * dx, ay + t0 * dy);
        }

        // if t1 < 1, vb needs to change
        // rhill 2011-06-03: we need to create a new vertex rather
        // than modifying the existing one, since the existing
        // one is likely shared with at least another edge
        if (t1 < 1) {
            edge.vb = this.createVertex(ax + t1 * dx, ay + t1 * dy);
        }

        // va and/or vb were clipped, thus we will need to close
        // cells which use this edge.
        if (t0 > 0 || t1 < 1) {
            this.cells[edge.lSite!.voronoiId].closeMe = true;
            this.cells[edge.rSite!.voronoiId].closeMe = true;
        }

        return true;
    };

    // Connect/cut edges at bounding box
    clipEdges(bbox: BBox) {
        // connect all dangling edges to bounding box
        // or get rid of them if it can't be done
        let edges = this.edges,
            iEdge = edges.length,
            edge,
            abs_fn = Math.abs;

        // iterate backward so we can splice safely
        while (iEdge--) {
            edge = edges[iEdge];
            // edge is removed if:
            //   it is wholly outside the bounding box
            //   it is looking more like a point than a line
            if (!this.connectEdge(edge, bbox) ||
                !this.clipEdge(edge, bbox) ||
                (abs_fn(edge.va!.x - edge.vb!.x) < 1e-9 && abs_fn(edge.va!.y - edge.vb!.y) < 1e-9)) {
                edge.va = edge.vb = null;
                edges.splice(iEdge, 1);
            }
        }
    };

    // Close the cells.
    // The cells are bound by the supplied bounding box.
    // Each cell refers to its associated site, and a list
    // of halfedges ordered counterclockwise.
    closeCells(bbox: BBox) {
        let xl = bbox.xl,
            xr = bbox.xr,
            yt = bbox.yt,
            yb = bbox.yb,
            cells = this.cells,
            iCell = cells.length,
            cell,
            iLeft,
            halfedges, nHalfedges,
            edge,
            va, vb, vz,
            lastBorderSegment,
            abs_fn = Math.abs;

        while (iCell--) {
            cell = cells[iCell];
            // prune, order halfedges counterclockwise, then add missing ones
            // required to close cells
            if (!cell.prepareHalfedges()) {
                continue;
            }
            if (!cell.closeMe) {
                continue;
            }
            // find first 'unclosed' point.
            // an 'unclosed' point will be the end point of a halfedge which
            // does not match the start point of the following halfedge
            halfedges = cell.halfedges;
            nHalfedges = halfedges.length;
            // special case: only one site, in which case, the viewport is the cell
            // ...

            // all other cases
            iLeft = 0;
            while (iLeft < nHalfedges) {
                va = halfedges[iLeft].getEndpoint();
                vz = halfedges[(iLeft + 1) % nHalfedges].getStartpoint();
                // if end point is not equal to start point, we need to add the missing
                // halfedge(s) up to vz
                if (abs_fn(va!.x - vz!.x) >= 1e-9 || abs_fn(va!.y - vz!.y) >= 1e-9) {

                    // rhill 2013-12-02:
                    // "Holes" in the halfedges are not necessarily always adjacent.
                    // https://github.com/gorhill/Javascript-Voronoi/issues/16

                    // find entry point:
                    switch (true) {

                        // walk downward along left side
                        case Voronoi.equalWithEpsilon(va!.x, xl) && Voronoi.lessThanWithEpsilon(va!.y, yb):
                            lastBorderSegment = Voronoi.equalWithEpsilon(vz!.x, xl);
                            vb = this.createVertex(xl, lastBorderSegment ? vz!.y : yb);
                            edge = this.createBorderEdge(cell.site, va!, vb);
                            iLeft++;
                            halfedges.splice(iLeft, 0, this.createHalfedge(edge, cell.site, null));
                            nHalfedges++;
                            if (lastBorderSegment) { break; }
                            va = vb;
                        // fall through

                        // walk rightward along bottom side
                        case Voronoi.equalWithEpsilon(va!.y, yb) && Voronoi.lessThanWithEpsilon(va!.x, xr):
                            lastBorderSegment = Voronoi.equalWithEpsilon(vz!.y, yb);
                            vb = this.createVertex(lastBorderSegment ? vz!.x : xr, yb);
                            edge = this.createBorderEdge(cell.site, va!, vb);
                            iLeft++;
                            halfedges.splice(iLeft, 0, this.createHalfedge(edge, cell.site, null));
                            nHalfedges++;
                            if (lastBorderSegment) { break; }
                            va = vb;
                        // fall through

                        // walk upward along right side
                        case Voronoi.equalWithEpsilon(va!.x, xr) && Voronoi.greaterThanWithEpsilon(va!.y, yt):
                            lastBorderSegment = Voronoi.equalWithEpsilon(vz!.x, xr);
                            vb = this.createVertex(xr, lastBorderSegment ? vz!.y : yt);
                            edge = this.createBorderEdge(cell.site, va!, vb);
                            iLeft++;
                            halfedges.splice(iLeft, 0, this.createHalfedge(edge, cell.site, null));
                            nHalfedges++;
                            if (lastBorderSegment) { break; }
                            va = vb;
                        // fall through

                        // walk leftward along top side
                        case Voronoi.equalWithEpsilon(va!.y, yt) && Voronoi.greaterThanWithEpsilon(va!.x, xl):
                            lastBorderSegment = Voronoi.equalWithEpsilon(vz!.y, yt);
                            vb = this.createVertex(lastBorderSegment ? vz!.x : xl, yt);
                            edge = this.createBorderEdge(cell.site, va!, vb);
                            iLeft++;
                            halfedges.splice(iLeft, 0, this.createHalfedge(edge, cell.site, null));
                            nHalfedges++;
                            if (lastBorderSegment) { break; }
                            va = vb;
                            // fall through

                            // walk downward along left side
                            lastBorderSegment = Voronoi.equalWithEpsilon(vz!.x, xl);
                            vb = this.createVertex(xl, lastBorderSegment ? vz!.y : yb);
                            edge = this.createBorderEdge(cell.site, va, vb);
                            iLeft++;
                            halfedges.splice(iLeft, 0, this.createHalfedge(edge, cell.site, null));
                            nHalfedges++;
                            if (lastBorderSegment) { break; }
                            va = vb;
                            // fall through

                            // walk rightward along bottom side
                            lastBorderSegment = Voronoi.equalWithEpsilon(vz!.y, yb);
                            vb = this.createVertex(lastBorderSegment ? vz!.x : xr, yb);
                            edge = this.createBorderEdge(cell.site, va, vb);
                            iLeft++;
                            halfedges.splice(iLeft, 0, this.createHalfedge(edge, cell.site, null));
                            nHalfedges++;
                            if (lastBorderSegment) { break; }
                            va = vb;
                            // fall through

                            // walk upward along right side
                            lastBorderSegment = Voronoi.equalWithEpsilon(vz!.x, xr);
                            vb = this.createVertex(xr, lastBorderSegment ? vz!.y : yt);
                            edge = this.createBorderEdge(cell.site, va, vb);
                            iLeft++;
                            halfedges.splice(iLeft, 0, this.createHalfedge(edge, cell.site, null));
                            nHalfedges++;
                            if (lastBorderSegment) { break; }
                        // fall through

                        default:
                            throw "Voronoi.closeCells() > this makes no sense!";
                    }
                }
                iLeft++;
            }
            cell.closeMe = false;
        }
    };

    // ---------------------------------------------------------------------------
    // Helper: Quantize sites

    // rhill 2013-10-12:
    // This is to solve https://github.com/gorhill/Javascript-Voronoi/issues/15
    // Since not all users will end up using the kind of coord values which would
    // cause the issue to arise, I chose to let the user decide whether or not
    // he should sanitize his coord values through this helper. This way, for
    // those users who uses coord values which are known to be fine, no overhead is
    // added.

    quantizeSites(sites: Site[]) {
        let ε = Voronoi.ε,
            n = sites.length,
            site;
        while (n--) {
            site = sites[n];
            site.x = Math.floor(site.x / ε) * ε;
            site.y = Math.floor(site.y / ε) * ε;
        }
    };

    // ---------------------------------------------------------------------------
    // Helper: Recycle diagram: all vertex, edge and cell objects are
    // "surrendered" to the Voronoi object for reuse.
    // TODO: rhill-voronoi-core v2: more performance to be gained
    // when I change the semantic of what is returned.

    recycle(diagram: Diagram) {
        if (diagram) {
            if (diagram instanceof Diagram) {
                this.toRecycle = diagram;
            }
            else {
                throw 'Voronoi.recycleDiagram() > Need a Diagram object.';
            }
        }
    };

    // ---------------------------------------------------------------------------
    // Top-level Fortune loop

    // rhill 2011-05-19:
    //   Voronoi sites are kept client-side now, to allow
    //   user to freely modify content. At compute time,
    //   *references* to sites are copied locally.

    compute(sites: Site[], bbox: BBox) {
        // to measure execution time
        let startTime = new Date();

        // init internal state
        this.reset();

        // any diagram data available for recycling?
        // I do that here so that this is included in execution time
        if (this.toRecycle) {
            this.vertexJunkyard = this.vertexJunkyard.concat(this.toRecycle!.vertices);
            this.edgeJunkyard = this.edgeJunkyard.concat(this.toRecycle!.edges);
            this.cellJunkyard = this.cellJunkyard.concat(this.toRecycle!.cells);
            this.toRecycle = null;
        }

        // Initialize site event queue
        let siteEvents = sites.slice(0);
        siteEvents.sort(function (a, b) {
            let r = b.y - a.y;
            if (r) { return r; }
            return b.x - a.x;
        });

        // process queue
        let site = siteEvents.pop(),
            siteid = 0,
            xsitex, // to avoid duplicate sites
            xsitey,
            cells = this.cells,
            circle;

        // main loop
        for (; ;) {
            // we need to figure whether we handle a site or circle event
            // for this we find out if there is a site event and it is
            // 'earlier' than the circle event
            circle = this.firstCircleEvent;

            // add beach section
            if (site && (!circle || site.y < circle.y || (site.y === circle.y && site.x < circle.x))) {
                // only if site is not a duplicate
                if (site.x !== xsitex || site.y !== xsitey) {
                    // first create cell for new site
                    cells[siteid] = this.createCell(site);
                    site.voronoiId = siteid++;
                    // then create a beachsection for that site
                    this.addBeachsection(site);
                    // remember last site coords to detect duplicate
                    xsitey = site.y;
                    xsitex = site.x;
                }
                site = siteEvents.pop();
            }

            // remove beach section
            else if (circle) {
                this.removeBeachsection(circle!.arc!);
            }

            // all done, quit
            else {
                break;
            }
        }

        // wrapping-up:
        //   connect dangling edges to bounding box
        //   cut edges as per bounding box
        //   discard edges completely outside bounding box
        //   discard edges which are point-like
        this.clipEdges(bbox);

        //   add missing edges in order to close opened cells
        this.closeCells(bbox);

        // to measure execution time
        let stopTime = new Date();

        // prepare return values
        let diagram = new Diagram(null);
        diagram.cells = this.cells;
        diagram.edges = this.edges;
        diagram.vertices = this.vertices;
        diagram.execTime = stopTime.getTime() - startTime.getTime();

        // clean up
        this.reset();

        return diagram;
    };

}

class BBox {
    xl: number;
    xr: number;
    yt: number;
    yb: number;
    constructor(
        xl: number,
        xr: number,
        yt: number,
        yb: number) {
        this.xl = xl;
        this.xr = xr;
        this.yt = yt;
        this.yb = yb;
    }
};

// ---------------------------------------------------------------------------
// Circle event methods
// Beachsection = CircleEvent 

// ---------------------------------------------------------------------------
// Red-Black tree code (based on C version of "rbtree" by Franck Bui-Huu
// https://github.com/fbuihuu/libtree/blob/master/rb.c

// rhill 2011-06-07: For some reasons, performance suffers significantly
// when instanciating a literal object instead of an empty ctor
class Beachsection {
    // rhill 2013-10-12: it helps to state exactly what we are at ctor time.
    arc: Beachsection | null | undefined = null;
    site: Site | null = null;

    rbLeft: Beachsection | null = null;
    rbRight: Beachsection | null = null;
    rbNext: Beachsection | null = null;
    rbParent: Beachsection | null = null;
    rbPrevious: Beachsection | null = null;
    rbRed: boolean = false;

    x: number = 0;
    y: number = 0;
    ycenter: number = 0;

    circleEvent: Beachsection | null = null;
    edge: Edge | null = null;

    constructor() {
        this.arc = null;
        this.rbLeft = null;
        this.rbNext = null;
        this.rbParent = null;
        this.rbPrevious = null;
        this.rbRed = false;
        this.rbRight = null;
        this.site = null;
    }
};



class RBTree {
    root: Beachsection | null;
    constructor() {
        this.root = null;

    }

    rbInsertSuccessor(node: Beachsection | null | undefined, successor: Beachsection) {
        let parent;
        if (node) {
            // >>> rhill 2011-05-27: Performance: cache previous/next nodes
            successor.rbPrevious = node;
            successor.rbNext = node.rbNext;
            if (node.rbNext) {
                node.rbNext.rbPrevious = successor;
            }
            node.rbNext = successor;
            // <<<
            if (node.rbRight) {
                // in-place expansion of node.rbRight.getFirst();
                node = node.rbRight;
                while (node.rbLeft) { node = node.rbLeft; }
                node.rbLeft = successor;
            }
            else {
                node.rbRight = successor;
            }
            parent = node;
        }
        // rhill 2011-06-07: if node is null, successor must be inserted
        // to the left-most part of the tree
        else if (this.root) {
            node = this.getFirst(this.root);
            // >>> Performance: cache previous/next nodes
            successor.rbPrevious = null;
            successor.rbNext = node;
            node.rbPrevious = successor;
            // <<<
            node.rbLeft = successor;
            parent = node;
        }
        else {
            // >>> Performance: cache previous/next nodes
            successor.rbPrevious = successor.rbNext = null;
            // <<<
            this.root = successor;
            parent = null;
        }
        successor.rbLeft = successor.rbRight = null;
        successor.rbParent = parent;
        successor.rbRed = true;
        // Fixup the modified tree by recoloring nodes and performing
        // rotations (2 at most) hence the red-black tree properties are
        // preserved.
        let grandpa: Beachsection;
        let uncle: Beachsection;
        node = successor;
        while (parent && parent.rbRed) {
            grandpa = parent.rbParent!;
            if (parent === grandpa.rbLeft) {
                uncle = grandpa.rbRight!;
                if (uncle && uncle.rbRed) {
                    parent.rbRed = uncle.rbRed = false;
                    grandpa.rbRed = true;
                    node = grandpa;
                }
                else {
                    if (node === parent.rbRight) {
                        this.rbRotateLeft(parent);
                        node = parent;
                        parent = node.rbParent;
                    }
                    parent!.rbRed = false;
                    grandpa.rbRed = true;
                    this.rbRotateRight(grandpa);
                }
            }
            else {
                uncle = grandpa.rbLeft!;
                if (uncle && uncle.rbRed) {
                    parent.rbRed = uncle.rbRed = false;
                    grandpa.rbRed = true;
                    node = grandpa;
                }
                else {
                    if (node === parent.rbLeft) {
                        this.rbRotateRight(parent);
                        node = parent;
                        parent = node.rbParent;
                    }
                    parent!.rbRed = false;
                    grandpa.rbRed = true;
                    this.rbRotateLeft(grandpa);
                }
            }
            parent = node.rbParent;
        }
        this.root!.rbRed = false;
    };

    rbRemoveNode(node: Beachsection) {
        // >>> rhill 2011-05-27: Performance: cache previous/next nodes
        if (node.rbNext) {
            node.rbNext.rbPrevious = node.rbPrevious;
        }
        if (node.rbPrevious) {
            node.rbPrevious.rbNext = node.rbNext;
        }
        node.rbNext = node.rbPrevious = null;
        // <<<
        let parent = node.rbParent,
            left = node.rbLeft,
            right = node.rbRight,
            next;
        if (!left) {
            next = right;
        }
        else if (!right) {
            next = left;
        }
        else {
            next = this.getFirst(right);
        }
        if (parent) {
            if (parent.rbLeft === node) {
                parent.rbLeft = next;
            }
            else {
                parent.rbRight = next;
            }
        }
        else {
            this.root = next;
        }
        // enforce red-black rules
        let isRed;
        if (left && right) {
            isRed = next!.rbRed;
            next!.rbRed = node.rbRed;
            next!.rbLeft = left;
            left.rbParent = next;
            if (next !== right) {
                parent = next!.rbParent;
                next!.rbParent = node.rbParent;
                node = next!.rbRight!;
                parent!.rbLeft = node;
                next!.rbRight = right;
                right.rbParent = next;
            }
            else {
                next.rbParent = parent;
                parent = next;
                node = next!.rbRight!;
            }
        }
        else {
            isRed = node.rbRed;
            node = next!;
        }
        // 'node' is now the sole successor's child and 'parent' its
        // new parent (since the successor can have been moved)
        if (node) {
            node.rbParent = parent;
        }
        // the 'easy' cases
        if (isRed) { return; }
        if (node && node.rbRed) {
            node.rbRed = false;
            return;
        }
        // the other cases
        let sibling: Beachsection;
        do {
            if (node === this.root) {
                break;
            }
            if (node === parent!.rbLeft) {
                sibling = parent!.rbRight!;
                if (sibling!.rbRed) {
                    sibling!.rbRed = false;
                    parent!.rbRed = true;
                    this.rbRotateLeft(parent!);
                    sibling = parent!.rbRight!;
                }
                if ((sibling.rbLeft && sibling.rbLeft.rbRed) || (sibling.rbRight && sibling.rbRight.rbRed)) {
                    if (!sibling.rbRight || !sibling.rbRight.rbRed) {
                        sibling.rbLeft!.rbRed = false;
                        sibling.rbRed = true;
                        this.rbRotateRight(sibling);
                        sibling = parent!.rbRight!;
                    }
                    sibling.rbRed = parent!.rbRed!;
                    parent!.rbRed = sibling!.rbRight!.rbRed = false;
                    this.rbRotateLeft(parent!);
                    node = this.root!;
                    break;
                }
            }
            else {
                sibling = parent!.rbLeft!;
                if (sibling.rbRed) {
                    sibling.rbRed = false;
                    parent!.rbRed = true;
                    this.rbRotateRight(parent!);
                    sibling = parent!.rbLeft!;
                }
                if ((sibling.rbLeft && sibling.rbLeft.rbRed) || (sibling.rbRight && sibling.rbRight.rbRed)) {
                    if (!sibling.rbLeft || !sibling.rbLeft.rbRed) {
                        sibling!.rbRight!.rbRed = false;
                        sibling.rbRed = true;
                        this.rbRotateLeft(sibling);
                        sibling = parent!.rbLeft!;
                    }
                    sibling.rbRed = parent!.rbRed;
                    parent!.rbRed = sibling!.rbLeft!.rbRed = false;
                    this.rbRotateRight(parent!);
                    node = this.root!;
                    break;
                }
            }
            sibling.rbRed = true;
            node = parent!;
            parent = parent!.rbParent;
        } while (!node.rbRed);
        if (node) { node.rbRed = false; }
    }

    rbRotateLeft(node: Beachsection) {
        let p: Beachsection = node;
        let q: Beachsection = node.rbRight!; // can't be null
        let parent: Beachsection = p.rbParent!;
        if (parent) {
            if (parent.rbLeft === p) {
                parent.rbLeft = q;
            }
            else {
                parent.rbRight = q;
            }
        }
        else {
            this.root = q;
        }
        q.rbParent = parent;
        p.rbParent = q;
        p.rbRight = q.rbLeft;
        if (p.rbRight) {
            p.rbRight.rbParent = p;
        }
        q.rbLeft = p;
    }

    rbRotateRight(node: Beachsection) {
        let p: Beachsection = node;
        let q: Beachsection = node.rbLeft!; // can't be null
        let parent: Beachsection = p.rbParent!;
        if (parent) {
            if (parent.rbLeft === p) {
                parent.rbLeft = q;
            }
            else {
                parent.rbRight = q;
            }
        }
        else {
            this.root = q;
        }
        q.rbParent = parent;
        p.rbParent = q;
        p.rbLeft = q.rbRight;
        if (p.rbLeft) {
            p.rbLeft.rbParent = p;
        }
        q.rbRight = p;
    }

    getFirst(node: Beachsection): Beachsection {
        while (node.rbLeft) {
            node = node.rbLeft;
        }
        return node;
    }

    getLast(node: Beachsection): Beachsection {
        while (node.rbRight) {
            node = node.rbRight;
        }
        return node;
    }
}

// ---------------------------------------------------------------------------
// Diagram methods

export class Site {
    x: number = 0;
    y: number = 0;
    voronoiId: number = -1;
}

export class Cell {
    site: Site;
    halfedges: Halfedge[];
    closeMe: boolean;

    constructor(site: Site) {
        this.site = site;
        this.halfedges = [];
        this.closeMe = false;
    }
    init(site: Site) {
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
    site: Site | null;
    vertices: Vertex[] = [];
    edges: Edge[] = [];
    cells: Cell[] = [];

    execTime: number = 0;


    constructor(site: Site | null) {
        this.site = site;
    };
}

class Vertex {
    x: number;
    y: number;
    constructor(x: number, y: number) {
        this.x = x;
        this.y = y;
    }
}

class Edge {
    lSite: Site | null;
    rSite: Site | null;
    va: Vertex | null;
    vb: Vertex | null;

    constructor(lSite: Site | null, rSite: Site | null) {
        this.lSite = lSite;
        this.rSite = rSite;
        this.va = null;
        this.vb = null;
    };

}

class Halfedge {
    site: Site | null;
    edge: Edge;
    angle: number;

    constructor(edge: Edge, lSite: Site, rSite: Site | null) {
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
            this.angle = Math.atan2(rSite.y - lSite.y, rSite.x - lSite.x);
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




// ---------------------------------------------------------------------------
// Debugging helper
/*
Voronoi.prototype.dumpBeachline = function(y) {
    console.log('Voronoi.dumpBeachline(%f) > Beachsections, from left to right:', y);
    if ( !this.beachline ) {
        console.log('  None');
        }
    else {
        let bs = this.beachline.getFirst(this.beachline.root);
        while ( bs ) {
            console.log('  site %d: xl: %f, xr: %f', bs.site.voronoiId, this.leftBreakPoint(bs, y), this.rightBreakPoint(bs, y));
            bs = bs.rbNext;
            }
        }
    };
*/

