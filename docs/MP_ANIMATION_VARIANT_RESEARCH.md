# Original animation variant selection and shared RNG

The new native variant owner preserves the complete `1CEDA0 → 1E1020` selection, authored linked variants, both caller-owned RNG words, and the order of required resource calls. It never substitutes a fixed packed animation or initializes a seed. The separate freeze is `analysis/multiplayer/integration/animation-variant`; earlier constructor, animation, grenade, audio and release freezes remain unchanged.

## Source and execution boundaries

Authority is the user's disc 1.1 `.03` XBE, SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`, and Lockout, SHA-256 `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`. The authenticated user-owned u5 XBE is `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

The unchanged original constructor/selection preparation executes first. Its complete selection calls and stores equal the earlier frozen `spawn-source/animation-selection/measured/report.json`; the new observer adds no guest fields, service return, selected animation or seed. Nine actual `1CEDA0` owning calls are retained. The subsequent 132 explicit pure inputs enumerate every linked authored animation in the Spartan, first-person SMG and first-person BR graphs, for each stream selector. These inputs are declared caller arguments, not chosen outputs. All return through original instructions. The recorded 1,233 calls and 5,280 stores retain complete nested resource returns, manager buffers, both RNG words and ordered accesses.

The original preparation's bounded seed/resource/clock/compatibility context remains an inherited fixture input. In particular, an older preparation supplies the two-word RNG backing; this study neither owns its initialization nor treats that value as a live default. Genuine ordinary multiplayer initialization and global ordering with other consumers remain separate obligations. Resident resource delivery is inherited and explicit; the original `1E13C0/124E40` instructions execute, but this is not a proof of ordinary asynchronous resource startup.

The additional observer runs the unchanged 519-tick idle/aim/movement/fire/full-reload/empty-reload schedules. Its result fields equal the older `spartan-equipped-unit-phase-suite` for every schedule. The observer's extra Cache parser reads and verifies metadata only; it writes no guest state. This retains those old preallocated-player/inventory/input/clock/service boundaries and does not promote them into natural live gameplay. Observer-only failed attempts are preserved separately, with no authoritative report substituted for them.

## Complete-body u5 correspondence

Both original images are read without patches. Unique full-body correspondence includes every internal branch, instruction and relocated memory operand:

| Disc 1.1 | Authenticated u5 | Instructions |
| --- | --- | ---: |
| `1CEDA0` | `1D3660` | 29 |
| `1E1020` | `1E58E0` | 114 |
| `1E0FC0` | `1E5880` | 33 |
| `1DE890` | `1E3150` | 52 |
| `1DE9B0` | `1E3270` | 17 |

The resource call maps `1E13C0 → 1E5C80`; its complete owning implementation is a separate provider, not established by this five-body comparison. Root's real metadata provider and the independent request/resource study retain that distinction.

| Original word | Disc 1.1 VA | u5 VA | Raw bits |
| --- | --- | --- | --- |
| Float one | `465030` | `46A0D0` | `3F800000` |
| RNG scale | `4650C0` | `46A160` | `37800080` |
| Unsigned conversion correction | `465028` | `46A0C8` | `4F800000` |
| Runtime two-word RNG pointer | `4EE9D8` | `4F3CD8` | Runtime-owned pointer |

The scale is the original word; replacing it with `1/65536` changes behavior. Fresh image zero at the RNG pointer is not a seed. `OriginalData.unit_animation_constants` imports the three immutable words; no literal fallback is permitted.

## Variant and resource contract

`1CEDA0` reads the **complete manager** at EDI: graph datum at `+68`, byte `+6C bit1`. Bit set selects the first RNG DWORD; clear selects the second. Packed high-word `FFFF` or a null resolved graph leaves the input unchanged, without resource or RNG work. The output is written only after the required owner returns. An outer channel offset is not a replacement manager base.

`1E1020` resolves the actual definition with `1DE890`, including its real resource call. Animation-bank count/pointer are graph `+2C/+30`, records are `6C` bytes. Record `+1C` is the float cumulative threshold; signed shorts `+30/+32` are the chain head and next index. Packed low-word `FFFF` uses the master graph; other signed indices select graph `+4C/+50`, inheritance stride `20`, graph datum at row `+4`.

An unlinked record invokes two resource calls: the initial definition resolution and final selected request. Linked records also use `1E0FC0`, whose two definition resolutions retain their resource calls: four calls total in these measured linked cases. Each final request still uses the original master graph and packed identifier. Native missing-resource ownership throws at its first actual invocation; it does not return success or silently preserve the old animation.

The head threshold is compared to original one using `COMISS/JBE`. Equal or unordered values skip RNG. Otherwise exactly one selected-word update occurs:

`after = before * 0019660D + 3C6EF35F` modulo `2^32`.

The source store is `1E10D1` for word0 or `1E10EF` for word1. The high16 integer is multiplied by the actual source scale with x87 and rounded at the original float32 store. The native double product followed by float32 store preserves this bounded exact product. The unsigned correction branch is retained as source evidence but cannot execute for the shifted unsigned32 high16 operand.

Chain traversal tests ordered `threshold >= random`; unordered thresholds advance. The final next-less record exits without a threshold comparison. The native owner retains signed index arithmetic and requires actual backing for its reads; it does not fabricate bounds fallback behavior. Measured graphs contain no inherited-row selection, so the inheritance branch is complete-body source-backed implementation, not a newly measured positive inheritance case. Invalid/unavailable backing remains an explicit failure.

## Native API and validation

[animation_variant.hpp](/home/eric/Developer/Halo-2-Decompiled/include/halo2/animation_variant.hpp) exposes `choose_source_animation_variant` and complete-manager `select_source_animation_variant`. Required services are actual `SourceAnimationGraphMemory`, three imported executable words, caller-owned two-word RNG state, and complete `1E13C0` resource ownership. An optional observer receives the ordered source RNG store. No runtime, Unit, Simulation, renderer, shader or release integration is included here.

Root's `UnitAnimationMetadataStore` retains the actual Cache, full graph packets and stable definitions. The registered native test reads that real provider and `OriginalData` for both 1.1/u5, checks every captured graph region, then compares every selected packed value, both seed words and the combined ordered resource/RNG stream. Resource test callbacks replay complete **recorded** source boundaries; they are not a successful live resource provider.

The canonical native fixture has 150 cases, 564 ordered resource callbacks, 132 RNG stores and 88 changed packed selections. The independent exporter reproduces the fixture bytes exactly. The standalone normal and Clang AddressSanitizer/UndefinedBehaviorSanitizer runs pass. Copied root normal and GCC sanitizer binaries also pass using the actual Cache and both executable providers. Separate ordinary-clock exports add 246 cases and 492 resource callbacks; all pass with both providers in both builds. Aim/movement schedules produce no variant invocation and retain explicitly empty exports. The six schedules capture 615 source calls over 519 ticks, with no additional variant RNG store. Independent original and u5 repeats are byte-identical; clock repeats are separately audited. None of these exports overwrite the canonical fixture.

`tools/audit_animation_variant.py` verifies original listing bytes, cache regions, earlier source pins, unchanged repeat pairs, complete call returns, LCG stores, clock correlations and exported fixture identity. The checkpoint manifest pins exact tools, module/test snapshots, observed build inputs and copied test binaries. It records observed source/archive provenance without asserting that every linked archive was freshly built from the current dirty checkout.

## Reproduction

Run new original observations into unused directories; the tools reject differing existing output:

```sh
PYTHONDONTWRITEBYTECODE=1 .tools/python/bin/python tools/oracle_animation_variant_1_1.py --output /tmp/halo2-animation-variant-new
PYTHONDONTWRITEBYTECODE=1 .tools/python/bin/python tools/oracle_animation_variant_clock_1_1.py --output /tmp/halo2-animation-variant-clocks-new
PYTHONDONTWRITEBYTECODE=1 .tools/python/bin/python tools/research_animation_variant_u5.py --output /tmp/halo2-animation-variant-u5-new
.tools/python/bin/python tools/export_animation_variant_fixtures.py /tmp/halo2-animation-variant-new/report.json /tmp/halo2-animation-variant-new/fixtures.txt
ctest --test-dir build-animation-metadata-store-20261008 -R '^original_animation_variant$' --output-on-failure
```

The remaining live gate is a caller that owns the true manager, both RNG words in original order, real metadata and complete resource producer. A universal unchanged-packed callback, preselected animation, supplied success, or fixed seed cannot satisfy it. This freeze proves the variant owner and its bounded comparisons, not completed multiplayer animation/resource/RNG parity.
