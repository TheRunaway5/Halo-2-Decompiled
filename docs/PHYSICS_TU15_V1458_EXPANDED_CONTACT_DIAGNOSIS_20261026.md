# TU1.5 v1458 expanded-field contact diagnostics

The strict raw-bank comparisons remain **RED**: Gemini column0 and column1 each differ by 129 bytes in 20 ranges; Elongation crates differs by 27 bytes in 11 ranges. This investigation changes no comparison exclusion, expected bank, native output or acceptance gate.

The [aggregate report](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/expanded-contact-diagnosis/report.json) links all original hardware observations and authenticated source listings. Each observation uses the exact existing input capture, unchanged English TU1.5 instructions and GDB hardware breakpoints/watchpoints. Complete 1,077,248-byte heap dumps must equal ordinary original execution from the same input before any lifetime conclusion is accepted.

## The earlier no-reuse assumption does not apply

| Capture | Actual contact buffer | Original lifetimes | Observed counts/capacities before free | Whole original heap SHA-256 |
| --- | --- | --- | --- | --- |
| Gemini column0 | `10082380`, size `400` | Allocate/free twice | `6/8`, then `6/8` | `ee0cd614927e4024eea974986259f4e1c34b3414fafc0ec9d96bdd0ad45fdff9` |
| Gemini column1 | `10082380`, size `400` | Allocate/free twice | `6/8`, then `6/8` | `ee0cd614927e4024eea974986259f4e1c34b3414fafc0ec9d96bdd0ad45fdff9` |
| Elongation crates | `1007E380`, size `400` | Allocate/free twice | `8/8`, then `7/8` | `8893e772656a7c833bcefecc431b1b31921525884f0fae54ccb557f17f2afcc7` |

Each buffer is reused within the same frame. Its payload becomes stable only after its **last** free return through root return. Allocator metadata in the final native and original heaps agrees. The first Gemini probe rejected the old assumption that a first free implied no later consumer.

Gemini0/1 have identical metadata and ABI, but seven of 4,033 input banks differ across the 91,356,057-byte input. [The identity report](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/gemini-column-input-equivalence.json) therefore rejects direct proof reuse. Column1 received its own actual original execution and all four observations.

## Gemini: invalid record payload with observed consumer guards

Both original frames copy twelve records through actual `303BA0`, with two genuine stack-capacity-four to heap-capacity-eight growths. Every final differing byte is in a record whose valid byte at `+7D` equals zero in both native and original output. Both actual consumer gates, `304858` and `3049A0`, observe twelve records with valid byte zero. Authenticated source branches skip the corresponding contact paths for those observations.

The source constructor initializes only its two flags before copying an uninitialized local record. Its explicit copy omits `+28..2F` and `+7E..7F`; bulk array growth copies the whole stride. A selected `+0C` lane watch observes genuine bulk-copy and record-swap accesses. The case-specific findings replace the earlier no-reuse hypothesis; they do not establish all byte/register consumers or paired native lifetime history.

## Elongation: a true contact also has differing gap bytes

Elongation cannot use an invalid-record-only classification. Final record0 is a true contact in both outputs. Its ten differing bytes are `+28..2F` and `+7E..7F`. The other seventeen differing bytes lie in final invalid records1..5, at `+2E`, `+4C` or `+7E`. Records6/7 also retain true flags but have no raw differences; final capacity is not the same as the source loop's observed active consumer count.

Twelve exact hardware watch groups cover **all 27 differing bytes** across both actual lifetimes. Each group's complete original heap matches the ordinary reference. The observations show only REP MOVSD destination writes and source-identified MOVAPS record-copy/swap loads or stores. The true contact's differing gap bytes receive bulk-copy writes; no source read of those bytes occurs in this captured frame. The `+4C` lanes also pass through actual `304130` record swaps, so their final invalid flag alone would have been insufficient evidence about earlier consumers.

Two authenticated straight copy regions, `304792..30482B` and `3041C7..3042C7`, carry the loaded XMM3 value to a record store without a branch, call or other read of that register. The report retains the complete source instructions and checks these local use chains. Register liveness after those stores was left open by this investigation. The subsequent [captured-frame XMM3 audit](PHYSICS_TU15_CONTACT_XMM3_LIVENESS_20261026.md) now follows the three exact frames through actual callers to real overwrites; all-input and future-frame memory provenance remain open.

## Remaining gates and storage

All three raw gates stay red. Paired native allocation/cohort history, complete register/byte-consumer closure across calls and future frames, and the full native physics migration remain open. These observations apply to these exact captured frames, not every map, contact or future tick.

The observer runner authenticates the original capture metadata, all before/after banks, executable and completed ordinary output before reusing that same case's reference across observation modes. It preserves the original failing comparison result. Different cases never reuse an ordinary execution. This avoids repeated compiler products while retaining independent hardware observations.

Every raw replay, ELF, object, heap dump and debugger script lives under `tools/run_test.py` scratch. The completed owners and case subdirectories were checked absent. Small reports, logs, source listings, hashes and required diagnostic source versions remain. Existing failure inputs are protected and have not been removed or overwritten. No newly generated large artifact is retained; scoped cleanup measurements are in `expanded-contact-diagnosis/cleanup.json`.
