<script lang="ts">
    import type { RBTree, Beachsection } from "./rbtree";
    import Voronoi from "./voronoi";

    export let node: Beachsection | null | undefined;
    export let sweepline: number = 0;
    
    let leftBreakPoint: number = 0;
    let rightBreakPoint: number = 0;
    if(node){
        leftBreakPoint = Voronoi.leftBreakPoint(node, sweepline);
        rightBreakPoint = Voronoi.rightBreakPoint(node, sweepline);
    }
    node?.edge?.va?.x

</script>
  
{#if node}

<circle cx={leftBreakPoint} cy={sweepline} r="3" fill={node?.rbRed ? "red" : "black"}></circle>
<circle cx={rightBreakPoint} cy={sweepline} r="3" fill={node?.rbRed ? "red" : "black"}></circle>
<path
d="M {node?.edge?.va?.x} {node?.edge?.va?.y} L {node?.edge?.vb?.x} {node?.edge?.vb?.y}"
stroke="purple"
stroke-width="1"
fill="none"
></path>
<svelte:self node={node?.rbNext}/>

{/if}
  