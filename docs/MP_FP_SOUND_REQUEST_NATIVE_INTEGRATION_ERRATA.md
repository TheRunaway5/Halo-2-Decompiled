# FP sound producer instruction-count correction

The completed first checkpoint's integration document says 767 instruction
rows for eleven bodies. The correct sum is **667 rows in each executable**,
or **1,334 exact byte-verified rows across the 22 old/u5 listings**. The audit
already records the correct 1,334 count. Per-body row counts are
38/33/26/52/57/68/137/184/18/25/29 in the document's table order.

This arithmetic correction does not change an instruction, input, native
output, body correspondence, provider or test result. The first checkpoint,
its exact document/source snapshot and manifest are retained unchanged.
`final-manifest.json` pins this correction and the completed stdout logs,
which were still being written when the first manifest was generated. Use
that final append as the final checkpoint identity.

The other correction, already captured in the main integration document,
remains essential: original46FB64/u5474C24 is the runtime active BSP word,
not an immutable location sentinel. The native producer requires it as a
live caller input; it never substitutes the fresh-image FFFF value.
