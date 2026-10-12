# Original fog scalar39/color41 providers through Xbox TU1.5

The unchanged original scalar39 and color41 dispatchers now complete in retail,
TU1.1 and TU1.5 for 768 declared finite input contexts. All three images produce
identical binary32 output words. This is 4,608 complete original calls, with
actual SSE intermediate registers and floating-point controls recorded; there
are no patched instructions or supplied returns. The prior captured 16B90 service
evidence remains unchanged.
[Source-only oracle](../tools/oracle_fog_externs.py),
[input/output fixture](../tests/fixtures/fog_externs_original.json),
[image pins, original field addresses and output hashes](../analysis/multiplayer/game-variants/fog-externs-20261011/report.json),
[separate captured pass-service context](MP_CAMOUFLAGE_PASS_SERVICES_SOURCE_RESEARCH.md)

| Original consumer | Retail | TU1.1 | TU1.5 |
| --- | --- | --- | --- |
| Scalar dispatcher | `336F0` | `336E0` | `336E0` |
| Scalar39 branch | `3385D` | `3384D` | `3384D` |
| Color dispatcher | `33980` | `33970` | `33970` |
| Color41 branch | `33EF9` | `33EE9` | `33EE9` |
| Fog datum word | `4B9F5C` | `4C147C` | `4C677C` |
| Near / far | `4B9F80 / 4B9F84` | `4C14A0 / 4C14A4` | `4C67A0 / 4C67A4` |
| Forward XYZ | `4B9DAC` | `4C12CC` | `4C65CC` |
| Default color pointer | `4686D4` | `46FB74` | `474C34` |

Each provider branch is located independently in the pinned executable; its
actual dispatcher is recovered from the corresponding jump-table entry. Global
field addresses come from that image's reached instructions. No common address
or relocation delta is assumed. Every executed instruction is compared against
the actual image bytes, and both calls must reach their real RET with the stack
restored. The saved executed lists include the dispatch and default branches.
[Retail executed bytes](../analysis/multiplayer/game-variants/fog-externs-20261011/retail-executed.json),
[TU1.1 executed bytes](../analysis/multiplayer/game-variants/fog-externs-20261011/tu11-executed.json),
[TU1.5 executed bytes](../analysis/multiplayer/game-variants/fog-externs-20261011/u5-executed.json)

The consumer first tests the datum word against NONE (`FFFFFFFF`). It does not
resolve a tag through that word in these branches. When the word is non-NONE,
it uses COMISS to require far greater than near. With the declared finite inputs,
failure selects scalar positive zero and copies the default color's three raw
DWORDs. The color copy loads the actual pointer global, adds4, then readsXYZ;
neither arithmetic nor clamping changes these default words.
[Original scalar39 instructions](../analysis/multiplayer/game-variants/fog-externs-20261011/retail-scalar-0003385d.asm),
[Original color41 instructions, including default pointer copy](../analysis/multiplayer/game-variants/fog-externs-20261011/retail-color-00033ef9.asm)

For the valid path, the required operation sequence is:

```text
delta      = SUBSS(far, near)
reciprocal = DIVSS(binary32(1), delta)
scalar39   = SUBSS(positive_zero, MULSS(reciprocal, near))
color41.x  = SUBSS(positive_zero, MULSS(forward.x, reciprocal))
color41.y  = SUBSS(positive_zero, MULSS(forward.y, reciprocal))
color41.z  = SUBSS(positive_zero, MULSS(forward.z, reciprocal))
```

Every operation rounds separately at binary32 precision. The reciprocal is
computed once per dispatcher call. The scalar and color callers each execute
their own SUBSS/DIVSS. Native arithmetic must preserve this order: replacing the
sequence with a direct numerator division, a sign-bit flip, fused operations
or a higher-precision unrounded reciprocal is outside this contract. With
near 10 / far 30 / forward (1, -.5, .25), case 1 returns scalar `BF000000` and color
`BD4CCCCD 3CCCCCCD BC4CCCCD`. Case 49 uses the stored binary32 values of .3 and 1.1
and returns scalar `BEC00000`. Cases 17/25 show that either sign of zero in the
forward vector yields positive-zero color on this valid path; raw default
negative zero is preserved on the fallback path.
[Exact original operations](../analysis/multiplayer/game-variants/fog-externs-20261011/retail-color-00033ef9.asm),
[Case inputs, outputs and SSE registers](../tests/fixtures/fog_externs_original.json)

The corpus contains 16 near/far pairs, six forward vectors, four default-color
records and two datum words. The pairs cover negative near/far, increasing and
decreasing intervals, equal endpoints, both zero signs, narrow adjacent-float
intervals and a 32768-unit interval. Forward/default records include asymmetric
values, signed zero, the smallest normal value and signed subnormal values.
There are 264 valid-distance contexts and 504 fallback contexts per image. All
actual outputs are finite, including gradual subnormal products. The supplied
datum 7 is a declared non-NONE consumer input, not a claim that a live fog tag
with that datum was loaded.
[Corpus construction and original execution assertions](../tools/oracle_fog_externs.py),
[Full declared records](../tests/fixtures/fog_externs_original.json)

Each original call starts with observed FPCW`027F` and MXCSR`1F80`: nearest
rounding, exceptions masked, DAZ/FTZ clear. FPCW is recorded for provenance;
these arithmetic instructions use SSE and therefore MXCSR. The perinstruction
observations are sampled **before** the named instruction; later samples and
the actualRET sample expose its result. Registers XMM0..3 are saved as raw low
DWORDs at each reached MOVSS/COMISS/SUBSS/DIVSS/XORPS/MULSS/MOVAPS/RET. The source
preserves the controls throughout the calls. This Unicorn execution does not
establish architectural exception-status delivery; no nonfinite inputs,
unmasked exceptions, other rounding controls or flush modes are admitted.
[Raw FP observations and controls](../tests/fixtures/fog_externs_original.json),
[FP setup and observation hooks](../tools/oracle_fog_externs.py)

The default pointer addresses a declared 20-byte backed record containing a
sentinel DWORD, the three default RGB words and a trailing sentinel. Before/after
bytes are stored for every case and must match exactly. Output guards and all
input global words are also checked unchanged. This proves the original
pointer+4 copy and preserves arbitrary finite defaults; it does not establish
the live pointer's initialization/reset owner or a cold-zero default. The
earlier capture's actual `4686D4 → 440A00` and zero RGB record remain evidence for
that specific capture only.
[Saved raw default records](../tests/fixtures/fog_externs_original.json),
[Captured default-pointer context and its separate limit](MP_CAMOUFLAGE_PASS_SERVICES_SOURCE_RESEARCH.md)

The fixture uses root `inputs` and `versions[name].results` arrays with matching
indices. Each input has `datum`, `near_bits`, `far_bits`, `forward_bits[3]` and
`default_rgb_bits[3]`; each result has `scalar39_bits`, `color41_bits[3]`,
`default_before_hex`, `default_after_hex`, `scalar_observations` and
`color_observations`. This supports a shared finite CPU provider primitive and
per-owner extern-bank comparison. Fog loading, camera/forward production,
visibility or layer admission, other extern indices, authored pass scheduling,
graphics resource readiness, target copies and framebuffer results remain
separate requirements. No gamemode admission follows from this corpus.
[Reproduction manifest](../analysis/multiplayer/game-variants/fog-externs-20261011/reproduction.json),
[Separate TU primitive correspondence](MP_RENDER_PRIMITIVES_TITLE_UPDATE_SOURCE_RESEARCH.md)
