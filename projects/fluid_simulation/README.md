# Fluid simulator

The contents in this folder have been adapted and translated
from the original WebGL implementation in https://github.com/PavelDoGreat/WebGL-Fluid-Simulation

# Simulation parameters

This simulation has several parameters that can be altered with command-line arguments. The default
values are mostly taken from the original WebGL implementation and some local experimentation.

## --curl <0.0~100.0>
Curl represents the rotational component of the fluid. It
determines the spin (vorticity) of the fluid at each point
of the simulation. Higher values indicate stronger vortices
or swirling motions in the fluid.

## --density-dissipation <0.0~10.0>
This controls the decay of the density field. It determines
how quickly the density in the fluid diminshes over time.
Higher values result in faster dissipation and smoother
density fields.

## --dye-resolution <1~2048>
This determines the level of detail in which the dye is
represented. This changes the clarity of the dye patterns in
the simulation. Higher values provide finer details and
sharper patterns.

## --pressure <0.0~1.0>
Indicates the force exerted by the fluid on its surrounding
boundaries. Higher values cause a greater force exerted on
the boundaries. This can lead to denser regions in the
fluid.

## --pressure-iterations <1~100>
This is the number of iterations performed when solving the
pressure field. Higher values produce a more accurate and
detailed pressure computation.

## --velocity-dissipation <0.0~1.0>
This simulates the loss of energy within the fluid system.
Higher values result in faster velocity reduction.

## --enable-bloom <true|false>
Enables bloom effects.

## --bloom-intensity <0.0~1.0>
Strength of the bloom effect applied to the image. It
determines how to enhance the bright areas and how
pronounced the bloom effect is. Higher values result in a
more pronounced effect that will make bright areas of the
image appear brighter and more radiant. Lower values produce
a more subdued glow.

## --bloom-iterations <1~20>
Number of iterations to use in the post-processing bloom
pass. Each iteration blurs a downsampled version of the
image with the original one. The number of iterations
determines how intense the bloom effect is.

## --bloom-resolution <1~2048>
Sets the size at which the bloom effect is applied. Higher
values provide a more precise bloom result at the expense of
computation complexity.

## --bloom-soft-knee <0.0~1.0>
This controls the transition between bloomed and non-bloomed
regions of the image. It determines the smoothness of the
blending between regions. Higher values result in smoother
transitions.

## --bloom-threshold <0.0~1.0>
Minimum brightness for a pixel to be considered as a
candidate for bloom. Pixels with intensities below this
threshold are not included in the bloom effect. Higher
values limit bloom to the brighter areas of the image.

## --enable-marble <true|false>
When set, this instantiates a marble that bounces around the
simulation field. The marble bounces above the fluid, but it
splashes down with certain frequency (controlled by
--marble-drop-frequency). This option is not available in
the original WebGL implementation.

## --color-update-frequency <0.0~1.0>
This takes effect only if the bouncing marble is enabled.
This controls how often to change the bouncing marble color.
This is the color used to produce the splash every time the
marble drops.

## --marble-drop-frequency <0.0~1.0>
The probability that the marble will splash on the fluid as
it bounces around the field.

## --num-splats <0~20>
This is the number of splashes of color to use at the start
of the simulation. This is also used when ## --splat-frequency
is given. A value of 0 means a random number of splats.

## --splat-force <3000.0~10000.0>
This represents the magnitude of the impact applied when an
external force (e.g. marble drops) on the fluid.

## --splat-frequency <0.0~1.0>
How frequent should new splats be generated at random.

## --splat-radius <0.0~1.0>
This represents the extent of the influence region around a
specific point where the splat force is applied.

## --enable-sunrays <true|false>
This enables the effect of rays of light shining through the
fluid.

## --sunrays-resolution <1~500>
Indicates the level of detail for the light rays. Higher
values produce a finer level of detail for the light.

## --sunrays-weight <0.0~5.0>
Indicates the intensity of the light scattering effect.
Higher values result in more prominent sun rays, making them
appear brighter.

## --sim-resolution <1~1000>
This determines the grid size of the compute textures used
during simulation. Higher values produce finer grids which
produce a more accurate representation.