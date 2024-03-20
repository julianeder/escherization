# How to Use

## Installation

Requirements:
- Node.js: https://nodejs.org/
- Webbrowser: e.g. Google Chrome

Clone the Repository:
```batch
git clone https://gitlab.lrz.de/ge25muy/escherization.git
```

Install with node package manager (npm):
```batch
cd escherization
npm install
```

## Development 
To test locally and for development (hot reloading) run:
```batch
npm run dev
```
and open the URL provided in the terminal in your browser:
http://localhost:5173/

## Production 
To compile the files, tn the project directory run:
```batch
npm run build
```
Serve the content if the ./dist directory trough a web-server of your choice.
To test this localy:
```batch
npm run preview
```

## Compiling Web Assembly Code
This is only needed if you intend to change functionality in the wasm subdirectory.

Inslall emscripten (otside the proeject dir) as described in: https://emscripten.org/docs/getting_started/downloads.html#installation-instructions-using-the-emsdk-recommended

```batch
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
emsdk.bat install latest
emsdk.bat activate latest
```
Modify the path to your instllation in wasm/setupEnv.bat

Use the scripts to compile:
- setupEnv.bat (run once before compiling)
- buildVoronoi.bat (compile voronoi.cpp)
- buildMorph.bat (compile morph.cpp)



