Module['compute'] = function(numPoints, numSegments) {
    const result = ccall('__Z7computemm', 
    'number', // return type 
    ['number','number'], // arg types
    [numPoints, numSegments] // args
    );
    return result;
};