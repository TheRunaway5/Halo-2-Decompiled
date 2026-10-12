# Retail posed object contact

`source_object_posed_target` implements complete retail `1EFF40` and the
`168B40` world-plane conversion after the original `1EFB40` lookup succeeds.
The input contains resolved collision metadata, actual current WORLD matrices
and active region bytes. It queries the complete segment independently of the
outer object's current closest fraction. Native owner lookup, registration,
animation and active-permutation production remain separate requirements.

The implementation preserves `1EFBD0`'s signed selection mapping and null-active
zero-selection branch, `1EFCA0`'s region/permutation/BSP order and low-byte node
selection, `141590`'s inverse matrix arithmetic, and `1EFF40`'s scalar local
vector operation order. It uses the retail SSE collision walker instead of
the separately proved title-update collision arithmetic. Every accepted lower
hit updates the iterator's shared fraction, including an equal accepted hit.
The caller's `169510` service then applies its separate strict-closer test.

`168B40` transforms the crossed geometric plane retained by the lower query.
The surface's plane field supplies its orientation and copied result field;
its low15 index does not replace the geometric plane pointer. The public
`source_collision_world_plane` also preserves the null-matrix branch and
original zero-minus reversal. Global material mapping uses the resolved model's
literal material row, as `D8AE0` does after successful lookup.

`tools/audit_source_object_posed_target.py` executes unchanged original
`1EFB40`, `1EFF40`, `141590`, `1DE6D0`, `168B40` and `D8AE0` against the immutable
Zanzibar RAM export paused at `12B33E`, tick12,474. The fixture freezes all
39 resolved model collision banks, 158 actual current owners, their complete
active byte banks and exact WORLD matrices. Object `+114` is a signed byte
length; dividing it by52 yields the actual node count. No neighboring memory
is exported as extra nodes. The original has 44 lookup misses among202 owners.

The 4,740 diagnostic segment queries and16 explicit data controls produce
4,436 hits and71,296 exact defined words. The controls deliberately make the
surface's low15 plane index differ from the crossed plane and independently
exercise both orientations. The retained authored corpus has matching plane
indices, so these controls prevent that coincidence from hiding the contract.
All1,029 executed instruction sites match the supplied original XBE. All223
protected owner/spatial banks remain unchanged. Controlled surface bytes are
restored and checked against the immutable export. No original service is
replaced; no guest input or advancement occurs.

These are declared diagnostic queries through retained owner spheres, rather
than captured game query scheduling. Missing/invalid consumed native banks
raise explicit errors; those guards do not reproduce malformed original memory.

The registered check is `retail_source_object_posed_target`. Normal and
address/undefined-behavior validation, with leak detection, are retained in
`analysis/multiplayer-shading/posed-target-native-validation.json`. The
independent peer review and retained plane census are recorded alongside it.
