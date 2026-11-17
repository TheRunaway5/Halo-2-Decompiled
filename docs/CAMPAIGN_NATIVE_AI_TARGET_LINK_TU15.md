# Native TU1.5 target-group actor linkage

`native_campaign_ai_target_link_call_tu15` admits the actual `274670` call from
`2734F3`: EAX is the destination clump-record pointer, `[ESP+4]` is the group
datum, and the void return removes four argument bytes. The adapter delegates
to the existing qualified `native_campaign_ai_perception_group_link_actors_tu15`.
Its real children allocate properties through `2745E0`/`16E6A0`, link them through
`274750`, and resolve their object points through `BBE00`. Allocation refusal
skips that actor exactly as the original does. No child result is supplied.

The adapter transports retained FP controls/status on normal return and on an
exception, preserves saved GP and caller XMM/live x87 values, and advances ESP
by eight bytes including the original return word. Existing semantic algorithms
are unchanged. Their actual memory reads and writes remain visible to the
memory provider; they do not emit child instruction-PC observation tags.

`tools/oracle_native_campaign_ai_target_link_tu15.py` executes unchanged physical
Original TU1.5 instructions from the pinned offline executable. It reuses the
existing actor-link *input* factory and enters the whole parent for every scene;
it does not reuse expected results. The fixture contains 166 completed calls and
nine exact memory interruption prefixes, covers all 62 parent instruction sites,
and records 11,999 ordered reads and 8,766 stores. Scenes include empty and
multi-actor clumps, existing list heads, allocation refusal and partial capacity,
record aliases, object-parent transforms, finite/exceptional floating-point
values, precision and rounding controls, and live caller x87 values.

Native replay compares retained DATA, ordered semantic reads/stores, saved
GP/ESP, full completed-call FP controls/status and caller live values. It checks
FP publication after a distance-store exception and restoration of real ambient
host FP. Volatile GP, EFLAGS, child store-PC tags, private child frames and pending
private x87 temporaries at arbitrary memory interruptions are outside this
semantic contract. The distance owner retains its existing masked-exception
and available x87-stack requirements. This qualification uses the normal
forward string-copy ABI and does not claim the direction-flag-set variant.

Six mutations are rejected (argument address, clump pointer, stack cleanup,
return FP, exception FP, and group-head publication). ASan/UBSan covers the
adapter, semantic children, allocator and replay memory driver. An independent
unchanged-original repeat is byte-identical.

Evidence is under
`build/test-results/armory-native-progress-20261113/placement-helper/target-link-*`.
The required original regression input is
`tests/fixtures/native_campaign_ai_target_link_tu15.json.gz`, 1,668,641 bytes,
SHA-256 `f4c50ae3e6cbaaf9b5eba17f47c88bd3168a2750171e3f90111f42a5b13ed9f4`.
Disposable compiler, observer, raw-bank and replay outputs were removed; the
closure receipt records the three absent scratch roots. The separate root-owned
incremental engine build remains active. This owner proof alone does not certify
its enclosing placement parent, the live tram sequence, rendered training, or a
launcher runtime.
