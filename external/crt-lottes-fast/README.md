CRT-Lottes Fast (CRTS), Timothy Lottes; GLSL/RetroArch adaptation by hunterk.
Public domain / Unlicense; the complete license is in the vendored source.

Pinned upstream: https://github.com/libretro/glsl-shaders/blob/a8136d8b8b5c6375296f833e7e81efa15ed76f11/crt/shaders/crt-lottes-fast.glsl

`../../shaders/crt_oled.frag` ports Phase-Distorter's flat OLED adaptation to
the Vulkan presenter. It retains Gaussian/cosine reconstruction, linear-light
phosphor mask and tone compensation. Geometry stays flat, without vignette or
rounded corners. Direct scenes sample the rendered subpixel positions before
applying source-height beams. Mask contrast and beam
strength taper at low display scales to avoid severe aliasing in small windows.
No temporal blending, persistence, frame history, or black-frame insertion.
The shader is compiled to SPIR-V at build time alongside the other game shaders.
