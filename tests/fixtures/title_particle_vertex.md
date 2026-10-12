# Captured original particle GPU vertex fixtures

28 vertices from original Xemu Vulkan draws 5879, 5883, 6156, 6188, 6227,
6463 and 6533. The input file contains actual captured corner data, generic
attributes and 192 constant registers, with original vertex datum `E20E0099`
and explicitly selected classification 7. Input SHA-256:
`c7f96c0963a564cdf0c741f3564a86e5b9039eed882165d56facbbecaa799caa`.

The expected file contains original post-VS outputs, ordered as oPos, oD0,
oD1, oB0, oB1 and oT0 through oT3. Expected SHA-256:
`22b50ef785b5561ab6c00475eba35ebaffdb3fb38334d312e2be778b6c5f0abb`.
Reference capture SHA-256:
`077524887d852e46af2e3cb8eb85c52dd777b8bda3e86039c29e904a2450b431`.
Raw extraction: `analysis/frontend/xemu-capture/title-billboard-capture.json`.
No expected value is obtained from the native implementation.

The GPU probe is generated from the actual disc's 51-instruction program by
`tools/generate_particle_vertex_probe.py`; it embeds no fixture values or
expected vertices. CTest compares all 1,008 output float words without a
tolerance. Original 1/16-pixel screen snapping and color saturation are applied
before comparison because the capture reports those post-VS values.

This verifies bounded vertex arithmetic on the tested GPU. Actual native
graphics-stage execution, interpolation, fragment shaders, draw sorting,
particle simulation and whole-frame pixels remain separate requirements.
