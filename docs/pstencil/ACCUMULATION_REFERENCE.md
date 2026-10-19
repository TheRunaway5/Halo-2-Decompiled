# May2003 accumulation program construction

Current integration: campaign-v10 executes the recovered normal producer and
normal-space utility, with native/GPU binding and scene controls retained in
[NORMAL_ACCUMULATION_2003.md](NORMAL_ACCUMULATION_2003.md). The original texture
constructor and its authenticated initial controls now have an installed
[current-asset adapter](TEXTURE_ACCUMULATION_2003.md), with native color,
shadow/specular, Original restoration and complete opening checks.
The construction receipts below retain their original historical scope; they
must not be read as evidence of a matched original GPU frame.

The resident pass inventory's single-combiner placeholders are not its runtime
programs. Reading the original cache's implementation `+10` field gives these
actual selections:

| Resident pass | Builtin passed to original `000964B0` |
| --- | ---: |
| `default_bump`, `anisotropic_bump` | 1 |
| `default_texture`, `default_texture_zwrite` | 3 |
| `light_utility_specular`, tangent-space variant | 6 |
| `light_utility_specular_dull`, tangent-space variant | 10 |
| `default_light` | 7 |
| Both default environment passes | 13, custom resident program |

The new read-only exporter authenticates those first four selections against
the unchanged `ec_interior_test.map` bytes and executes the complete original
constructor from `000964B0` to the boundary immediately before `00097BB4`.
No instruction or service is replaced. The fixture declares already-cached
bitmap bindings so bitmap allocation and GPU submission remain outside this
arithmetic experiment. These are declared inputs, not a captured game frame.

Across101 fixtures,548 distinct original instructions execute. There are50
distinct output programs: one bump program and49 texture programs across
clamped input pairs at original globals `0048F494/0048F498`. The original x87
control word is `023F`. Every instruction byte is checked against the pinned
May2003 executable. Program words and hashes are retained in
`analysis/pstencil/2003-presentation/accumulation-construction-v1/reference.json`.

The bump program's texture modes are `[1,17,17,14]`, with dot mapping `111`.
Its final combiner returns T3. The texture program at both input values1 uses
four general combiners and modes `[1,1,0,1]`. This establishes the constructed
programs; it does not establish every current material's coordinate, texture,
target or alpha crosswalk. Both named bump families selecting builtin1 also
does not establish a modern anisotropic specular response.

Original `000E8040`, `000E80B0` and `000E88D0` select targets9,11 and10,
respectively. The remaining implementation needs their actual vertex/binding
producers, target encoding and alpha ownership, completed utility/light
scheduling, and frame comparisons. These programs are reference evidence and
are not yet installed as a replacement accumulation renderer.

Reproduce without modifying the reference or replacing retained evidence:

```sh
.tools/python/bin/python tools/pstencil/extract_reference_accumulation_programs.py \
  --output analysis/pstencil/accumulation-construction-new/reference.json
```

See [presentation pipeline](PRESENTATION_PIPELINE_REFERENCE.md) for the default
layered renderer, target formats and the recovered environment response.

The [Chief visor companion](COMBINED_VISOR.md) now executes that recovered
environment response on current combined visor consumers, retaining their
actual bump/cube and current independent texture alpha. Its normalization cube
matches all 42 decoded original mip images. This is a supported current-asset
adapter; the complete original accumulation targets and alpha scheduling above
remain separate work.

The new read-only binding audit checks 12,526 decoded fields against the
original cache and retains 164 exported-pass implementation selections across
17 resident shaders. It records their actual bitmap/frame/override bindings,
literal render states and offsets, without treating resource presence as live
selection. Original `00096200` dispatches texture overrides 2, 3 and 4 to
targets 9, 11 and 10 respectively; the pinned jump table and target literals
are verified against the executable.

The original visor has both legacy and tangent-space lighting-model rows. Each
has three base-lightmap implementations. In the tangent-space implementation 2,
the base program returns its stage-0 alpha, and stage 0 selects texture target
11 through override 3. The legacy implementation 2 instead returns zero alpha.
Consequently, a universal assumption that every original base pass simply
copies a bitmap alpha is invalid. The actual frame's lighting model,
implementation selector and preceding texture program remain necessary.

Bindings are retained in
`analysis/pstencil/2003-presentation/accumulation-bindings-v1/reference.json`.
The scalar diagnostic is `analysis/pstencil/visor-response-v1/alpha-probe.log`;
it is arithmetic evidence, not a native original frame.

```sh
.tools/python/bin/python tools/pstencil/audit_accumulation_bindings.py \
  --output analysis/pstencil/accumulation-bindings-new/reference.json
```

The complete original constructor also now runs for resident utility/light
builtins 6, 7 and 10. The read-only exporter executes 560 declared fixtures,
authenticates 522 distinct original instructions and stops before GPU
submission. It produces 64 distinct specular utility programs, 64 dull utility
programs and 24 default-light programs across the explicit light-context,
context selector and renderer-global inputs. No instruction or service is
replaced. Selector fields keep their literal offsets; the producer/schema
mapping below must be carried through a verified current-asset adapter.

These programs are retained in
`analysis/pstencil/2003-presentation/light-construction-v3/reference.json`.
They are reference evidence, not newly installed lighting or a GPU frame.

The [native light-response component](LIGHT_RESPONSE_2003.md) now retains all
560 complete constructor outputs and passes 4,480 two-stage GPU arithmetic
fixtures. Its actual sphere image/vertex crosswalk also passes in Cairo and
the opening cache. The new component is separate from the installed campaign
renderer until the real targets, alpha, material attributes and stencil
consumers are scheduled and verified.

```sh
.tools/python/bin/python tools/pstencil/extract_reference_light_programs.py \
  --output analysis/pstencil/light-construction-new/reference.json
```

The matching primary tool's `light` schema is now identified at `007E2D58`,
with a 608-byte root. Its literal fields establish these authoring roles:

| Light offset | Original field | Authored choices |
| --- | --- | --- |
| `+DC` | Specular mask | Default, none, gel alpha, gel color |
| `+174` | Falloff function | Default, narrow, broad, very broad |
| `+176` | Diffuse contrast | Default linear, high, low, very low |
| `+178` | Specular contrast | Default one, high linear, low, very low |
| `+17A` | Falloff geometry | Default, directional, spherical |

The read-only audit fully consumes 69 original loose light files, including
the plasma grenade light. Another 80 use a serialized form unsupported by the
strict reader and remain listed as unresolved. No bytes are reinterpreted to
make them pass. The schema and every decoded field retain original offsets and
hashes in `analysis/pstencil/2003-presentation/light-tag-fields-v1/reference.json`.

The primary producer now establishes the context ownership. `render_lights`
at `000F8050` obtains the definition through the typed `ligh` getter at
`000F81E9`, passes it as argument 3 to `render_dynamic_light`, then
`rasterizer_light_begin` at `000A4C50` stores that same pointer at `007438A4`.
The constructor's `+DC`, `+176` and `+178` reads therefore consume those light
fields. The builder `000A3CF0` also supplies the completed light constants.
All 13 exported function spans match the unchanged original executable bytes;
exports and provenance are under `light-context-v4/`.

`007438A8` comes from the first word of the live light geometry record. It is
not the shader's lighting-model row. The latest fixtures retain its literal
context name and cover all four accepted values; `000952C0` distinguishes zero
from the other three. Earlier construction artifacts retain their initial
labels with correction notes; their program words remain evidence, not a
semantic authority.

Current-tag crosswalk, complete original target/alpha scheduling and GPU frame
validation remain open. This evidence must not be treated as an armor gloss
preset or installed GPU light-response change.
