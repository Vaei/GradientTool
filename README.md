# GradientTool

> [!IMPORTANT]
> **Gradient assets that keep a texture baked**
> <br>Author colour stops, get a `Texture2D` that is always current
> <br>And its **FREE!**

> [!NOTE]
> <br>A gradient is an asset you can edit, not a PNG you re-export
> <br>Every edit rewrites the texture, so materials never sample a stale ramp
> <br>Four blend spaces, so saturated hues stop going muddy in the middle

> [!TIP]
> Editor-only tooling; the baked texture is an ordinary `Texture2D` at runtime
> <br>Tested with UE5.8+

> [!CAUTION]
> GradientTool is currently in beta

## How to Use

Right click in the Content Browser and pick **Texture ▸ Gradient**.

You get two assets side by side:

| Asset | What it is |
|---|---|
| `GR_MyGradient` | The gradient. Double click to edit the stops. |
| `T_MyGradient` | The baked `Texture2D`. **This is what you assign to materials.** |

Drop `T_MyGradient` into a Texture Sample or a `Texture2D` material parameter like any other texture. Edit
the gradient and the texture updates underneath it, including in any open material editor.

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
throughout. The toolbar has **Reverse**, **Distribute** (space stops evenly), and **Rebuild**.

## Features

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

Texture width is configurable (default 256). The baked texture is 1 pixel tall, uncompressed, non-sRGB,
no mips, clamped on both axes, in the `ColorLookupTable` group. Those settings are owned by the bake and
are not editable on the texture.

### Blueprint

`UGradientAsset::Evaluate(float Time)` returns the colour at a position without sampling the texture, and
works in a cooked build. The stop array itself is `BlueprintReadWrite`.

## Companion Texture

The gradient owns its texture rather than being one. `UTexture2D` is declared `MinimalAPI` with unexported
virtuals, so no plugin can subclass it without modifying the engine, and this plugin does not.

The texture is created beside the gradient and written to disk immediately, so the gradient's reference to
it never dangles.

- **Rename or move** a gradient and its texture follows.
- **Delete** a gradient and the texture stays, because materials may still be using it. Delete it yourself
  if you want it gone.
- **Delete the texture** and the gradient rebuilds a new one the next time it loads.

## Changelog

### 1.0.0
* Initial Release
# GradientTool
