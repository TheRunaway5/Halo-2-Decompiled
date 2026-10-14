# Original TU1.5 fixed heavy-vehicle choices

The local `tu15_heavy_vehicle_choices.original.json` fixture executes unchanged
TU1.5 `15CC70`, SHA256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
`tools/oracle_heavy_vehicle_choices_tu15.py` reads the owned cache's Matg -> Mulg
runtime vehicle references and singleton collection identities independently of
native content preparation. Its36 cases cover default, Scorpion, Wraith and
None on Containment, Waterworks, Coagulation and Zanzibar. It executes35/58
instructions; the excluded random branch requires its real per-birth RNG owner.
No instruction or callee is replaced. The native regression compares selected
definition/variant, original position/rotation bits and cold admission, and
checks authored model-child preparation. It is not a framebuffer comparison.

The fixture retains executable/cache paths and hashes locally. Missing owned
input is not parity evidence. Generate it in the repository oracle Python
environment, then run `original_tu15_heavy_vehicle_choices`.
