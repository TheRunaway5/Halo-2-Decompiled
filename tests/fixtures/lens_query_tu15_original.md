# Original TU1.5 lens query fixtures

These ignored JSON fixtures contain declared inputs and unchanged-original
outputs from the pinned English TU1.5 XBE:
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
They are not native expected outputs used as original inputs.

| Fixture | Cases | Scope |
| --- | ---: | --- |
| `lens_quad_tu15_original.json` | 45 | Unicorn execution of original projected quads |
| `lens_quad_tu15_hardware.json` | 45 | Unchanged host i386 execution, including retained scratch and x87 status |
| `lens_query_state_tu15_original.json` | 172 | Original issue, completion, visibility and normalization |
| `lens_query_state_tu15_hardware.json` | 172 | Unchanged host i386 execution of the same complete calls |
| `lens_query_reuse_tu15_original.json` | 18 | Original ready/pending reuse of occupied query identities |
| `lens_query_reuse_tu15_hardware.json` | 18 | Unchanged host i386 execution of both reuse branches |

The quad corpus uses nine actual authored lens definitions from the original
request fixture, with five declared positions/viewports each. Whole original
TU1.5 `2FD70` produces the projection input. The state corpus includes genuine
observed GPU samples and declared ratio/pending/view cases. Hardware establishes
Unicorn's missing x87 precision flag and fixed control-word bit six; native
tests retain and compare the hardware values without masking those differences.

The 235 query cases reach 930 instruction sites in 16 of the 20 registered
complete query bodies. They do not cover every linked-readout, CRT exceptional,
SDK allocation/flush or retirement branch. GPU acceptance and live map
scheduling have separate reports.

Use fresh output directories and preserve existing fixtures when regenerating.
The following assumes the destination fixtures do not exist. The request
fixture recipe is in [lens_request_tu15_original.md](lens_request_tu15_original.md).
The original image path is selected and authenticated by
`research_mp_sandbox_update_functions.py`.

```sh
.tools/python/bin/python tools/generate_lens_query_tu15.py \
  --directory analysis/lens-query-reproduction/generation
.tools/python/bin/python tools/oracle_lens_quad_tu15.py \
  --fixture tests/fixtures/lens_quad_tu15_original.json \
  --report analysis/lens-query-reproduction/original-quads.json
.tools/python/bin/python tools/oracle_lens_quad_hardware_tu15.py \
  --input tests/fixtures/lens_quad_tu15_original.json \
  --bodies analysis/lens-query-reproduction/generation/bodies.json \
  --output analysis/lens-query-reproduction/hardware-quads \
  --fixture tests/fixtures/lens_quad_tu15_hardware.json
cmake --build build-multiplayer --target lens_query_gpu_tests lens_query_tu15_tests
HALO2_NATIVE_GPU_SHADER_DIR="$PWD/build-multiplayer/shaders" \
  xvfb-run -a build-multiplayer/lens_query_gpu_tests \
  tests/fixtures/lens_quad_tu15_hardware.json analysis/lens-query-reproduction/gpu
.tools/python/bin/python tools/oracle_lens_query_state_tu15.py \
  --gpu-report analysis/lens-query-reproduction/gpu/report.json \
  --fixture tests/fixtures/lens_query_state_tu15_original.json \
  --report analysis/lens-query-reproduction/original-state.json
.tools/python/bin/python tools/oracle_lens_quad_hardware_tu15.py \
  --input tests/fixtures/lens_query_state_tu15_original.json \
  --bodies analysis/lens-query-reproduction/generation/bodies.json \
  --output analysis/lens-query-reproduction/hardware-state \
  --fixture tests/fixtures/lens_query_state_tu15_hardware.json
.tools/python/bin/python tools/oracle_lens_query_state_tu15.py --reuse-only \
  --gpu-report analysis/lens-query-reproduction/gpu/report.json \
  --fixture tests/fixtures/lens_query_reuse_tu15_original.json \
  --report analysis/lens-query-reproduction/original-reuse.json
.tools/python/bin/python tools/oracle_lens_quad_hardware_tu15.py \
  --input tests/fixtures/lens_query_reuse_tu15_original.json \
  --bodies analysis/lens-query-reproduction/generation/bodies.json \
  --output analysis/lens-query-reproduction/hardware-reuse \
  --fixture tests/fixtures/lens_query_reuse_tu15_hardware.json
ctest --test-dir build-multiplayer -R '^original_tu15_lens_query_' --output-on-failure
```

The hardware tool requires the Linux host i386/ptrace backend; the GPU test
requires a real Vulkan graphics queue with precise queries and timestamps.
It compares all 45 original quad sample areas, actual visible/occluded/empty
results, generation reuse, unchanged color/D24/stencil bytes, and safe renderer
retirement with validation enabled. It does not establish Xbox GPU coverage,
final flare bitmap rendering or the full all-map goal.
