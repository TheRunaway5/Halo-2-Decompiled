# TU1.5 v1458 paired contact scratch diagnosis

The actual v1458 lower0 frame still fails the unchanged raw comparison:
**307 bytes in 73 ranges**. This is a new capture with a different temporary
array footprint from v1457. No comparison exclusions, expected banks, physics
outcomes, or raw acceptance gate were changed by this diagnosis.

Evidence root:
`analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458`.
Required input is `iteration-00-lower-body1-phase0/original/required-failure-inputs`.
The complete raw difference report is
`lower0-byte-difference-diagnostic/differences.json`.

## Paired input and observed original lifetime

The new hardware-only tracer observes unchanged original `2E3250` allocation
returns and genuine `149E00` free entry/return addresses. It also monitors
pool `233250` after the second growth. At root return it dumps the complete
1,077,248-byte watched heap under owned scratch and requires exact equality
with ordinary ptrace original execution from the same input. The matching
heap SHA-256 is
`74153f9e0f806f7050b53bb7b214ad610ab99d1e04ba3b9a212cdbb63855fde7`.

The observed original contact array starts with stack capacity four and then:

| Event | Buffer | Extent | Header count/capacity | Actual return |
| --- | --- | --- | --- | --- |
| First heap growth | `10082380` | `400` | previous stack count4/capacity4 | `2E327E` |
| Second heap growth | `10092380` | `800` | old count8/capacity8 | `2E327E` |
| Free old allocation during growth | `10082380` | `400` | old count8/capacity8 | `2E32B3` |
| Final array destruction | `10092380` | `800` | count13/capacity16 | `304AA3` |

Both free calls pass class/tag `12`. Both buffers' complete payloads remain
unchanged from their corresponding free return through root return `1CCACA`.
The monitor sees zero subsequent class13/14 pool allocations after the second
growth, so neither buffer is reused before this frame returns.

All six allocator metadata fields match in native input, native output, and
ordinary original output:

| Field | Address | Value at each boundary |
| --- | --- | --- |
| Class13 free-list head | `10076098` | `10082380` |
| Class13 in-use count | `1007635C` | 1 |
| Class13 free-block count | `101663FC` | 7 |
| Class14 free-list head | `1007609C` | `10092380` |
| Class14 in-use count | `10076360` | 0 |
| Class14 free-block count | `10166400` | 4 |

## Exact difference ownership

There are 121 differing bytes in 29 ranges inside the freed old allocation
`[10082380,10082780)`, and 186 bytes in 44 ranges inside the freed new allocation
`[10092380,10092B80)`. Every differing byte belongs to a record with original
and native valid byte `+7D` equal to zero. The body, component, key, and valid
fields agree. The new allocation contains 13 records; its sole true-contact
record is index12 and has no differences. The old allocation's true contact
is index5 and also has no differences.

Differing offsets are `0C..19`, `28..2F`, `4C..4D`, and `7E..7F`. Actual
`303EE0/303BA0` construction and `304350` contact processing retain copied
inactive private payload and unwritten alignment gaps in these locations.
The source constructor initializes private flags, then copies an uninitialized
local record. Full-record growth copies the alignment gaps as well. Source
false-contact paths do not replace the inactive vector payload. The relocated
saved frame-pointer lane remains original `600EFF38` versus native `600EEF3C`.
This is a specific source/lifetime diagnosis, not permission to disregard
these offsets in live allocations or true contact records.

The exact captured native write trace has 243 records at tick33 with zero
drops. It shows genuine first-buffer copy and pool metadata writes. Its
original filter covers only `10082000..10084000`, so it cannot observe the
second buffer, and generic pool free writer `23346E` cannot identify the
caller's growth/destructor site. It therefore does **not** establish a native
capacity-eight limit or a native/original contact-cohort mismatch. An extended
native trace with header/count publication and class14 metadata is still
required for the complete paired native lifetime history.

## Remaining isolation and retained artifacts

A genuine live `319D50` entry capture with the same GP/SIMD/x87 inputs, world,
eight-byte step packet, and stack storage is the next isolation check. It
separates remaining collision translation from the typed root caller's private
stack relocation. Original `31B1A0` supplies world in ECX and a step pointer on
the stack; no guessed incoming volatile-register repair is justified.

The bounded checked lifetime report and log are
`lower0-contact-allocation-lifetime/report.json` and `lifetime.log`, totaling
83,662 bytes. They record original image/capture/source hashes, actual free
return addresses, allocator metadata, all range/record ownership, and the
ordinary-reference postimage comparison. ELF, raw heap dumps, replay outputs,
and debugger scripts were generated under `tools/run_test.py` owned scratch
and removed. The successful report's scratch path was checked absent. Required
existing reference inputs were preserved.

The raw 307-byte/73-range gate remains red. Full live-world parity and native
physics closure remain open.
