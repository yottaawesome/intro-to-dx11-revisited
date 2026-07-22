# Introduction to 3D Game Programming with DirectX11 Revisited

## Introduction 

Revisited source code of Frank D. Luna's "Introduction to 3D Game Programming with DirectX 11".

## Running log of changes

* Removed the deprecated library dependencies dxerr, effects11, and d3dx11.
* Converted the code to inline C++ modules, improving the code locality and removing the need for headers and their baggage.
* Added a lightweight ComPtr wrapper for COM types. I did not use Microsoft's WRL::ComPtr class as I'm not a fan of constructor semantics.
* `InitMainWindow()` and `InitDirect3D()` now throw exceptions on errors, which also simplifies the code be removing the `bool` checks.
* ShaderFX compilation has been replaced with `D3DReadFileToBlob()`, `CreateVertexShader()`, and `CreatePixelShader()`.
* Replaced the `Colors` namespace with `DirectX::Colors`.
* Functions have been updated to use trailing return type syntax.
* Variable initialization has been updated to use right-hand braced and agggregate initilization (where applicable).
* The demo classes now call Init() in their constructors, removing the original two-phase initialization process.
* Replaced some macros with `constexpr` functions and conditionals.
