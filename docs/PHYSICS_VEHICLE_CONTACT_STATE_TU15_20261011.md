# TU1.5 vehicle contact-state and reference-release port

English TU1.5 is the sole parity target. This component adds three complete
native bodies and 160 instruction sites to the sealed vehicle motion graph:

| TU1.5 entry | Instructions | Recovered behavior |
| --- | ---: | --- |
| `20FD70` | 65 | Initialize default contact fields; copy active contact data and clamp the original distance expression against the executable's authored bound and record limit. |
| `20FCB0` | 61 | Resolve the object's component and constraint list; select positive-reference records with the matching signed kind; invoke their real release owner. |
| `22E1B0` | 34 | Preserve the original signed subtype checks and decrement the record's reference count; invoke the actual cleanup owners when the source requires cleanup. |

The original call at TU1.5 `20EBD2` authenticates `20FD70`; the retained caller
graph authenticates both calls to `20FCB0`. The call in `20FCB0` authenticates
`22E1B0`. New recovery, generation and execution read only the owned TU1.5
executable pinned to
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Earlier correspondence IDs remain internal parent identifiers. All new
instruction and store IDs are actual TU1.5 virtual addresses. No older image
is required to recover these new bodies or establish their parity.

The composed graph has 99 bodies and 11,216 distinct instruction sites. The
TU1.5-only API `source_run_physics_vehicle_contact_state_runtime` preserves the
parent calling convention, retained memory, ordered stores, GP/SSE/x87 state,
FP controls and exact required-owner boundaries. Generated C++ contains static
native operations and branches; it executes no original code or runtime opcode
decoder. No contact, successful solver result or cleanup outcome is supplied.

The bounded corpus has 12,518 cases, including 1,328 new contact-record inputs
and 2,592 list/reference-release inputs. New inputs cover inactive/active
contacts, three record indices, aliasing with output fields, finite and special
SSE operands, four rounding modes, FTZ/DAZ, retained sticky flags and x87 stack
state, absent/negative/empty/multiple list records, signed kinds, reference
counts and the real cleanup boundary. The executable itself supplies the
default vector and clamp constant. Direct cases explicitly declare the mutable
default material word as input. 151 of the 160 new sites are
exercised; nine sites after unported cleanup calls remain unexercised.

All 432 staged original builder-to-motion inputs now return: 27 tank, 27 wheeled,
81 hover, 27 flight and 270 generic cases. Both original and native builders
compute their own state. These inputs declare a component with no vehicle
solver object and no constraint list; they do not establish suspension, solved
contacts or driving. Class 3 remains unexercised. Across the full corpus, 10,086
cases return and 2,432 stop at real required owners.

The diagnostic Unicorn run discovers original memory footprints. The final
oracle independently single-steps unchanged TU1.5 graph bytes on this host's
Linux i386 x87/SSE backend. Test-only capture stubs at unported owners preserve
the reached registers, flags and stack and supply no result. The reused
hardware observer and its original harness are hashed separately. This proves
the captured host cases, not Xbox CPU equivalence or other platforms.

Evidence and the sealed source cohort are under
`analysis/multiplayer/sandbox-source/vehicle-contact-state-tu15-20261011/`.
`validate_vehicle_contact_state_tu15_20261011.py` audits exact native and local
Python dependencies, repeated recovery/generation/original/hardware evidence,
strict FP flags and Release/ASan/UBSan/production comparisons. Standalone builds
can compile the full graph or reuse the unchanged, already validated parent
archives; reuse records their exact hashes and the parent validation.

TU1.5 cleanup owners `22E390`, `22FE30` and `22FEA0` remain required. Other
animation, activation, physics-object and math-error owners remain open. The
live player mover is still temporary; actual world/contact/solver ownership,
player/vehicle/object integration, authored maps/scenarios, split-screen and
LAN remain required by the active full physics goal.
