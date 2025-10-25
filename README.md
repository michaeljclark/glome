# glome

_glome_ is a tesselated octahedron demo using a nanovg 3D canvas.

this project implements a 3D canvas API on top of the nanovg 2D 
canvas API. it adds a new set of 3D canvas rendering commands and a
transformation stack using 4D matrices with homogenous coordinates
to transform 3D coordinates to 2D coordinates with perspective.

![glome](/images/glome.gif)

## Build Instructions

```
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```
