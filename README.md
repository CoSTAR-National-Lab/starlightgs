# StarLightGS

![](Resources/Demo.gif)

Co**STAR** Re**Light**ing **G**aussian **S**plats

Gaussian splat renderer for Unreal Engine 5.7+ with a focus on relighting and native shadow rendering (without proxy meshes).

## Description
This plugin features:
- High performance rendering handling 10+ million splats.
- Relighting support.
- Interoperability with meshes and splats.
- Native shadow-casting for Gaussian Splats onto meshes and other splats.
- Splats receive shadows and global illumination from meshes.

This plugin supports rendering of splats with and without relighting. To use relighting, your PLY files must have per-splat normals. 
## Getting Started
### Requirements
- Unreal Engine 5.7+
### Installing
- Place StarLightGS into the Plugins folder in your project.
- Regenerate Visual Studio project files and compile.
### Basic Usage
- From the content browser, click 'Import'.
- Navigate to and select your PLY file.
- Click 'Import' in the import dialogue to import the PLY as a Gaussian Splat asset.
- Drag and drop the Gaussian Splat asset onto the viewport to place it into the level.
- If your PLY supports relighting, relighting will be function out-of-the-box.

See the [Usage Guide](Docs/StarLightGS_Usage_Guide.md) for how to use all features of the plugin.

## License
This project is licensed under the MIT License - see the LICENSE file for details.
