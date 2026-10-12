# Model palette and transparent part centers

`source_model_palette.hpp` ports the resident, zero-compound path of original
`137800`, including the complete `1376C0` base palette producer. It consumes
explicit actual WORLD nodes in model order, literal inverse binds, source
geometry LOD, and one selected-section record per original region. WORLD
nodes must already include any caller `BD610` composition.

`1376C0` composes each WORLD transform with its inverse bind through `142A60`,
then writes three float4 rows. Uniform scale is applied to the nine basis
components; translation occupies row components 3, 7, and 11. The caller
cannot replace this operation with a bounding sphere center or reconstruct
these WORLD transforms from a rounded skinning matrix.

`137800` preserves region ordinals and NONE sections. A literal resident node
map appends copies from the complete base palette in authored byte order.
Repeated selected sections therefore append repeated maps. Without a map,
the range starts at zero and uses the literal section+24 count, or
`min(model_node_count,40)` when that count is zero. Model mask4 first collapses
the output base bank to its first matrix, then appends mapped region banks.

`45CE0` uses the rigid node's matrix translation for ordinary models. With
model mask4 it reads the actual region range start from context+4, ignoring
the rigid node ordinal. Rigid NONE always reads the first palette translation.
`source_model_palette_part_center` implements precisely that choice. Depth
bias, selected shader LOD, visibility, collector tokens, and command order
remain separate explicit producers.

The Cache definition loader retains raw mode flags, region count, inverse
binds, and the six actual compound counts. The transparent section loader now
also retains the literal resource100 node bytes and section+24 fallback count;
it does not expand an empty map into an inferred identity map.

Untouched original `137800 → 1376C0 → 142A60` matches 1,891 cases and 208,917
output words. This includes 1,188 literal model/LOD/selected-region cases from
all 21 maps and the entire retained 33-node Zanzibar player WORLD bank.
The retained player case explicitly selects NONE regions as an arithmetic
control; it does not assert original draw absence. The complete base bank is
checked even when mask4 later collapses it. No source callee is replaced.
The resulting context is also fed through untouched `45CE0 → 1D6B0` for
9,576 actual center selections, covering rigid NONE/zero/last-node inputs and
the mask4 region-start interpretation.
The replay explicitly supplies a post-`BD610` WORLD bank and synchronous
residency (`4E6494=0`).

The independent asset fixture checks 193 model definitions and 328 selected
resident sections covering the 741 automatic SCEN/MACH/BLOC owner census.
Four Zanzibar scenery selector records remain retained diagnostic choices,
not a restored production RNG history. Every admitted census LOD has zero
compound nodes. The helper rejects nonzero compounds, missing WORLD nodes,
invalid maps, and wrong region-bank sizes.

The normal and ASAN/UBSAN tests (including leak detection) pass. Tools and fixtures are:

- `tools/differential_source_model_palette.py`
- `tests/fixtures/retail_source_model_palette.tsv` and its provenance
- `tests/fixtures/retail_source_model_palette_assets.tsv` and its provenance
- `retail_source_model_palette` and `retail_source_model_palette_assets`

These prove matrix arithmetic and metadata extraction. Native pose/controller
equality, asynchronous residency transitions, compound nodes, successful
registration chronology, actual current collectors, and global transparent
queue emission order remain outside this proof. Production scene modules do
not yet supply this palette.

## Weapon compound matrices

The native weapon loader now retains the selected LOD's literal 16-byte compound
records. `append_source_model_compounds` follows the complete `137800` loop:
ascending influences, byte255 terminator, three authored weights, and the fourth
weight from one minus the accumulated total. Every matrix component is separately
multiplied and added before appending the result to the palette. Beam-rifle FP
vertices refer to six of these matrices beyond the four ordinary skeleton nodes.
`tools/check_weapon_compound_palette.py` runs untouched `137800/1376C0/142A60`
with actual beam-rifle records and declared WORLD arithmetic controls. The
`original_weapon_compound_palette` test compares all resulting words against the
retained executable outputs. Full original pose/admission/residency history is
not implied by this arithmetic comparison.
