# Revisited source code for _Introduction to 3D Game Programming with DirectX 11_

## Introduction 

Revisiting and updating the source code of [Frank D. Luna's](https://www.d3dcoder.net/default.htm) excellent [_Introduction to 3D Game Programming with DirectX 11_](https://www.d3dcoder.net/d3d11.htm). This is very much in the same spirit as my [DX12 one](https://github.com/yottaawesome/intro-to-dx12-2nd-edition-revisited/tree/main), but this repo also aims for the complete removal of the deprecated FX11 framework (abandoned by Microsoft) and the conversion of the FX files into standard Shader Model 5 HLSL files, thus allowing the samples to be compiled on a modern Visual Studio setup with nothing more than vcpkg required. You'll still need the original book to understand what's going on in each sample.

## Is it worth investing in the book and learning DirectX 11 instead of just jumping to DirectX 12?

While it's true Microsoft is no longer actively developing DirectX 11, many games are still written in it, and Microsoft won't be deprecating DX11 anytime soon. Being a higher level API, learning DX11 allows you to learn the core concepts of graphics rendering without worrying about the low level bookkeeping that's forced on you in lower level APIs like DirectX 12 and Vulkan. This is particularly useful if you're a novice to graphics programming (more experienced developers coming from other APIs like Vulkan and Metal can probably just skip straight to DX12).

## Building and running

You'll need Visual Studio 2026 with the C++ desktop development and C++ game development workloads installed. `vcpkg` is used to manage dependencies to avoid packaging them in the repo. Note that I have set the projects to use the MSVC Preview Build tools to catch compiler bugs earlier, but this is not strictly necessary: you can disable the use of the MSVC preview tools in the project properties if you do not wish to install them.

## Status

The effort is ongoing. The following projects are functional.

* [01/XMVECTOR](./src/01/XMVECTOR)
* [02/XMMATRIX](./src/02/XMMATRIX)
* [04/Init Direct3D](<./src/04/Init Direct3D>)
* [06/Box](./src/06/Box)
* [06/Hills](./src/06/Hills)
* [06/Shapes](./src/06/Shapes)
* [06/Skull](./src/06/Skull)
* [06/Waves](./src/06/Waves)
* [07/Lighting](./src/07/Lighting)
* [07/LitSkull](./src/07/LitSkull)
* [08/Crate](./src/08/Crate)
* [08/TexturedHillsAndWaves](./src/08/TexturedHillsAndWaves)
* [09/BlendDemo](./src/09/BlendDemo)
* [10/Mirror](./src/10/Mirror)
* [11/TreeBillboard](./src/11/TreeBillboard)
* [12/Blur](./src/12/Blur)
* [12/VecAdd](./src/12/VecAdd)
* [13/BasicTesselation](./src/13/BasicTesselation)
* [13/BezierPatch](./src/13/BezierPatch)
* [14/Camera](./src/14/Camera)
* [15/InstancingAndCulling](./src/15/InstancingAndCulling)
* [16/Picking](./src/16/Picking)
* [17/CubeMap](./src/17/CubeMap)
* [17/DynamicCubeMap](./src/17/DynamicCubeMap)
* [18/NormalDisplacementMap](./src/18/NormalDisplacementMap)
* [19/Terrain Rendering](./src/19/Terrain)

## Running log of changes

* Update MSVC toolset to v145 and enable `c++latest`.
* Removed the deprecated library dependencies dxerr, effects11, and d3dx11.
* Converted the code to inline C++ modules, improving the code locality and removing the need for headers and their baggage.
* Added a lightweight `ComPtr` wrapper for COM types. I did not use Microsoft's `WRL::ComPtr` class as I'm not a fan of its constructor semantics. My version also includes a `Uuid()` member, removing the need for the ugly `IID_PPV_ARGS` macro.
* `InitMainWindow()` and `InitDirect3D()` now throw exceptions on errors, which also simplifies the code be removing the `bool` checks.
* The deprecated FX11 framework has been removed. All FX files are being migrated to standard HLSL files with Shader Model 5.0. To facilitate this, they've also been split up into individual files, so that they mesh better with Visual Studio's compilation properties.
* Replaced the `Colors` namespace with `DirectX::Colors`.
* Use of the `ZeroMemory` macro was replaced with simpler empty initialisers.
* Functions have been updated to use trailing return type syntax.
* Variable initialization has been updated to use right-hand braced and aggregate initialisation (where applicable).
* The demo classes now call `Init()` in their constructors, removing the original two-phase initialisation process and its mixed return value/exception checking.
* Replaced some macros with `constexpr` functions and conditionals.
* Various preprocessor numeric definitions have been converted into `constexpr` variables.
* Removed some redundant null pointer checks, such as when `std::make_unique()` is used (this throws on error).
* Incorrect padding and alignment issues in constant buffers that were encountered in some samples (e.g. Lighting) have been fixed.
* Constructor value setting has been moved to inline initialisers, where possible.
* FX-specific `SampleState` definitions in .fx files have been replaced with proper sampler definitions, set through the C++ side.
* `DirectXTK` has been added as vcpkg dependency, specifically for DDS texture loading, replacing the old `D3DX11CreateShaderResourceViewFromFile()` calls with `DirectX::CreateDDSTextureFromFile()`.
* The chapter 13 samples included shaders that were not actually used, and have been removed.

## License and copyright

Most of the code is copyrighted by Frank D. Luna, and based on my experience with the DX12 one, some of the code is repurposed from existing Microsoft samples, and therefore copyrighted by Microsoft. I'll be preserving the relevant copyright notices for the source code, however, some of the source code has no notice, or the notices (particularly dates) don't match up between header and source file pairs, so it's not exact.

It's unclear what license applies to Luna's code, as no notices are posted anywhere including on the official site, but given there are various longstanding copies/remixes of Luna's code on Github, I assume the author is OK so long as they retain the copyright notices. The Microsoft portions are licensed under MIT. Code files I've exclusively authored are copyrighted by me and licensed under the MIT license.
