# TU1.5 PRT bank and fallback fixture

`tu15_prt_fallback.original.json` retains the Cairo tram's original selected
sections, WORLD matrices, prior cache incident and completed coefficient banks.
The executable is the owned OG Xbox TU1.5 English update, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

`tools/capture_live_model_base_tu15.cpp` constructs the actual scenario placement,
performs a real resource warmup, starts another original frame/view and invokes
the native full model builder. Its explicit camera-distance input is zero.
The two controlled cases change only the current lightmap group's SH count to
zero, or bitmap datum to NONE, before original per-view PRT initialization.
They are declared input controls, not captured gameplay states.

`tools/differential_live_model_builder_tu15.py` independently executes unchanged
TU1.5 `4B5C0`, including `23670`, over each exported input bank. All 58,962,006
compared retained bytes match in each case. The valid, empty and invalid cases
execute 42,382, 42,722 and 40,997 original instructions respectively. No model,
PCA, SH, lighting query or residency output is supplied to the reference.
The diagnostic queue mutex/unused GPU-metadata boundary is documented by that
tool and its reports.

The retained source cases live under `analysis/shader-parity-20261013/`:
`live-cairo-tram-base-v3`, `live-cairo-tram-base-empty-v1` and
`live-cairo-tram-base-invalid-v1`. Each fixture case retains its comparison hash.

`source_prt_fallback_tu15_tests` checks 2,277 consumed incident/coefficient words
bit for bit across nine section banks. It excludes only original unwritten
ambient padding and c68.w, and rejects fallback assembly without explicitly
supplied retained incident. A valid empty SH bank produces green SH; an invalid
lightmap bank produces white SH. Both preserve the previous cache incident.
This proves these original arithmetic and state cases, not matched-frame pixel
parity, physical NV2A precision, all PRT cache pressure or every model.
