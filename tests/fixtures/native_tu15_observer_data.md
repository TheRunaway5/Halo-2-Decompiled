# Original TU1.5 observer loader data

Generate the ignored fixture from the authenticated English OG Xbox TU1.5 XBE:

```sh
python tools/generate_native_tu15_observer_data_fixture.py --xbe /path/to/default.xbe --output tests/fixtures/native_tu15_observer_data.json
```

The generator independently reads the XBE section table, original file bytes,
virtual zero-fill extents and section write flags. It exports only fourteen
named observer-module regions (3,519 bytes). It does not consume the native
helper, a running world, a capture or preconstructed camera values.

`original_native_tu15_observer_loader_data` compares those bytes, permissions
and exact complete region boundaries against the isolated native helper and
the actual no-XBE cold data factory. It checks gaps, removal of the obsolete
twelve-byte position fragment, native backing, write admission and checkpoints.
The test requires no World, Simulation, map cache or graphics device.

A missing fixture returns 77. A present malformed or mismatched fixture fails.
Do not force-track the ignored original-byte JSON. The generator and this
guide remain available in a clean checkout.

This is loader-data evidence. Original `171E10` allocates the separate
`51DF70` arena owner; `171F40`/`171EF0` call `172310`/`172220` to initialize
observers. Those lifecycle owners and actual outer-frame execution remain
separate requirements. Mounting data presets no FOV, reset sentinel or ready
flag and establishes no framebuffer parity.
