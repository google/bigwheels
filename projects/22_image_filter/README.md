# Image filtering

Displays an image on screen, where the image is filtered by a number of filters.

Both the target image and the filter can be selected through the ImGui interface. The following filters are available:

- [Gaussian blur](https://en.wikipedia.org/wiki/Gaussian_blur)
- [Sharpening](https://en.wikipedia.org/wiki/Unsharp_masking)
- Desaturation (grayscale)
- [Sobel edge detection](https://en.wikipedia.org/wiki/Sobel_operator)

## Shaders

Shader             | Purpose for this project
------------------ | ---------------------------------------
`ImageFilter.hlsl` | Apply the selected filter to the image.
`Texture.hlsl`     | Draw the image to screen.
