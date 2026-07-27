# Revisited source code for _Introduction to 3D Game Programming with DirectX 11_

## Introduction 

Revisited source code of Frank D. Luna's _Introduction to 3D Game Programming with DirectX 11_. This is very much in the same spirit as my [DX12 one](https://github.com/yottaawesome/intro-to-dx12-2nd-edition-revisited/tree/main), but somewhat more extensive due to the original source code relying on deprecated and unavailable (or not easily available) technologies that prevent compilation "out of the box". As with my DX12 one, the primary aim is again to make the source code easier understand by updating it with modern C++ approaches.

## Running log of changes

* Update MSVC toolset to v145 and enable `c++latest`.
* Removed the deprecated library dependencies dxerr, effects11, and d3dx11.
* Converted the code to inline C++ modules, improving the code locality and removing the need for headers and their baggage.
* Added a lightweight `ComPtr` wrapper for COM types. I did not use Microsoft's `WRL::ComPtr` class as I'm not a fan of its constructor semantics. My version also includes a `Uuid()` member, removing the need for the ugly `IID_PPV_ARGS` macro.
* `InitMainWindow()` and `InitDirect3D()` now throw exceptions on errors, which also simplifies the code be removing the `bool` checks.
* ShaderFX compilation has been replaced with `D3DReadFileToBlob()`, `CreateVertexShader()`, and `CreatePixelShader()`.
* Replaced the `Colors` namespace with `DirectX::Colors`.
* Functions have been updated to use trailing return type syntax.
* Variable initialization has been updated to use right-hand braced and aggregate initialisation (where applicable).
* The demo classes now call `Init()` in their constructors, removing the original two-phase initialisation process and its mixed return value/exception checking.
* Replaced some macros with `constexpr` functions and conditionals.
* Various preprocessor numeric definitions have been converted into `constexpr` variables.
* Removed some redundant null pointer checks, such as when `std::make_unique()` is used (this throws on error).

## License and copyright

Most of the code is copyrighted by Frank D. Luna, and based on my experience with the DX12 one, some of the code is repurposed from existing Microsoft samples, and therefore copyrighted by Microsoft. I'll be preserving the relevant copyright notices for the source code, however, some of the source code has no notice, or the notices (particularly dates) don't match up between header and source file pairs, so it's not exact.

It's unclear what license applies to Luna's code, as no notices are posted anywhere including on the official site, but given there are various longstanding copies/remixes of Luna's code on Github, I assume the author is OK so long as they retain the copyright notices. The Microsoft portions are licensed under MIT. Code files I've exclusively authored are copyrighted by me and licensed under the MIT license.
