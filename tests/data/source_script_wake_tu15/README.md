# TU1.5 script wake and external sleep fixtures

The reference is the original English OG Xbox TU1.5 executable, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
`provenance.json` records the fixture and original-map hashes.

The 23 small declared programs run through unchanged 2115D0 normal dispatch,
16EA60 pool traversal,211B10 continuations,210AA0 sleep,210F00/211F40 wake
and212130 lookup. Only literal print and the empty object-list GC tail have
explicit non-timing host boundaries. No Xbox instruction is patched. The
tests compare event order, effective wake and first external saved schedule
after every dispatch. They also restore campaign snapshots and independently
import typed script authority before continuing.

An existing later thread can run in the same dispatch after wake. Earlier
threads wait until the next dispatch. Wake does not allocate absent threads
or restart completed threads. Wake of NONE or an absent ordinal is a no-op.
Two-argument sleep with NONE (-1) sleeps this thread; other absent ordinals
are no-ops. External sleep retains the first interrupted
schedule, including dormant -2; wake restores it before any sleep_until pop.
The delayed restore cases capture the active saved schedule before a future
wake, so future execution verifies that snapshot/import did not drop it.

The production archive uses named aggregate fields. Codec tests retain that
saved schedule through the actual encode/decode API and typed script import.
A valid-checksum old 8-field thread-extent control must be rejected. These
tests supply an inert owning envelope; they do not claim live full-world
authority replay.

The 49 opening cue test reads scripts and syntax directly from the owned
original introduction map. Original 2115D0 executes the same49 dormant cue
programs after external wake before tick 0. Print, subtitle, emotional-state
and sound submission are declared non-timing void boundaries. Native object
operands resolve to NONE at those boundaries. Expected values are request
ticks and sound datums, with Pillar 100, Council 279 and Truth 490. This does not
verify live voices, readiness, complete scene scheduling or an audio-stall
cause. The actual opening caller wakes earlier pool ordinals, so its timing
must not be inferred from the synthetic later-thread case.

Reproduce the original fixtures from the repository root with Python+Unicorn:

```sh
python tools/oracle_source_script_wake_tu15.py --xbe /owned/u5English/default.xbe --output-root /tmp/hs-wake-reference
python tools/oracle_source_script_opening_cues_tu15.py --xbe /owned/u5English/default.xbe --map assets/disc/maps/00a_introduction.map --output-root /tmp/hs-cue-reference
```

`.hsw` is a little-endian declared-input format: magic`HSW1`, three u32
counts(nodes,scripts,string bytes),20-byte raw syntax rows,40-byte script
records(name32,type16,return16,root32),then strings. `.events` contains tick
and print label. `.states` contains tick,script,effective wake,external-sleep
flag,saved wake. Dormant is -2 and completed is -1. Saved wake is significant
while the external flag is set. `opening49.events` contains tick and sound
datum; `opening49.names` selects actual authored script names.
