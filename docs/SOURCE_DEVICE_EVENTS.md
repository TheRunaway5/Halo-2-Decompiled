# Authored device events

The simulation now drains the actual common-device/control event producer.
Previously `SourceDeviceWorld::take_events()` had no consumer: the computer
could move Zanzibar's gate while its authored button and gate sounds remained
silent. Native playback now uses the emitted tag and full device generation,
once for the shared world rather than once per local viewport. Live local
one/four-view playback, real UDP playback and a four-view moving-gate late join
all passed on 2026-10-12, with zero Vulkan validation errors and no source drift.

## Original evidence

`tools/differential_source_device_event.py` executes unchanged base-disc
`107980`, pinned to executable SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
Its `189060` sound and `176780` effect callees are captured external services.
All 1,608 cases match: 25,728 output words and 47 distinct original instructions,
including NONE, unsupported groups, signed tag indices, full device handles,
literal location words and arbitrary channel bits. The checked fixture is
`tests/fixtures/retail_source_device_event.tsv`.

`source_device_event.hpp/.cpp` preserves the dispatcher through those explicit
service boundaries. NONE reads no object/tag state. Other requests read the
actual object location before selecting the tag group. Sound requests retain
the device owner, marker NONE, control 1, zero local position and original
forward vector. Effect requests retain the literal location record, object
power at `+134`, position at `+140` and the two zero stack arguments.
No sound/effect allocation result is manufactured.

The separate native adapter shares the proven tag-group selection. It sends
authored sound tags to the existing SDL PCM adapter with the actual emitted
device handle and control 1. It does not reconstruct a source object-location
record from a native render transform.

`tools/audit_source_device_events.py` requires all 21 supplied Xbox maps and
hashes their caches. The current placements have 37 nonempty common/control
event references, all `snd!`. Zanzibar's three requests are the computer's
`button_push`, the gate's `house_gate_open`, and `house_gate_opened`.
There are no authored device `effe` references in this Xbox placement set.
This inventory does not establish that every device's producer is live.

## Simulation and native LAN ownership

The authoritative simulation assigns each emitted request a monotonic native
serial and its completed game tick, retaining the latest 64 events. Native LAN
protocol 42 carries that journal with the full device state. Transport validates
capacity, ordering, tick bounds and salted owner identity before publication.
Events retain their emitting generation even if that object later disappears.

The client establishes its playback baseline from the first accepted bank,
skipping historical requests. Every later accepted bank is consumed, including
multiple banks drained in one presentation frame and same-tick publications.
Serial tracking suppresses repeated snapshots. Retention recovers requests
after packet loss within the retained window; it is not an acknowledged,
unbounded event-delivery protocol. Local playback consumes each completed host
tick before rendering. Checkpoint restoration clears transient requests while
keeping the serial monotonic for the existing runtime. A fresh simulation owns
a fresh journal and playback cursor.

## Acceptance commands and limits

The isolated build is
`analysis/map-world-features/device-event-snapshot-20261012/build`; its source
snapshot contains the accepted previous world-control baseline plus this
task's changes, preserving concurrent work in the shared checkout. Every input
is hashed in `input-manifest.json`.

```sh
ctest --test-dir analysis/map-world-features/device-event-snapshot-20261012/build \
  -R '^(retail_source_device_event|source_device_world|source_device_network)$' \
  --output-on-failure
python3 analysis/map-world-features/device-event-live/check.py \
  --output /tmp/halo2-device-event-live-check
```

The live runner exercises actual SDL computer selection/hold and gate traversal
with one/four local views, requiring exactly one native PCM voice for each
authored cue. A separate real UDP host/client probe checks later event playback
and a moving-gate four-view late join that skips the two earlier sounds and
hears the later endpoint once. That LAN probe supplies an already-admitted
host action; it does not prove a client's computer-use request.

Accepted evidence is in
`analysis/map-world-features/device-event-live/accepted-v1/verification.json`
and `report.json`. Both local view counts submit one voice for each of the three
authored cues. The ordinary LAN client receives all three once. The moving
four-view late join submits zero button/opening voices and one later endpoint
voice. The paired focused checks (original dispatcher, device world and real
UDP state/events) pass; the unchanged-original comparison and address/undefined
sanitizers also pass. The pre-fix executable completed interaction/traversal but
failed the same native PCM assertion; its log is retained under `pre-fix`.

Original `189060` attached sound admission, spatial attenuation/occlusion,
`176780` attached effect creation/location services, complete live player-action
producers, Havok behavior, and all-map live acceptance remain open. Unsupported
device `effe` playback reports the missing owner rather than inventing a world
effect position. The full all-map goal remains active.
