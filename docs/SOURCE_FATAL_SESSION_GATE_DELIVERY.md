# Connected fatal cleanup session gate

The complete original `A8620` now executes through the native Match's existing
fatal cleanup path: `finish_death → kill_biped → CBDC0 → … → FB120`, whose
original call at `FB2FC` reaches this owner through the shared dispatcher.
Its actual session bank decides the branch. There is no separate mode check or
substitute result in the host adapter.

The unchanged TU1.1 body contains 84 instructions and uniquely pairs with
TU1.5 `B1A70`, including internal branch topology. Both complete listings and
the direct-call closure are authenticated in the
[original owner audit](../analysis/damage-shields-goal-20261011/live-fatal-cleanup/effect-research/reached-a8620/audit.json).
The object-phase generator now emits 257 complete bodies, 27,298 instructions.
Independent regeneration is byte-identical. The updated entry registry joins
the complete body to the same source memory and ABI used by the live objects.

The owner reads signed `BYTE[*4EDF18+0C]`. Only literal value 5 enters the entity
branch. Every other value returns through original `RET 4`; those paths do not
read an object pool, inventory or entity manager. The native session's actual
mode byte is passed through unchanged. For mode 5, the first genuine lower is
`A8720`, with `(object,1,4,&count,types,objects)` on the stack. Missing lower
ownership remains an error; no entity identity or successful callback is added.

The [repeatable original capture](../tools/oracle_source_fatal_session_gate_1_1.py)
executes all 256 byte inputs in each unchanged executable. Its output is
byte-identical to the independently captured reference. The
[native comparison](../tests/source_fatal_session_gate_tests.cpp) uses the shared
production dispatcher and compares all GP registers, flags, instruction counts,
exit kinds and pending arguments. It mounts only declared game-pointer,
session and stack scratch. It produces 255 actual returns and one stop before
the first lower, with no supplied outcome. This proves the session gate and ABI;
it does not prove active entity creation.

The actual prepared two-player Match now passes the former
`FB2FC → A8620` boundary and reaches **`CFB80 → BA980`**. Its source-born players,
WORLD nodes, resource pools and region-state selectors execute in the retained
bank. Cleanup has not returned to the corpse-detachment or connected death-view
assertions. This increment therefore remains progress, not a completed death
cycle or shield effect.

The [saved-binary validation](../analysis/damage-shields-goal-20261011/live-fatal-cleanup/session-gate-validated-v1/validation.json)
records the selected 56 checks, 19 binary hashes, source projection and actual
Match probe. The projection does not claim a complete binary/source bundle or
whole-checkout stability during concurrent implementation. Launchers were not
refreshed. Original entity lifecycle, natural damage delivery, shared host/source
vitality authority, cache-file I/O and fixups, death/rebirth and shield submission
and pixels remain required by the broader goal.
