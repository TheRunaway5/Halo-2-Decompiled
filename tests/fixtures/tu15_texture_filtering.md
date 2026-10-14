# TU1.5 texture filtering submission

`tools/oracle_texture_filtering_tu15.py` executes unchanged complete `409EE0`
and its real bias-conversion helper `406FE0` from the owned OG Xbox TU1.5 XBE,
SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Every executed instruction is checked against the original bytes. Declared
inputs provide a device bank, actual-layout texture-state banks and sufficient
pushbuffer capacity. No instruction, allocator or callback is replaced.

The 540 cases cover all four stages, min/mag filters 1..3, mip filters 0..2 and
anisotropy ordinals 0..4. The fixture retains all five original submitted command
words per case. It executes 132 of 151 whole instructions; allocation, special
filters and nonzero/clamped bias branches are outside these cases.

`409F62..409FCB` treats anisotropy as an ordinal when either min or mag requests
anisotropic filtering. Zero replaces both with point filtering; ordinals 1..4
replace both with linear filtering and put ordinal-minus-one in the NV2A
anisotropy field. The hardware field selects 1/2/4/8 samples. The original
Wraith GPU capture independently shows 8 for its authored ordinal 4.

The old native interpretation failed 228 cases. The corrected Vulkan sampler
passes all 540 and rejects disabled or insufficient native anisotropy support.
Raw authored state decoding remains unchanged. This verifies state submission,
not host-GPU filtering precision or complete framebuffer parity.
