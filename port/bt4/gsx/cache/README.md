# ARMSX2's Vulkan shaders, compiled in advance

`vulkan_shaders_<stamp>.idx` / `.bin` are ARMSX2's own SPIR-V cache (`pcsx2/GS/Renderers/Vulkan/VKShaderCache.cpp`,
`GSCacheFile`'s blob store): every shader the game asked for in sessions on a Mali (RG Cube) and an Adreno
(Retroid Pocket 5), both devices' in one file (merge_cache.py). `build_bt4.py --armsx2` puts them in the APK
(`assets/bt4gsx/cache`) and `Bt4Activity` copies them to `bt4/gsx/ARMSX2/cache`, where ARMSX2 looks for its cache.
Without them the first start compiles them on the CPU first (about 25 s of black screen on the RG Cube).

The file's stamp (in its name) is ARMSX2's shader cache version, the shaderc it was built with and the compile
options: the same on every device. Each entry's key is a hash of the shader's GLSL text, and that text does depend on
the GPU (ARMSX2 writes the features it found into it): of the RG Cube's 275 shaders and the Retroid Pocket 5's 235,
6 are the same. So the file spares the compiling on GPUs like those two (Mali-G57 and the like; Adreno 6xx); another
GPU compiles what it does not find and adds it to its own copy, as before.

Made again when ARMSX2's revision, its shaderc or `patches/armsx2-bt4.patch`'s shaders change (a new stamp in the
name): on each device, delete `bt4/gsx/ARMSX2/cache`, play the menus and a fight with the renderer ARMSX2 on Vulkan
and take its `vulkan_shaders_*` files; then join them (needs the Python package xxhash):

    python3 port/bt4/gsx/merge_cache.py port/bt4/gsx/cache/vulkan_shaders_<stamp> cube/vulkan_shaders_<stamp> rp5/vulkan_shaders_<stamp>

and delete the old pair here.
