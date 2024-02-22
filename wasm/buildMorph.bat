@REM Example Prject: 
@REM https://github.com/wolfmcnally/svelte-emscripten


call emcc ^
-l embind ^
morph.cpp geometricTool.cpp ^
-O0 ^
-g2 ^
-o ../src/lib/wasm/wasmMorph.js ^
-s EXPORT_ES6=1 ^
-s MODULARIZE=1 ^
-s ENVIRONMENT='web' ^
-s NO_DISABLE_EXCEPTION_CATCHING ^
--embind-emit-tsd wasmMorph.d.ts ^
-s ASSERTIONS ^
-sINITIAL_MEMORY=65536000 ^
-s EXPORTED_RUNTIME_METHODS=['cwrap','ccall']
@REM -s USE_BOOST_HEADERS=1 ^

echo export default function instantiate_wasmMorph(mod^?: any): Promise^<MorphWasmModule^>^; >> ../src/lib/wasm/wasmMorph.d.ts

powershell -Command "(gc ../src/lib/wasm/wasmMorph.d.ts) -replace 'MainModule', 'MorphWasmModule' | Out-File -encoding ASCII ../src/lib/wasm/wasmMorph.d.ts"