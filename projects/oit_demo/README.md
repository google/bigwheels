# Order-independent transparency demo

An application that implements various order-independent transparency (OIT) techniques.

## Background

Transparency rendering is a technique used to render objects that partially let light go through them.
It is heavily used in games to represent glass, fluids (e.g. smoke) and elements that have no physical equivalent in reality (e.g. magic).
It is also often used in computer-aided design to let creators see through multiple layers of architectural and mechanical designs.

Transparency is typically achieved by alpha blending fragments from back to front from the camera's perspective.
Technically, this means sorting the geometry on the CPU before sending it to the GPU for rendering.
However, in addition to having a non-negligeable CPU cost, that solution cannot produce accurate results in several situations, e.g. intersecting geometry or meshes with concave hulls.

OIT is a family of GPU techniques that removes the need of sorting geometry before rendering.
Some algorithms produce accurate transparency rendering results, while others aim for fast approximations.
Each algorithm has advantages and disadvantages, trading off between quality, memory usage and performance.

## Inner workings

The sample has an opaque pass, a transparency pass and a composite pass.
The opaque pass renders background geometry common to all algorithms.
The transparency pass renders the transparent geometry using the selected OIT algorithm.
The composite pass combines the opaque and transparency pass results to form the final image.

The opaque and composite passes are independent of the OIT algorithm used to render transparent objects.
They are handled by the sample.

It is the algorithms' responsibility to record the transparency pass.
The pass render target is a 16-bit float RGBA texture.
The RGB channel must contain the color with alpha premultiplied.
The A channel must specify the coverage, i.e. how much of the background is obscured by the transparent geometry.

The formula used to compute the final color during the composite pass is:

    final_color = transparent_color + (1 - coverage) * opaque_color

## Algorithms

Each algorithm has its own CPP and shader files.
Algorithms are kept separated on purpose, so that each algorithm can be studied on its own.

The following algorithms are currently supported.

| Algorithm                           | Type              | Additional options      | References
| ---                                 | ---               | ---                     |---
| Unsorted over                       | Approximate       | Split draw calls for back/front faces | [PD1984]
| Weighted sum                        | Approximate       | | [MK2007], [BM2008]
| Weighted average                    | Approximate       | | [BM2008]
| Weighted average with coverage      | Approximate       | | [BM2008], [MB2013]

## References

[MB2013] Morgan McGuire and Louis Bavoil. Weighted Blended Order-Independent Transparency. 2013.

[BM2008] Louis Bavoil and Kevin Myers. Order Independent Transparency with Dual Depth Peeling. 2008.

[MK2007] Houman Meshkin, Sort-Independent Alpha Blending. 2007.

[EC2001] Everitt Cass. Interactive Order-Independent Transparency. 2001.

[PD1984] Thomas Porter and Tom Duff. Compositing digital images. 1984.

