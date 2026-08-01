# GradientTool

> [!IMPORTANT]
> **Gradient atlases that keep a texture baked**
> <br>Author colour stops, get a `Texture2D` that is always current
> <br>And its **FREE!**

> [!NOTE]
> <br>A gradient is an asset you can edit, not a PNG you re-export
> <br>Every edit rewrites the texture, so materials never sample a stale ramp
> <br>Pack many gradients into one asset and address them by name in the material graph
> <br>Four blend spaces, so saturated hues stop going muddy in the middle

> [!TIP]
> Editor-only tooling; the baked texture is an ordinary `Texture2D` at runtime
> <br>Tested with UE5.8+

> [!CAUTION]
> GradientTool is currently in beta

<img width="622" height="626" alt="UnrealEditor-Win64-DebugGame_2026-08-01_16-19-24" src="https://github.com/user-attachments/assets/ca286cd7-8ebf-4f8e-9fd4-9b24c980164d" />

## How to Use

Right click in the Content Browser and pick **Texture ▸ Gradient**.

You get two assets side by side:

| Asset | What it is |
|---|---|
| `GR_MyGradient` | The gradient atlas. Double click to edit it. |
| `T_MyGradient` | The baked `Texture2D`. One row per gradient. |

Point a **Sample Gradient** node at `GR_MyGradient`, pick a gradient by name, and feed it a `Time`.
Or drop `T_MyGradient` straight into a Texture Sample if you only have one gradient. Edit the
gradient and everything updates underneath, including any open material editor.

### Editing

| Input | Does |
|---|---|
| Left click the bar | Add a stop at that position, in the colour already there |
| Left drag a stop | Move it |
| Double click a stop | Open the colour picker |
| Middle click a stop | Delete it |
| Right click a stop | Colour, interpolation, delete |
| <kbd>Delete</kbd> | Delete the selected stop |

A gradient always keeps at least two stops. Every edit is transacted, so <kbd>Ctrl</kbd>+<kbd>Z</kbd> works
throughout. The toolbar has **Add**, **Reverse**, **Distribute** (space stops evenly), and **Rebuild**;
Reverse and Distribute act on the selected gradient.

## Features

### Atlases

An asset holds any number of gradients, each baking to one row of the texture. Add and remove them
from the toolbar, rename them in the box beside each bar, and reorder them from the `Gradients` array
in the Details panel. Names are kept unique because material nodes address a row by name.

### Material graph

| Node | Gives you |
|---|---|
| **Sample Gradient** | `RGB` / `R` / `G` / `B` / `A` / `RGBA` read out of the named gradient |
| **Gradient Coordinate** | The `UV` (and just the `V`) that reads the named gradient, for a texture sample you build yourself |

Both take the atlas asset and a gradient name, and a `Time` input for the position along the ramp.
`Time` 0 and 1 land exactly on the first and last texel centres, so a sample matches `Evaluate`.

### Driving it from a material instance

Both nodes have two override inputs. Leave them unconnected and the node uses the asset and name set
on it; connect them and instances take over.

| Input | Feed it | Instance gets |
|---|---|---|
| `Atlas` | a **Texture Object Parameter** defaulted to `T_MyGradient` | an atlas picker |
| `Row` | a **Scalar Parameter** | a gradient picker, see below |

To pick by gradient *name* rather than row number, set that Scalar Parameter's **Control Type** to
`Enumeration` and its **Enumeration** to your `GR_MyGradient` asset. A gradient asset is a material
enumeration of its own gradient names, so the instance shows a dropdown of them.

Both have to be stock parameter nodes. The material instance editor only displays parameters whose
expression is one of the engine's own parameter classes, so a plugin node cannot be one itself.

The row's V is derived from the live texture height, so **adding** gradients does not disturb
materials that are already compiled. Renaming or reordering does change which row a name resolves to;
loaded materials are recompiled for you, but a material that was closed at the time picks the change
up the next time it is translated.

### Interpolation

Set per stop, describing how the gradient reaches that stop from the one before it.

| Mode | Behaviour |
|---|---|
| `Linear` | Straight interpolation |
| `Constant` | Hold the previous colour, then jump |
| `Ease` | Smoothstep in and out |
| `Cubic` | Catmull-Rom through the neighbouring stops |

### Blend Space

Set per gradient. This is the difference between a gradient that looks right and one that goes grey
through the middle.

| Space | Blue → yellow midpoint |
|---|---|
| `Linear` | Physically correct, and a flat mid grey |
| `sRGB` | Matches how most DCC tools draw gradients |
| `HSV` | Shortest-arc hue, saturation preserved across the sweep |
| `OkLab` | Perceptually uniform, even lightness ramp |

Alpha always blends linearly regardless of the space.

### Format

| Format | Source | Compression |
|---|---|---|
| `HDR` | `RGBA16F` | `TC_HDR`, holds values outside 0-1 |
| `LDR` | `BGRA8` | `TC_VectorDisplacementmap`, a quarter the memory, clamped |

Texture width is configurable (default 256) and applies to every gradient. The baked texture is one
pixel tall per gradient, uncompressed, non-sRGB, no mips, clamped on both axes, in the
`ColorLookupTable` group. Those settings are owned by the bake and are not editable on the texture.

### Blueprint

`UGradientAsset::Evaluate(FName GradientName, float Time)` returns the colour at a position without
sampling the texture, and works in a cooked build. `EvaluateByIndex`, `IndexOfGradient`,
`GetGradientNames` and `GetGradientV` are exposed alongside it, and the gradient array itself is
`BlueprintReadWrite`.

## Companion Texture

The gradient owns its texture rather than being one. `UTexture2D` is declared `MinimalAPI` with unexported
virtuals, so no plugin can subclass it without modifying the engine, and this plugin does not.

The texture is created beside the gradient and written to disk immediately, so the gradient's reference to
it never dangles.

- **Rename or move** a gradient and its texture follows.
- **Duplicate** a gradient and the copy bakes its own texture, named after the copy. The original's
  texture is left alone.
- **Delete** a gradient and the texture stays, because materials may still be using it. Delete it yourself
  if you want it gone.
- **Delete the texture** and the gradient rebuilds a new one the next time it loads.

## Changelog

### 1.1.0
* Atlases: an asset now holds any number of named gradients, one per row of the texture
* Added the Sample Gradient and Gradient Coordinate material nodes, with `Atlas` and `Row` inputs so
  material instances can override the atlas and the gradient
* A gradient asset is a material enumeration of its gradient names, so an Enumeration scalar
  parameter offers those names in a material instance
* `Evaluate` now takes a gradient name; assets saved by 1.0.0 migrate into a single gradient on load

### 1.0.0
* Initial Release
