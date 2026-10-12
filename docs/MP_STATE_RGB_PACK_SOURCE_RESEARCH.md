# Original RGB packing and conversion bounds

Whole retail `131ED0` packs RGB by rounding `input*255` to signed32,
masking each result with FF and arranging `R<<16 | G<<8 | B`. It does
not clamp input colors to0..1. The new fixture preserves 8,156 complete
original calls with asymmetric finite inputs and observed rounding controls.
This is a CPU primitive contract; state-extern import, graphics-state
publication and framebuffer behavior remain separate.
[Original fixture](../tests/fixtures/state_rgb_pack_original.json),
[provenance report](../analysis/multiplayer/game-variants/state-rgb-pack-20261011/report.json),
[complete103-byte owner](../analysis/multiplayer/game-variants/state-rgb-pack-20261011/rgb_pack-00131ed0.asm)

The authority is `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
All reached instructions are byte-checked against that image. No instruction,
game return or graphics service is patched or supplied. The original owner
receives a pointer to three binary32 RGB values, reads binary32 255 at
`45DC38`, loads all three into x87, multiplies them, then converts and
masks in **B,G,R** order. EAX returns RGB24, input words survive unchanged,
and ECX survives (the state caller uses it after the call). The upper output
byte is zero; this owner does not preserve an incoming alpha byte.
[Complete source owner and conversion order](../analysis/multiplayer/game-variants/state-rgb-pack-20261011/rgb_pack-00131ed0.asm),
[unchanged-owner oracle](../tools/oracle_state_rgb_pack.py)

Each case records raw input before/after, EAX/ECX before/after, scratch
before/after, the actual FPCW at all three FISTPs, the raw80-bit x87 top
value, and the produced signed32 integer **before** the original AND FF.
All calls return normally. The 2,039 declared vectors include a complete
finite grid with negatives, over1 values, both signed zeros, .25/.5/.75,
and fade residue `31C00000`; adjacent binary32 values around conversion
ties and signed32 edges; minimum/maximum normal/subnormal controls; finite
maxima; and128 deterministic finite bit-pattern triples. Each runs with
FPCW027F/067F/0A7F/0E7F: nearest-even/down/up/toward-zero respectively.
The precision field remains53-bit, exception masks are set, initial x87
stack is empty and MXCSR is1F80. These controls are observed at the real
instructions, not merely supplied as labels.
[All declared inputs and24,468 FISTP observations](../tests/fixtures/state_rgb_pack_original.json),
[control and corpus provenance](../analysis/multiplayer/game-variants/state-rgb-pack-20261011/report.json)

For RGB(.25,.5,.75), the actual outputs are:

| Rounding | Packed RGB |
| --- | --- |
|nearest-even|`4080BF`|
|down|`3F7FBF`|
|up|`4080C0`|
|toward-zero|`3F7FBF`|

The half conversion .5*255=127.5 exposes nearest-even rounding. Negative
and over1 valid signed conversions wrap through AND FF instead of saturating;
the corpus contains9,183 such channel conversions. Positive fade residue
converts to0 with nearest/down/chop and1 with up. These are controlled
generic input probes, not a claim that natural camouflage colors go outside
the producer's clamped interval.
[Observed conversion results](../tests/fixtures/state_rgb_pack_original.json),
[derived interpretation audit](../analysis/multiplayer/game-variants/state-rgb-pack-20261011/audit.json)

There are780 channel conversions whose rounded value exceeds signed32
bounds. Unchanged `FISTP` produces `80000000` in this oracle; the following
original AND FF makes that channel zero. The other channels still pack
normally. An exact-rational audit independently checks that every recorded
x87 value equals its binary32 input times255, every recorded conversion
matches the selected rounding and signed32 bounds, and every final word
matches the original byte mask/order. The audit reads recorded observations;
it does not generate the oracle's expected outputs or execute a second
native/original implementation.
[Raw overflow products and signed results](../tests/fixtures/state_rgb_pack_original.json),
[audit implementation](../tools/audit_state_rgb_pack.py),
[audit counts and limits](../analysis/multiplayer/game-variants/state-rgb-pack-20261011/audit.json)

The emulator has an explicit status limitation: its recorded FPSW invalid
and precision flags remain clear even for those finite overflow conversions.
This evidence supports the observed integer/mask output under masked
controls, while architectural exception-status delivery and unmasked
exceptions remain unverified. No status bit was forced to conceal this.
Nonfinite inputs, other precision-control settings, title-update owner
equivalence and hardware execution are outside this corpus.
[Oracle provenance and status limitation](../analysis/multiplayer/game-variants/state-rgb-pack-20261011/report.json)

Alpha preservation belongs to the separate complete `1A170` color branch:
it combines packed RGB24 with the existing state word's high byte, emits
the original immediate-state command for state75, then updates the state
cache. The earlier5,040 binder compositions already observe this for three
alpha baselines and actual authored color40 rows. A native color primitive
can use this RGB pack contract and that separate alpha operation; neither
establishes a state-range importer or successful GPU publication.
[Complete original state consumer](../analysis/multiplayer/game-variants/camouflage-binder-20261011/state_extern-0001a170.asm),
[separate whole-state evidence](../analysis/multiplayer/game-variants/camouflage-binder-20261011/binder-report.json),
[binder interpretation and boundaries](MP_CAMOUFLAGE_BINDER_SOURCE_RESEARCH.md)

The fixture SHA-256 is
`96d5c97e9140cc4b1c97f0c15eab12fc457574894eae6d14dc7c28901e616550`.
Fresh generation and the derived audit reproduce byte-for-byte using the
commands in the adjacent manifest. Existing vertex/binder evidence, native
sources, CMake, progress and admission are unchanged by this research.
[Reproduction manifest](../analysis/multiplayer/game-variants/state-rgb-pack-20261011/reproduction.json)
