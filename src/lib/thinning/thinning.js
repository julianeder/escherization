// trace_skeleton.vanilla.js
// Trace skeletonization result into polylines
// Copyright (c) 2020 Lingdong Huang
// MIT License
// https://github.com/LingDong-/skeleton-tracing/tree/master
// Modified by Julian Eder


var TraceSkeleton = new function(){ var that = this; 
  
  /** Binary image thinning (skeletonization) in-place.
    * Implements Zhang-Suen algorithm.
    * http://agcggs680.pbworks.com/f/Zhan-Suen_algorithm.pdf
    * @param im   the binary image
    * @param w    width
    * @param h    height
    */
  that.thinningZS = function(im, w, h){
    var diff = true;
    do {
      diff &= thinningZSIteration(im,w,h,0);
      diff &= thinningZSIteration(im,w,h,1);
    }while (diff);
    return im;
  }
  // 1 pass of Zhang-Suen thinning 
  function thinningZSIteration(im, w, h, iter) {
    var diff = 0
    for (var i = 1; i < h-1; i++){
      for (var j = 1; j < w-1; j++){
        
        var p2 = im[(i-1)*w+j]  &1;
        var p3 = im[(i-1)*w+j+1]&1;
        var p4 = im[(i)*w+j+1]  &1;
        var p5 = im[(i+1)*w+j+1]&1;
        var p6 = im[(i+1)*w+j]  &1;
        var p7 = im[(i+1)*w+j-1]&1;
        var p8 = im[(i)*w+j-1]  &1;
        var p9 = im[(i-1)*w+j-1]&1;
  
        var A  = (p2 == 0 && p3 == 1) + (p3 == 0 && p4 == 1) + 
                 (p4 == 0 && p5 == 1) + (p5 == 0 && p6 == 1) + 
                 (p6 == 0 && p7 == 1) + (p7 == 0 && p8 == 1) +
                 (p8 == 0 && p9 == 1) + (p9 == 0 && p2 == 1);
        var B  = p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9;
        var m1 = iter == 0 ? (p2 * p4 * p6) : (p2 * p4 * p8);
        var m2 = iter == 0 ? (p4 * p6 * p8) : (p2 * p6 * p8);
  
        if (A == 1 && (B >= 2 && B <= 6) && m1 == 0 && m2 == 0)
          im[i*w+j] |= 2;
      }
    }
    for (var i = 0; i < h*w; i++){
      var marker = im[i]>>1;
      var old = im[i]&1;
      im[i] = old & (!marker);
      if ((!diff) && (im[i] != old)){
        diff = 1;
      }
    }
    return diff;
  }
  
  that.imageDataToBinary = function(im){
    var w = im.width;
    var h = im.height;
    var data = im.data;
    var m = [];
    for (var i = 0; i < data.length; i+=4){
      if (data[i] || data[i+1] || data[i+2]){
        m.push(1)
      }else{
        m.push(0)
      }
    }
    return m;
  }  

}

export default TraceSkeleton 