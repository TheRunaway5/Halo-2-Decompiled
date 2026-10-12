# Power-module retained-support/simplex owner

The new native `source_physics_gjk_1_1` executes the original `30AE40` owner over real retained shape/support/relative-transform storage. It retrieves encoded vertices, computes new shape support points, reduces the simplex and emits its result vector. The actual two canister terrain children compose through the preceding native `2E14E0 → 2F4BD0` owners without replaying a finished workspace or contact. This is a bounded CPU owner, not a live canister collision, solver, damage or explosion implementation.

The reciprocal-estimate boundary is significant. Original `RSQRTSS` bytes execute in the Unicorn emulator; this is **not verified Xbox/Pentium III hardware evidence**. Exact comparison tests inject only that measured instruction estimate. Production accepts a required real platform estimate provider. Actual host x86 estimates are separately measured and differ; the second canister child's refined vector also differs by one ULP. General Xbox and contact-decision parity remain unverified at that boundary.

No Scene, SourcePropScene, Simulation, runtime, renderer, shader or release code changes. The earlier [narrowphase-owner freeze](MP_POWER_NARROWPHASE_OWNERS_RESEARCH.md), all its files and all ancestors remain immutable.

## Original execution and repeat

[The new oracle](../tools/oracle_mp_power_gjk_owners_1_1.py) verifies the preceding 67-file manifest and its ancestors. It inherits the unchanged actual physical-tag initialization, authored Lockout power-module allocation, real player/PVS producer and ordinary/world/dirty schedule. It compares the complete preceding narrowphase report and original power schedule before publishing. No new field, tick, position, pose, shape, pair, contact, activation, admission or successful callback result is assigned.

The original TU1.1 executable is `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`. The canister is actual authored Lockout row 1, `power_module` definition `E6F80583`, retained object `E26F0000`. Its original factory/body/PVS/world producers and their preparation/platform/stored-clock boundaries remain those of the preceding freezes. Resident cache geometry owns the convex and BSP triangle inputs. This is not complete title startup or continuous simulation.

[Measured report](../analysis/multiplayer/sandbox-source/power-gjk-owners/final/measured/report.json), SHA-256 `13dca353f10d75b3e9211a220f177104c300301d0c368a969a4dd139d37954c2`, contains 31 calls, five full retained states, 3,943 reads and 1,573 writes. Its independent repeat and all ten full root listings are byte-identical. The audit checks 2,796 full root rows and 507 executed descendant read/store instruction sites against the original executable.

Five enclosing GJK calls belong to the genuine unchanged player-spawn constructor. Two subsequent calls belong to the actual canister world-physics path:

| Source child | GJK workspace / retained support state | Actual source result |
| --- | --- | --- |
| `20000813 / 0601AAE0` | `021FC9E0 / 0601AAEC` | `EAX=0`; enclosing separation owner stores separation and output byte zero |
| `2000081D / 0601A5C0` | `021FC9E0 / 0601A5CC` | `EAX=0`; enclosing separation owner emits the admitted packet and output byte one |

The first uses the actual BSP triangle at `004819D0`; the second uses `00481AD0`. Both consume the actual wrapper shape `0601AD20`, backed by its source-created convex shape `8025CC40`. These retained identities are observation correlations, never native allocator return values or callbacks selecting geometry.

## Complete enclosing ABI and ownership

Native files are [physics_gjk.hpp](../include/halo2/physics_gjk.hpp), [physics_gjk.cpp](../src/physics_gjk.cpp) and [physics_gjk_tests.cpp](../tests/physics_gjk_tests.cpp). `30AE40` receives the workspace in ECX and five stack arguments, returns status in EAX and uses `RET 14`:

| Input | Owner |
| --- | --- |
| Argument 0 / 1 | Actual allocated A / B shape |
| Argument 2 | Source-produced relative transform, four 16-byte rows |
| Argument 3 | Retained support state, 16 bytes |
| Argument 4 | Source caller's output vector, 16 bytes |
| Entry ESP | Real scratch aliases; aligned frame is `((entryESP-4)&~15)-80` |
| FP environment | Actual masked x87 control word `023F`, MXCSR `00001F80` in these calls |

The implementation reads source storage through `SourcePhysicsContactMemory`, observes every owning store and requires the original constants/tables/vtable words. The caller owns this storage and its lifetime. No allocations, body creation, contact admission or solver result occur inside the new native GJK owner.

| Storage | Actual source owner/use |
| --- | --- |
| Support `+08/+09` | Signed current A/B counts; imported at `30AE54/30AE64`, rewritten at `30BB9F/30BBA4` |
| Support `+0A/+0B` | Signed maximum A/B support counts; source eligibility comparisons, never inferred shape capacities |
| Support `+00` | Encoded A keys followed by B keys, two bytes each |
| Workspace `+00/+04` | Current A/B counts |
| Workspace `+08` | Retained selected-axis/count discriminator |
| Workspace `+0C` | Existing degeneracy/fallback byte |
| Workspace `+10` | Source support-direction suppression discriminator, zeroed each iteration |
| Workspace `+20` | Four retained A vectors, including encoded W words |
| Workspace `+A0` | B vectors transformed into A coordinates |
| Workspace `+E0` | Original B-local vectors and encoded W keys |
| Workspace `+120` | Source search direction |
| Workspace `+130` | Segment-pair result packet |
| Workspace `+160` | Triangle-side vector / ordered swap inputs |
| Frame `+14/+1C/+20` | Source epsilon, iteration counter and result status |
| Frame `+10/+18/+24` | Source spilled projection limit, current projection and squared direction length |
| Frame `+70` | B-local support direction, then exact source length vector and reciprocal estimate spill |

Initial B point transforms use literal SSE order `(column0*x + column1*y) + (column2*z + translation)`, all four lanes. The encoded W keys remain in B-local storage. Retained-key stores at `30BBB3/30BBD3` read actual W low words after the source count stores; this does not assign new object identities.

Each iteration doubles the existing source epsilon, increments the source counter and clears workspace `+10`. The original compressed table `30BC1C` (25 bytes) selects the 11-entry jump table `30BBF0`; native code reads those original bytes instead of inventing a simplex classifier. Point, line, triangle and segment-pair branches preserve the original sign-bit tests, swaps, stores and reduction order. Actual execution covers the one/one, two/one, one/two, two/two and one/three paths.

The source alternates support generation through its counter bit, consumes the source maximum-count/suppression fields and compares actual spilled projections with original x87 instructions. It never chooses a point from an approximate hull. First-candidate `TEST AH,41` rejects less/equal/unordered; subsequent `TEST AH,5/JNP` rejects ordered less. Preserving that parity condition matters: changing it chooses a different simplex.

## Retrieve, support and simplex dependencies

The new complete retained-key retrieve dispatcher owns the actual methods:

| Owner | Literal behavior |
| --- | --- |
| `2E2320` wrapper | Calls the underlying `+34` retrieve method, adds source `shape+10` offset and restores each encoded W word |
| `3039C0` convex | Imports actual SoA bank `shape+30`, retrieves source U16 vertex keys, writes X/Y/Z and `key|3F000000`; full short and four-wide bodies are represented |
| `2E4320` capsule | Uses the encoded key as the source byte offset into `shape+10`, retaining the original W encoding |
| `1F1DA0` triangle | Copies actual retained triangle vertices in original reverse-index order |

All actual retained-key counts in these calls are one. The convex four-wide branch is source-listed and paired, not additional executed coverage.

New support points execute the already established original complete wrapper `2E22B0`, convex `3037F0`, capsule `2E42A0` and triangle `1F1D20` arithmetic. The new module carries their literal bodies so the enclosing owner computes its own output. It reads actual resident vtables, offsets, SoA vertices, compressed convex selection tables, source direction and original scalar constants. Strict comparison and tie order are retained. Their earlier full source and authenticated correspondence remain in [the child-agent freeze](MP_POWER_CHILD_AGENTS_RESEARCH.md).

Complete `30EB30` computes segment parameters, clamp mask, closest point, difference and squared distance. Its SSE dot order is `(Y+X)+Z`. x87 retained extended arithmetic and float spills follow the original source control word and store sites, including reverse subtract/divide, candidate clamps and parameter aliases. Four actual calls are compared.

Complete `309900` computes actual triangle side signs, workspace `+160`, the source direction and orientation-dependent ordered point/vector/side swaps. Four actual calls are compared. This is a literal owner over the existing simplex, not an independent SAT or capsule approximation.

Unknown real shape methods require `SourcePhysicsGjkServices::required_owner` with the original call instruction, entry, receiver, entry ESP and stack/register arguments. The unexecuted `309D40`, `309AE0` and `30A370` simplex fallback bodies remain required dependencies. Their entire original instructions and authenticated1.5 correspondence are retained; the native orchestration does not return a chosen fallback result. `309AE0` explicitly requires its original EAX/EBX inputs. Missing callback/storage or missing reciprocal-estimate provider stops.

Unmasked FP exceptions/status effects, unsupported compiler x87 backends, alternate shape methods, positive degeneracy fallback and unmeasured reductions have no new execution claim. No selected pose, active body or finished simplex is supplied to admit them.

## Reciprocal boundary and host diagnostic

At `30BB1D`, unchanged original bytes spill the full squared-length vector. At `30BB22`, unchanged `RSQRTSS` bytes execute in Unicorn. At `30BB26`, the original estimate low word is stored to the same frame address. The exporter takes these actual stores; it does not recompute an expected inverse square root. A `Q input estimate` fixture record represents this explicit **emulator instruction boundary only**.

The exact native differential returns that captured estimate only inside the test callback. The native production module contains no captured estimate lookup, contact replay or successful default. A live caller must pass its actual platform instruction provider, such as the existing `source_host_x86_rsqrtss_estimate`. The following source Newton refinement and all final stores execute natively in either case.

[The actual host diagnostic](../analysis/multiplayer/sandbox-source/power-gjk-owners/final/host-estimate-diagnostic.txt) uses the real host instruction for all seven measured enclosing calls. Every estimate differs from Unicorn. Six refine to byte-identical final output vectors; the admitted second canister child differs:

| Observed value | Unicorn original-instruction execution | Actual host instruction path |
| --- | --- | --- |
| Squared-length input | `4217C907` | `4217C907` |
| Reciprocal estimate | `3E263B71` | `3E263800` |
| Final vector, raw little-endian bytes | `605d1cb755d495b7ffff7fbf220f243c` | `615d1cb756d495b7000080bf230f243c` |

The host path has five differing owning stores in that call, including the estimate and refined tail. It is preserved independently. Equality to Xbox hardware, general contact-decision parity and positive live collision are unproved. The source emulator result must not be installed as a production platform service.

## Differential, production and authenticated correspondence

[The exporter](../tools/export_mp_power_gjk_owners.py) consumes only pinned original reports, literal read/write timelines, complete retained buffers and original executable vtable bytes. The independently repeated [31-call fixture](../analysis/multiplayer/sandbox-source/power-gjk-owners/final/exported-owner/fixtures.tsv) contains seven enclosing calculations and 24 complete retrieve/simplex calls. Its SHA-256 is `c3618d4180b39d6f1d37627eeeec2838714a2ee00650be202461b2ecf90fd897`.

[The 11-call composition fixture](../analysis/multiplayer/sandbox-source/power-gjk-owners/final/exported-composition/fixtures.tsv), SHA-256 `35c15a2d9e8b97a8e3e488670ff03f67defce7d6e630fb851fcfa9acc837d65e`, additionally compares the seven original enclosing GJK calls in the prior report and the two actual complete separation plus two actual complete wrapper-motion calls. This uses the new calculated owner after the frozen native producers. No continuation begins from a replayed finished workspace.

All **42 complete calls, 49,713,238 full buffer bytes and 3,349 ordered owning writes** match under the explicitly measured emulator-estimate test boundary. Fourteen unchanged enclosing inputs also stop at the exact missing-estimate boundary and match the complete original owning prefix before `30BB26`. Optimized execution, independent exported repeat, ASan+UBSan with leak checking and fresh production `original_physics_gjk` CTest pass. The sanitizer instruments the new GJK module and the actually composed narrowphase/child/relative-transform modules. Compiled binaries, production archive, flags/link commands and localized CMake registration are retained in `final/native-artifacts`.

[The full-body correspondence tool](../tools/research_mp_power_gjk_correspondence.py) compares all ten complete bodies, internal branch topology, source constants and jump-table mappings against authenticated1.5 SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`:

| TU1.1 | Authenticated1.5 |
| --- | --- |
| `1F1DA0` | `1F6690` |
| `2E2320` | `2E5E20` |
| `2E4320` | `3070F0` |
| `3039C0` | `3079C0` |
| `309900` | `30D3F0` |
| `309AE0` | `30D5D0` |
| `309D40` | `30D830` |
| `30A370` | `30DE60` |
| `30AE40` | `30E930` |
| `30EB30` | `312620` |

Main tables map `30BBF0→30F6E0` and `30BC1C→30F70C`; topology/byte selectors match. Source scalar `419BDC→41D658` is byte-identical; source sign mask `418AE0→461960` is byte-identical. These are verified instruction correspondences, not1.5 gameplay execution or evidence that raw1.1 storage addresses can be used with1.5. The `_1_1` API still requires an explicit image-bound storage/table/vtable adapter for1.5.

## Minimum remaining live bridge

The new owner can replace the former required `30AE40` callback in the genuine retained canister `2E14E0 → 2F4BD0` chain, with a declared real reciprocal-estimate platform provider. Its inputs must come from the actual constructor/body/world/relative-transform/support-state producers. It accepts no borrowed contact or artificial velocity, pose, membership or body-ready state.

The earliest remaining native owner is complete `300870` packet batching/contact retention. The actual source path then calls `300650` at `300D9F`, `309210` at `300EE5` and `309450` at `300F2C`, consuming genuine child result packets and source contact-manager/body storage. Actual manager/listener/entity/body/property and solver production must be composed before a live Scene can claim a blocking collider. Full repeated natural scheduling and hardware-estimate/contact-decision parity remain explicit boundaries. Canister incoming damage, destruction and explosion are separate unclosed owners.

The independent [audit](../analysis/multiplayer/sandbox-source/power-gjk-owners/final/audit.json) and [freeze manifest](../analysis/multiplayer/sandbox-source/power-gjk-owners/final/manifest.json) pin this scope. Earlier reports and archives remain unchanged.
