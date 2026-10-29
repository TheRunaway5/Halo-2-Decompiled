# Native speech cluster mask

This closes `203C00`, the cached cluster-mask owner used by real `203D80`
speech dispatch, and its actual `252F30` visibility predicate. It reuses the
sealed semantic `253070` cluster scale and `2531E0` legacy byte algorithms from
`native_campaign_audio_listeners.hpp`. No generated routine, virtual CPU,
backend result or visibility success value is supplied.

The API is `include/halo2/native_campaign_ai_speech_cluster_mask_tu15.hpp`.
`cmake/CampaignNativeAiSpeechClusterMask.cmake` adds the source and its original
reference test. It does not enable whole speech dispatch or an audio group.

The predicate preserves full signed32 equality/ordering, the modern visibility
bit bank, signed triangular indexing and the fallback's signed WORD narrowing.
That narrowing matters when distinct full32 inputs have identical low words.
The legacy predicate consumes bit7 of the actual byte rather than choosing an
admission result.

The parent reads the real WORD cache key at `502240` before resolving structure
owner `4ECC18`. On a miss it clears actual DWORDs at `502200`, rereads the live
cluster count, tests each candidate, and publishes only strict scale values
below40. It retains the source's low16 signed candidate progression and fresh
loop-bound reads, then writes the requested WORD key last. It neither seeds
nor repairs the cache. Existing native final-startup storage already owns the
64-byte bitmap and two-byte key; this change introduces no alternate backing.

The shared geometry algorithms execute under one native SIMD environment. This
keeps sticky exception flags across child calculations and the parent's final
comparison. The existing dependency contract represents immutable literals by
authenticated compiled constants; the original image's corresponding words are
checked independently. Every retained mutable-data read and write, plus the
parent's threshold load, is compared in exact order with its before/after data.
The caller's extended operands, control word and status remain unchanged.

The original-reference matrix has121 unchanged physical i386 calls, all201
sites in the closed four-owner graph,26,198 ordered reads,2,317 stores and232,961
instructions. Cases include cache hits with an unavailable structure pointer,
signed indices, wrapped triangular products, low-word equality, legacy bit7,
modern scale sentinels, threshold equality, NaNs, DAZ/FTZ and rounding modes.
Alias cases overlap the live cluster count, lower scale, visibility bank or
legacy byte table with the bitmap. A mapped513-cluster stress case preserves
the source's overlap with its adjacent key; it does not enlarge production's
existing512-cluster backing.

Seven mutants reject rebuilding on a cache hit, omitting the last cluster,
accepting equal/unordered thresholds, logical mask-word shifts, using bit0 in
place of bit7, retaining the initial count across clearing, and nonzero clear
publications. The original invokes its real child owners throughout; no return
or intermediate geometry result is supplied.

Two actual `203D80` block receipts begin at `203E8B`, call
`203E9A -> 203C00 -> 203E9F`, and naturally execute the returned-pointer store
at `203E9F` before stopping at `203EA3`. This bounds qualification to the typed
mask pointer, retained state and FP contract. Arbitrary SIMD scratch is outside
that interface; no generic register ABI override is enabled. The preceding
dispatch body and later actor/control calls remain separate dependencies.

Evidence is under `analysis/campaign-native-speech-cluster-mask-20261028/`.
Mapped input owners and masked SIMD traps are required. Unmapped-input faults,
arbitrary source-stack aliases, generic scratch-register ABI, whole `203D80`,
`215840/218190` retirement and `116D60/117240` submission remain unqualified.
