# TU1.5 incoming camera interval oracle

The generated `campaign_source_frame_interval_tu15.inc` is a local, ignored
fixture. Its source is the owned original English Xbox TU1.5 executable with
SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Vista and other title update executables are rejected.

From the repository root, with Python's `unicorn` package available:

```sh
python3 tests/fixtures/generate_campaign_source_frame_interval_tu15.py \
  --xbe "/path/to/owned/default.xbe"
```

The generator verifies the executable hash and executes the unchanged original
`12DD47..12DD6E` interval clamp. An unchanged `FLD` instruction supplies the
explicit binary32 boundary input. It writes the C++ fixture and a local receipt
with the executed original addresses, bytes and all 15 input/output cases.
There are no instruction patches or supplied game service returns.

This qualifies the scalar boundary after counter division and binary32 spill;
it does not claim that signed zero or subnormal inputs occur from physical
Xbox counters. Nonfinite values are outside the original counter domain and
are tested only as the native host interface's explicit rejection contract.

Configure the owned executable for the registered native acceptance test:

```sh
cmake -S . -B build -DHALO2_TU15_GAMEPLAY_XBE="/path/to/owned/default.xbe"
cmake --build build --target campaign_camera_frame_tu15_tests
ctest --test-dir build -R native_campaign_camera_outer_frame_tu15 --output-on-failure
```

The test requires `assets/disc/maps/00a_introduction.map` and
`assets/disc/maps/03b_newmombasa.map`. A clean checkout without the generated
fixture still compiles and exits 77. Missing required assets also exit 77;
a present invalid executable, malformed fixture or failed parity control fails.
The supported offline-XBE world test does not establish native-cold player
startup, physical Xbox RSQRT estimates, final Vulkan frames or launcher delivery.
