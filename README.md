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
