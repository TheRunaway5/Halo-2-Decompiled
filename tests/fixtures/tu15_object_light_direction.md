# TU1.5 object lighting direction

`tu15_object_light_direction.original.json` was produced by
`tools/oracle_object_light_direction_tu15.py` from unchanged OG Xbox TU1.5
`11A360`, XBE SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Every executed instruction is checked against that image. No services, instructions
or return values are replaced. The 1,200 retained cases include pole, horizontal
heading and threshold boundaries. They execute 110 of the 111 whole instructions.

The native regression compares all three output float words exactly. Before the
correction it rejected 469 cases. The original retains the horizontal square root
on x87 through the component multiplies at `11A48F..11A4A7`; the previous native
implementation rounded the product, subtraction and square root to float early.
The fixed implementation passes all 1,200 cases. The same correction makes the
20 independently computed lighting shader floats exact for the captured Waterworks
Scorpion and its child. This fixture verifies the direction helper, not complete
framebuffer parity or every object-lighting query branch.
