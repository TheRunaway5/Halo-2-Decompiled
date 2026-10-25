# 4K60 split-screen performance tools

Support for [the native 4K60 goal](../../docs/NATIVE_4K60_SPLITSCREEN_GOAL.md).
Run everything through `tools/run_test.py`: builds of the preload libraries,
captures and timing files stay in `HALO2_TEST_SCRATCH` and are removed
afterwards. Only the small reports you direct elsewhere remain.

| Script | Purpose |
|---|---|
| `profile_run.sh` | One 4K-internal multiplayer run; timing summary, optional main-thread profile (`SAMPLE_DELAY` argument) or CPU ms/frame (`H2_CPUMON=1`) |
| `ab.sh`, `ab_env.sh` | Interleaved A/B of two runtimes, or one runtime with/without an environment assignment; reports main-thread CPU ms/frame (`H2_PERF_REPORTS` = report directory) |
| `parity.py` | Byte comparison of deterministic 3840×2160 captures between two runtimes (`VAR=VALUE,...@runtime` adds role environment) |
| `analyze.py`, `buckets.py`, `callers.py`, `children.py`, `diffprof.py` | Self/inclusive profiles, cost buckets, caller/child breakdowns and profile diffs from `sampler.c` output |

```bash
python3 tools/run_test.py -- tools/perf_4k60/profile_run.sh "$PWD/build/halo2_native" /tmp/reports base 4 300 ""
H2_PERF_REPORTS=/tmp/reports python3 tools/run_test.py -- tools/perf_4k60/ab.sh name 3 "$OLD/halo2_native" "$NEW/halo2_native"
python3 tools/run_test.py -- python3 tools/perf_4k60/parity.py /tmp/reports/parity.json \
    "HALO2_REFERENCE_MAIN_CULL=1@$PWD/build/halo2_native" "$PWD/build/halo2_native"
```

Run runtimes from their build directory (the EOS worker must sit beside the
binary). Compare parity only within one session: the Vulkan pipeline cache
changes 4p output between sessions. `parity.py` forces X11 under `xvfb-run`
so captures keep the full internal size.
