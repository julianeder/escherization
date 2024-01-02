
// ---------------------------------------------------------------------------
// Circle event methods
// Beachsection = CircleEvent 

import type { Edge, SiteEvent } from "./voronoi";

// ---------------------------------------------------------------------------
// Red-Black tree code (based on C version of "rbtree" by Franck Bui-Huu
// https://github.com/fbuihuu/libtree/blob/master/rb.c

// rhill 2011-06-07: For some reasons, performance suffers significantly
// when instanciating a literal object instead of an empty ctor
export class Beachsection {
    // rhill 2013-10-12: it helps to state exactly what we are at ctor time.
    arc: Beachsection | null | undefined = null;
    site: SiteEvent | null = null;

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

export class RBTree {
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
            node!.rbPrevious = successor;
            // <<<
            node!.rbLeft = successor;
            parent = node;
        }
        else {
            // >>> Performance: cache previous/next nodes
            successor.rbPrevious = null;
            successor.rbNext = null;
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

    getFirst(node: Beachsection | null): Beachsection | null {
        while (node?.rbLeft) {
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
