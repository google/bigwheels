# Text drawing

Displays on-screen text.

This project showcases the `grfx::TextDraw` class. The font is loaded at setup time, each glyph is converted to bitmap, and the text is drawn to screen after generating the letters' vertex data from strings. Each letter's texture coordinates get associated with the corresponding glyph on the bitmap.

Both static and dynamic text is rendered. Dynamic text changes every frame.

## Shaders

Shader          | Purpose for this project
--------------- | ----------------------------
`TextDraw.hlsl` | Draw colored text.
