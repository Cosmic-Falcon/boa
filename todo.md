TODO
====

Requirements for Version 0.0.1
------------------------------

###Non-monotone partitioning edge cases
* ~~Fails when diagonals are created that are not bound within the polygon~~ (fixed)
* ~~Function that assigns vertices to partitions is naive and does not work in many cases~~ (fixed)
* ~~Triangulation of ears does not work. Sometimes it does not run, and often adds non-existend vertices when it does~~ (fixed)
* Run extensive tests to verify the accuracy of triangulation

###Different colors
* ~~Add ability to change the color of polygons~~
* ~~Color attributes added to shaders, need to add into gl_data now~~
* Run extensive tests to verify that attributes work under a wide range of scenarios

###Outline mode
* Add ability to draw outlines of polygons instead of solids. This will be useful for debugging
* Possibly also add vertex mode that only draws vertices

###More comprehensive debug and error statements
* Improve clarity of debug statements
* Add more assert and error statements

Future items
------------

###Create a more comprehensive abstraction layer over OpenGL
