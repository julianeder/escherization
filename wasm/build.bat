@REM Example Prject: 
@REM https://github.com/wolfmcnally/svelte-emscripten

copy wasmVoronoi.d.ts "../src/lib/wasm/" /Y

emcc ^
-l embind ^
voronoi_basic_tutorial.cpp ^
-O3 ^
-g2 ^
-o ../src/lib/wasm/wasmVoronoi.js ^
-s EXPORT_ES6=1 ^
-s MODULARIZE=1 ^
-s ENVIRONMENT='web' ^
-s NO_DISABLE_EXCEPTION_CATCHING ^
-s USE_BOOST_HEADERS=1 ^
-s "EXPORTED_RUNTIME_METHODS=['cwrap','ccall']"
@REM --post-js voronoi.post.js ^
