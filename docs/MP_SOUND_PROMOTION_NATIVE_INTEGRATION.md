# Original sound promotion selector and clock

The new isolated `sound_promotion` module ports complete title-update 1.1
`128610` and its unchanged 1.5 counterpart `1299D0`. The output is a **numeric
promotion selector**, not another sound datum. The caller keeps the original
sound datum when it passes that selector to `127750`. This preserves the
separate [submit naming erratum](MP_SOUND_SUBMIT_PROMOTION_OUTPUT_ERRATUM.md)
without changing any previously frozen source or evidence.

`source_sound_promotion_clock` also represents complete `128140` → `129500`.
It requires the actual tick provider and retained source state. It never
initializes a clock origin, produces a listener, selects a permutation, seeds
RNG, admits a sound, or sets availability flags.

## Original authority and independent reproduction

The owned original executable inputs are:

| Image | SHA-256 |
| --- | --- |
| Disc update 1.1 `default.xbe.03` | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| Authenticated owned `u5English/default.xbe` | `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd` |

The unique whole-body/internal-branch correspondence is in
`analysis/multiplayer/integration/sound-promotion-native/paired/report.json`,
SHA-256 `e2678f66ad01e0b6aaf91f6a35a6f392a47df5ae748199ab033b99474124f368`:

| 1.1 | 1.5 | Instructions | Role |
| --- | --- | ---: | --- |
| `128610` | `1299D0` | 109 | Complete promotion owner |
| `128140` | `129500` | 108 | Complete clock and gain-fade consumer |
| `126DE0` | `1281A0` | 58 | Map clock-origin owner |
| `18A4F0` | `18B8B0` | 159 | Promotion reset within broader sound reset |
| `127C10` | `128FD0` | 130 | Separate broader clock/sound reset |
| `12D600` | `12E9A0` | 34 | Mutable campaign-fade control query |

Every listed and executed instruction is checked against both original images.
The latter two reset owners and the fade-control owner are static whole-body
evidence here; this experiment does not claim their complete runtime execution.

The unchanged original experiment loads all **23 owned map banks**, including
the authenticated Desolation and current Tombstone files. It uses their literal
tag tables, full `ugh!` payloads, and all 29,780 resident 20-byte `snd!` records.
The inventory is the prior immutable
`sound-metadata-provider-20261008/measured/report.json`, SHA-256
`f750772e6951ff065711dd3c91b207d657e85fe69c43c89d06135eb717e072d3`.
All map files are hashed again and retained in the new manifest.

Each image/map case restores the whole previously source-created 143,360-byte
startup arena, then executes unchanged `126DE0` against the actual map metadata.
The original routine resolves the map `ugh!`, stores the genuine tick origin at
system `+80`, and writes system `+84=0`. It subsequently stops at the absent
driver dereference (`2217E8` in 1.1, `2260E8` in 1.5); no device return is
provided. Its earlier `+7A=1` is **not** completed activation or admission.

Four actual `clock_gettime_ns(CLOCK_BOOTTIME)` observations are recorded once,
with real waits of approximately 25, 25 and 2,050 milliseconds. Their recorded
low 32-bit millisecond values are replayed explicitly through the unchanged
original `GetTickCount` data thunk. This is a declared host clock adapter/replay,
not an Xbox clock granularity claim. The complete source clock consumer alone
updates `+80/+84` and its float accumulators. Loader-created options are NULL in
this bounded context; that is a recorded boundary, not an active multiplayer
options default.

The source experiment calls every resident sound once, then calls the actual
promotion sounds in 32 repeated rounds and two later aged rounds. These are
declared raw ABI requests, not newly claimed game/animation events. No counter,
selector, last-clock value, deadline or RNG state is assigned to reach a branch.
There are 215 literal promotion rows across the 23 maps. Every initial counter
array is the actual serialized zero byte sequence, and every initial row has
serialized selector `FFFFFFFF`, last clock zero and deadline zero. `18A4F0`
independently identifies the original row-state reset writer; it does not
invent a second counter initializer in this experiment.

The original result, independently re-executed with the recorded clock inputs,
is byte-identical:

- **74,318** complete clock/promotion calls across 46 image/map cases.
- **249,740** ordered source writes.
- **15,050** numeric selector outputs, including selectors 0, 1 and 2.
- **846** source suppression returns and **59,130** no-promotion outputs.
- **138** complete ordinary clock updates.
- **19,211,104** complete retained region bytes compared by the native tests.

Whole old/new bank outputs, every ordered write and every return agree. The
new native owner passes fresh normal and GCC ASan/UBSan builds and independent
native reruns. Missing raw-memory and tick services stop explicitly. The
exporter reconstructs every full output from original writes and validates the
original final arena/`ugh!`; it never uses native outputs as expectations.

Original report SHA-256:
`0a4994ed64d3070f600041ce53e26a107755ff917a16131db433a93c3586ff55`.
Fixture SHA-256:
`36f685b59311b87ec0a7e305513c083ff512456740b751a77ab4c09147479f30`.

## Exact promotion memory and write contract

`snd!+0C` is read as a **signed byte**. `-1` writes caller output `FFFFFFFF`
and returns zero. Otherwise the row address is `ugh!+4C` plus index × `1C`;
the original computed-null branch has the same no-promotion output.

| Row offset | Source use |
| --- | --- |
| `+00` | Signed rule count |
| `+04` | Rule array, 16-byte stride |
| `+08` | Signed counter count |
| `+0C` | Mutable DWORD counter array |
| `+10` | Previous numeric selector |
| `+14` | Previous source system clock |
| `+18` | Signed cooldown deadline |

If deadline ≤ current signed system `+84`, source writes selector `NONE` and
deadline zero. It then subtracts the wrapping elapsed clock difference from
every counter. Both writes are significant: `128693` first writes the raw
subtraction, and `128698` writes the signed-negative clamp to zero.

For each candidate rule, source adds rule `+0C` to its counter. It advances to
the next selector only when signed rule `+02` is positive, counter ≥ signed
rule `+08`, and the current selector is before the last rule. That advancement
first writes the current counter zero. The terminal selector is written at
`1286EA` to the caller's raw output DWORD.

Suppression is one only when new selector ≤ previous selector and the current
clock is before the deadline. If new selector ≥ previous selector and source
does not suppress, source writes the new previous selector and computes its
deadline in this exact order: signed clock → `CVTSI2SS`; rule `+04` float ×
milliseconds-per-second; add clock float; `CVTTSS2SI`; store. It finally stores
the current clock into row `+14` and returns the suppression integer.

The memory service must resolve actual retained fields. In particular, the
same **mutable map-owned `ugh!` copy** must survive repeated submits. A fresh
per-request copy would discard the original counters and cooldown. The
read-only metadata store alone is not that mutable owner.

## Clock and immutable operands

`128140` obtains a mandatory real tick value, performs wrapping subtraction
from `+80`, stores the new origin and increments `+84`. Its signed delta is
converted to float and multiplied by the exact seconds-per-millisecond word.
Source stores float `+1FC`, `+1E0` and `+204` in original arithmetic and write
order. The measured NULL-options lane then writes exact one into `+1E4`.

The complete native body retains the other source options/fade branches and
requires the complete `12D600` control dependency when that branch executes.
Those alternate branches are statically ported, **not** claimed as positively
executed native/original campaign-fade tests here. No live multiplayer options
lifecycle or device/listener admission is established by the NULL-options case.

`SoundPromotionConstants`, in header order, contains only these immutable
original DWORDs:

| Field | 1.1 VA | 1.5 VA | Literal bytes |
| --- | --- | --- | --- |
| `milliseconds_per_second_bits` | `46513C` | `46A1DC` | `00007a44` |
| `seconds_per_millisecond_bits` | `4650E0` | `46A180` | `6f12833a` |
| `one_bits` | `465030` | `46A0D0` | `0000803f` |

`paired/providers.json` pins the instructions and both image operands. No
mutable options/system/map pointer, clock, counter, selector, listener or
availability field is an immutable import.

## Native integration and reproduction

Production files are `include/halo2/sound_promotion.hpp`,
`src/sound_promotion.cpp` and `tests/sound_promotion_tests.cpp`. They are isolated;
this task does not edit submit/startup owners, runtime, shared audio, shaders or
the launcher. The native raw-output address is explicit because the original
caller owns that DWORD. It can back the existing submit promotion callback
without treating the selector as a sound identity.

The new immutable evidence is under
`analysis/multiplayer/integration/sound-promotion-native/`. Reproduction uses:

```text
.tools/python/bin/python tools/research_sound_promotion_u5.py --output <new-pair-directory>
.tools/python/bin/python tools/oracle_sound_promotion_native.py --output <new-original-directory> --replay analysis/multiplayer/integration/sound-promotion-native/measured/report.json
.tools/python/bin/python tools/export_sound_promotion_fixtures.py --report analysis/multiplayer/integration/sound-promotion-native/measured/report.json --providers analysis/multiplayer/integration/sound-promotion-native/paired/providers.json --output <new-fixture-directory>
.tools/python/bin/python tools/validate_sound_promotion_native.py --directory <new-native-directory> --fixture-directory analysis/multiplayer/integration/sound-promotion-native
sound_promotion_tests <absolute-path-to-fixtures.tsv>
```

Fresh standalone builds use strict float arithmetic and snapshot their actual
source/header dependencies. They do not assert freshness of a shared archive
or reproducibility of an unrelated dirty tree. Both original and native runs
retain the unavailable driver/map/session/admission boundaries. This closes
the promotion CPU seam; it does not claim live reload audio, scheduler/voice
creation, active BSP/local listener ownership or full sound playback.
