# Native raw component allocation and initialization

The new raw datum-pool owner implements complete16D2C0 against retained raw
pool, element and bitmap memory. It neither constructs a host replacement pool
nor derives a capacity or salt. Signed cursor/high-water/capacity gates,
arithmetic bitmap indexing, occupancy, wrapped counters, element DWORD/byte
clear order, salt increment/rollover and defined returned datum follow the
original body. Missing backing stops; a full pool returns the original NONE.

Complete1D2AA0 now composes that native allocator with complete1D3340 and the
original inlined object dirty-word/counter update. It uses the actual allocated
component address and object, counter and clock pointer slots. The source
failure path checks the calculated component address, rather than testing the
returned datum for NONE; the native owner preserves that distinction and its
subsequent required memory boundary.

The initializer retains all original field writes and their order. Its tick
field reads the actual signed16 clock word, performs CVTSI2SS/MULSS with the
authenticated465198/u5 operand, then FLDS/FISTP under the caller's actual x87
control word. No approximate decimal multiplier, host default rounding or
synthetic tick is substituted. The provider must supply the owned-image word;
automatic OriginalData import is a separate bridge, not a fallback here.

The GNU x86 backend requires explicit caller CW/MXCSR with exceptions masked.
It returns actual native SSE sticky status in the mutable source context for
later owners and restores the host floating environment. The full Source x87
stack/sticky state, unmasked exception delivery ABI and unsupported-platform
backend remain explicit limits. Unicorn's literal observed status is retained
in evidence and is not assigned to native hardware or asserted as Xbox status.

The unchanged original canister factory newly captures the complete actual
1D2AA0 invocation, including its real16D2C0/1D3340 descendants,71 owning stores,
complete retained component/object banks and actual clock/counter inputs. Its
entire factory result and inherited39-call/19-state body-slot sequence match
the prior checkpoint. Independent re-execution is identical. No new original
instruction patch, allocation result, pool capacity or callee reply is supplied.

All three bodies have unique full-body/internal-branch correspondence to the
authenticated u5 executable,149 instructions in total. Independent pairings
are identical. Direct original old/u5 controls cover the actual input, a
sequence through the actual512-slot capacity and subsequent NONE, declared
salt/stride/bitmap-hole cases, and240 signed16-clock/CW/MXCSR combinations per
image. The u5 runs reuse explicitly retained old direct ABI inputs; they do not
represent a complete u5 startup or world.

The510 complete original controls contain1560 calls and59,716 stores. The new
native test additionally composes complete1C6F50 with the actual native raw
allocator/initializer/component owner, without replaying any lower result or
memory effect. It matches the unchanged original73-store prefix and all
retained bytes, stopping exactly when complete1D2B10 world/component rebuilding
is required. That stop is not presented as completed physics construction.

Together,511 complete cases/required stops compare1561 calls,59,789 stores and
3,775,976 full retained output bytes. Normal and actual GCC address/undefined
sanitizer checks pass. They also verify host rounding, CW/MXCSR and sticky
flags are restored while the source context retains native SSE status.

The registered production check also adds the separately frozen first-person
raw sound module to Cache. Three original authored sound events compose with
the real world request and whole raw submit, matching30 world/99 construction
stores and all160 caller bytes. The source availability rejection remains
natural; no voice, listener, device or successful reply is supplied.

The initial bank/clock fields used by these tests come from genuine retained
original execution. Native pool/title-arena startup, an advancing live source
clock, complete1D2B10/world/solver ownership and live Scene/Unit bindings remain
required. There is no final unit/body injection, fixture-backed live provider,
renderer/runtime change, launcher release or claim that playable canisters,
grenade throws, reload audio or all-map sound occurrence are fixed.

Evidence: `analysis/multiplayer/integration/physics-component-construction-native`.
