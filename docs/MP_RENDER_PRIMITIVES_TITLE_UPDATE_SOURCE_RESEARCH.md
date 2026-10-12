# Render primitive correspondence through Xbox TU1.5

The retail RGB packer, vertex extern consumer and seven-descriptor state
consumer have matching complete owners in the owned TU1.1 and TU1.5 images.
The new evidence checks18 whole-owner comparisons across nine routines,
including internal branch topology, both value-provider switch tables,
the relevant direct-call destinations and referenced read-only bytes.
The seven descriptor rows and92 immediate-state methods are byte-identical.
This is static original-code correspondence; no title-update execution,
resource readiness or framebuffer result is inferred from the retail corpus.
[Reproducible source inspector](../tools/inspect_render_primitives_title_updates.py),
[complete comparisons and literal tables](../analysis/multiplayer/game-variants/render-primitives-title-updates-20261011/report.json)

The three pinned sources are retail disc SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`,
owned map-pack TU1.1 SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`,
and owned u5English TU1.5 SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Masked prefixes only locate candidates in each actual executable section.
Complete recursive decoding, rather than a reused address or matching prefix,
decides acceptance. Identical truncation leaves exist in more than one section;
the accepted SDK leaf is constrained by its real line-width caller.
[Source image pins, candidate locations and call constraints](../analysis/multiplayer/game-variants/render-primitives-title-updates-20261011/report.json)

| Owner | Retail | TU1.1 | TU1.5 | Complete instructions |
| --- | --- | --- | --- | --- |
| Vertex extern | `19BE0` | `19BC0` | `19BC0` |66|
| Scalar/color value provider | `1B2C0` | `1B2A0` | `1B2A0` |167|
| Render-state extern | `1A170` | `1A150` | `1A150` |646|
| RGB pack | `131ED0` | `133640` | `1349E0` |27|
| Immediate state packet | `3F5CF0` | `3FC330` | `3FFDD0` |14|
| Fog-color SDK packet/cache | `3F5370` | `3FB9B0` | `3FF450` |28|
| Line-width SDK packet/cache | `3F5730` | `3FBD70` | `3FF810` |34|
| Truncating float conversion | `3FCF00` | `403540` | `406FE0` |2|
| Material-bank reset | `16B10` | `16AF0` | `16AF0` |30|

All27 complete instruction listings and their hashes are preserved. Each
comparison records every relocated image operand. Referenced `.rdata` bytes
match exactly, including RGB's binary32 constant255, line width's8/.5 and
the value provider's constants. Its two switch tables preserve widths,
counts and target instruction indices. The report constrains19BE0's call
to the actual1B2C0 counterpart, the state consumer's relevant RGB/immediate/
fog/line SDK calls and the line-width conversion leaf.
[All whole-owner listings and comparisons](../analysis/multiplayer/game-variants/render-primitives-title-updates-20261011/report.json),
[retail vertex contract](MP_CAMOUFLAGE_BINDER_SOURCE_RESEARCH.md),
[retail RGB execution](MP_STATE_RGB_PACK_SOURCE_RESEARCH.md)

| Literal input | Retail | TU1.1 | TU1.5 |
| --- | --- | --- | --- |
| Seven render descriptors | `467020` | `46E4C0` | `473580` |
|92 immediate method words | `43E6F0` | `444D30` | `4487B0` |

The descriptor rows remain
`4B000000 4B000100 3D000202 4D000301 4E000401 9D000501 8A000600`.
The later addresses are recovered from the corresponding complete state-owner
instructions; both full byte spans are independently compared with retail.
Thus these title updates preserve the recovered seven-row indexing and
conversion selection, including RGB/alpha separation and the state138/157
raw-cache contracts. This does not admit adjacent texture-state descriptors
as render-state choices.
[Actual table addresses and full literal bytes](../analysis/multiplayer/game-variants/render-primitives-title-updates-20261011/report.json),
[separate retail generic state execution](MP_RENDER_STATE_EXTERN_SOURCE_RESEARCH.md)

Mutable D3D cache, material-bank, provider/default pointers and resource state
remain title-specific live inputs. Equal initial executable data does not
establish their state after startup. Capacity-exhaustion/device progress and
other downstream services are outside the executed retail fixture boundaries;
this static comparison supplies no successful service outcomes. The periodic
and texture-dimension value branches remain outside the admitted native vertex
seam despite their matching static owners. Fog scalar39/color41 providers,
whole title-update16B90, authored resources and world/render phase order are
separate source tasks. Existing retail execution fixtures remain unchanged.
[Retail captured-service composition and its limits](MP_CAMOUFLAGE_PASS_SERVICES_SOURCE_RESEARCH.md),
[fresh byte-identical comparison reproduction](../analysis/multiplayer/game-variants/render-primitives-title-updates-20261011/reproduction.json)
