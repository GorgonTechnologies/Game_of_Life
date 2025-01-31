/*
Alternatives?

- Using geometry shaders will not fix anything
  geometry shader output support only lines or only triangles
  so it will be still 2 drawcalls
  plus compatibility
  Has to be supported in gl4 / gles3.2, however heavily depends if
  target GPU supports this functionality

- Using glPolygonMode
  Again - two draw calls
  not supported on GLES
  working only with usage of GL_TRIANGLES / solids (with all lines) 
  and not with GL_LINES / wireframes - so it is NOT performing magic
  and not triangulate and fill the polygons
*/ 
