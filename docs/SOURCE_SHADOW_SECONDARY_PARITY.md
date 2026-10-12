# Secondary shadow queue classification

`source_shadow_secondary_classify` ports complete original `165A20` over
explicit current owner, definition/model metadata and mutable queue banks.
The untouched executable differential checks **5,181 cases / 305,627 exact
output words**. There are 20,785 changed flag records; untouched row fields
and inactive storage are checked as well. No native shadow activation is
introduced.

The authority is `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The complete `165A20..165BB4` byte range has SHA-256
`8c1dcff227df37fcfbae0cbb93dcdd18b5727bfe7691a86974fe797d4604b06d`.
Files are [source_shadow_secondary.hpp](../include/halo2/source_shadow_secondary.hpp),
[source_shadow_secondary.cpp](../src/source_shadow_secondary.cpp),
[source_shadow_secondary_tests.cpp](../tests/source_shadow_secondary_tests.cpp),
and [differential_source_shadow_secondary.py](../tools/differential_source_shadow_secondary.py).
The frozen fixture is `tests/fixtures/retail_source_shadow_secondary.tsv`.
Its companion provenance and
`analysis/multiplayer-shading/shadow-secondary-original.json` preserve source
execution, captured-field hashes and temporal limits.

## Complete original decisions

The fixed `32350` caller passes the actual root, receiver byte0 and type mask0.
Other explicit argument values are verified as controlled source branches.
The helper does not construct an admitted queue or choose a root.

| Source branch | Original effect |
|---|---|
| Root datum NONE | Return without reading owner, tag or queue fields |
| Current root definition+38 is model NONE | Write cluster queue count0 only; retain every underlying row and the object/instance queues |
| Object row ID equals root; current hlmt+4 is mode NONE | Zero the root row's entire 16-bit flag word |
| Root row has a render model | Preserve its incoming flags |
| Root receiver byte is0 | Additionally clear root bit`0x1000`; every nonzero byte preserves it |
| Other object row | Clear bit`0x2000`; clear bit`0x1000` as well unless the supplied type-mask bit is set or object+4 bit31 is set |
| Active cluster/instance row | Clear bit`0x2000` only |

Object type is the literal byte at `+AA`. The original x86 shift masks it
with31; values32..255 are not converted to a guessed object category. The
other owner's flag word is read only after the type mask misses. No definition
lookup is performed for other owners, and the root's flags/type are unread.
The root model's render-model field is read only if a queued row actually
equals the root. No parent-root traversal, geometry bound or material-class
inference occurs in `165A20`.

Queue count is the original signed short at header+4. Negative and zero
counts skip that queue while preserving storage. Positive counts process
only the supplied prefix. IDs, the other/token stream and association words
remain unchanged; row order is preserved. The native API separates signed
count from mutable resident storage so the model-NONE branch can clear the
count without erasing retained bytes.

## Independently retained root row

The immutable OpenGL/llvmpipe phase `32875-secondary-admission` is immediately
before the original `165A20` call. The next retained
`32893-after-part-admission` phase follows `165A20`, `2CBC0` and `44720`.
Both are tick12475, view0, with actual root `E2E60077`. The single object row is:

| Field | Before | Computed `165A20` | Later retained |
|---|---|---|---|
| Object count | 1 | 1 | 1 |
| Full owner ID | `E2E60077` | `E2E60077` | `E2E60077` |
| Flags | `F000` | `E000` | `E000` |
| Other/token | `00000000` | `00000000` | `00000000` |
| Association | `FFFF` | `FFFF` | `FFFF` |

All five object count/row words match independently retained outputs. The
root's current definition datum comes directly from its `32875` body.
Definition `EDF10C6B` model `EDF20C6C` and hlmt render model `E9B90844` come from
the later read-only `3290A` complete metadata prefixes. Those prefixes equal
the literal Zanzibar cache and two earlier retained RAM banks, as recorded in
`fade-metadata-3290A/comparison.json`; the definition datum also matches the
later owner. This is explicit temporal corroboration, not a simultaneous
metadata export at `165A20` entry.

Actual cluster/instance queue streams were not independently exported at
these phases. The root-row control supplies explicitly empty controlled
side queues. The comparison does not claim full live secondary queue
reconstruction or an immediately captured `165A20` exit. The later object row
is observed after the additional part-admission calls, and the original20
Vulkan framebuffer remains a distinct capture.

## Validation and remaining producer services

The oracle executes unchanged `165A20`, including actual source object/tag
table lookups and queue mutations over explicitly relocated valid banks.
Every executed instruction is checked against the pinned executable. There
are no lookup hooks, branch patches, return values supplied as services,
guest I/O or advancement. Controlled cases cover all 256 type bytes, both
flag31 states, type-mask hits/misses, repeated root rows, model/mode NONE,
nonzero receiver bytes, signed counts and inactive queue tails.

Regular and ASAN/UBSAN checks with leak detection enabled pass all 305,627
words. Nine native input-rejection controls and five unread-field/early
branch controls also pass. CTest is `retail_source_shadow_secondary`.
Absent source-required fields, duplicate/missing lookup identities and
positive counts beyond supplied storage reject unresolved native input;
these checks do not claim original crash or rollback behavior.

Classification is now an available pure service. Native gameplay must still
produce the actual secondary `1320F0` context/queues and current metadata,
then the ordered `2CBC0/44720` draw units, admitted resources and completed
material bindings. Main-mode0/auxNULL admission helpers cannot stand in for
the secondary producer. Original owner registry/cache/history services and
mode0 six-camera shadows remain separate. See
[SOURCE_NATIVE_SHADOW_ACTIVATION.md](SOURCE_NATIVE_SHADOW_ACTIVATION.md).
