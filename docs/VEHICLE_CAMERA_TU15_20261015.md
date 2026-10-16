# Vehicle camera parity, October 15, 2026

The PC camera runs the original English Xbox TU1.5 look and complete player-camera director. The owned executable SHA-256 is `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`. Original Xbox TU1.5 is the gameplay authority.

## Completed behavior

- Original `18A300` and `11F7E0` own seat-relative yaw limits, pitch bounds, velocity-dependent automatic pitch, wrapping and x87 spill rounding. Native raw input accumulators remain separate from completed source look. Rendering does not advance command look.
- Successful driver entry at original `EC780`/`ECB98` now forwards the original reset into each retained native player input. Original `18A2A0` computes the reset. The event occurs before the Xbox local-view gate, so remote players receive it too. The adapter applies private input updates after the complete original call, preserving shared input and original-call diagnostics.
- Original `157FE0` and `158290` retain the actual player director across entry, seated motion, exit, death, watch/cycle and distinct respawn. Controller and observer state survive the unit-NONE interval. Completed source perspective controls weapon hands and body visibility independently of observer blending.
- Raw SDL mouse accumulators now survive source respawn. Remote TU1.5 views use processed source look instead of overwriting it with raw PC angles. A real SDL mouse/UDP regression failed before the fix and passes after boarding, death, respawn and three subsequent input commands.
- The full original death owners (`246CF0`, `247140`, `247020`, `247090`, `246E90`, `246F30`, `BCEE0`) replace the separate approximate lifetime handoff in the TU1.5 path. Private camera RNG preserves the gameplay auxiliary stream.
- Campaign, local multiplayer and authority peer cameras use the retained player binding. Protocol 85 carries source perspective and the post-seat observer. Source cinematic camera admission remains with the cinematic owner.
- The default vehicle FOV is the original 70 degrees (`381710`, literal `46AB88`). Its source lens, original observer aspect and Xbox split-grid projection/lighting factors are preserved. Explicit PC vehicle FOV customization scales the source lens in tangent space; on-foot FOV remains separate.

## Authored seats and original execution

[All 61 authored third-person definition/seat pairs](VEHICLE_CAMERA_TU15_COVERAGE_20261015.md) are accounted for. Fifty-five camera components pass 180 observer frames each; six player-prohibited seats remain rejected by original admission. All 61 captures match unchanged original i386 instructions, including **3,665,642,622 final memory bytes** and GP/SSE/x87/FP state.

The component fixtures use actual source object construction and original `EC780` with declared force/animation-selection inputs. Original `BAF10` and `EC995` own attachment and seat writes. Force admission is explicitly a component boundary; it does not certify ordinary boarding or scripted campaign availability.

Separate command-driven checks cover 18 ordinary multiplayer cases (14 pairs), 23 boarding seats, campaign Ghost admission, and four seated death/watch/cycle/respawn cases. The Banshee left boarding fixture is rejected by original `CB7A0`; its selected player mode has no entry animation in that fixture. The second Wraith mortar boarding seat has no ordinary authored approach. Their camera components match original execution without changing admission rules.

The earlier 83 actual look/director/perspective captures also pass complete original-CPU comparisons (4,987,677,666 bytes). Ten death/respawn captures add 600,925,020 compared bytes. Actual campaign director, dialogue-admission and driver-input-reset calls are independently checked. The full driver-attachment call reaches unresolved Xbox kernel import ordinal `800000EA` in the host replay; its failed replay is retained and is not counted as a passed camera comparison.

## Xbox reference and presentation

An unchanged owned TU1.5 session in xemu supplies the independent reference. The entire live `.text` SHA-256 is `d9c6fd7e7475862b1f19c2da80d7f2f6dc41da53619991002e9546e5f9e629fe`. No guest code or memory was patched.

- Scorpion seated, orbit, pitch-limit and exit states match all 48 words of the complete native view/projection bank and the source lens/lighting scale.
- Original `1723D0` adoption matches two 214-word observers and 43-word packets, including an active update with 14 changed observer words.
- Original `172CF0` exit-transition motion matches all 214 words at captured source dt 0.05.
- The actual submitted `next-view-header` is the projection reference. The inactive global header has a different lens and must not be used for this comparison.

Visible real-UDP render checks cover all 13 earlier cases. The final continuation reruns Ghost, a moving Warthog passenger and the primary human turret: 1,575 exact UDP state/camera comparisons, 956 submitted seated camera frames, 275 source exit-blend frames and 10 completed ordinary frames. Every submitted seated pose/basis/FOV equals the authority source camera. Projection checks use the final submitted split-grid bank.

The fixed-turret acceptance now checks exact source pose/perspective instead of a guessed half-unit eye-distance minimum; the original matching turret observer can be 0.498986 units away. The primary turret's broader acceptance passes. The earlier secondary turret camera passes while a separate firing/projectile check remains failed.

The complete `2F9D70` sphere-filter body is lowered. Declared disjoint and exact-tangent component inputs pass unchanged original execution (120,185,004 bytes); these are not claimed as source-created live obstruction/overlap captures.

## Build and evidence

Evidence is retained under `analysis/camera/vehicle-tu15-completion-20261015/`:

- `authored-seat-camera-coverage.json`, `authored-seat-original-comparisons.json`, `authored-seat-captures/`: all 61 pairs, with original memory and register comparisons.
- `look-reset-generation/`, `director-death-generated/`, `dialogue-generated-v2/`: authenticated complete bodies and generated-source hashes.
- `completed-final-regressions.log`: 17/17 completed camera-owner, local/remote reset, 18 ordinary cases, four death cases, campaign, projection, scope, FOV and controller checks.
- `relative-look-red3/` and `relative-look-green/`: real SDL/UDP/GPU regression before and after the respawn accumulator fix; seated observer, corpse skin, source death camera and settled first-person restoration. The declared fatal stimulus does not certify credited weapon kills or automatic scoreboard admission.
- `coherent-remote-layout.json`: matching compiled vehicle-record layouts after the stale-object repair.
- `reference-scorpion-v3/` and `*-comparison.json`: authenticated Xbox execution, projection, active adoption and transition motion.
- `final-rendered/ghost/`, `final-passenger/`, `final-fixed/`: actual executed ELF receipts, unaltered recordings and submitted camera assertions.
- `sphere-components/`, `sphere-original-comparisons.json`: explicit boundary components.
- `mixed-build-layout.json`: a shared-checkout build defect detected during remote validation. Actual DWARF showed a 224-byte vehicle presentation record in current code and 108 bytes in an older rendering object. All consumers were rebuilt; compiled test/render/match/network records were verified at the same 224-byte size.

The rebuilt application is selected by both installed launchers at `build-multiplayer/halo2_native`. Native MCC gameplay admission remains an independent project gate; owned Xbox-cache acceptance is not a claim that the launcher can already run converted MCC gameplay.

## Validation limits

Camera-owner and authored-parameter comparisons are complete at the stated boundaries. Physical Xbox behavior, matched framebuffer pixels, exhaustive live obstruction/overlap cases, every ejection prerequisite and a full campaign playthrough remain unverified. Emulator, component, host-CPU and GPU results are kept distinct.
