# obj2difPlus

A converter to convert any-size obj to lag-free difs
Difs are capped at 16000 triangles, obj will be split and multiple difs will be exported accordingly

# Usage

No moving platform support. Textures are exported from the texture files linked with the materials in the mtl file.

```
obj2difPlus <file> [-flip]
file: path to obj file, can also drag files onto the program
flip (optional): flip normals, use if the resultant dif becomes inside out
double: (optional) make all faces double sided
splitcount <count>: (optional) changes the amount of triangles required till a split is required
```

# Fixes to common problems

## Missing Faces in Difs

Split the dif using the splitcount argument. Recommended value is the value that is half of the triangle count. If it doesnt work, divide that number and try again till it works.

## Inverted "Inside out" Dif

Use the flip argument

## Messed up UV

Keep the face with the messed up UV away from the coordinate planes
