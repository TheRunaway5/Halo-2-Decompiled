# Main menu local pose and grip evidence

The main menu models run the unit local callback after the primary channel.
An inactive secondary animation channel does not establish that this callback
is absent. Retail `BDB60` samples the primary, calls `BD970`, then dispatches
`109050` to unit callback `112070`. Its final single-held branch starts the
control's `animation_10C` and samples it at frame zero and weight one.

The paused ten-object control snapshot is
[original-model-menu-hidden-grip-control.json](../analysis/frontend/profile-regression/original-model-menu-hidden-grip-control.json).
It records primary channels, local and world banks, the entire unit header,
and the 292-byte unit animation control together. All menu units have selected
weapon slot zero and offhand `NONE`; `animation_10C` is Spartan clip43 or
Elite clip99. The authored slots are retained as bytes, including
`07070103` for slot0.

## Lookup and codec execution

[oracle_frontend_model_grip.py](../tools/oracle_frontend_model_grip.py) executes
unchanged original `112070[112C59,112DAE)` with captured headers, control,
state, mask and destination. The observed sample sink has flags0, rate1,
frame0, weight1 and the exact packed clip43/99. It does not substitute a
hand animation selected by name or infer callback absence from channel1/2.

The lookup probe separately executes original `1DBC80` and `1DB170` on the
complete authored graphs and the frozen `1CC2F0` combat name-resolution
outputs. Both species' grip is in the overlay table; the ordinary table
returns `NONE`. Original `1CBAD0` searches the complete overlay fallback
sequence before starting the complete ordinary action fallback sequence.
The cache getter uses structural optional results and propagates malformed
metadata failures. Weapon type aliases come from the graph's actual alias
block, including the Elite's plasma-rifle class alias.

Both grips are authored animation type2, one frame, codec3 and rotation-only:
11 Spartan rotations and 15 Elite rotations. They replace those components;
they are not type1 additive overlays. The probe executes unchanged `2798A0`,
`279860`, `27A100` and the codec `28C960`, requiring the actual dispatch and
decoder call counts. Translations, scales and unselected nodes remain intact.

The initial stack-only `2798A0` probe was invalid: the weight is passed in
`XMM4`, so that attempt returned before any codec dispatch. Its earlier
zero-difference full-sampler claim is retracted. The corrected probe supplies
data in `ESI`, frame in `XMM1`, weight in `XMM4` and the remaining ten stack
arguments, matching original `279D80[279DFC,279E2E)`. Its call-count assertions
prevent that early return from being accepted as a result.

## The captured bank's stage matters

The authored model `hlmt+A4` mask defers particular nodes. Original `BD090`
initial sampling uses its complement, and `BDB60` limits the callback count
to the highest selected node plus one. The actual source proof is
[frontend-model-node-admission-original.json](../analysis/frontend/profile-regression/frontend-model-node-admission-original.json),
covered in [FRONTEND_MODEL_SCENE_RESEARCH.md](FRONTEND_MODEL_SCENE_RESEARCH.md).
The initial callback prefixes are21 for Spartan and39 for Elite. All grip
rotations lie outside the initial active mask.

The corrected unrestricted codec diagnostic therefore changes520 quaternion
words compared with the ten captured deferred banks. This is expected: those
banks have not yet sampled the deferred fingers. The separately executed
initial-mask diagnostic runs the real dispatcher, admits zero grip decoder
calls and preserves all3,520 captured local words. That is a mask/skip result,
not proof that the full rendered pose is unchanged.

Visible models can enter `137800`, call `BD610` and sample deferred nodes.
The actual verified fourway snapshot has bit30 cleared for the visible first
Spartan and set for the other nine hidden objects. Comparing every object to
an unrestricted pose would confound these stages and produce a false missing
jaw/grip conclusion. The comparator now serializes captured bit30 and an
independent stage fixture while preserving all original root/node fixture bits.

The later render producer captures resolve the visible-stage distinction:

- [Spartan post-admission](../analysis/frontend/profile-regression/original-model-visible-spartan-render-post-admission.json)
  records original `137800` after `BD610`, its exact primary frame and selected
  bank. The grip probe really executes all11 decoder calls and reproduces
  the captured finger quaternion words with zero differences.
- [Elite fresh-tick post-admission](../analysis/frontend/profile-regression/original-model-visible-elite-render-post-admission-fresh-tick.json)
  records the same stage for a visible Elite. All15 decoder calls reproduce
  its captured finger quaternion words with zero differences.

## Native helper and verification boundary

[frontend_model_primary.cpp](../src/frontend_model_primary.cpp) reads the
literal source primary packed int16 keys, times, masks and float vector/scale
inputs. The sampled quaternion bank is consumed directly, without calling the
generic `sample_animation` normalization step. The grip reader retains the actual
packed int16 quaternion inputs and mask. The grip kernel uses the original
norm grouping `(x*x+z*z)+(y*y+w*w)` and multiplies the lanes by the inverse
square-root seed. Its binary32 inverse-root default is an explicit software
convention; a supplied observed seed is also supported. No host reciprocal
instruction is substituted for the separate `source_model_pose` convention.
Exact original codec and actual captured visible-local fixtures validate the
menu inputs; this does not identify every physical Xbox approximation seed.

The helper accepts an explicit destination mask for both primary and grip.
`load_frontend_model_deferred_mask` returns the literal authored mask, leaving
the source initial/deferred scheduling decision to its caller. Runtime ordering
must preserve initial primary locals, world callbacks, then deferred resampling
and hierarchy reconstruction using any adjusted parent nodes. An unconditional
initial mask or unrestricted resampling before world IK would change the source
order. The shared final pose helper preserves this ordering; the independent
comparison below invokes that same production helper.

The focused regression omitting grip fails at the first Spartan's node21.
The corrected full codec and initial-mask fixtures check3,520 local words
each, and the two actual visible fixtures add264 Spartan and440 Elite words.
These checks prove the local grip/mask seam, not complete model or framebuffer
parity. The independent pose comparator still reports every differing word.

## Primary sampling time and packed inputs

The original menu channels retain fractional frame clocks, but flags `003F`,
rate 1 and the captured 30 Hz clock disable interpolation in `1C6F60`. The
unchanged source caller passes a zero interpolation byte to `279D80`;
`2798A0` truncates that frame without changing the controller's retained clock.
Actual initial-stage producer captures confirm the same argument before the
deferred render pass:
[Spartan](../analysis/frontend/profile-regression/original-model-primary-spartan-sampler.json)
and [Elite](../analysis/frontend/profile-regression/original-model-primary-elite-sampler.json).

The generic float-key representation divided packed rotations by 32767 and
interpolated the fractional clock. Original codecs instead normalize the
integer key, or the integer blend between sparse keys. The noninterpolated
sparse path directly loads a key at its authored timestamp; scaling that same
key by the following interval would introduce different rounding. Vector and
scale decoders copy exact keys or evaluate `(next-current)*weight+current`.
The new narrow reader retains the bytes and follows those branches. It does
not reconstruct quantized inputs by multiplying generic floats by 32767.

[oracle_frontend_model_primary.py](../tools/oracle_frontend_model_primary.py)
executes unchanged `1C6F60`, `2798A0`, the static/animated dispatcher and actual
primary codecs. The frozen [three-clip proof](../analysis/frontend/profile-regression/frontend-model-primary-three-clips-original.json)
and [fixture](../analysis/frontend/profile-regression/frontend-model-primary-original.tsv)
cover Spartan combat 47/UI 132 and Elite combat 73, 54 full/initial/deferred mask
cases, actual frames and fractional frame boundaries. The regression failed
on the generic codec and now checks 6,480 authored component words exactly.

The independent [corrected visible-local comparison](../analysis/frontend/profile-regression/frontend-model-visible-locals-corrected-comparison.json)
reports zero differences in the actual 264 Spartan and 440 Elite local
words. Before this correction, those same paused banks had 75 and 103 differing
words, respectively. This isolates the primary error before world transforms;
the remaining world-bank comparison must use the composite primary, world IK
and deferred hierarchy helper. Exact local fixtures are not a framebuffer or
physical Xbox approximation claim.

The [paired initial producer comparison](../analysis/frontend/profile-regression/frontend-model-initial-locals-corrected-comparison.json)
uses the real destination masks and the unchanged before/after primary clock
from the two live `BDB60` stops. It also has zero admitted local-word differences.
The real stack captures have unrelated upper bits in the interpolation word;
the original callee consumes its low byte. The comparator validates that byte
and the frame/weight against the actual sampler entry rather than inferring a
previous update from the later render clock.

Before the primary codec correction and composite-helper comparison, hidden Elite world-node discrepancies
fall from large jaw/finger differences to maximum components between
0.00011864 and0.00012755, with translation differences at most1.52588e-5.
The actual visible Elite post-admission bank has maximum0.000211224 and
translation4.19617e-5 before the full shared callback pipeline is included.
The actual visible Spartan remains at maximum0.38149 and translation0.01421,
principally from held-weapon world IK that this local-only comparison omits.
These are measured residuals, not an accepted tolerance or a claim of exactness.
The reports are
[fourway staged comparison](../analysis/frontend/profile-regression/frontend-pose-fourway-staged-comparison.json),
[visible Spartan](../analysis/frontend/profile-regression/frontend-pose-visible-spartan-staged-comparison.json)
and [visible Elite](../analysis/frontend/profile-regression/frontend-pose-visible-elite-staged-comparison.json).

## Composite visible-bank comparison

The independent comparator's `--shared-pose` mode really calls
`frontend_model_pose`, the same helper used by the native frontend. It reads
the captured slot0 bytes and evaluates the original aim curve rather than
hardcoding its weight, retains the captured root/frame, loads the verified
packed primary and grip, applies world IK and reconstructs only deferred
nodes. Reports retain every expected/actual float word and have no tolerance
that can convert differences into success.

The [Elite composite comparison](../analysis/frontend/profile-regression/frontend-pose-visible-elite-shared-comparison.json)
has zero differences in all 715 world-node words of the actual visible
fresh-tick bank. The [Spartan composite comparison](../analysis/frontend/profile-regression/frontend-pose-visible-spartan-shared-comparison.json)
has 88 differing words among 429. Those differences are confined to the left
upperarm, forearm, hand and six descendant finger nodes; the other 24 nodes
are byte-exact. Its maximum component error is 0.0000598729, with translation
error at most 0.00000762939. Both actual local banks are exact, isolating this
remaining Spartan discrepancy to the world IK/held-weapon target stage.
The remaining difference is unresolved; it is not accepted as a tolerance.
Neither captured-bank result establishes complete animation lifecycle,
physical Xbox arithmetic or framebuffer parity.
