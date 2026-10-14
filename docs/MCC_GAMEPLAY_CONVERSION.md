# MCC data import and Xbox TU1.5 conversion

MCC installation discovery, private asset copying, a native v13 reader,
geometry/model/bitmap/animation/audio adapters, compiled immutable TU1.5 engine
operands, preserved native shader libraries and the two recovered TU1.5
multiplayer postprocessors are implemented. **Full
MCC-to-Xbox gameplay conversion and native MCC playback are unfinished.** Both launchers now select only the private MCC cache.
Native gameplay remains disabled until the unfinished conversion/startup work
is complete; the launcher never falls back to Xbox/Vista sources.

```sh
./Halo2-Launcher --prepare-mcc-gameplay
# Nonstandard installation, or the halo2 / h2_maps_win64_dx11 directory:
./Halo2-Launcher --prepare-mcc-gameplay --mcc-source '/path/to/MCC'
# Works without a built runtime:
python3 tools/setup_assets.py --prepare-mcc-gameplay
```

Preparation requires installed MCC Halo 2 Classic data, Python and enough disk
space. It requires no Xbox ISO, Vista installer, running MCC, mod download or
downloaded format schemas. The release target requires **MCC only**. The OG Xbox ISO and TU1.5 patch are reference-only material for recovering and validating behavior and visuals; they must not become onboarding or normal runtime inputs. The existing executable-backed gameplay runtime still violates that target and remains unfinished.
The native Vulkan shader selection is unchanged.

## Converted data is vanilla

The requested target is the engine's official internal Xbox TU1.5 data set.
MCC is an offline donor. Once fully normalized and validated, the converted
definitions must use the normal vanilla loading path, with their authored
group/path identities. They must not be registered as an MCC mod, mounted as
an override, or given source-specific tag prefixes. Donor provenance belongs
in conversion records, rather than changing gameplay tag identity.

TU1.5 equivalence concerns authored data and observed behavior. The requested
native 64-bit representation can differ from the packed Xbox binary layout;
that does not authorize changing gameplay precision, timing or visuals.

The current raw v13 reader is a conversion/inspection foundation. It is not a
claim that raw MCC structures are ready for vanilla admission. Normal onboarding selects the prepared private cache through
`assets/mcc-source.tsv`. Selection does not promote incomplete data to playable
vanilla definitions; provenance still records `runtime_active=false` and
`full_tu15_parity=false`. Historical Xbox/Vista selections are ignored.

## Native 64-bit addresses

The native executable already runs as a 64-bit process. The engine-facing tag
module now retains actual 64-bit addresses of its immutable owned backing for
tag definitions, roots, embedded structures and nested block elements.
`NativeTagAddress` is a separate type with no implicit conversion to a 32-bit
integer. Native reads check overflow, allocation ownership and residency.
Views retain their owner across closing the tag system or changing BSPs.

Serialized directory pointers, block/data descriptors and salted handles keep
their specified file-format widths. Reading them resolves an owned native
address; it does not turn a source pointer into a host pointer by casting.
The explicit original-code boundary resolves a native address back through
its owner to the original 32-bit source token, checking that it identifies the
same bytes. This preserves the recovered Xbox routines' packed structures
and arithmetic. Gameplay values, floating-point precision, update timing,
shader programs and texture contents are unchanged by this address migration.

Live engine world, object/player pool, object-body and clock interfaces now also use `NativeRuntimeAddress`. Datum-pool views, object lifetime tracking, scene publication, vehicle readback and death-camera bindings use checked native addresses. A separate allocation index checks owner identity, permissions, commitment holes and bounds. Checkpoint files retain source tokens; restored worlds rebind native addresses to their new allocations. Focused checks exercise actual backing above 4 GiB and reject cross-world pointers.

This covers the native tag and live-state interfaces. It does not widen
the recovered Xbox execution ABI or remove its physical cache reservations,
tag-count limits or remaining MCC physics/layout conversion requirements.
Large native mod banks and their gameplay admission still require those
remaining consumers to be ported or explicitly adapted; replacing an integer
type alone would not make those banks playable. Full gameplay and framebuffer
parity remain separate validation requirements.

The checked 64-bit tag layer passes four focused native test groups and all
13 import tests. Complete payload comparison covers 84 original/Vista maps,
469,014 resident payloads, 995,569,592 bytes and 184 BSP selections. All 43 MCC
maps pass native 64-bit BSP ownership/geometry checks, and all 114,765 animation
clip occurrences pass decoding and the owned resource I/O boundary.

A fixed six-tick original Lockout run using the English TU1.5 executable
retains the same player position, first-person pose, ammo and RNG state as the
pinned pre-migration native build. Its 1280x720 captures match all 921,600 RGB
pixels exactly. This is a bounded native-before/native-after comparison; it
does not establish physical Xbox framebuffer parity or complete gameplay
coverage. The frozen tested executable and source/build pins live under
`.local/mcc-native-20261013/native64-snapshot/`. The detailed evidence and
remaining work are in the continuation's `status.json`.

The live-state interface continuation is separately built under
`.local/mcc-native-20261013/runtime64-snapshot/`. All four TU1.5 runtime groups
pass: live constructors/controller movement, 120 physics frames, fatal cleanup
and a full headless Match death/ragdoll/observer/respawn cycle. Native memory,
scene handoff, tag, datum-pool and original asset/oracle checks also pass.
A legacy TU1.1 fixture still lacks source owner `2DD1D0`; the same failure
reproduces in the source32 reference and is not covered by the TU1.5 passes.

For address-only validation, a second build uses identical frozen sources with
only four interface files restored to source32 world/pool/body/clock tokens.
Both execute the same six fixed TU1.5 reference ticks with identical captured
state and zero differing RGB pixels. This supersedes comparison against a
moving checkout and does not establish physical Xbox framebuffer parity.
Evidence is in `runtime64-fixed-comparison.json`; all 26 migration source/test
files match that tested snapshot. The current importer passes 16 tests.

## Native process-heap startup, 2026-10-14

`NativeTu15HeapBootstrap` constructs and publishes the actual TU1.5 process heap
without loading an executable or captured startup bank. Its immutable defaults,
platform import identities, aligned-copy tail branch identities, cold policy
bytes and heap binding occupy 54 explicitly classified bytes. Its caller stack,
exception chain and parameter record are fresh native allocations. Native VM
and lock owners execute the complete existing heap constructor. The published
heap uses a checked `NativeRuntimeAddress`; foreign owners are rejected and
immutable defaults remain read only.

The live reference world's caller also uses the recovered native reserve and
commit configuration instead of reading XBE header fields at runtime. This
removes that header consumption, while its other startup banks remain
reference-backed. The standalone bootstrap is the first native startup owner;
it does not initialize a game arena, deliver gameplay tags or create a world.

All six normal/default/rounded/no-lock constructor cases match runs with the
authenticated reference operands, including GP/XMM state, flags, instruction
trace, ordered game/lock stores, resource requests and retained outputs. Two
no-lock cases additionally match unchanged original TU1.5 x86 execution:
2,256 instructions and 857 ordered game stores, with real native reserve/commit
resources. This original-code comparison does not cover lock initialization or
returned SSE/x87 state and does not claim Xbox kernel behavior. Lock operations
retain their separately validated native compatibility ABI. A traced bootstrap
run opens no game assets or executable.

The bounded standalone build freezes its headers, source and linked libraries
under `.local/mcc-native-20261014/native-heap/`. Evidence is under
`analysis/asset-setup/mcc-native-20261014/native-startup/`. The remaining startup
owners and native physical-cache delivery still gate MCC gameplay.

## Native physical and game-arena startup, 2026-10-14

`NativeTu15ArenaBootstrap` now composes the native process heap with the complete
physical, async-worker and game-arena startup procedures. Twenty named cold
storage/operand regions occupy 43,562 bytes: physical partition fields, 150
async jobs and their resource/list bindings, an IO buffer, arena/file bindings,
CRC storage, and the savegame filename format. The authenticated TU1.5 reference
confirms every initial byte. The source procedures construct all handles, job
links, cursors, CRC entries and completion state. No executable section,
constructed bank, ready flag or supplied gameplay result is imported.

The native owner creates actual physical pages, synchronization objects, a host
worker and two private savegame files. It waits for the worker's original idle
wait after all four file jobs, then stops and joins it before handing the bank
to later constructors. This matters because a source job signals completion
before recycling its queue record. Both heap and arena are exposed through
checked 64-bit addresses; foreign and released addresses are rejected.

A fresh unchanged-original-code run executes 1,065,823 TU1.5 instructions.
All 1,049,656 ordered non-stack stores match across 13 complete startup and
allocator calls, including instruction identity, address, width, before and
written values. Returned GP/defined flags match within the explicit native
platform ABI; the unused priority BOOL follows the real host outcome. The full
51,265,536-byte physical allocation also matches. This proves the bounded
startup/allocator graph, not Xbox kernel/thread timing or a loaded game world.
An ungated native startup and a file-access trace additionally pass without
game assets or executable input. The four focused integrated heap/arena CTest
groups pass.

The exact store comparison uses a declared cooperative worker schedule. The
ungated retirement check requires an empty work queue and all 150 jobs exactly
once in the free list, without requiring one scheduling-dependent free-list
order. Twenty-four repeated runs in four parallel processes pass this check.

Frozen sources, headers and libraries are in
`.local/mcc-native-20261014/native-arena/`; receipts, the fresh original-CPU run
and file trace are in `analysis/asset-setup/mcc-native-20261014/native-startup/`.
This composition is available to subsequent native startup owners. The live
reference world still uses executable-backed banks for its later startup and
map constructors. Remaining owners, native cache delivery and full authored
TU1.5 tag conversion are required before MCC gameplay is admitted.

## Native static CRT owners, 2026-10-14

`NativeTu15StaticOwners` adds the next three complete startup constructors:
the static rigid-body deactivator, borrowed Havok scratch buffer and game-engine
class registration. Seven explicitly classified data regions occupy 16,484
bytes. They contain the cold deactivator, the authored scratch header and cold
16-KiB buffer, six authored engine-class bindings and the cold registration
table. The initial bytes match the authenticated TU1.5 reference. The source
constructors publish the vtable/refcount, scratch address/extent and table
entries; completed constructor state is never imported. Engine-class values
are opaque source identities, while native storage uses checked 64-bit addresses.

The owner retains actual native exit registrations and executes both original
shutdown callbacks in reverse order. Its static deactivator retains the original
zero allocation-size flags, so this shutdown path frees no heap object. The
scratch callback restores its source vtable before the enclosing bank releases
the borrowed buffer. Explicit close reports errors and repeated close is inert.

All five constructor/shutdown calls match unchanged original TU1.5 execution:
53 instruction events, returned GP/defined flags, 15 exact ordered data stores,
and all seven resulting data regions. The deactivator's canonical instruction
IDs use the existing authenticated complete-body correspondence. The production
owner also composes successfully with the native heap/physical/arena startup;
its writes match the separately checked source graph. A traced normal run opens
no game assets or executable. All six focused integrated heap/arena/static-owner
CTest groups pass. Native exit registration is an explicit host ABI;
this does not claim that the Xbox CRT table or kernel ran.

The frozen build and pins are under
`.local/mcc-native-20261014/native-static/`; evidence is in
`analysis/asset-setup/mcc-native-20261014/native-startup/static-report.json`.
This completes these three owners, not the whole native world startup. Game
storage, remaining subsystem/class data and map construction, native MCC cache
delivery, and complete gameplay/visual conversion still gate normal admission.

## Native game-storage initializer, 2026-10-14

The complete TU1.5 `13A700` entry is now compiled as
`source_run_game_storage_startup_tu15`, including its original 68-callback loop.
Its native data owner declares only the 68 consumed initializer identities and
the cold game-options binding: 276 bytes in 69 regions, all checked against the
authenticated reference. The remaining lifecycle-table columns are separate
owners. There is no executable or captured-bank input to native startup.

The native heap/arena composition executes the original warm arena call, then
allocates and clears the 4,608-byte game-options block, updates the source CRC,
sets the null selected-BSP short and runs the complete original `__control87`
path. At the first unimplemented subsystem, `6DE60`, it retains the outstanding
call and full caller state. It does not return a successful initializer result.
An absent arena owner also stops at its actual earlier call, before allocating
or publishing game options. This replaces neither missing owners nor their
results with no-op callbacks.

A fresh original-code run first constructs the actual arena, worker and files,
then reaches the same subsystem boundary. The native prefix matches all 1,340
instruction events, 1,156 ordered non-stack stores, GP/defined flags and the
complete game-options bytes. Floating-point controls/status and the retained
empty FP/XMM state also match, with one explicit reserved-bit exclusion:
the emulator reports x87 control word `0x023F`, while native hardware reports
`0x027F`. Bit 6 is reserved in
[Intel's x87 control-word definition, Figure 8-6](https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-1-manual.pdf#page=199).
Every other control bit is compared; neither the original instructions nor
their requested control word is patched. Raw values remain in the receipts.
This does not establish physical Xbox floating-point or kernel behavior.

The game-options backing has a checked native 64-bit address, and a traced
normal run opens no executable or game assets. All eight focused integrated
heap/arena/static-owner/game-startup tests pass. Frozen inputs are under
`.local/mcc-native-20261014/native-game-startup/`, with evidence under
`analysis/asset-setup/mcc-native-20261014/native-startup/game-startup-report.json`.

The next stage below completes that callback's shared allocation and storage
construction through `XNetGetEthernetLinkStatus`. The XNet platform owners and
remaining subsystem/map construction are still required. The 68-callback
initializer has not completed; native MCC cache delivery, full TU1.5 conversion
and normal gameplay admission remain unfinished and gated.

## Native network storage and shared allocator, 2026-10-14

The complete `6DE60`, `70660` and `7EC90` paths are now compiled, with required
owners retained for platform operations. Their native data owner declares 512
bytes in eight regions: the shared allocator binding/singleton/method identities,
cold callback and network-state storage, the QoS state/pool binding, the cached
link-status word and the authored pool-name string with alignment padding.
Every initial byte matches the authenticated TU1.5 reference. Together with
the game-startup data, this stage uses 788 classified bytes in 77 regions.

The existing native raw-storage owner executes the complete original shared
allocator and its real allocation outcome. Source `16E450` constructs the
`transport qos attempts` pool: 32 eight-byte records in a `0x150`-byte object,
backed by an actual `0x1000`-byte native allocation. Header fields, callback
registrations, initialized bytes, pool flags and publication come from source
instructions. The checked host address is 64-bit; source method identities and
the retained execution ABI keep their format width. This is not a claim that
the full 64-bit migration is finished.

The combined game/network prefix matches a fresh unchanged-original-code run:
1,741 instruction events, 1,322 ordered non-stack writes, GP/defined flags,
defined FP controls/status, game-options bytes, all network banks and the full
allocated raw page. The same explicit reserved x87 control-bit exclusion noted
above applies. Real native allocation/return-frame ownership is checked. A
foreign bank is refused, and the complete source release method retires the
allocation and invalidates its checked native address after measurement.

This storage-only stage stops at the actual `6DE80 -> 37654D` XNet link-status call with
its return and parent subsystem frames still outstanding. No fabricated link
status or XNet-startup result skips the later network branch. The original
code sets a local network initialized byte before this query; that byte does
not admit MCC gameplay or complete the game initializer.

The standalone build, sources and library pins are under
`.local/mcc-native-20261014/native-network-storage/`; evidence is under
`analysis/asset-setup/mcc-native-20261014/native-startup/network-storage-report.json`.
All ten focused integrated startup groups pass, including the reference checks.
Native XNet operations, the remaining subsystem/map owners, native MCC cache
delivery, complete authored conversion and final visual/loading/audio proof
remain required before normal gameplay admission.

## Native host link and connection startup, 2026-10-14

`NativeTu15NetworkLink` now implements the named link-query API using actual
Linux interface observations. It enumerates non-loopback link devices even
before an IPv4 address exists and reads carrier, speed and duplex from sysfs.
Interface selection is stable and can be explicit. An unreadable carrier on
an enabled device raises an error; unknown speed or duplex stays unknown.
The source ABI's active/10 Mbps/100 Mbps/duplex flags are encoded from that
observation. Newer speeds retain their measured value without an invented
legacy speed flag. Foreign banks, sites and return frames are refused before
the query or ABI mutation.

The measured host connection was `enp7s0`, active, 1 Gbps and full duplex,
giving flags `9`. This selects the original active-link branch. The initial
game/network prefix matches unchanged original game execution for that API
input: 1,748 instruction events, 1,323 ordered non-stack writes, GP/defined
flags, defined FP state, options, all declared network banks and the complete
raw allocation. This is a native cdecl platform API contract, not emulation
or proof of the Xbox PHY, IRQL, kernel register clobbers or network protocol.

The complete `6E010` connection-initializer body is also compiled, retaining
required owners for all external APIs and callbacks. Its cold prefix has been
validated through the real `6E097 -> 3D7656` XNetStartup call: 1,885 source
instruction events, the same 1,323 non-stack writes, all retained state and
the complete 460-byte pending stack match the original. Source instructions
construct `0c000c0804081808080828ff`, clear the 400-byte WSADATA area and make
a second real host link query. The API and parent subsystem return frames
remain outstanding. No XNetStartup result or later initialized flag is
supplied, and the rest of the body is not yet validated as completed.

All fourteen focused integrated startup tests pass. The frozen build and pins
are under `.local/mcc-native-20261014/native-network-link/`; evidence is under
`analysis/asset-setup/mcc-native-20261014/native-startup/network-link-report.json`.
File traces of both native stages show the actual sysfs observations and no
Xbox executable, ISO, map or other source-asset opens. The reference executable
is used only by the separate offline comparison. Native XNet/Winsock/online
owners, the remaining game/map owners, full native address migration, complete
MCC gameplay conversion and final visual/loading/audio proof remain required.
Normal MCC gameplay admission remains disabled.

## Native XNet/Winsock context, 2026-10-14

The native startup owner now completes the game's XNetStartup and WSAStartup
API calls using an owned desktop network service. The OS supplies IP/driver
initialization. The service checks the actual socket provider, owns a native
48 KiB private pool and creates real nonblocking UDP sockets on demand. Pool
addresses use the explicit 64-bit native type and reject retired contexts.
The authored Xbox record requests a
24-socket budget, 8 KiB receive/send buffers, eight key slots, 40 association
slots and 1,020 QoS data bytes. Socket limits and requested buffers are enforced
by this native transport; key/association/QoS configuration is retained for
the separate owners that implement those operations. This is the PC port's
transport, not proof or emulation of Xbox secure packets, Ethernet hardware,
Xbox Live or console kernel behavior.

The decoder uses TU1.5's actual 12-byte prefix and 76-byte extended record,
with the original default/minimum/maximum tables. Zero selects the default;
other values clamp to the recovered bounds. Invalid size identifiers select
defaults, as the original code does, while a truncated accepted input is
refused before reading past its owner. All 22,783 null, invalid-size and
per-field byte cases match unchanged `3DFD6A` execution through its complete
normalization prefix. Vista's different 13-byte layout is not used. Startup
flags outside the game's verified zero-flag request remain unsupported.

The service test sends and receives actual loopback UDP bytes, verifies the
24-socket limit and slot reuse, and closes and invalidates endpoints on teardown.
Linux reported 16 KiB buffers for the requested 8 KiB values; those are recorded
host observations, not asserted Xbox memory accounting. The API owner validates
its native bank, call site and return frame. It publishes the verified version2
WSADATA fields and retains independent XNet/Winsock references; cleanup retires
the service only when both reference counts reach zero.

This combined game-startup stage reaches `6E0BD -> 3AAD19` XOnlineStartup.
For the measured native platform API inputs, 1,897 original game instruction
events, 1,323 ordered non-stack writes, GP/defined flags, defined FP state,
all previously owned banks and the complete pending stack match. The reference
run declares the native platform API ABI and uses its measured return values;
it does not execute or validate Xbox's XNet/driver initialization. XOnlineStartup
has not returned, `4DC061` is still zero, and the QoS pool has not been activated.

All eighteen focused integrated startup tests pass. Frozen sources/libraries
and binaries are under `.local/mcc-native-20261014/native-network-platform/`;
the receipt is
`analysis/asset-setup/mcc-native-20261014/native-startup/network-platform-report.json`.
The native file trace contains no Xbox executable, ISO or map opens. Native
online startup, later network/game/map owners, the rest of the 64-bit migration,
complete MCC conversion and final loading/graphics/audio/video validation remain
unfinished. Normal MCC gameplay admission remains disabled.

## Native online service ownership, 2026-10-14

`NativeTu15OnlineStartup` implements the game's zero-flag library startup using
the port's actual login, account, directory and matchmaking clients. It retains
those native service owners in a 64-bit context, validates the bank/call/return
frame, and leaves authentication and connection state with the clients. A
missing provider set cannot complete the call. Warm startup retains the same
context; matching releases retire its address after the final reference. This
is native desktop service ownership, not Xbox Live authentication, console
singleton-bank initialization or a replacement Xbox Live protocol.

The frontend now bundles its existing clients into `FrontendLaunch`. Launch
retains those same objects across menu teardown without placing credentials in
launch data or narrowing pointers. The frontend and menu-state objects compile
with this handoff. A live GUI/game transition remains unverified while native
MCC gameplay is gated; no new installed runtime is claimed by this build.

The startup test creates the real configured clients and their idle workers.
It makes no login, discovery-refresh or matchmaking request. Login stays
signed out, the account stays unauthenticated and the directory/matchmaking
clients stay idle. Missing/foreign service owners are refused. Warm references
and context retirement are checked without changing account authentication.

The complete original `70CE0` title-address helper is also compiled. Combined
startup now stops at `70D0C -> 3D725D` XNetGetTitleXnAddr, with its initialized
36-byte output record, return frames and full 500-byte pending stack retained.
For the measured native API inputs, 1,915 original game instruction events,
1,324 ordered non-stack writes, GP/defined flags, defined FP state, game options
and all owned network/raw banks match unchanged original game execution. The
reference declares the native platform API ABI; it does not execute Xbox's
network/online library initialization. Source `4DC061` becomes one after the
three returning platform calls, as its original instruction requires. That
local byte does not finish the network callback or admit gameplay: the title
address has not returned and the QoS pool has not been activated.

All twenty focused integrated startup checks pass. Frozen sources, 18 static
archive dependencies and the validated frontend objects are under
`.local/mcc-native-20261014/native-online-services/`; evidence is under
`analysis/asset-setup/mcc-native-20261014/native-startup/online-services-report.json`.
The native trace has no Xbox executable, ISO or map reads and no external
service connection or packet send. Native title-address/security/QoS operations,
the remaining game/map owners, full 64-bit migration, complete MCC conversion
and final loading/graphics/audio/video validation remain required. Normal MCC
gameplay admission remains disabled.

## Completed cold network callback, 2026-10-14

The native title-address owner now returns actual host interface data. On this
Linux host, the MAC and IPv4 address come from the kernel and are cross-checked
with NetworkManager's active configuration and DHCP lease. Static assignment,
DHCP assignment, address acquisition and Ethernet-only status remain distinct.
Gateway/DNS capability flags come from the observed configuration. Unknown or
conflicting assignment information raises an error instead of manufacturing
an offline/static result. The provider requires Linux/libnm/NetworkManager;
other platform/assignment providers are not claimed as implemented here.

The 36-byte LAN record contains the real IPv4 and six-byte Ethernet address.
It has no fabricated Xbox Live endpoint or identity. The original flag decoder
is independently checked across 24 scalar cases, without emulating IRQ/driver
behavior. DHCP with gateway and DNS is `0x68`: the original Ethernet-only bit
is not added to an acquired-address result. The native owner checks its bank,
source return frame and frame-owned output before querying or writing data.
DNS manager strings are retained without forcing DNS-over-TLS configuration
into an IPv4 payload.

The complete `707D0` availability body is compiled alongside `70CE0` and
`6E010`. Original instructions publish the observed MAC, copy the address
record, select the IPv4 endpoint, activate the real QoS pool through the
existing complete `16E600` body and finish the initial network callback. The
next two initializer slots execute the authenticated one-byte `31D400` RET
body. No supplied module-completion result skips these operations.

For the measured active DHCP input, the combined prefix matches unchanged
original game execution: 2,353 instruction events, 1,395 ordered non-stack
writes, GP/defined flags, defined FP state, game options, all network banks and
the complete raw page. The platform API ABI remains explicitly native. This
proves the cold callback for that observed input, not the entire XNet API
surface, console networking hardware or subsequent network runtime. The three
first game-initializer slots have returned. The actual next boundary is
`13A766 -> 148D40` (RNG initialization), with parent return `13A76C`, offset
`0x6C` and the complete 20-byte pending stack retained.

All twenty-two focused integrated startup checks pass. Frozen sources,
libraries and binaries are under
`.local/mcc-native-20261014/native-network-complete/`; evidence is under
`analysis/asset-setup/mcc-native-20261014/native-startup/network-complete-report.json`.
The native trace contains local manager IPC, no Xbox executable/ISO/map reads
and no external service packet send or connection. The remaining initializer
and map owners, full native address migration, complete MCC conversion and
final loading/graphics/audio/video proof remain unfinished. Normal MCC gameplay
admission remains disabled.

## Completed cold RNG callback, 2026-10-14

The source `148D40` RNG initializer now runs with typed cold CRT/RNG fields,
native UTC and monotonic tick data, and retained native main-thread identity.
`GetTickCount` reads a real native data cell; its imported ordinal is never
used as a pointer. The native ordinary executor has no Xbox TLS expansion or
kernel allocation-notification callback. Those are explicit native platform
policies, not captured console process/thread records.

The actual CRT `time`, `rand`, thread-state allocation and source heap routines
execute. `calloc(1, 0x88)` uses the process heap already constructed by native
startup, including its shared critical-section owner. No allocation result,
random result or completed game-bank contents are supplied. The source shared
stream starts at `0x78A8`; its local stream still uses the original
`time ^ rand ^ tick` calculation. RNG and CRT allocations have native backing
addresses above 4 GiB. Wrong bank, caller/output frame and executor thread are
rejected before mutating the caller or reading a clock.

The original-code oracle now constructs the process heap too, then executes
the unchanged combined startup prefix against the measured native UTC/tick and
thread inputs. All 2,808 instruction events, 1,460 ordered non-stack source
writes, GP/defined flags, defined FP state, RNG/CRT fields, committed heap page,
game options, network banks and pending stack match. Native API writes and
synchronization remain declared platform boundaries. This is bounded startup
proof, not a full gameplay or hardware parity claim.

Four of the 68 game-initializer slots have returned. The actual next boundary
is `13A766 -> 6E830`, parent return `13A76C`, initializer offset `0x90`.
The next network-related owner contains 216 instructions and 26 direct calls;
its first helper `673C0` also has an indirect callback requiring ownership if
selected. Authenticated listings and cold operand inventory are saved under
`analysis/asset-setup/mcc-native-20261014/native-startup/next-network-owner/`.

Frozen sources and libraries are under
`.local/mcc-native-20261014/native-rng-startup/`. The native file/network trace
contains no Xbox executable, ISO or map opens and no external Internet service
connection or packet send. All twenty-four integrated startup checks pass,
including the native and original-code RNG cases. The current shared world
runtime also declares its raw datum identity library dependency, so its
projectile observer links in standalone startup executables. Evidence is under
`analysis/asset-setup/mcc-native-20261014/native-startup/rng-startup-report.json`.
The complete 64-bit migration, remaining native game/map owners, all gameplay
tag conversion, final graphics/loading/audio proof and requested video remain
unfinished. Normal MCC gameplay admission remains disabled.

## Completed cold network session callback, 2026-10-14

The complete `6E830` callback now returns from native startup. Its authored
indirect configuration initializer (`673B0 -> 67400`) executes, followed by
the endpoint/message catalog, gateway, three session records, simulation
channels, online-task pool, player defaults and saved-session constructors.
The generated module contains 183 complete source bodies and 10,857 static
instruction sites. Typed mutable destinations and immutable operands match
the authenticated executable before construction; two nonzero authored mutable
defaults are explicit fields. No executable section or constructed bank is a
runtime input.

Seventeen reached native socket API calls own four real nonblocking IPv4
datagram resources, their binds, buffer queries and broadcast options. This
Linux host restricts port 1000. The native transport therefore translates
source logical ports `1000/1001/1005/1006` to the existing PC session base:
`2000/2001/2005/2006`. Source port operands remain unchanged, and the evidence
records the actual bound endpoints. Both source datagram channel categories
use the PC transport here; Xbox VDP encryption/framing, TCP, later packet APIs
and complete online/security/QoS behavior remain separate unfinished owners.
Native resource errors stop; no failed bind is replaced by a success result.

The player-configuration probe uses the native private save directory. Actual
absence selects the original source defaults. An existing source-format player
configuration deliberately requires its complete file/profile owner; that
case is not implemented by this cold-start result. The source still constructs
the defaults and registers its callbacks itself.

The combined prefix matches unchanged original execution for the measured
native inputs: 97,736 instruction events, 64,707 ordered non-stack writes,
GP/defined flags, all XMM lanes, defined FP state, all declared session banks
and six complete raw allocation pages. Unicorn omits SSE sticky status updates,
so an offline x86-64 backend executes each reached register-only arithmetic
instruction's exact authenticated bytes. It verifies the emulator's result
lanes and supplies hardware MXCSR status for 88 operations. MXCSR `0x1FA0`,
including precision status, is compared without excluding that bit. This
backend is not linked into the game and changes no original game opcode.

Nine of the 68 initializer slots have returned, including six literal RET
callbacks. The next actual call is `13A766 -> 21C0A0`, parent return `13A76C`,
offset `0x144`. All twenty-six focused integrated checks pass. Frozen validation
sources/libraries/binaries are under
`.local/mcc-native-20261014/native-network-session/`; evidence is under
`analysis/asset-setup/mcc-native-20261014/native-startup/network-session-report.json`.
The reference-free trace contains no Xbox executable, ISO or map reads and
no external Internet send/connect operation.

This proves the fresh session constructor for the observed native inputs. The
remaining game/map owners, complete 64-bit execution migration, full gameplay
tag conversion and final graphics/loading/audio/video proof remain unfinished.
Normal MCC gameplay admission remains disabled.

## Completed SGF cache and language startup, 2026-10-14

The actual `21C0A0` callback now returns with six complete source bodies and
173 static instruction sites. It constructs its own cache state, probes the
source logical `n:\sgf` path, creates the corresponding native private `sgf`
directory when absent, queries native language preferences and executes the
original SDK range check and `11F440` game-language conversion. The source
establishes an SGF directory; it does not establish this initializer as an
audio-cache owner merely because an audio string is nearby.

The file adapter performs real private-directory status and creation calls.
Its language provider uses SDL's observed preferred locale. Numeric platform
language definitions are checked against
[pinned primary definitions](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded-legacy/blob/96aabe72e238674a22a61695fb6f5295259edf62/src/common/EmuEEPROM.h);
the settings query follows the
[documented platform ABI](https://github.com/XboxDev/nxdk/blob/master/lib/xboxkrnl/xboxkrnl.h#L3955).
These are owned native preferences and filesystem resources, with no console
EEPROM or fabricated hardware table. English on this host is observed as
`en/US`, encoded as platform1 and converted by original instructions to game0.
Portuguese and unsupported values retain the original game's fallback behavior.

The complete prefix matches unchanged original game execution for those native
inputs: 99,225 instruction events, 64,869 ordered non-stack writes, GP/defined
flags, all XMM lanes, defined FP state, every declared cache/session bank and
six complete raw pages. Thirteen original-CPU language-remap cases cover the
authored range and out-of-range boundaries. A separate warm native invocation
reuses the directory and retained language binding, preserves the complete
cache bank and does not create or query them again. Wrong bank, executor thread
and settings frame are rejected before publication.

All twenty-eight focused integrated checks pass. A shared effect-location
reader was also changed to read its four-byte source words directly, removing
an unnecessary cache-library link dependency from standalone runtime targets;
the byte interpretation remains little-endian. Frozen validation inputs are
under `.local/mcc-native-20261014/native-cache-startup/`, with evidence under
`analysis/asset-setup/mcc-native-20261014/native-startup/cache-startup-report.json`.
The reference-free run reads no Xbox executable, ISO or map files.

Ten of the 68 initializer slots have returned. The next call is
`13A766 -> 21E5D0`, parent return `13A76C`, offset `0x168`. It reads four
bindings at `4F2A9C`; their live native lifecycle must be established before
admitting that state. The remaining game/map owners, complete 64-bit migration,
full tag conversion and final graphics/loading/audio/video proof remain
unfinished. Normal MCC gameplay admission remains disabled.

## Physics definition conversion, 2026-10-14

Normal `Cache` loading now converts `phmo`, `coll`, instanced `sbsp` physics,
and the shapes embedded in `bipd`, `crea` and `vehi`. The resulting definitions
use the ordinary internal tag directory, names and full salted handles. Their
host backing uses checked native 64-bit addresses. Serialized address fields
retain their format width. No Xbox executable or mod overlay is a conversion
input. Original constructors still must bind runtime-only slots before use.

The conversion preserves authored geometry, dimensions, masses, filters,
transforms and MOPP bytecode. MCC's 64-bit tool/vtable pointers are discarded
instead of narrowed. Collision records use the actual TU1.5 `0x70` compiled
stride, rather than the generic plugin's `0x74`. Vehicle phantom banks reserve
four multisphere slots: `0x310` MCC bytes become a `0x2A0` original-layout
prefix. The original cache audit confirms all 428 inspected Xbox vehicle banks
reserve `0x2A0`; the generic plugin only describes their first multisphere.
BSP and unit allocations, surrounding render/resource data and other gameplay
fields remain in place. Unsupported counts, ownership escapes and overlapping
shape layouts are rejected.

All 43 MCC maps pass content and geometry loading with these conversions.
The unchanged English TU1.5 reference constructors match the native procedures
for the actual converted Cache payloads:

| Definitions | Distinct procedure inputs | Constructor phases | Exact ordered stores |
| --- | ---: | ---: | ---: |
| Physics models | 4,070 | 8,140 | 114,568 |
| Collision models | 5,079 | 5,079 | 5,440 |
| Structure BSPs | 93 | 93 | 47,392 |
| Bipeds | 205 | 205 | 5,639 |
| Creatures | 18 | 18 | 622 |
| Vehicles | 233 | 233 | 198 |

Comparisons include complete output bytes, returned general registers, defined
flags and ordered stores. Original execution also proves each cleared active
pointer is written before consumption and produces identical output with those
slots poisoned. Collision, BSP and unit store instruction identities match;
the shared native physics-model procedure retains canonical diagnostic labels.
These checks do not compare returned SSE/x87 state or establish simulation,
authored Xbox-tag equivalence or Xbox framebuffer parity.

`tools/inspect_mcc_physics.cpp`, `tools/oracle_mcc_physics_tu15.py` and
`tools/verify_mcc_physics_constructor.cpp` retain the offline comparison path.
The reference XBE is used only by the oracle. A traced normal MCC content load
opens no Xbox executable, ISO, legacy asset registry or Vista loading bank.
Evidence is under
`analysis/asset-setup/mcc-native-20261013/continuation/physics-normalization/`.
Native world startup, physical cache delivery, the remaining gameplay/audio
consumers and complete authored TU1.5 semantic conversion still block Play.

## Preserved port shader builds

Normal CMake shader targets now build from `shaders/native-programs/` without
opening private Xbox/Vista caches or `assets/asset-sources.tsv`. The source tree
contains the existing port GLSL and program identities, excluding original
token payloads. Menu catalog admission matches the full program SHA-256,
extent, class and ordered declaration. Its fetch/output/UBO ABI remains 2.
Unknown or ambiguous programs still fail admission.

All 78 supported class0..6 GLSL and SPIR-V modules match offline regeneration
exactly. All four particle programs also match their existing generator output.
The menu, effect-light, multiplayer-skinned and campaign-skinned GPU reference
groups pass. An isolated source-only CMake build compiles the menu catalog and
particle shader targets without game assets or reference checkouts; it disables
the separate optional upscaled-movie bundle. This establishes shader build
independence, not complete material or gameplay render parity. The currently
preserved per-map shader metadata libraries are still private import inputs.

Offline maintenance uses `generate_menu_world_vertex_catalog --map REFERENCE_MAP
--output PRIVATE_DIRECTORY` and the explicit `--cache` mode of
`tools/generate_particle_graphics_program.py`. Once the reference results pass,
`tools/export_native_shader_sources.py --catalog PRIVATE_DIRECTORY/sources.json
--particle-directory PRIVATE_PARTICLE_OUTPUTS --output NEW_SOURCE_DIRECTORY`
exports only GLSL, declarations, hashes and provenance. Normal builds use the
preserved-source mode. Neither path substitutes MCC shader programs.

## Native immutable TU1.5 operands

MCC caches now select `OriginalData::native_tu15()` before their load-time
postprocessors run. This factory uses compiled typed initializers for all
named immutable fields recovered by the existing TU1.5 `OriginalData` reader:
61 record types, 377 fields and 12,261 scalar operands. It creates native
container/shared ownership and performs no file I/O. Every scalar bit pattern,
container extent and optional/shared presence matches the authenticated
English TU1.5 reference (12,638 projection entries). A traced factory run opens
only system libraries, and a traced MCC content load opens no Xbox executable.

`tools/generate_native_tu15_operand_schema.py --check` verifies the complete
declared-field inventory. `generate_native_tu15_operands` is an offline
reference tool; rebuilding its output from the reference reproduces
`src/native_tu15_operands.generated.cpp` byte for byte. Executable headers,
sections, object padding, host pointers, mutable runtime banks and the input
path are excluded. Source routine/vtable identities remain opaque source IDs,
not callable host pointers. This is the already classified immutable data
surface, not a substitute for recovering the remaining startup globals.

The frozen build under `.local/mcc-native-20261013/native-operands-snapshot/`
passes all eight selected native test groups, including the four TU1.5 live
world/physics/death groups. All 43 MCC maps pass content loading with no Xbox
executable argument; all 25 multiplayer maps apply 41 verified combat stores.
The cut Falcon RPG bank still reports its missing authored barrel. Reading
content and applying those stores does not establish complete tag semantics,
native gameplay startup or physical Xbox parity. The current MCC gameplay
attempt stops at the explicit native world-startup/gameplay conversion gate.

The campaign checks also exposed a raw-chunk decoding error. Installed MCC
negative-size cache entries start their literal payload at the table's file
offset and have no two-byte zlib header. The old retained Reclaimer reader's
skip shifted those raw chunks and borrowed bytes from the following chunk.
Both port readers now retain their complete literal extents. The three
affected maps pass geometry, models and animation/resource checks after the
fix; synthetic adjacent raw/zlib/raw regression fixtures verify both chunk
ends, including raw data beginning with zlib magic and truncated final data.
No collision normal is clamped or regenerated. Floodlab's 37,509 script
expressions are retained for inspection within the serialized 16-bit datum
index space; its Xbox reference has 36,670. That difference remains conversion
work, rather than being treated as Xbox-equivalent scripts.

Evidence: `native-operands-tests-final.json`,
`native-operands-allmaps-fixed/report.json`,
`native-operands-raw-chunk-evidence.json` and
`native-operands-raw-affected/report.json` in
`analysis/asset-setup/mcc-native-20261013/continuation/`.

## Discovery and cache ownership

`tools/mcc_content.py` searches Steam's Linux, Flatpak, Windows and macOS paths,
Steam's configured external libraries and its MCC application manifest. On
Windows it also reads Steam's installation registry entries and checks common
XboxGames installation paths. These Windows paths are implemented but have not
been exercised on a physical Windows machine. Nonstandard installations can
use `--mcc-source`, the CLI's `--mcc-root`, or `HALO2_MCC_ROOT`.

The imported data lives in `assets/imported/mcc-<id>/halo2`. All regular content
files under the installed `halo2` folder are copied, including Classic maps,
`textures.dat`, sound banks, compiled fonts, variants, videos, terminals and
opaque anniversary asset packages. Standalone program/debug binaries and
`prebuild/shaders_2_cache` are excluded. Shader/material metadata embedded in
maps and opaque packages remains original source data for inspection; it is
never selected as native Vulkan shader code.

Relative filenames, case and directories are retained, including empty
directories and any `tags/` or `data/` trees inside the selected donor.
Derived bitmap payloads and reports are separate cache artifacts. They do not
rename or flatten the source trees, and preserve the original tag namespace.
Installed mods are not automatically activated as vanilla content.

The source is read only. Copying streams SHA-256 and rereads the private result;
the importer rejects source-file or installation-set changes during preparation.
It rejects symlinks, overlapping source/output directories, malformed caches,
excessive decompression, bad full salted tag IDs and invalid texture extents.
Each failed preparation removes only its new staging directory. Successful
preparation publishes a new directory with an explicitly inactive provenance
report. It never writes `asset-sources.tsv` or the selected gameplay executable.

`bitmap-resources.json` retains every resident bitmap bank, frame descriptor,
dimensions, format, flags, authored mip count and all three resource partitions.
Content-addressed `bitmap-resources/*.bin` contain unchanged decoded partition
bytes. This covers diffuse/detail/bump maps, cube/volume textures and UI without
assuming that every format is an ordinary RGBA image. It performs no resizing,
normal-vector regeneration, channel swapping or guessed palette substitution.

## TU1.5 scope and remaining runtime work

`tag-conversion.json` records the ordered stores of the previously recovered
`0011F060` netgame and `00165B30` combat routines. Their source executable is
pinned to SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Exact shared datum/group/path identities and resident-allocation bounds remain
mandatory. Campaign and menu maps are inventoried without merging multiplayer
shared metadata into their independent virtual address spaces.

Modified private caches retain version 13 and their original virtual pointers;
they are serialized as uncompressed MCC caches and independently reopened.
Untouched caches remain byte-identical to their copied source. Relabeling a
version-13 cache as Xbox version 8 is not a conversion.

The native reader now resolves v13 compressed/raw chunks, exact local/shared
owners, player starts, BSP geometry, model geometry and bitmap partitions.
It retains the PC floating vertex layouts and handles PC cube face/mip order
and linear RGBA volumes without selecting MCC shader programs.

Compiled physics/collision and remaining consumer normalization are still
needed. The animation adapter now deep-copies shared banks and expands MCC's
0x60 entries into the original 0x6C entries with owned resource groups. All
114,765 clip occurrences across all 43 maps decode through the existing Xbox
animation codec. This verifies layout/resource decoding, rather than proving
that MCC's additional or changed authored clips equal TU1.5's animation sets.

The generated animation resources now own complete 512-byte sectors,
including zero padding, and use aligned file offsets. The native original-code
I/O boundary resolves only actual graph descriptors and checks their complete
owned resource extent, instead of trying to read generated resources from the
original donor file. All 114,765 clip occurrences across all 43 maps also pass
the owned sector-read check with unchanged codec bytes.

The sound adapter resolves each sound's exact map/shared gestalt owner,
localized permutation records and neutral/English/remastered bank selection.
The installed original MCC executable (`halo2.dll`, SHA256
`de65b4f4fdbf3f0a5eab7431fe530da17dd815599182dfd6ae9b7e21cf171946`)
establishes the signed Opus packet framing, 240-frame decode calls and chunk
allocation rounding. Opus, ADPCM and serialized WMA now use native PCM owners.
Lockout decodes all 4,532 Classic permutations; the tutorial decodes all 2,678
payloads. The latter still exposes the existing sound-variation reader's
unsupported count above 31 for `marine_shortmove`. Sound gestalts/classes
remain source-format metadata behind adapters, rather than fully converted
vanilla Xbox structures. Audio decoding does not establish original Xbox
mixing, timing, waveforms or physical device behavior.
Two postprocessors do not resolve the broader
semantic differences across all gameplay tag groups. The existing
[PC tag comparison](PC_GAMEPLAY_TAG_COMPARISON.md) records those differences and
its explicit schema/opaque-data exclusions. Reports therefore retain
`runtime_active: false` and `full_tu15_parity: false`.

## Graphics restoration reference

The user supplied [Halo 2 Reformation's patch notes](https://www.nexusmods.com/halothemasterchiefcollection/mods/2660)
as investigation leads. They describe changed image content, launch/DLC asset
collisions and material parameter differences as well as optional enhancements.
Restoration must use each original Xbox map and TU1.5 behavior as authority.
Higher resolution alone does not identify a defect or justify downsampling.
The mod's assets have not been downloaded, redistributed or applied.

Initial direct Zanzibar checks confirm different compiled vertex-constant
arrays for `metal_roof`, `basic_concrete`, `zanzibar_seawall` and
`massive_concrete`. The last also uses different template/parameter ordering;
blindly copying the same constant slot would be incorrect. The currently
installed MCC BR bump map remains 256x256 but uses format 11; the Xbox one is
256x256 format 17 (P8 bump). Identical dimensions therefore do not establish
equivalent pixels or sampling semantics.

The first base-image check expands Xbox P8 with the fingerprinted retail
executable's actual palette and unswizzles its texels. BR's resulting RGBA
pixels exactly match MCC's decoded format-11 base image (zero differing
texels). Zanzibar massive concrete differs at 258,506 of its 262,144 base
texels. This corroborates an actual image change for that asset; it does not
attribute every rendering difference to its pixels or compare all mip levels.
Hashes, descriptors and material arrays are retained in
`analysis/asset-setup/mcc-gameplay-20261013/visual-source-check.json`.

The graphics work still requires broader bitmap/palette/channel comparisons,
launch/DLC separation, and rendered TU1.5 validation. The bounded restoration
below is available in a separate inactive converted cache.

An installed Workshop copy of Reformation was located at item `3361280895`.
It is reference evidence, not an automatically mounted runtime mod. The
current restoration work must still distinguish legacy restoration from
enhancements and independently check all affected mip levels and bindings.

The native `inspect_bitmap` tool independently checks Zanzibar's
`massive_concrete_bump` through the engine's ordinary bitmap decoders. The
installed Reformation bank's 512x512 base mip matches the original Xbox bank
exactly; all nine smaller decoded mips differ. Stock MCC differs at 258,506
base-mip texels. The report in
`analysis/asset-setup/mcc-native-20261013/concrete-native/comparison.json`
retains all ten levels and source-map hashes. This is decoder evidence; it
does not establish physical Xbox sampling or justify replacing the whole bank
with Reformation's regenerated mip chain.

`tools/preserve_native_shaders.py` preserves existing Xbox shader metadata,
programs and function tables as owned relocatable libraries. It binds bitmap
references by exact group/path identity and relocates pointers and StringIDs;
it imports no MCC shader program. The selected library is carried into future
MCC preparations independently of the disc. Converted definitions occupy the
normal tag directory under their existing names and handles. No shader mod
overlay or source-specific tag namespace is used. Existing Vulkan shader
source files are unchanged.

The preserved catalog contains 39 original Xbox counterpart map/menu libraries.
Both retained MCC imports now include their verified copies. All 43 caches
still load native BSP geometry after preservation. The two PC-exclusive maps
and shared resource containers have no separate original-map shader library.
That catalog is not a claim of complete original rendering support.

`tools/restore_mcc_zanzibar_bump.py` creates a separate private converted tree,
retaining the complete MCC folder structure and donor provenance. It changes
only Zanzibar's exact bitmap descriptor and appends an owned BGRA resource to
the private texture bank. Its ten mips come from the independently decoded
original bank; Reformation independently corroborates the base image.
Terminal and other maps retain their original references. Its output is
`assets/imported/mcc-zanzibar-restored-20261013`; it is inactive and does not
change the installed MCC files or the game launch selection.

The selected owned correction profile is also applied automatically to future
MCC preparations. It requires the measured MCC map fingerprint, verifies the
original complete pixel bank, appends the converted texture partition and
records donor/converted hashes. It needs the preserved correction profile,
rather than rereading the disc or Workshop mod. A profile-application replay
produces exactly the same map and texture-bank bytes as the native-verified
restoration above. Unrecognized source revisions fail preparation explicitly.

`native_mcc_preserved_shaders` compares Zanzibar's `metal_roof`,
`basic_concrete`, `zanzibar_seawall` and `massive_concrete` against the original
cache. Shader templates, constants, switches, bitmap identities, assembled
pixel programs, vertex programs and existing rendering limitations match.
All ten restored concrete bump mips match the original decoded pixels.
This preserves the existing shader pipeline and its current limitations; it
does not establish final Xbox framebuffer parity or a native MCC playthrough.

MCC also splits a bitmap's old depth word into depth8 and creation flags8.
The native reader and new import inventories now keep these fields separate;
sampling flag 8 no longer makes a 2D image appear to have depth 2049.

## MCC loading assets and authored textures

The current requirement supersedes the earlier Vista loading-bank selection.
MCC-only preparation does not read `loading.bin`, `halo2.exe`, the selected
Vista loading registry, an Xbox ISO or the TU1.5 executable. The old Vista
capture and extracted bank remain historical reference material, rather than
inputs to new MCC onboarding.

`tools/mcc_loading_assets.py` prepares all seven Classic loading bitmap banks
from MCC `mainmenu.map`/`textures.dat`: 13 frames, including the backdrop,
sweep, spinners, logos and tickers. DDS exports retain their source formats,
mips and normal tag/frame identities under `loading/mcc/bitmaps/`. Original
MCC files retain their exact structure under `halo2/`. The exporter does not
invent a correspondence with Vista's twelve executable texture slots.

The two existing hand-authored mechanical-ring SVGs and their rendered DDS
textures are retained in `assets/native-loading-custom/authored-rings-20261013`
and copied to `loading/custom/`. Their hash-pinned profile contains authored
geometry and its rasterizations only. They are review studies, not admitted
replacement textures. Rendering/presentation mapping and actual loading audio
remain separate work; no old Vista ambience is imported.

Both private MCC preparations now contain these 13 MCC frames and four
custom source/texture files. Historical copied Vista files are ignored by this
preparation path. Their prior video is retained for reference only at
`analysis/asset-setup/loading-screen-assets-20261013/final-video-with-sound/vista-match-loading-with-sound.mp4`.

## Requested MCC menu inventory

The importer verifies all 287 requested menu banks, 648 frame owners and 561
unique DDS pages against `tools/reference/mcc-vista-menu-inventory.tsv`.
645 owner rows are Vista-identical; three retain MCC's measured server-icon
alpha variants. The unavailable fourth server-icon state remains explicit.
The twelve original Vista executable loading pages are outside the revised
scope and are never requested as dependencies.

All 40 MCC font/table files retain their exact filenames and case under
`halo2/h2_fonts`, including `MSLCD-14` and the CJK banks. The original map
caches and `textures.dat` retain the images; `vista-menu-inventory.json`
records checked identities and hashes without changing their namespace.
Both private preparations pass complete frame and copied-font checks.

## Native reader verification, 2026-10-13

The separately configured Release build completes the native reader, cache
tests, inspection tool and onboarding launcher. Seven focused CTest groups
pass. They cover native chunk/directory/resource boundaries, cube face/mip
mapping, volume texels, the existing Vista ownership regression, import
transactions, original Vista loading preservation and launcher routing.

All 43 copied MCC caches pass native BSP geometry loading. Lockout additionally
loads 181 drawable model banks (329,292 expanded vertices), its BR texture and
12,882 collision triangles. Its broad content inventory reports an additional
Falcon RPG definition whose primary trigger has no authored barrel; this has
not been hidden or counted as a verified vanilla definition. Compiled shape
normalization is covered separately above; authored gameplay semantics and a
native MCC gameplay run remain outside this geometry result. The animation, audio and bounded
shader/bitmap checks above are separate evidence.

The layout coverage report inventories 79 source tag groups, with explicit
differences including 36 animation-layout entries, 218 physics-layout entries,
41 collision-layout entries and 163 sound-gestalt-layout entries. This is an
inventory of needed work, not a successful semantic conversion. Evidence is
under `analysis/asset-setup/mcc-native-20261013/`.

## Local verification, 2026-10-13

The installed MCC copy prepared 883 asset files (29.10 GiB), 43 Classic caches,
25 multiplayer maps, 19,686 resident bitmap-bank occurrences and 50,283 bitmap
frames. It decoded 19,518 distinct texture partitions (1,773,777,471 bytes).
All 1,025 combat destinations already contained their expected values; there
were zero numeric changes. These are import/field checks, not a playthrough or
Xbox framebuffer comparison.

The retained import is
`assets/imported/mcc-4c5bac9f9a4549f295603709972b148a`.
Evidence lives in `analysis/asset-setup/mcc-gameplay-20261013/`.
Focused synthetic tests cover discovery, compressed and raw chunks, malformed
data, exact salted references, texture segments, private map serialization,
source mutations, publication/rollback, shader exclusion and cache ownership.
Relocated launcher tests cover the MCC preparation path before first-run setup.
Independent verification rereads all 883 original/cached file hashes, all 43
directories through the preexisting PC cache reader, and all 19,518 texture
partitions directly from the original segmented stream. It passes in
`verification.json`. Both rebuilt installed launchers pass 27 relocated routing
checks each and retain the previously selected native runtime and TU1.5 XBE.
`analysis/asset-setup/mcc-native-20261013/continuation-verification.json` chains
that historical verification to the current provenance, rehashes all 883
copied files, confirms all 22 source directories and verifies the 14 selected
loading-bank files. The complete gameplay conversion remains inactive.

The continuation builds the native game executable and passes seven focused
CTest groups, including the standalone movie/audio build. The engine-facing
`TagSystem` now uses MCC's checked resident metadata, just as the converted PC
owner does, instead of requiring an original Xbox disk BSP header. All 43 maps
pass this tag-system BSP handoff and native geometry loading.

The earlier reference-executable Zanzibar attempt stopped at
`Original runtime BSP mapping differs from its cache owner`. Raw MCC metadata
is now explicitly rejected at the original-ABI cache boundary, instead of
being delivered as packed Xbox structures. MCC-only loading also refuses an
implicit `default.xbe` fallback. Classified immutable TU1.5 operands now use
the native factory described above; mutable world startup still must replace
the remaining executable-backed banks. The physical cache mapping and authored
semantic conversion remain unfinished. Compiled physics normalization is
covered separately above.
This is a concrete gameplay admission failure, not a successful map-loading
run. It must be resolved together with full metadata/layout and authored
gameplay conversion; bypassing its original cache contract would not establish
the requested vanilla TU1.5 dataset. The reports and logs are retained under
`analysis/asset-setup/mcc-native-20261013/continuation/`, including `status.json`.

## Completed native controller producer and input presets, 2026-10-14

The actual controller producer `1273C0` now executes before `13A700`, and the
complete input preset callback `21E5D0` returns. Ten complete authenticated
bodies contain 571 static instruction sites. The source opens and publishes
its own four controller bindings at `4F2A9C`; executable-initial zero handles
are not supplied as a substitute for device initialization.

`NativeTu15InputDevices` retains actual SDL subsystem/controller resources,
enumerates native instance IDs, reports insertion/removal masks, opens and
closes controllers, samples buttons/axes and submits actual motor requests.
Native controller pointers remain 64-bit; source handles are separate opaque
IDs. The modern controller profile maps RB/black and LB/white, digital face
buttons to 0/255, SDL trigger values to the source byte range and vertical
thumb axes to the source orientation. Original integer dead zones, analog
hysteresis, hold counters, durations and preset/presence construction remain
source instructions. The capability boundary supplies only the subtype byte
consumed by this authenticated caller. The native registry currently provides
game controllers; the second console peripheral class has no registered
desktop provider. Console USB, pressure-button hardware, asynchronous event
feedback and later UI hotplug consumers are not claimed.

The independent SDL virtual-device proof covers four actual controllers,
stick extrema and dead-zone boundaries, state changes and unchanged samples,
removal, preset construction and complete source teardown, plus an empty
selection. It matches unchanged original execution across 15,547 instructions
and 1,940 ordered non-stack writes, complete GP/defined flags/XMM/FP state and
every declared input bank. A nonzero native motor probe reaches the actual
SDL callback and then stops both motors. This host's exact SDL2-compat
revision has a virtual-callback return conversion defect in
[its upstream implementation](https://github.com/libsdl-org/sdl2-compat/blob/release-2.32.74/src/sdl2_compat.c#L11174-L11178).
Only the virtual test fixture adapts that identified revision; production
continues to require a successful actual SDL rumble return value. Wrong bank,
executor, return frame, call site, tail flag and device type are rejected.

The integrated producer prelude matches 302 instructions and 112 non-stack
writes. The following main prefix matches 99,567 instructions and 65,086
ordered writes, its pending stack, all GP/defined flags/XMM/FP state, declared
input/cache/session banks and six complete raw pages. The RNG owner now records
the clock input at the actual seed instruction `148D98`, so earlier input
initialization clock reads cannot change which receipt is interpreted as the
RNG seed. Thirty-three focused integrated checks pass; the final five input
checks also pass using the current oracle fixture. The separate frozen build
passes both original comparisons. Its ordinary native run opens no Xbox image,
ISO or map and sends no external INET traffic.

Frozen inputs are under `.local/mcc-native-20261014/native-input-startup/`;
the evidence index is
`analysis/asset-setup/mcc-native-20261014/native-startup/input-startup-report.json`.
Eleven of 68 initializer slots have returned. The next actual pending call is
`13A766 -> 1589D0`, table offset `0x18C`, with return `13A76C` at `6000EFF0`.
Its constructor clears a `0xE0`-byte destination and calls `3EC900` with two
authored callback identities. That target is a jump in the original `BINK`
section to `377460`; its ownership is the next task, not a supplied successful
registration. The full native execution migration, all-tag conversion,
MCC-only world admission, graphics parity and loading video remain unfinished.
The execution ABI remains 32-bit and normal MCC gameplay admission stays gated.

## Completed Bink memory registration and level-handle startup, 2026-10-14

The complete `1589D0` constructor, actual `BINK`-section `3EC900` jump and
`377460` registration body now execute as compiled source code. They clear
their own `0xE0`-byte destination and publish the authored `159240` allocation
and `159340` release callback identities. These are memory callbacks, not
file callbacks; their later buffer consumers remain unfinished. No successful
platform registration result is supplied. This adds three complete bodies
with seventeen static instructions and another authored literal RET callback.

The following complete `1A5410` initializer now creates its `level handles`
and `multiplayer level handles` pools through the already-owned native raw
allocator. Original constructors define the twenty/fifty eight-byte slots,
pool headers, names and ownership flags. Their source requests are `0xF0` and
`0x1E4` bytes, backed by two real native pages. Source code also initializes
all sixty-four level-handle pairs to the original invalid/zero state. The new
constructor adds fifty static instructions and 564 typed cold-data bytes;
registration adds 232 bytes. Total declared cold data is 217,616 bytes.

The combined main prefix matches unchanged original TU1.5 execution across
100,468 instructions and 65,358 ordered non-stack writes, its pending stack,
all GP/defined flags/XMM/defined FP state, all declared constructor banks and
eight complete raw pages. The separate input prelude remains 302 instructions
and 112 writes. Both the current optimized integrated build and the separate
pinned-library build pass the original comparison. All thirty-seven focused
checks pass. File/network tracing of the ordinary pinned build finds no Xbox
image, ISO or map accesses and no external INET sends/connects.

The comparison harness also handles the pool module's newly selected TU1.5
instruction callback IDs while keeping older pinned canonical IDs usable.
The authenticated canonical/actual site sets contain 585 entries each and are
disjoint. Only known actual instruction callbacks bypass translation; store
observers retain their canonical mapping and the entire original trace/store
comparison remains exact. The temporary shared physical-memory API build
failure was resolved by its ongoing owner work; no compatibility workaround
or revert was added here. A tracing/test UDP-port overlap was resolved by
running the checks sequentially, preserving actual bind errors.

Evidence is indexed by
`analysis/asset-setup/mcc-native-20261014/native-startup/level-handles-startup-report.json`.
Pinned inputs are under `.local/mcc-native-20261014/native-level-handles-startup/`.
Fourteen of 68 initializer slots have returned. The next actual pending call
is `13A766 -> 134B00`, table offset `0x1F8`, with return `13A76C` at `6000EFF0`.
It requests a `0x452E8`-byte allocation through `33B3AC`, publishes `51DF44` and
calls `137460` twice. Those constructor/resource owners are the next task;
the binding is not supplied from a captured bank. Full native64 execution,
large mod-bank admission, all-gameplay-tag vanilla conversion, MCC-only world
loading, graphics parity and the loading video with sound remain unfinished.
Existing port shader programs are preserved; gameplay admission stays gated.

## Completed renderer storage and ordinary heap growth, 2026-10-14

The complete `134B00` renderer initializer now returns. It executes the
already-authenticated renderer producer, both `137460` descriptor constructors
and their `166B20` child constructors. The source requests `0x452E8` bytes of
raw storage, constructs both parameter blocks and publishes `51DF44` from the
actual native allocation. Two `0x2AE0`-byte cold constructor destinations and
the four-byte binding add 21,956 declared bytes; total typed cold data is now
239,572 bytes. No initialized descriptor or renderer bank is imported.

All forty-one ordinary allocation calls execute original `32A3BB` and its
malloc/RTL heap path against the constructed native process heap. Native
critical-section ownership covers the complete allocator call. Seven complete
TU1.5 heap-growth helper bodies add 796 static instruction sites, including
the actual `2DD367` commitment path reached by `2DDEB9`. Its native boundary
validates the bank, source return, base/size cells and complete request before
committing real pages. Four commitments add 4,096, 4,096, 4,096 and 40,960 bytes,
leaving 57,344 committed heap bytes including the original page. Pool/heap
headers, free lists, descriptor pointers and constructor results remain
products of source instructions. The renderer's raw request has 286,720 bytes
of native backing; its source token is `10008000`. Native backing addresses
for the raw contexts and heap descriptors are checked above 4GiB. Source
tokens remain separate from those native pointers.

The combined prefix matches unchanged original TU1.5 execution across 112,590
instructions and 66,405 ordered non-stack writes, its full pending stack,
GP/defined flags, all XMM/defined FP state, declared constructor banks, nine
complete raw allocations and the entire committed process heap. The existing
two-instruction `2E0C80` bit-scan body also executes unchanged. The input
prelude remains separately matched at 302 instructions and 112 writes.
Both the current optimized integrated build and separate pinned-library build
pass the original comparison. All thirty-nine focused checks pass. Renderer
allocation calls reject a foreign bank, return frame or call site before
publication. The reference-free native trace opens no Xbox image, ISO or map
and sends no external INET traffic.

Evidence is indexed by
`analysis/asset-setup/mcc-native-20261014/native-startup/renderer-startup-report.json`.
Pinned inputs are under `.local/mcc-native-20261014/native-renderer-startup/`.
Fifteen of 68 initializer slots have returned. The next actual pending call is
`13A766 -> 22C100`, table offset `0x21C`, with return `13A76C` at `6000EFF0`.
Its seven source instructions clear four fields at `50EBFC` and then set the
third field to `FFFFFFFF`; its consumer semantics still need evidence.
The execution ABI remains 32-bit. Full native64 execution, large mod-bank
admission, all-tag vanilla conversion, MCC-only gameplay loading, full graphics
parity and the loading video with sound remain unfinished. Existing port
shader programs are preserved and gameplay admission remains gated.


## Completed clock, state context and player storage startup, 2026-10-14

Twenty of 68 initializer slots now return through native-owned storage.
Original `22C100` executes all seven instructions and its five ordered field
writes. `126640` executes the actual already-initialized arena branch; no flag
or return is forced. Existing `149050` allocates and clears the 36-byte game
clock through the source arena/CRC path and publishes `51DF54`. This proves its
constructor, not the later clock governor or playback. Original `1E7710`
allocates and clears 228 bytes and publishes `5020C4`; its consumer meaning
remains unresolved.

Original `14DFE0` allocates the sixteen-slot player pool (stride `0x21C`,
allocation `0x2210`) through the exact `127210` arena method and builds the
304-byte player-state allocation through the source CRC path. Pool/header,
validity, ownership and state fields are all written by source instructions.
No players are created or admitted. The authored allocator method identity,
player pool name and constructor bindings add twenty cold bytes. Total typed
cold data is 239,616 bytes; the constructed arena cursor is 18,652. Native
backing for all three new allocations is checked above 4GiB; source tokens
remain distinct from those native pointers.

The cumulative main prefix matches unchanged TU1.5 execution across 113,191
instructions and 66,539 ordered non-stack writes, full pending stack, GP,
defined flags, XMM/defined FP, all declared constructor banks, nine raw
allocations and the entire committed process heap. Both current optimized and
separate pinned-library builds pass. All 47 focused checks pass. The ordinary
native trace opens no Xbox image, ISO or map and sends no external INET traffic.

A controller appeared on the host after the original empty-device fixtures
were captured. Regression input selection is now explicit: empty-registry
reference cases still execute the actual SDL provider and complete source
producer, while a new cumulative case opens four real SDL virtual controllers.
Its 2,212-instruction/308-write input prelude and entire main prefix match the
unchanged original. The explicitly empty prelude remains 302/112. Default
startup continues to enumerate actual host devices; the host lifecycle test
observed one controller. The fixtures supply neither source controller
bindings nor completion bytes. Earlier device-list-dependent comparison
failures are retained as diagnostic evidence.

Evidence is indexed by
`analysis/asset-setup/mcc-native-20261014/native-startup/player-startup-report.json`.
The separately validated source/library/binary snapshot has 1,013 checked pins
under `.local/mcc-native-20261014/native-player-startup/`. The next pending call
is `13A766 -> 1888A0`, table offset `0x2D0`, return `13A76C` at `6000EFF0`.
Its nineteen instructions allocate `0x264` arena bytes, update the CRC, publish
`4F9B54` and clear the first byte; it has not been admitted by this stage.
Execution remains 32-bit. Full native64 execution, large mod-bank admission,
all-tag vanilla conversion, MCC-only world loading, complete graphics parity
and the loading video with sound remain unfinished. Existing port shader
programs are preserved and gameplay admission stays gated.


## Completed local-view and action-cache arena storage startup, 2026-10-14

Twenty-three of 68 initializer slots now return. Three existing complete
TU1.5 constructor bodies are reused without another instruction lowering:
`1888A0` allocates `0x264` bytes, publishes `4F9B54` and clears its first byte;
`1EECF0` allocates `0x6C8` bytes and publishes `52BE64`; `159F80` allocates and
clears `0xC9C` bytes, clears 156 static bytes at `50ED30`, publishes `4F63B8`
and sets its allocation's `+0xC14` field to `FFFFFFFF`. Original `1EECF0` does
not clear the allocation: the native path preserves that behavior. The local
view and action-cache bindings have existing original consumer evidence;
no new gameplay meaning is assigned to `159F80`'s fields. The action-cache
constructor dispatch verifies its actual main-startup call and return frame.

The three complete bodies contain 63 static instruction sites. All arena
allocation/cursor and CRC updates execute source instructions. Native backing
for all three allocations is checked above 4GiB; no constructed bank or binding
is supplied. The additional cold bindings/static destination total 168 bytes,
raising declared cold data to 239,784 bytes and the constructed arena cursor
to 24,228. Later view/action/gameplay consumers remain outside this proof.

The cumulative prefix matches unchanged original TU1.5 execution across
114,280 instructions and 67,396 ordered non-stack writes, complete pending
stack, GP/defined flags, XMM/defined FP, declared constructor banks, all nine
raw allocations and the complete committed process heap. Explicit empty
input and four actual SDL virtual-controller cases pass in both current
optimized and separate pinned-library builds. Their producer preludes remain
302/112 and 2,212/308 instructions/writes. All 51 focused checks pass without
skips. The ordinary native file/network trace opens no Xbox executable, ISO
or map and sends no external INET traffic.

Evidence is indexed by
`analysis/asset-setup/mcc-native-20261014/native-startup/view-storage-startup-report.json`.
The separate source/library/binary snapshot has 1,016 checked pins under
`.local/mcc-native-20261014/native-view-storage-startup/`. The next actual
pending call is `13A766 -> 69220`, table offset `0x33C`, with return `13A76C`
at `6000EFF0`. Its complete 29-instruction constructor calls the 47-instruction
`85E70` registration helper; these bodies have been inventoried but remain
unowned by this startup path. Execution ABI remains 32-bit. Full native64,
large mod-bank admission, all-gameplay-tag conversion and vanilla admission,
MCC-only world loading, complete graphics parity and the loading video with
sound remain unfinished. This work does not modify the existing port shader
programs or install a launcher runtime; gameplay admission remains gated.


## Completed static definition registration startup, 2026-10-14

Twenty-five of 68 initializer slots now return. Complete original `69220`
and `85E70` add 76 authenticated static instruction sites. The constructor
clears both 32-entry static arrays, the helper writes 43 literal source
definition identities, and the caller publishes counts 17/26 and its final
publication byte. The following literal RET callback also executes. All
bindings, counts, table values and publication come from source instructions;
no initialized registration bank is supplied. The main constructor's actual
call/return frame is validated. The constructor destination and bindings add
277 cold bytes, for 240,061 declared bytes total; the arena cursor stays 24,228.
These opaque source addresses are not native host pointers. This stage does
not establish their downstream definition consumers or widen their ABI.

The cumulative prefix matches unchanged TU1.5 across 114,429 instructions and
67,511 ordered non-stack writes, complete pending stack, GP/defined flags,
XMM/defined FP, declared banks, nine raw allocations and the complete committed
process heap. Explicit empty input and four real SDL virtual controllers pass
in both optimized integrated and separate pinned-library builds. All 55
focused checks pass without skips. The ordinary native trace opens no Xbox
executable, ISO or map and sends no external INET traffic.

Evidence is indexed by
`analysis/asset-setup/mcc-native-20261014/native-startup/static-registration-startup-report.json`.
The separate snapshot has 1,021 verified source/library/binary pins under
`.local/mcc-native-20261014/native-static-registration-startup/`. The next
actual pending call is `13A766 -> 1F3830`, table offset `0x384`, return
`13A76C` at `6000EFF0`. Its 33-instruction constructor allocates 28 arena bytes
and reads three authored scalar constants plus a three-element source vector;
its original operands have been inventoried. Consumer meaning is unresolved
and this owner has not been admitted by this stage.

Execution remains 32-bit despite native resource backing above 4GiB. Full
native64 execution, large mod-bank admission, all-tag vanilla conversion,
MCC-only world loading, complete graphics parity and the loading video with
sound remain unfinished. This work preserves the port shader programs and
does not install a launcher runtime. Gameplay admission remains gated and
the overall goal remains active.


## Completed native object storage and following arena states, 2026-10-14

Thirty-one of 68 main initializer slots now return. Original `1F3830`
constructs its 28-byte arena state from the three exact scalar bit patterns,
three-element vector and last cleared field, then the following literal RET
executes. The new immutable operands and binding add 28 bytes while sharing
the existing authored scalar-one operand. Its consumer meaning remains
unresolved. No scalar or vector result is substituted for source execution.

Complete original `B87F0` now constructs widget, antenna, cloth, liquid, light,
device-group and object-header pools; its object memory pool has a 1MiB payload.
The original seventeen object-type records build their actual mutable `+C4`
chain, execute all four literal RET type constructors and the device-group
constructor, and retain their authored program metadata separately from those
mutable links. The light, collideable-object and noncollideable-object cluster
storage and their reference pools also execute. Fourteen complete existing
source bodies contain 442 static sites; shared pool/CRC producers are reused.
The arena allocator's six object caller sites validate the native bank,
method identity, original return and allocator binding. The existing arena
module also executes the complete original `11F390` name wrapper through its
authenticated canonical identity.

A native CRT boundary implements only the reached constructor-name formats
at `11F39F -> 32BA80` (or its authenticated arena canonical site `11E0FF`).
It validates the bank, thread, call/return frame, original variadic argument
cell, owned destination and literal format, then uses actual native formatting.
All twelve calls return real results. Foreign bank/thread/site/frame cases
are rejected before ABI mutation. Pool names, headers, validity, allocation
cursors, CRC and bindings remain products of source instructions around that
boundary. No constructed object bank is imported. The new cold metadata,
strings, links and constructor destinations total 11,914 bytes. All 1,486,128
new arena bytes are compared with unchanged original execution, and their
native backing is checked above 4GiB. This proves storage construction, not
object factory execution, map admission or full object gameplay.

The following `10E3B0`, `10FA30` and `1F4700` constructors allocate `0x1D88`,
`0x304` and `0x24` bytes and publish their original bindings. The first clears
its entire allocation; the others clear only their original `+0x300` and
`+0x20` bytes. Two bodies are reused and the complete nineteen-instruction
`10FA30` body is newly lowered. Their consumer meanings remain unresolved.
Twelve new cold binding bytes bring total typed cold data to 252,015 bytes;
the constructed arena cursor is 1,518,752.

The cumulative prefix matches unchanged TU1.5 execution across 124,778
instructions and 72,648 ordered non-stack writes, complete pending stack,
GP/defined flags, XMM/defined FP, all declared constructor banks, all nine raw
allocations and the complete committed process heap. Explicit empty input
and four real SDL virtual controllers pass in both current optimized and
separate pinned-library builds. All 67 focused checks pass without skips.
Reference-free tracing opens no Xbox executable, ISO or map and sends no
external INET traffic. The earlier object-storage milestone independently
matches 122,648 instructions and 70,747 writes in both builds and input cases.

Evidence is indexed by
`analysis/asset-setup/mcc-native-20261014/native-startup/object-and-auxiliary-startup-report.json`.
The object-storage snapshot has 1,029 verified pins under
`.local/mcc-native-20261014/native-object-storage-startup/`; the cumulative
snapshot has 1,034 verified pins under
`.local/mcc-native-20261014/native-aux-object-state-startup/`. The next actual
pending call is `13A766 -> 12560`, table offset `0x45C`, return `13A76C` at
`6000EFF0`. Its 187-instruction renderer/resource initializer has 41 direct
calls; the native resource owners are incomplete. Original graphics-driver
code is not a native port platform boundary, and that initializer has not
been bypassed or admitted.

Execution ABI remains 32-bit despite native backing above 4GiB. Full native64,
large mod-bank admission, all-tag conversion and vanilla admission, MCC-only
world loading, complete graphics parity and the loading video with sound
remain unfinished. Existing port shader programs are preserved by this work;
no launcher runtime is installed. Gameplay admission stays gated and the
full goal remains active.


## Renderer device-request producer, 2026-10-14

The thirty-second initializer has started; 31 of 68 slots are fully complete.
Complete source bodies `12560`, `169D0`, `16A10` and `128A0` add 322 compiled
static instruction sites. Their current fresh-input prefix executes the
original display fields, logical outer rectangle `(0,0,480,640)`, safe rectangle
`(36,48,444,592)`, source SDK allocation-size inputs and all seventeen device
presentation words. The literal operands and cold destinations add 87 bytes;
total declared cold data is now 252,102 bytes. These are source logical display
inputs, not a change to the port's configured physical output resolution.

The real pending call is now `129AD -> 3FF320`, return `129B2`, stack `6000EF80`.
Its parameter block is `6000EF94`, output token is `515E90`, and flags are `40`.
A typed frame decoder validates the source caller, return, output, parameter
cell and flags, and resolves both retained destinations to native addresses
above 4GiB. It preserves all seventeen presentation DWORDs as opaque fields
until their native mappings are admitted. Foreign flags, call site and output
are rejected. Decoding creates no GPU device. The graphics-device binding and
`4922A8` renderer-completion byte remain cold zero. No device result, initialized
SDK bank or completion byte is supplied.

The cumulative source prefix matches unchanged TU1.5 across 124,899 instructions
and 72,666 ordered non-stack writes, all GP/defined flags, XMM/defined FP,
constructor banks, nine raw allocations, complete committed process heap and
all 132 bytes of the pending stack. Explicit empty input and four real SDL
virtual controllers pass in both current optimized and separate pinned-library
builds. All 71 focused checks pass without skips. Native file/network tracing
opens no Xbox executable, ISO or map and sends no external INET traffic.

The offline SSE reference helper now supports the newly reached absolute
scalar memory operands. The original emulated instruction and memory remain
unchanged; the hardware helper relocates only its own operand addressing to
retained scratch containing the actual emulated scalar. It compares every
result lane and imports real MXCSR status. An independent exact multiplication
case and a fractional case establish both lane equality and the precision
sticky bit that Unicorn omits. The complete original prefix includes ninety
hardware-checked scalar operations. No source-game opcode is patched, and
precision status is not excluded. The first unsupported-helper diagnostic is
retained separately; the corrected original captures use the `-v2` directories.

Evidence is indexed by
`analysis/asset-setup/mcc-native-20261014/native-startup/renderer-request-progress-report.json`.
The separate snapshot has 1,039 verified source/library/binary pins under
`.local/mcc-native-20261014/native-renderer-request/`. The original 45-site
`3FF320` D3D SDK wrapper and both actual four-byte stack-cleanup returns have
been inventoried as references. Its Xbox driver is not executed by native
startup. An actual port GPU/device owner is still required before the pending
call can return; source initializer `12560` is not complete.

Execution ABI remains 32-bit. Full native64, large mod-bank admission, all-tag
conversion and vanilla admission, MCC-only world loading, complete graphics
parity and the loading video with sound remain unfinished. This work preserves
the port shader programs and does not install a launcher runtime. Gameplay
admission stays gated and the full goal remains active.

## Native port graphics-device binding, 2026-10-14

The actual `129AD -> 3FF320` source SDK call now binds the port's live Vulkan
renderer through an owned device lease. The opaque source handle `F9000001`
stays distinct from the real native device address above 4GiB. Only a live
Renderer can issue the lease; retired, missing, foreign and duplicate cold
requests are rejected. An active use retains the actual device implementation.
Presentation release invalidates old leases, and rebuilding issues a fresh
lease. The SDK boundary binds the existing native device; it does not execute
Xbox graphics-driver code or import an initialized SDK state bank.

After the measured native return, original `128A0` copies 53 authored control
DWORDs, writes its selectors and publishes its own device-binding byte. The
added literal and cold storage totals 434 bytes, bringing declared cold data
to 252,536 bytes. Original `12560` then reaches `125C8 -> 16AF0`, return `125CD`
at `6000EFD8`. This is still initializer 32 in progress; only 31 of 68 slots
are fully complete, and the renderer-completion byte remains cold zero.

The cumulative prefix matches unchanged TU1.5 across 124,981 instructions and
72,723 ordered non-stack writes, all GP/defined flags, XMM/defined FP,
constructor banks, nine raw allocations, the complete committed process heap
and all 44 bytes of the pending stack. Empty input and four real SDL virtual
controllers pass in both the optimized integrated build and the separate
pinned build. All 76 focused checks pass without skips. Live GPU startup,
ordinary/custom-border draws, device retirement and presentation rebuilding
pass on the RTX 5080 with no Vulkan validation messages. These checks do not
establish Xbox framebuffer or full gameplay visual parity.

The separate build pins current optimized static libraries and rebuilds its
39 direct startup sources against current headers. It does not rebuild every
library body. Compiled port shaders and the validation layer are also pinned;
the previous renderer-request libraries are not reused. Its first diagnostic
retains a missing per-source network-provider build setting; the corrected
build preserves the integrated setting and passes both original comparisons.
Reference-free tracing opens no Xbox executable, ISO or map and sends no
external INET traffic. Evidence and immutable pins are indexed by
`analysis/asset-setup/mcc-native-20261014/native-startup/graphics-device-progress-report.json`
and `.local/mcc-native-20261014/native-graphics-device/pins.json`.

Execution ABI remains 32-bit. Full native64, large mod-bank admission, all-tag
conversion and vanilla admission, MCC-only world loading, complete graphics
parity and the loading video with sound remain unfinished. Existing port
shader programs are preserved by this work. No launcher runtime is installed;
gameplay admission stays gated and the full goal remains active.

## Native renderer state and table construction, 2026-10-14

Original `16AF0` and `1C190` now execute their complete fresh cold-input paths.
The two authenticated bodies add 94 compiled instruction sites. They construct
the renderer selectors and exact scalar fields, then clear and build the
complete 2,176-byte bit-mask table using all sixteen authored signed coordinate
pairs. Their declared cold destinations and coordinate operands add 7,348 bytes;
the total declared cold data is 259,884 bytes. The existing owned `3.0f` operand
is reused. No constructed table or renderer-completion value is supplied.
The optional warm-release branch of `16AF0` still requires its external owner.

The full prefix matches unchanged TU1.5 across 310,450 source instructions and
81,988 ordered non-stack writes, with GP/defined flags, XMM/defined FP,
constructor banks, nine raw allocations, the committed process heap and all
44 pending stack bytes compared. Both explicit empty input and four real SDL
virtual controllers pass in the optimized integrated and independent pinned
builds. All 80 focused checks pass without skips. Both builds bind the actual
live port renderer with Vulkan validation enabled and no validation messages.
Reference-free tracing opens no Xbox executable, ISO or map and sends no
external INET traffic.

The separate validation rebuilds 41 direct startup sources and preserves their
exact integrated per-source definitions. Its 63 current static libraries,
compiled shaders, validation layer and source provenance are pinned separately;
library bodies are not rebuilt by this validation project. Evidence is indexed
by `analysis/asset-setup/mcc-native-20261014/native-startup/renderer-state-progress-report.json`
and `.local/mcc-native-20261014/native-renderer-state/pins.json`.

The actual next pending call is `125D2 -> 1C7F0`, return `125D7` at `6000EFD8`.
Its real static allocator chain and `13FC50` pool constructor are inventoried,
but have not been admitted for this caller. Initializer 32 remains in progress;
31 of 68 slots are complete and `4922A8` remains cold zero. Full native64
execution, large mod-bank admission, all-tag TU1.5 vanilla conversion, MCC world
loading, graphics parity and the loading video with sound remain unfinished.
Existing shaders are preserved, gameplay remains gated and the full goal is
active.

## Native renderer cache pool construction, 2026-10-14

Original `1C7F0` now constructs the vertex-shader LRU cache pool through its
actual raw allocator and existing complete `13FC50`/datum-array constructors.
The added body has 39 static instruction sites; its cold counters, publication,
name and exact allocator identity add 96 bytes. The source requests 384 bytes
and the native owner supplies a real 4KiB page. Original code constructs every
header, callback identity, embedded array and validity field. There are now ten
owned raw allocations; the arena cursor remains 1,518,752. No Xbox SDK state
bank, pool header or renderer completion is supplied.

The cumulative prefix matches unchanged TU1.5 across 311,073 instructions and
82,114 ordered non-stack stores. Defined CPU state, all constructor banks,
ten complete raw allocations, the committed process heap and all 52 pending
stack bytes match. Empty input and four real SDL virtual controllers pass in
both optimized integrated and independently rebuilt startup-source binaries.
All 84 focused checks pass without skips. The source and library snapshot has
43 direct startup sources and 63 current static archives; archive bodies are
copied rather than rebuilt. It also pins compiled shaders and the validation
layer. Live native startup emits no Vulkan validation messages and the
reference-free trace opens no Xbox executable, ISO or map and sends no
external INET traffic. Evidence is indexed by
`analysis/asset-setup/mcc-native-20261014/native-startup/renderer-pool-progress-report.json`
and `.local/mcc-native-20261014/native-renderer-pool/pins.json`.

Initial diagnostics are retained. The constructor test's incorrect field
expectations were corrected against a separate unchanged original constructor
run. The reference harness now recognizes the authenticated `13FC97` name-copy
caller; original game instructions are unchanged. The corrected cumulative
captures are in the `original-graphics-pool-v2` directories.

The actual next call is `125E6 -> 1D3D0`, return `125EB` at `6000EFD0`. It
constructs a 256-entry color palette from `474DA8`, not shader bytecode. The
complete 1,024-byte table is identical to the current retail decoder's palette
and a unique table in the installed MCC `halo2.dll`; SHA-256 is
`d077fbea214db2ecde0fcd58967f777a7b7c63f9f5ed1796969b95511c3e38cc`.
Native palette storage/stage binding remains pending. Initializer 32 is still
in progress; 31 of 68 slots are complete. Full native64 execution, large mod
admission, all-tag TU1.5 vanilla conversion, MCC world loading, graphics parity
and the loading video with sound remain unfinished. Existing shaders are
preserved and gameplay remains gated.

## Fixed P8 bump palette without runtime executable input, 2026-10-14

The bitmap decoder no longer opens `default.xbe` for P8 bump colors. Its native
fixed table contains the same 256 entries and 1,024 bytes, generated from the
unique matching table in the installed MCC engine and independently checked
against the authenticated TU1.5 table. This is color data, not a shader program.
The existing retail decoder's complete palette fingerprint remains identical.
The normal build and bitmap loader use the generated constant; executable
reads occur only in the offline generation/reference tool.

Both the existing bitmap path and MCC 2D format17 path now use that fixed table.
BGRA-to-RGBA expansion still occurs before filtering, and authored mip bytes
remain intact. Palette identity/channel/mip checks and the complete MCC cache
test pass. The latter also exposed a pre-existing synthetic vehicle group-order
defect; fixture substitutions now preserve the source-format sorted group
directory, without relaxing production validation. File tracing of the passing
MCC cache test opens no Xbox/MCC executable or ISO. The selected installed MCC
bitmap inventory contains no format17 frames, so that new MCC format path is
validated with explicit synthetic metadata/resources rather than an observed
format17 gameplay render.

Evidence is indexed by
`analysis/asset-setup/mcc-native-20261014/native-startup/fixed-bump-palette-progress-report.json`.
The copied tested binaries, cache/snapshot archives and selected source
provenance are pinned in `.local/mcc-native-20261014/native-bump-palette/`.
This decoder delta has not had an independent source rebuild or GPU visual
comparison. It preserves the port shaders and does not complete the still
pending `125E6 -> 1D3D0` startup palette allocation/stage binding. Full native64,
all-tag vanilla conversion, MCC world loading, graphics parity and the loading
video with sound remain unfinished; gameplay stays gated and the goal is active.

## Native renderer palette resources and stage binding, 2026-10-14

The renderer now owns immutable 256-entry color resources and four shared stage
bindings. Only a live Renderer issues their factory. Its color storage uses
native 64-bit addresses, while the retained source caller receives opaque
handle `F9000101`. The factory retains resource ownership across bindings and
rejects access after presentation/device retirement. The existing P8 decoder
expands the bound colors before filtering; the renderer then uses its existing
single-level image update/fence/upload path. Complete mip chains remain available
through palette decoding and the ordinary static texture upload path. Existing
shader programs are preserved.

Engine graphics wrapper `1D3D0` and SDK setter `4025B0` are declared native
graphics API boundaries. Their actual cold callers, palette index3, complete
input/output frames, 1,024 color bytes and four stage arguments are validated.
Creation returns a genuine renderer-owned color resource and binding updates
the renderer's shared stage state. Foreign bank/thread/caller/type/input,
duplicate creation and missing owner requests are rejected without changing
the retained source or native resources. The Xbox `1D3D0`/`15AF0` allocation
internals and SDK driver object layouts are not executed or claimed by this
native resource implementation. The original SDK setter's four-byte stack
cleanup and EAX stage input are independently inventoried.

Original `12560` continues through all four native bindings and its own
96-DWORD cache clear. The complete compared source prefix is 311,198 instructions
and 82,210 ordered non-stack writes, including defined CPU state, constructor
banks, ten raw allocations, the committed process heap and all 44 pending
stack bytes. The added palette input/output/clear declarations total 1,412
bytes; total cold data is 261,392 bytes. These comparisons use the measured
native graphics API results and do not assert Xbox allocation/driver internals.

Empty input and four real SDL virtual controllers pass both optimized and
independently rebuilt startup-source comparisons. All 90 focused checks pass
without skips, including the cache decoder checks. GPU probes use the actual
bound color resource, update the resident image and draw through the preserved
programs. The interior sample changes from `(32,80,128)` to palette entry0
`(126,126,255)` exactly, with no Vulkan validation messages. Retirement is also
verified. This proves native resource use and the sampled palette color, not
an Xbox gameplay framebuffer comparison. Reference-free tracing opens no
Xbox/MCC executable, ISO or map and sends no external INET traffic.

Evidence is indexed by
`analysis/asset-setup/mcc-native-20261014/native-startup/renderer-palette-progress-report.json`.
The snapshot in `.local/mcc-native-20261014/native-renderer-palette/` rebuilds
45 direct startup sources and pins 64 current static libraries, their source
provenance, compiled shaders and validation layer. Library bodies are copied
rather than independently rebuilt. The initial image-update diagnostic is
retained: the bridge now removes only a redundant single-level mip descriptor
to match the existing upload interface, preserving every pixel and retaining
multi-level admission rules.

The actual next call is `12653 -> 400EF0`, return `12658` at `6000EFD8`, EAX `11`.
Its SDK body is inventoried and its native mapping remains pending. Initializer
32 is still incomplete; 31 of 68 slots have returned and `4922A8` remains cold
zero. Execution ABI remains 32-bit. Full native64 gameplay, large mod admission,
all-tag TU1.5 vanilla conversion, MCC world loading, complete graphical parity
and the loading video with sound remain unfinished. Gameplay stays gated and
the full goal remains active.


### Native renderer constant-mode selection

The reached `12653 -> 400EF0` call now selects the existing native application
constant configuration through a live Renderer control. The authenticated
Xbox SDK path for mode `0x11` sets its configuration fields and returns without
executing the mode0 program-upload branch. The native mapping validates the
real material and extended Vulkan buffers, mapped storage and physical uniform
limits. It imports no Xbox driver device bank. Material storage is 3,712 bytes
and the existing extended bank is 2,544 bytes; native addresses remain above
4GiB. Shader source, layout and the existing c172 policy remain unchanged.

The native void API preserves GP, defined flags and FP state while removing
the return word. It rejects missing, duplicate, foreign or retired owners.
Empty input and four SDL virtual controllers pass original-reference
comparisons in both the integrated build and the independently rebuilt startup
sources. The source prefix now covers 311,200 instructions and 82,210 ordered
non-stack writes. All 96 focused checks pass without skips. GPU tests compare
all 192 logical constant slots through the existing GLSL, including nonzero
c58, c59 and c191 controls. Palette image probes still produce exactly
`(126,126,255)` with no Vulkan validation messages. All 168 shader source files
match the preceding palette snapshot byte for byte. These checks do not prove
a complete Xbox gameplay framebuffer match.

Evidence is indexed by
`analysis/asset-setup/mcc-native-20261014/native-startup/renderer-constants-progress-report.json`.
The frozen snapshot in `.local/mcc-native-20261014/native-renderer-constants/`
rebuilds 46 direct startup sources and copies 65 current static libraries;
library bodies are not independently rebuilt. It also retains the tested
constant-bank binary and source, compiled shader files, validation layer,
original-oracle provenance and verified file pins. The standalone constant-bank
binary retains its original fixture paths; it is validation evidence, not a
relocatable game package. Reference-free startup tracing opens no Xbox/MCC
executable, ISO or map and sends no external INET traffic.

The next reached call is `1265A -> 3FF5D0`, return `1265F` at `6000EFD4`, with
argument2. Its direct depth-test and Z-min/max methods are identified against
the pinned NV2A register source; native W-depth mapping and its called SDK
helpers remain pending. No additional cold data was needed for constant-mode
selection. Initializer32 remains incomplete, with31 of68 slots returned.
Execution ABI remains32-bit. Full native64 gameplay, large mod admission,
all-tag vanilla TU1.5 conversion, MCC-only world loading, complete graphical
parity and the loading video with sound remain unfinished. Gameplay stays
gated and the goal remains active.


### Native world W-depth mode selection

The reached `1265A -> 3FF5D0` mode2 request now resolves through a live Renderer
control to the existing world W-depth path and its resident D24S8 attachment.
The native owner validates the actual image, memory, attachment view, pipeline,
format support and device identity. Its native state backing remains above
4GiB; the Vulkan image identity remains a 64-bit opaque handle. The selected
mode feeds the existing world camera push constants. Each authored pass keeps
its own depth/stencil policy. Mode2 was already the native default, so existing
presentation stays unchanged. All 168 shader source files and their ABI remain
unchanged.

The authenticated SDK body and its helper bodies are inventoried separately.
Direct depth-test and Z-min/max methods are verified against the pinned NV2A
source, and helper400470 supplies CONTROL0's W/perspective bit16. The native
runtime reuses its existing per-view source camera/viewport constant path; it
does not execute or import Xbox driver banks, matrix/program uploads or SDK
register clobbers. Its void ABI removes the return and one argument (`ESP+=8`)
while preserving the remaining GP, defined flags and FP state. Missing,
duplicate, foreign and retired owners are rejected.

The GPU proof uses two overlapping triangles through the actual renderer and
preserved programs. Equal integer W accepts the later red triangle with depth
123456; a position-dependent W row rejects the farther triangle and retains
palette blue with depth65536. All 64,896 covered pixels in both controlled
surfaces match their expected integer depth and color; both coverage masks
match and stencil stays zero. These checks pass for empty input and four SDL
virtual controllers in the integrated and independently rebuilt startup
executables, plus the reference-free trace run. This proves the controlled
native W-depth/write/LEQUAL behavior, not an Xbox gameplay framebuffer match.

Original-reference comparisons retain 311,203 source instructions, 82,210
ordered non-stack writes, defined CPU state, constructor banks, raw allocations,
process heap and pending stack. No cold data was added: typed cold data remains
261,392 bytes. All 100 focused checks pass without skips. The reference-free
trace opens no Xbox/MCC executable, ISO or map and sends no external INET
traffic. The initial missing-gradient probe fixture and subsequent bloom/color
control diagnostic are retained; `native-empty-v3.json` and
`native-controllers.json` are the passing native receipts.

Evidence is indexed by
`analysis/asset-setup/mcc-native-20261014/native-startup/renderer-depth-progress-report.json`.
The frozen snapshot in `.local/mcc-native-20261014/native-renderer-depth/`
rebuilds 47 direct startup sources and copies 65 current static libraries;
library bodies are not independently rebuilt. The preserved shader sources,
compiled programs, validation layer, full controlled GPU surfaces, oracle
provenance and file digests are retained.

The next reached call is `12669 -> 3FFDD0`, return `1266E` at `6000EFD8`, with
ECX4035C and EDX1: the original depth-write-enable method. Its SDK body is
inventoried, and its native owner remains pending. Initializer32 is incomplete;
31 of68 slots have returned. Execution ABI remains32-bit. Full native64
execution and large mod admission, all-tag vanilla TU1.5 conversion, MCC-only
world loading, complete graphical parity and the loading video with sound
remain unfinished. Gameplay stays gated and the full goal remains active.
