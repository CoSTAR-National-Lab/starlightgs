# StarLightGS Usage Guide

---

## Table of Contents
- [Overview](#overview)
- [1. Installation](#1-installation)
- [2. PLY File Requirements](#2-ply-file-requirements)
- [3. Importing PLY Files](#3-importing-ply-files)
- [4. Adding PLY Files to Level](#4-adding-ply-files-to-level)
- [5. Culling Volumes](#5-how-to-use-culling-volumes)
- [6. Using Relighting](#6-using-relighting)
- [7. Using Splat Shadows](#7-using-splat-shadows)
- [8. Console Variables & Stats](#8-console-variables--stats)
- [Version & Changelog](#version--changelog)

---

## Overview
- **What it is:** A Gaussian splat renderer for Unreal Engine 5.7+.
- **Key capabilities:** Relighting, Native Splat Shadow Casting, Mixed-Media Workflows.

---
## 1. Installation
This plugin requires Unreal Engine version 5.7 or later.
To install:

1. Place plugin folder into `Plugins/` folder within your project.
2. Right-click on `.uproject` and `Generate Visual Studio project files`.
3. Open the generated `.sln` (`.slnx` from UE 5.8 onwards).
4. Build and run from visual studio.
## 2. PLY File Requirements
All 3DGS models in PLY format should work with this plugin. However, certain properties are required for a PLY to support relighting, which not all PLY files will provide. 

Where a table shows `[A|B]`, that means that the importer will accept a value of either `A` or `B`. You should not mix conventions - e.g., if you use `f_dc_0`, then you should also use `f_dc_1` and `f_dc_2`. Similarly, if `red` is a `uint8`, then `green` and `blue` must also be `uint8`.

### Required Properties
These are mandatory fields that any PLY must have to be able to be imported as a Gaussian Splat asset. Note that these properties do not facilitate relighting support.

| Property | Type | Description | Encoding |
| -------- | ---- | ----------- | -------- |
| `x` | `float` | Position (X) | None |
| `y` | `float` | Position (Y) | None |
| `z` | `float` | Position (Z) | None |
| `rot_0` | `float` | Rotation (W) | None |
| `rot_1` | `float` | Rotation (X) | None |
| `rot_2` | `float` | Rotation (Y) | None |
| `rot_3` | `float` | Rotation (Z) | None |
| `scale_0` | `float` | Scale (X) | Logarithmic |
| `scale_1` | `float` | Scale (Y) | Logarithmic |
| `scale_2` | `float` | Scale (Z) | Logarithmic |
| `opacity` | `[float\|uint8]` | Opacity | Sigmoid \| None |
| `[f_dc_0\|red]` | `[float\|uint8]` | Base Colour | Zero-Order Spherical Harmonic Coefficient \| None |
| `[f_dc_1\|green]` | `[float\|uint8]` | Base Colour | Zero-Order Spherical Harmonic Coefficient \| None |
| `[f_dc_2\|blue]` | `[float\|uint8]` | Base Colour | Zero-Order Spherical Harmonic Coefficient \| None |

### Required Relighting Properties
The following properties are required for a PLY to be able to be relit.

| Property | Type | Description | Encoding | Notes |
| -------- | ---- | ----------- | -------- | ----- |
| `nx` | `float` | Surface Normal (X Component) | None | Do not use if `octa_n[x\|y]` are specified
| `ny` | `float` | Surface Normal (Y Component) | None | Do not use if `octa_n[x\|y]` are specified
| `nz` | `float` | Surface Normal (Z Component) | None | Do not use if `octa_n[x\|y]` are specified
| `octa_nx` | `uint8` | Surface Normal (X Component) | None | Do not use if `n[x\|y\|z]` are specified
| `octa_ny` | `uint8` | Surface Normal (Y Component) | None | Do not use if `n[x\|y\|z]` are specified

### Optional Relighting Properties
Inclusion of the following optional relighting properties will result in optimal relighting. It is highly recommended to include these material properties - if you train your PLY file with the CoSTAR GS-IR implementation, then these fields should be satisfied.

| Property | Type | Description | Encoding | Notes |
| -------- | ---- | ----------- | -------- | ----- |
| `[metallic\|metallicFactor]` | `[float\|uint8]` | Material Metallic | None | Must be same type as roughness - otherwise will be disabled
| `[roughness\|roughnessFactor]` | `[float\|uint8]` | Material Roughness | None | Must be same type as metallic - otherwise will be disabled

### Property Encodings
This table explains how each encoding is decoded in the importer, to help understanding the expected input. The importer can be configured to skip decoding on some properties, see [Import Options](#import-options).

For an encoded value `x`, the decoding column shows how `x` will be imported for each encoding.
| Encoding | Decoding |
| -------- | -------- |
| None | `x` |
| Logarithmic | `exp(x)` |
| Sigmoid | `1 / (1 + exp(x))` |
| Zero-Order Spherical Harmonic Coefficient | `0.5 + x / sqrt(4 * pi)` (where pi is &#960;) |
---

## 3. Importing PLY Files

1. Open the Content Browser
2. Click 'Import' / right-click → Import
3. Navigate to and select your PLY file
4. Review the import dialog options - see below for more details
5. Select 'Import'

### Import Options

| Option | Default | Description |
| ------ | ------- | ----------- |
| `Import Normals` | `True` (if available) | Whether to import normals, if they are found in the PLY. If normals are not found, this option is disabled. |
| `Import High-Order SH` | `False` | Currently unsupported. |
| `Import Metallic/Roughness` | `True` (if available) | Whether to import metallic & roughness material properties, if they are found in the PLY. If they are not found, this option is disabled. |
| `Populate Metallic/Roughness With Default` | False | If the PLY does not supply metallic/roughness, checking this will populate with default values. Useful in the case where a PLY has normals but no material properties. Not available if PLY contains Metallic/Roughness already. |
| `Metallic Default Value` | `0.0` | The value to use when populating metallic with a default value. Valid range is `[0-1]` |
| `Roughness Default Value` | `1.0` | The value to use when populating roughness with a default value. Valid range is `[0-1]` |
| `Scale Factor` | `100.0` | Scaling to apply to the splat model on import. This is to account for cases where other software uses metres, while Unreal using centimeters. Valid range is real numbers greater than 0. |
| `X Swizzle` | `+Z` | Change of basis on import. Used to convert between axis conventions. By default converts from X right, Y up, Z forward to X forward, Y right, Z up. Swizzle must form an orthonormal basis. |
| `Y Swizzle` | `+X` | Change of basis on import. Used to convert between axis conventions. By default converts from X right, Y up, Z forward to X forward, Y right, Z up. Swizzle must form an orthonormal basis. |
| `Z Swizzle` | `-Y` | Change of basis on import. Used to convert between axis conventions. By default converts from X right, Y up, Z forward to X forward, Y right, Z up. Swizzle must form an orthonormal basis. |
| `Base Colour is RGB` | `False` | Check if base colour is encoded as RGB values (as opposed to zero-order spherical harmonics coefficients). Only applies when PLY uses `f_dc_[0\|1\|2]` to specify base colour. |
| `Apply Gamma Correction` | `False` | Check if base colour values are stored with sRGB encoding. |
| `Import Opacity as Alpha` | `False` | If checked, skips sigmoid decoding on opacity values. Check if PLY uses opacity property to encode alpha values directly (in range `[0-1]`). |

### Reimporting
To make changes to the import settings, you can right-click on the asset in the content browser and select `Reimport`. This will bring back the import dialogue with the most recently used import settings for that asset. These settings can be modified to re-import the data for the asset.

### Troubleshooting
***Imported base colour is wrong.***
- Try reimporting your splat with different combinations of `Base Colour is RGB` and `Apply Gamma Correction`.

***My splat has an unusually high number of floating splats.***
- Messy splats, with lots of floating splats occupying space that should be otherwise empty, may require `Import Opacity as Alpha` to be checked on import. Try reimporting your splat.
---

## 4. Adding PLY Files to Level
Once you have imported a PLY file as a `Gaussian Splat Data` asset, placing a Gaussian Splat into the scene is straightforward.

### Placement
- Navigate to the `Gaussian Splat Data` asset that you wish to place in the content browser.
- Drag and drop the asset onto the viewport.
- This will create a new Gaussian Splat actor in the level.

Alternatively, you can add a splat as a component to an existing actor or in a blueprint:
- Select `Add` in the components hierarchy view.
- Search for `Gaussian Splat Component`.
- Assign your desired Gaussian Splat asset to the `Gaussian Splat Asset` property in the `Gaussian Splats` category.

If a Gaussian Splat asset supports relighting, then that Splat will automatically be relit by the lighting in the level. No further configuration is necessary to enable or control relighting - just adjust the scene lights as you would normally.

If your Gaussian Splat asset does not support relighting, it will still render in the scene but will not be affected by the lights in the scene.
### Component Properties Reference

| Property | Category | Default | Description |
| -------- | -------- | ------- | ----------- |
| `Gaussian Splat Asset` | `Gaussian Splats` | `None` | The Gaussian Splat asset to be rendered by this component. If the asset supports relighting, then this component will be relit.
| `Splat Scale Multiplier` | `Gaussian Splats` | `1.0` | Modulates the size of the individual splats within the asset multiplicatively. E.g, a value of 2.0 will make all individual splats twice as large (but will not make the model twice the scale).
| `Tags` | `Clipping` | `StarLightGS.CullingVolumes.Default` | [Gameplay tags](https://dev.epicgames.com/documentation/unreal-engine/using-gameplay-tags-in-unreal-engine) are used to describe which clipping volumes apply to which splats. StarLightGS defines a default tag that is used as standard by splats and culling volumes.
| `Exact Match Only` | `Clipping` | `False` | If true, then a culling volume must contain a tag that is an exact match to apply. For example, if this splat had a tag of `Outdoor.Forest`, while ordinarily a culling volume with the tag `Outdoor` would apply, when `Exact Match Only` is true, it would no longer apply.
| `Shadow Material` | `Advanced` | `M_SplatShadowMaterial` | The shadow material is used to shape the shadows cast by splats. It is recommended to only modify this if you know what you're doing.

---

## 5. How to Use Culling Volumes
Splat Culling Volumes can be used to tidy up splat assets, or to segment splat assets into individual parts.

### Modes
Multiple culling modes are available to control the behaviour of culling volumes, which are described the values of `EGaussianSplatClippingVolumeMode`.

| Mode | Description |
| ---- | ----------- |
| `Clip Entirely Inside` | Clips splats which are entirely contained by the culling volume. |
| `Clip Any Inside` | Clips splats which at least partially intersect with the culling volume. |
| `Clip Entirely Outside` | Clips splats which are at least partially outwith the culling volume. |
| `Clip Any Outside` | Clips splats which are entirely outwith the culling volume. |

### Setup Steps
1. Open the Place Actors menu.
2. Add a [Gaussian Splat Clipping Volume] actor to the level.
3. Select the clipping mode to control the behaviour of the volume.
4. Position and resize the volume to the desired region. Volumes can be attached to other actors.

### Actor Properties Reference

| Property | Default | Description |
| -------- | ------- | ----------- |
| `Enabled` | `True` | Toggles whether this clipping volume will affect the world. |
| `Mode` | Clip Entirely Inside | Controls the effect that this clipping volume has on the world, whether it culls splats within or outside the volume. |
| `Priority` | `0` | Controls the order in which culling volumes are applied. Volumes with a lower priority value are applied first. |
| `Tags` | `StarLightGS.CullingVolumes.Default` | [Gameplay tags](https://dev.epicgames.com/documentation/unreal-engine/using-gameplay-tags-in-unreal-engine) are used to describe which clipping volumes apply to which splats. StarLightGS defines a default tag that is used as standard by splats and culling volumes. |
| `Exact Match Only` | `False` | If true, then a splat must contain a tag that is an exact match to apply. For example, if this culling volume had a tag of `Outdoor`, while ordinarily this volume would affect a splat with the tag `Outdoor.Forest`, when `Exact Match Only` is true, it would no longer apply. |

### Saving Clipped Splats as a New Asset

A useful workflow is to use clipping volumes to tidy up or segment a splat model in a separate level, and then save the clipped and tidied splat model as a new asset to use in a different level. This can be done as follows:

1. Place your splat into an empty level.
2. Place clipping volumes until you have tidied or segmented your splat to your satisfaction.
3. Select the splat actor in the outliner.
4. Select the `GaussianSplatComponent` from the component hierarchy at the top of the details panel.
5. In the `Gaussian Splats` category, press the `Export Clipped Splat` button.
6. The clipped splat will be saved as a new `Gaussian Splat Data` asset in the `Content/Splats` folder in the content browser.
7. This new splat asset can be placed into other levels (it is a new standalone asset and does not depend on the existence of the original splat asset).

---

## 6. Using Relighting
The relighting system works by rendering Gaussian Splat assets into Unreal Engine's GBuffer. The GBuffer is an intermediate stage in Unreal Engine's rendering pipeline, which holds all of the geometry and material information about everything currently on-screen. The lighting and post-processing pipeline ingests the GBuffer to produce a fully lit and processed image. Leveraging Unreal Engine's own lighting pipeline ensures consistent lit appearance between meshes and splats, and 'just works' with the majority of Unreal's lighting features.

### How To Use
- Relighting is enabled by default.
- Your PLY must have all the required properties to support relighting. See [Required Relighting Properties](#required-relighting-properties).
- Any relighting-compatible splat placed into the scene will be affected by the lights in the scene.

### Features
- Splats will receive light from all built-in light types.
- Splats will receive shadows from meshes (regardless whether the splat itself casts a shadow).
- Splats will receive global illumination from Lumen. For example, emissive materials on meshes will illuminate splats.
- Splats will contribute to screen-space effects, such as screen-space reflections.

### Limitations
- The GBuffer can only store information about _opaque_ geometry and materials. Therefore, any relightable splats will be turned opaque. In some cases, this can result in artefacts around the edges of objects. See the [section on CVars](#control-plugin-features-via-cvars) for how to control this behaviour.
- Splats do not contribute to the lumen scene for casting global illumination and (non-screen-space) reflections onto other objects.
- Splats do not currently support raytracing. They will not work with any raytracing features, including raytraced shadows, MegaLights, or Lumen with hardware raytracing.
- The method of rendering splats into the GBuffer will not work in projects with Substrate enabled.

---

## 7. Using Splat Shadows
StarLightGS supports native splat shadow casting - that is, splats can cast shadows without requiring a proxy mesh (which can often be difficult to acquire, especially at a good quality).

Native splat shadow casting works by sending our splat geometry as a mesh batch to the shadow rendering pipeline. We use the vertex factory system to inject our own shader code to reshape a single quad into a view-aligned quad for each splat, appropriately sized and positioned. Because this process depends on the light's position and direction, it needs to be repeated for each light source. The advantage of this system is the depth of integration with the existing shadow and lighting system - splat shadows instantly work with all meshes and splats.

### How To Use
- Enable Virtual Shadow Maps (VSMs) in your project.
- Set your splat component's `Cast Shadow` found in (`Lighting`) to true.

### Features
- Both relightable and non-relightable splats can cast shadows (however, non-relightable splats cannot receive shadows).
- Splats can cast shadows from any built-in light type.
- Splats can cast shadows onto other splats, including self-shadowing.
- Splats can cast shadows onto meshes.

### Limitations
- Performance will scale with the number of shadow-casting lights in the scene. I.e., consider lighting performance constraints as you would traditionally before MegaLights. Being careful with the attenuation radius on your light sources can help.
- VSMs does not support translucent shadows, therefore, splats must be made into opaque ellipsoids for shadow-casting. This can result in harder-edged shadows than expected.

### Troubleshooting
***I have VSMs enabled and Cast Shadows enabled, but no shadows appear.***
- Check that your light source has both `Cast Shadows` (found in `Light`) and `Cast Dynamic Shadows` (found in `Light > Advanced`) enabled.
- Check your splat component has `Cast Dynamic Shadows` (found in `Lighting > Advanced`) enabled.
- Ensure the `Shadow Material` (found in `Gaussian Splats > Advanced`) on your splat component is set to `M_SplatShadowMaterial` (found in `Plugins Content > StarLightGS > Materials`).

---

## 8. Console Variables & Stats

Some console variables are available to assist with control features and debugging.

### Stats

> `stat Gaussians`

Displays statistics about the Gaussian renderer that can be useful for understanding culling efficiency, and performance characteristics of scene.

### Control Plugin Features via CVars
The following CVars control the behaviour of the plugin and can be used to toggle features, improve performance, and adjust appearance.
| CVar | Type | Default | Description |
| ---- | ---- | ------- | ----------- |
| `r.Gaussians.MaxSplats` | `int` | `5,000,000` | Controls the maximum number of splats that can be rendered on the screen in one frame. This limit can be increased as much as you have VRAM available. This limit does not apply to shadow-casting, an unlimited number of splats can cast shadows in a single frame. |
| `r.Gaussians.Relighting` | `bool` | `True` | Global toggle for all splat relighting features. When disabled, relightable splats will be rendered as default unlit splats. |
| `r.Gaussians.Shadows` | `bool` | `True` | Global toggle for all splat shadow casting. When disabled, no splats will cast shadows regardless of per-actor `Cast Shadow` settings. |
| `r.Gaussians.AlphaCutoff` | `float` | `0.3` | Valid in range `[0,1]`. Used when compositing relit splats into the rest of the scene. Pixels with an alpha below this cutoff will be discarded. Used to reduce artefacts around edges of relit splats. |
| `r.Gaussians.OpacityThreshold` | `float` | `0.0` | Valid in range `[0,1]`. Discards splats with an opacity below this threshold. Used to improve performance and remove near-transparent 'floaters', potentially at the cost of some details. |

### Developer Tools
The following CVars were added as tools to assist in plugin development. Recommended to avoid unless you know what you're doing.
| CVar | Type | Default | Description |
| ---- | ---- | ------- | ----------- |
| `r.Gaussians.Debug.CullEveryFrame` | `bool` | `False` | Forces processing of culling volumes every frame. By default, culling is only executed when a splat or culling volume is modified. Forcing it to run every frame can be used for debugging, but is not recommended due to performance overheads. |
| `r.Gaussians.Debug.DisableViewExtension` | `bool` | `False` | Disables rendering of splats in primary view via view extension to assist debugging splat shadow casting. |

---
*Last updated: `24/08/2026`*