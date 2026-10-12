# Complete original model fade bytes

`source_model_fade` implements the complete retail Xbox `4BAF0` routine,
including unchanged `BAF80` parent-root selection and both output bytes. This
is a pure service over explicitly supplied owner/definition/model banks. It
does not produce successful object registration, current model metadata,
observer metrics or the original `4BA019` byte.

The authority is `assets/disc/default.xbe`, SHA256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The differential executes original `4BAF0..4BCB8`, `BAF80..BAFA7` and
`322B0C..322B80`; no arithmetic, lookup, root traversal or callee result is
hooked. The observation hook records executed addresses, checks them against
the original bytes and stops after the original `ret8`. Complete function
regions are checked unchanged again after all executions.

## Literal lookup and early-return contract

`SourceModelFadeOwner` supplies the current full handle and literal runtime
fields: flags at object+4, parent at+14, definition at+0 and current sphere
radius at+3C. `SourceModelFadeDefinition` supplies definition+38's hlmt datum;
`SourceModelFadeModel` supplies hlmt+28/+2C, the five floats at+34..+44 and
the unsigned count at+4C. Optional fields distinguish unresolved state from
an explicitly supplied source NONE sentinel.

The incoming XMM1 scalar and actual byte `4BA019` are separate mandatory
caller inputs on branches that read them. They are not reconstructed from a
native camera distance, map name, source scenario flags or the model radius.
The known global producer is original `2F970`: camera feature+58 OR feature+68
OR a nonnull scenario of type2. The already verified
`SourceFrustumProjection.view_modifier` preserves that exact side effect.
Original `2C560` subsequently forces `4BA019=1` for its special draw context.
The fade caller must supply the actual state after its current setup sequence;
neither a universal0 nor an earlier projection result is sufficient for every
context.

The source order is significant:

1. Both outputs start at0. Current flag19 OR any nonzero `4BA019` returns
   255/255 before current definition/model lookup. Thus force plus modelNONE
   still returns255/255.
2. Without force, current definition+38 modelNONE returns0/0 before parent,
   radius or metric processing. A valid parent model cannot rescue it.
3. `BAF80` follows literal object+14 handles until parentNONE. If that root
   differs from the current owner and its definition has a valid model, its
   model metadata replaces the current metadata. A root modelNONE retains
   the current model. Intermediate definitions, root flags and root radii
   are not consumed by this selection.
4. Both arithmetic outputs use selected model metadata. The width continues
   to use the CURRENT object+3C radius, including attached objects.

The native seam requires full valid handle banks and rejects missing read
fields, nonfinite read scalars, missing handles, duplicate resolved identities,
parent cycles and negative current spheres. This is bounded native input
validation; it does not claim the original unchecked slot lookup had those
error responses. Fields not read by an early branch remain optional. Explicit
NONE and absent metadata remain distinct.

## Both arithmetic bytes

The second byte uses the cutoff index `4-count`, capped at4 when positive.
Counts of4 or more use original `44AED4`, float maximum. The width is the
current radius multiplied by literal7. The fade is1 before the cutoff,0 at
or beyond cutoff+width, otherwise `1-(metric-cutoff)/width` in the original
SSE operation order. The source clips the interpolated factor and suppresses
values at or below literal `45DF60`, bits`3DC28F5C` (approximately0.095).

The first byte starts at1. Positive selected hlmt+28 is the fade end: metrics
at or beyond it return0; metrics after selected hlmt+2C interpolate
`1-(metric-start)/(end-start)`. A nonpositive end keeps1. This first factor
does not use the radius or the second factor's suppression threshold.

Each stored float is multiplied by literal255 in source x87 PC53 mode, then
the unchanged `322B0C` conversion truncates toward zero. That mode matches
the retained actual call registers' FCW`023F`. The two conversions occur in
first-byte/second-byte output order. Finite fields can overflow intermediate
subtractions; the fixture includes that case and checks the original NaN
conversion's low byte0. Source `3D7F0` consumes the second output byte.
The earlier `source_shadow_distance_fade_byte` remains its separately bounded
second-byte arithmetic service; no existing shadow submission API is changed.

## Independent differential and validation

`tools/differential_source_model_fade.py` freezes19,612 complete original
executions/39,224 output bytes. Coverage includes parent depths0/1/3, valid
and absent root models, current modelNONE, force precedence, nonboolean view
override bytes, current versus root radius, count0/1/2/3/4/5/65535, zero and
small spheres, cutoff/end/start boundaries, interior byte values, suppression
boundaries, inverted/nonpositive authored fade ranges and finite inputs whose
intermediate arithmetic becomes NaN.

The cache census supplies2,277 literal authored hlmt records across all21
multiplayer maps, for18,216 of these cases. It retains each actual datum/name
and the exact source model words in
`analysis/multiplayer-shading/model-fade-authored-banks.json`. Runtime owner
fields, links, metrics and view modifiers in this subset are explicitly
controlled fixtures. They are not relabelled as live gameplay admission or
the current model bank of a different saved phase.

Original calls recorded by the oracle are19,612 to4BAF0,15,035 toBAF80 and
30,070 to322B0C. All39,224 native bytes match exactly through the production
linked helper and its probe. The focused CTest also checks19 unresolved-input
rejections and three unread-field early-return controls. Normal and
ASAN/UBSAN runs pass with CPU leak detection enabled. Strict floating options
disable contraction for the source/helper test; no GPU or render state is
modified.

CTest: `retail_source_model_fade`. Frozen fixture:
`tests/fixtures/retail_source_model_fade.tsv`, with unchanged-executable proof
in its `.provenance.json` and the source report
`analysis/multiplayer-shading/model-fade-original.json`. Validation commands,
logs and source/test/binary hashes are recorded separately in
`analysis/multiplayer-shading/model-fade-validation.json`.

## Actual live observations and remaining producer inputs

The immutable copied OpenGL/llvmpipe capture retains actual4BAF0 incoming
metrics and outputs:

| Owner | Incoming XMM1 bits | Observed first/second bytes |
| --- | --- | --- |
| E2FF0090 | 41A2D29E | 255 / 0 |
| E2E60077 | 3E5146C0 | 255 / 255 |

Their raw owner records and definition prefixes are from actual
`33400-entry`. Those earlier definition prefixes end before+38. A subsequent
read-only export from the same copied session paused at `3290A`, tick12,475,
frame111,186/view0, supplies both complete definition and hlmt prefixes.
Actual Warthog definition`E64E04D9` refers to model`E64F04DA`; Chief
definition`EDF10C6B` refers to model`EDF20C6C`. Both actual parents are NONE.
The fresh complete prefixes are byte-identical to the literal Zanzibar cache,
and each earlier exported definition prefix agrees with its fresh prefix.
The oracle validates manifest sizes/hashes before using these actual fields.

Definition, parent and current radius match across `33400-entry` and `3290A`.
The Warthog flags match completely. Chief flags change from`40001B10` to
`00001B10`, clearing only the unused bit30; consumed bit19 remains clear.
The fixture keeps the literal earlier flags rather than silently replacing
them with the later value. This records the temporal change without changing
the original fade branch.

The incoming call registers independently establish ordinary processing:
both XMM2/XMM3 low lanes enter as0, while they exit as actual radius×7 and1
(`40F13C0B`/`3F800000` for Warthog,
`40451EB8`/`3F800000` for Chief). The unchanged force path
`4BB2F..4BB39` and modelNONE return perform no SSE writes. Only the ordinary
path's `4BBB7/4BBBF/4BBC4` produces those changes. Consequently both actual
calls had flag19 clear and modifier0. The later direct `4BA019` export is also0,
but is separately labeled as the later byte, not a simultaneous call-entry
export. Chief's255/255 output alone would not establish that branch.

These two corroborated reconstructions are included as the final two frozen
fixture rows. Their untouched source replay and native helper reproduce the
independently observed255/0 and255/255 bytes exactly.
`model-fade-live-observations.json` records both phase manifests/hashes, exact
metric/output words, matching consumed fields, the Chief flags difference,
complete model identities and the register/control proof. Complete model
metadata and the literal global byte were exported later in the same frame;
this is a corroborated input reconstruction, not a claim that every input was
exported simultaneously at the fade call.

Runtime integration must supply the actual current definition/model banks,
root ownership links, current sphere, incoming source metric and actual4BA019
producer. This helper closes the original lookup/control/arithmetic service
when those inputs exist. It does not fabricate missing metadata or activate
gameplay shadows. Fixture generation reads immutable exports only. The later
metadata export used read-only guest memory access with unchanged paused
status, CPU registers and clock bytes; it supplied no input or advancement.
