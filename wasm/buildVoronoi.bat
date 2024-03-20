@REM Example Prject: 
@REM https://github.com/wolfmcnally/svelte-emscripten


call emcc ^
-l embind ^
voronoi.cpp ^
-O3 ^
-g2 ^
-o ../src/lib/wasm/wasmVoronoi.js ^
-s EXPORT_ES6=1 ^
-s MODULARIZE=1 ^
-s ENVIRONMENT='web' ^
-s NO_DISABLE_EXCEPTION_CATCHING ^
-s USE_BOOST_HEADERS=1 ^
--embind-emit-tsd wasmVoronoi.d.ts ^
-s ASSERTIONS ^
-sINITIAL_MEMORY=65536000 ^
-s EXPORTED_RUNTIME_METHODS=['cwrap','ccall']
@REM --post-js voronoi.post.js ^

echo export default function instantiate_wasmVoronoi(mod^?: any): Promise^<VoronoiWasmModule^>^; >> ../src/lib/wasm/wasmVoronoi.d.ts

powershell -Command "(gc ../src/lib/wasm/wasmVoronoi.d.ts) -replace 'MainModule', 'VoronoiWasmModule' | Out-File -encoding ASCII ../src/lib/wasm/wasmVoronoi.d.ts"