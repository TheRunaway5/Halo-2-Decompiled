# macOS engine port

Goal: complete the remaining native macOS engine and launcher port checklist,
with arm64 + x86_64 universal apps and bundled MoltenVK. This checklist is
the progress record for the active Codex goal. An initial graphics check is
separate from a playable engine. Preserve original source numerical behavior
and depth semantics while recovering the remaining platform owners.

## Progress checklist

- [x] Connect to the user's Apple Silicon Mac over SSH (`codex-mac`).
- [x] Read the existing build, renderer, test-storage and Steam login owners.
- [x] Add universal CMake defaults and a macOS build entry point.
- [x] Enable Vulkan portability enumeration and the advertised portability subset.
- [x] Add an app bundle with relocatable shader lookup and explicit SDL MoltenVK selection.
- [x] Add dependency closure, architecture checks, license inclusion and ad-hoc signing.
- [x] Organize macOS-specific CMake, runtime helpers, tools and tests in macOS subfolders.
- [x] Build and run the universal SDL/MoltenVK graphics check on the Mac.
- [x] Validate the x86_64 slice with Rosetta (physical Intel Mac remains separate).
- [x] Record real GPU format/features and native host page size.
- [x] Attempt the full engine build and retain the first actionable blockers.
- [x] Build, sign and relocate the complete x86_64 engine on macOS; verify its actual CLI and bundled-library loading with the existing source arithmetic.
- [x] Confirm Linux portability tests and compilation of the changed shared code.
- [x] Remove private build/dependency downloads after each test; retain bounded evidence.
- [x] Recover Darwin host memory adapters, including 16 KiB host / 4 KiB source pages, within the documented source API contract.
- [x] Preserve 4 KiB virtual reservations, commitment holes, rollback and continuation on Darwin.
- [x] Preserve 4 KiB raw allocation tokens and physics-pool allocation/continuation on Darwin.
- [x] Recover Darwin direct physical backing and 4 KiB source permission guards.
- [x] Recover Darwin independent disk snapshots and physical snapshot adoption.
- [x] Port actual UDP resources, packet boundaries, socket budgets and teardown to Darwin.
- [x] Recover Darwin link/title-address observations and verify native network call adapters.
- [ ] Validate full original network startup, offline transitions and VPN/interface changes on macOS.
- [x] Add Darwin event/readiness and wall/monotonic timer resources with cancellation fencing.
- [x] Run the production startup-timer resource/ABI on both Mac architectures.
- [x] Preserve private 4 KiB sound aliases over real 16 KiB ARM64 backing; match all six original cache constructor cases on both universal slices.
- [ ] Execute complete source sound notification/codec callbacks and process startup on macOS.
- [ ] Recover remaining Darwin persistent storage platform owners.
- [x] Recover Darwin arena/death startup file resources and worker identity/priority; compile both owners universally and verify real host operations on both slices.
- [x] Execute the complete arena startup sequence under Rosetta and compare its results with the authenticated Linux TU1.5 regression.
- [x] Preserve atomic, exclusive movie/prompt/loading-bank publication on Darwin.
- [ ] Recover and differentially verify source x87/SSE numerical paths on arm64.
- [ ] Resolve Metal depth/geometry feature gaps with source-faithful behavior.
- [x] Implement exact integer D24 storage in D32S8 for the production weather depth/color alias; verify every 24-bit value, stencil preservation and writeback on both Mac slices.
- [x] Port the retained sky-stencil diagnostic to instanced vertices without geometry shaders; verify canonical D32S8 depth comparisons and original depth/stencil endpoints on both Mac slices.
- [x] Compile the real cache/draw chain universally and execute the original hologram diagnostic, scene copy and flat-W D32S8 depth-state controls on both Mac slices.
- [x] Build and execute the pinned Rust DSP archive through its production C bridge on both Mac architectures.
- [x] Build pinned SDL for both Mac architectures by default and verify native resources, signed relocation and MoltenVK presentation from the production build.
- [x] Replace the thin host libdeflate selection with a pinned universal SDK; verify MCC decompression contracts, complete Lockout bytes and relocated execution on both Mac slices.
- [x] Audit the installed OpenSSL, Opus and FFmpeg dependency architectures and transitive libraries.
- [x] Build pinned universal Opus, run the production MCC decoder on both slices and verify a relocated signed dependency bundle.
- [x] Preserve material/texture ownership checks with the C++20 shared-pointer API and use GLM's supported header-only target.
- [x] Build pinned universal Ogg/Vorbis, compile the real loading converter, and verify all four codec libraries in signed relocated bundles.
- [x] Isolate the loading PCM mismatch to decoder build behavior with the same authenticated input and production converter.
- [x] Trace matching transform inputs and the inverse-transform/overlap-add boundary against the admitted loading PCM reference.
- [x] Package the universal native loading converter, private codec libraries and codebook; verify actual launcher preflight and execution on both Mac slices.
- [ ] Match the authenticated loading-track PCM profile on macOS before admitting fresh setup output.
- [x] Build pinned universal OpenSSL and verify crypto, certificate-checked TLS, identity flows and a relocated signed bundle on both architectures.
- [x] Verify local direct ICE/DTLS/SCTP peer connectivity on both Mac architectures.
- [x] Verify the production encrypted WAN probe from both Mac slices to the isolated public-server endpoint.
- [ ] Validate packaged network permissions, two-consumer NAT paths and live cross-machine gameplay.
- [x] Build pinned universal FFmpeg/dav1d and execute the production movie/sound decoders in a relocated bundle.
- [x] Build all three universal source launchers and recover Darwin executable identity, relaunch and update forwarding.
- [x] Separate packaged launcher resources from writable Mac user storage and verify sealed resources on both architectures.
- [x] Supply and relocate the universal Python/Tk/Pillow setup and launcher UI runtime.
- [ ] Validate native source-file selection and complete real MCC preparation from the bundled setup UI.
- [ ] Supply universal dependencies for the complete engine (including Rust DSP archive).
- [ ] Validate menu, maps, campaign, multiplayer, audio, input and display on macOS.
- [ ] Produce a playable universal engine and launcher bundle; sign/notarize for release.
- [x] Locate existing multiplayer Steam OpenID sign-in and inspect download authentication.
- [x] Recover Valve's current MCC/Halo 2 depot relationships and the importer's required shared archive.
- [x] Execute a self-contained Steam metadata helper and real Valve requests on both Mac architectures.
- [x] Implement the local QR session and verify actual challenges, cancellation and process teardown on both Mac architectures.
- [x] Bundle both private CLR CPU trees with the universal Python/SDL launcher and MoltenVK, and validate the real setup QR window on both architectures.
- [x] Verify confirmation, disk-budget and process-owner boundaries in focused policy/UI tests while preserving manual import.
- [x] Verify real MCC/Halo 2 campaign and multiplayer licenses, four authorized manifests and the complete selected-file plan.
- [x] Add Steam-owned MCC acquisition to macOS setup; preserve manual installation import.
- [x] Validate real ownership, required depot/file selection, disk checks and interrupted-download resume/cancel.
- [x] Verify all 883 acquired files and recognize the downloaded source with the production readers.
- [ ] Complete import/preparation from the acquired MCC source after resolving the loading-audio admission gate.

The user deferred the shared exact x86/ARM numerical backend on 2026-10-20:
continue the universal build / bundled MoltenVK track first. Later numerical
work must preserve source precision, rounding, flags and spill behavior, with
bit-for-bit differential validation; changing intrinsics merely to compile is
insufficient.

## Building

macOS-specific logic lives under `cmake/macos`, `include/halo2/macos`,
`src/macos`, `tools/macos` and `tests/macos`.
The root `build-macos.sh` is a thin entry point. Shared Vulkan
portability negotiation remains with the shared engine code. Historical
bring-up evidence keeps the source paths and hashes from the original run.
The [folder-layout verification](macos/layout-20261020.json) records the moved
source hashes, passing Mac graphics/bundle tests, complete engine CMake
configuration, and Linux shared-code compilation. Its test builds and SDKs
were removed after validation.

Run `./build-macos.sh` on macOS with Xcode Command Line Tools, CMake 3.24+,
Ninja, Python 3.11+, Vulkan headers and the dependencies listed in the main
README. The default is `CMAKE_OSX_ARCHITECTURES=arm64;x86_64` and a macOS 13.0
deployment target. Set `HALO2_MOLTENVK_LIBRARY` and `HALO2_MOLTENVK_LICENSE`.
SDL 2.32.10 is source-built for the requested architectures by default, with
its own source notice included in the bundle. To use an external universal SDL,
set `HALO2_MACOS_BUILD_SDL=OFF`, supply `CMAKE_PREFIX_PATH` / `SDL2_DIR` and set
`HALO2_SDL_LICENSE` to that library's matching notice.

Enabled MCC decompression acceleration also source-builds pinned libdeflate 1.26
for both CPUs. `HALO2_MACOS_BUILD_LIBDEFLATE=OFF` selects an external pkg-config
library; include its matching source notice through `HALO2_MACOS_ENGINE_LICENSES`
and supply the requested architectures. `HALO2_USE_LIBDEFLATE=OFF`
keeps the existing generic zlib path.
Every bundled non-system dylib must contain every requested architecture.
Single-architecture Homebrew dependencies cannot produce the universal engine.

Mac engine builds now build checksum-pinned Opus 1.6.1 for the requested
architectures automatically. Its SDK lives under the build's
`macos-dependencies/opus`; the normal engine bundle includes its source license.
`HALO2_MACOS_BUILD_OPUS=OFF` selects an externally supplied pkg-config Opus
instead. That external library must still supply every requested architecture
and its matching license must be included when packaging.

The default loading-audio converter also builds pinned Ogg 1.3.6 / Vorbis 1.3.7
for both architectures under `macos-dependencies/vorbis`.
`HALO2_MACOS_BUILD_VORBIS=OFF` selects externally supplied pkg-config libraries.
The universal SDK and executable build successfully, but the real track still
fails the existing exact PCM admission profile on macOS. This is an open setup
gate; the checksum has not been relaxed.

Mac engine builds also source-build pinned OpenSSL 3.5.9 for both architectures
under `macos-dependencies/openssl`. `HALO2_MACOS_BUILD_OPENSSL=OFF` selects
external OpenSSL instead. The owned SDK contains universal `libcrypto` and
`libssl`, byte-identical upstream headers, the source license and relative
pkg-config files. Upstream's built-in provider option preserves default, base
and legacy algorithms without shipping a separate provider module directory.

Playback builds checksum-pinned FFmpeg 9.0.2 and dav1d 1.5.4 under
`macos-dependencies/ffmpeg`. `HALO2_MACOS_BUILD_FFMPEG=OFF` selects external
pkg-config libraries instead. The owned playback SDK includes all native
decoders, demuxers and parsers, explicit dav1d AV1 decoding, VideoToolbox,
AudioToolbox and system zlib. Encoding, muxing, filters and capture devices are
outside this playback SDK. External library autodetection is disabled to keep
thin host libraries out of the universal dependency graph. Pinned Meson and
native NASM build tools stay in the owned build tree; they are not installed
globally or shipped in the app.

For the initial platform-only bring-up, use
`HALO2_MACOS_TARGET=halo2_macos_graphics_probe ./build-macos.sh -DHALO2_MACOS_BRINGUP_ONLY=ON`.
This needs only SDL2, Vulkan headers and MoltenVK; it exercises a real SDL Metal
surface, Vulkan device, swapchain, transfer clear and presentation. Its JSON
report records the original D24S8 format requirements without substituting D32.
It does not run game logic or certify visual parity. Direct MoltenVK linkage
does not load SDK validation layers; validation through a Vulkan loader remains
a separate development path.

The bundle includes MoltenVK and all discovered non-system dylibs, checks both
slices, rewrites install names, retains dependency licenses and hashes, then
ad-hoc signs. It needs neither Homebrew nor an installed Vulkan SDK at runtime.
Notarization, release publication, complete resource packaging and launcher
onboarding remain explicit completion gates.

All validation uses `tools/run_tests.py`; dependency downloads and other probes
run under `tools/run_test.py` and `HALO2_TEST_SCRATCH`. Keep bounded JSON/log
reports under `build/test-results/macos-*`; use `--retain-report` for reports
generated in disposable builds. Remove downloads, objects, apps and private
builds after each validation unless a deliverable is explicitly retained.

## Verified bring-up, 2026-10-20

On the user's Apple M2 Pro, all four macOS CTests passed: native arm64
presentation, x86_64 presentation under Rosetta, portability negotiation and
Mach-O bundle integration. Each graphics test runs a relocated app copy and
checks the actual loaded SDL and MoltenVK paths. The packaging test also runs
both executable slices after removing its synthetic SDK, verifies signatures
and final file hashes, checks repeated bundle fixup, and rejects a thin dylib.
The dependency inputs are checksum-pinned SDL 2.32.10 and MoltenVK 1.4.2.

| Capability | arm64 native | x86_64 under Rosetta |
| --- | --- | --- |
| Presented swapchain frames | 3 | 3 |
| Host page bytes | 16384 | 4096 |
| Required source D24S8 image | unavailable | unavailable |
| Geometry shader | unavailable | unavailable |
| Depth clamp | available | available |
| Portability subset enabled | yes | yes |

Direct MoltenVK does not advertise portability enumeration in this run; the
shared negotiation enables it whenever advertised. These results prove SDL /
MoltenVK device creation and presentation, with both architectures in the
executable and every bundled dylib. They do not prove gameplay, source depth
parity or physical Intel Mac compatibility.

The complete engine configured successfully for both architectures, then failed
compilation at `src/physics_step_internal.hpp:6`: its unguarded `xmmintrin.h`
include is x86-only. Reached solver and vehicle paths also use x87 control/status
and extended precision. The remaining checklist preserves these source owners
rather than substituting different arithmetic or depth formats.

Linux validation compiled the changed renderer, desktop session, custom variant
lookup, menu-world shader lookup and shadow shader lookup, and passed the
portability regression. Bounded logs and JSON evidence are under
`build/test-results/macos-universal`, `macos-engine-initial`,
`macos-linux-shared-compile` and `macos-linux-resource-compile-final`.
The small [checked-in evidence record](macos/bringup-20261020.json) includes
dependency checksums, source hashes, bundle hashes, capability results and the
full-engine compile boundary. All generated test builds, SDK downloads, disk
mounts and app copies were removed. No large test artifact was retained.

To repeat the disposable Mac validation, from the checkout on the Mac:

```sh
python3 -B tools/run_test.py -- python3 -B tools/macos/test_port.py
```

It downloads verified dependencies into owned scratch, unmounts the SDL image,
runs CMake through `tools/run_tests.py`, retains small reports, then removes the
SDKs, app copies and private build. Add `--engine-check` to the inner command to
reproduce the full engine compile boundary with installed host dependencies.
Use `--engine-configure-check` to check the complete CMake target graph and
the portable Vulkan negotiation test without rebuilding the unfinished engine.

## Darwin virtual and raw storage, 2026-10-20

`src/macos/source_runtime_host_pages.hpp` owns Darwin reservation alignment,
commit protection and rollback. Shared virtual/raw/pool owners use that backend
without changing the 4 KiB source token namespace, returned allocation sizes,
commitment bits or existing source bytes. Only the private native mapping
extent rounds up to the actual host granule. A rejected commitment keeps a
native granule writable when another source page in it was already committed;
source-bank accesses still reject every uncommitted 4 KiB source page.

The current virtual owner retains source byte storage independently of its
native reservation, as before this port. Physical allocations use the separate
direct backing/protection owner described below. Blindly rounding physical
`mprotect` calls would change access to neighboring source pages.

The standalone platform suite compiles the production virtual/raw/pool owners
into universal test binaries without SDL, MoltenVK, the shader compiler or the
unfinished numerical backend. It runs both arm64 and x86_64 on the Mac and also
supports Linux regression validation:

```sh
python3 -B tools/run_tests.py --source tests/macos --jobs 2
```

The tests cover small/tail reservations, source holes via token and native
address APIs, recommits retaining source writes, failed commitment and native
granule rollback, VM ownership import, raw allocation sizes/cursors, and the
reached pool allocation ABI and continuation. Native protection is queried
through Mach VM on macOS and `/proc/self/maps` on Linux. The original virtual
memory tests also run unchanged; Linux additionally runs the existing combined
VM/physical/pool continuation regression. These are resource-owner tests, not
proof of full game execution or recovered Xbox kernel failure/status behavior.
The [memory-owner evidence](macos/memory-20261020.json) records four passing Mac
checks across native arm64 and Rosetta x86_64, three Linux regression checks,
native page sizes/protection output, source hashes and cleanup. All private
memory-test builds were removed; no large artifact was retained.

Relevant platform contract: Apple's [mprotect manual](https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/mprotect.2.html).

## Darwin physical storage, 2026-10-20

`src/macos/source_runtime_host_physical_pages.hpp` supplies actual contiguous
native mappings while retaining the source token bounds, allocation sizes and
4 KiB protection rows. Token and native-address writes, mutable views and
validation use the existing source write guards. Rejected writes leave both
bytes and mutation observers unchanged.

Native protection aggregates those rows at the actual host page size. A
uniformly read-only host page receives native read-only protection; a mixed
page stays writable so its writable source neighbors remain usable. The source
API guards still reject every read-only source page. This does not supply
hardware faults for individual 4 KiB portions of a 16 KiB host page. As already
required by `source_runtime_write_protection.hpp`, mutable borrowed spans must
not be retained across protection transitions. Arbitrary raw pointer stores
are outside that guarded API contract.

Ten resource-owner checks passed on the Mac across arm64 and Rosetta x86_64;
six passed on Linux, including the unchanged Linux native-protection test.
Coverage includes mixed/uniform permissions, unaligned requests, tail padding,
stable native aliases, cancellation/repreparation, exact-bank ownership
publication, the original-size physical request, original release checks and
combined virtual/physical/pool continuation. The
[physical-memory evidence](macos/physical-memory-20261020.json) records commands,
source hashes, reports and cleanup. Independent disk snapshots and adoption
were subsequently validated as described below. All private test builds were
removed; no large test artifact was retained.

## Darwin independent disk snapshots, 2026-10-20

`src/macos/cache_file_snapshot_storage.hpp` owns Darwin snapshot directory,
filesystem, private-file lifecycle and writeback policy. The default location
is `~/Library/Caches/halo2-native/file-snapshots`; an absolute `XDG_CACHE_HOME`
override is supported. Files are private, unlinked before writing and closed
on exec. Bounded streaming copies preserve the source descriptor cursor and
work across volumes without requiring APFS clones. Large checkpoint bytes use
this backend at the existing 1 MiB boundary. `fsync` batches make written file
pages reclaimable; this is temporary storage, with no crash-durable save claim.

All 26 platform CTests passed on the Mac, covering both universal slices; all
14 Linux regressions passed. The original snapshot tests check complete large
checkpoint/restore/copy bytes, isolation from earlier mutable borrowers, source
file edits/truncation/unlink, storage-boundary growth and atomic disk failure.
The physical adoption test retains the restored native address through staging
and publication. A separate source-sized tail case proves the complete native
mapping fallback when the logical extent does not fill a native granule.
The [snapshot evidence](macos/snapshots-20261020.json) records APFS backing,
private inode properties, test reports, source hashes and verified cleanup.

This is resource-owner correctness evidence. The standalone suite does not run
the Linux cgroup memory-budget wrapper or certify complete game execution.
Every snapshot test directs its cache into owned `HALO2_TEST_SCRATCH` storage;
all private files and builds were removed, with no retained large artifacts.
Platform references: Apple's [filesystem query](https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/statfs.2.html)
and [file synchronization](https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/fsync.2.html) contracts.

## Darwin UDP transport, 2026-10-20

`src/macos/native_datagram.hpp` owns Darwin descriptor creation and native
socket-address layout. The existing shared transport retains source socket
budgets, buffer requests, native pool identity and lifetime rules. Shared
`recvmsg` output flags reject truncated datagrams on both Linux and Darwin.
A private one-byte span handles zero-capacity receive requests without letting
Darwin return a synthetic zero-byte result before obtaining a real endpoint.
Valid zero-byte packets remain distinct from an empty nonblocking queue.

Four transport checks passed across native arm64 and Rosetta x86_64; the two
corresponding Linux checks passed. The original service test covers real UDP
loopback bytes/endpoints, the 24-socket configured limit, slot reuse, 64-bit
pool identity and retirement. Additional checks cover zero-byte packets,
zero-capacity and short-buffer truncation, endpoint preservation, options,
bind failure, move ownership and descriptor flags. The earlier zero-capacity
failure and its focused reproduction remain as bounded diagnostics in the
[transport evidence](macos/datagrams-20261020.json), alongside final passing
reports, source hashes and cleanup. The shared zero-capacity fix was followed
by focused transport runs; the 26 passing Mac memory checks and 14 Linux
memory checks above remain their separate validation record.

This exercises local native UDP resources. It does not certify live multiplayer,
Xbox secure protocols, full original network startup or the 22,783-case source
configuration reference matrix. Link/title-address observations were
subsequently validated below. Platform references: Apple's [receive API](https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/recvmsg.2.html)
and [socket implementation](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/kern/uipc_socket.c).
All owned test builds were removed; no large transport artifact was retained.

## Darwin network observations, 2026-10-20

`src/macos/native_network_observation.cpp` reads a SystemConfiguration snapshot
and kernel interface rows. It prefers the OS primary Ethernet interface; an
explicit interface remains supported. Carrier comes from the configuration
daemon and/or BSD media query; conflicting observations fail admission. Only
reported media values set speed/duplex, leaving Wi-Fi's unreported values
unknown. Framework/source integration lives in `cmake/macos/Network.cmake`.

The title-address owner selects configuration for that same interface. It
cross-checks published IPv4 addresses against kernel rows and uses the current
kernel six-byte MAC. DHCP requires an observed lease; manual and DHCP INFORM
addresses must match configured addresses. Unmatched services, kernel/config
mismatches and unsupported assignment methods fail explicitly. Automatic
link-local fallback is not labeled DHCP or static. DNS/router data belongs to
the selected service. With no published IPv4, the configuration daemon's
`IPConfigurationBusy` marker supplies pending state; the marker aggregates
IPv4 and IPv6 acquisition. Published IPv4 takes precedence over that marker.

Six focused network checks passed on the Mac across arm64 and Rosetta, plus
three Linux checks. On the Mac, the live `en0` observation matched kernel MAC
and IPv4, observed DHCP, a gateway and two DNS entries; speed and duplex were
unknown and the source link flags were `1`. Tests also exercised the existing
XNet/Winsock initialization, result records, reference cleanup and retained
socket retirement. The title-address call serialized the admitted live result
through its source output frame. Malformed-call and typed configuration cases
cover rejection, manual/INFORM origins, missing leases and absent/pending IPv4.

The complete engine CMake graph configured with the new framework integration,
and its portability check passed. This does not build or execute the unfinished
engine, differentially run the original network startup, mutate live network
configuration, validate actual offline/VPN transitions or certify gameplay.
The [network-observation evidence](macos/network-observation-20261020.json)
retains commands, bounded reports, source hashes and cleanup. Every private
test build, downloaded SDK, disk-image mount and app copy was removed; no
large artifact was retained.

Platform references: Apple's [SystemConfiguration APIs](https://developer.apple.com/documentation/systemconfiguration)
and [DHCP information API](https://developer.apple.com/documentation/systemconfiguration/scdynamicstorecopydhcpinfo-apicollection).
The busy marker's presence semantics and aggregate scope follow Apple's
[IPConfiguration daemon](https://github.com/apple-oss-distributions/bootp/blob/main/IPConfiguration.bproj/ipconfigd.c),
`IFStateReportState` and `IFStateProcessBusy`. Media reporting follows the BSD
interface contract used by Apple's [ifconfig](https://github.com/apple-oss-distributions/network_cmds/blob/main/ifconfig.tproj/ifmedia.c).

## Darwin events and timers, 2026-10-20

`src/macos/native_event_resources.hpp` owns nonblocking signal pipes, tagged
`kqueue` readiness and native Dispatch timers. The shared facade retains Linux
`eventfd`/`epoll`/`timerfd` resources. Signals remain real pollable descriptors;
readiness does not consume receipts. The source owners admit one pending event
per route, so this does not promise an arbitrary eventfd-sized counter on Darwin.

Relative startup timers and the default source interrupt clock use native
`CLOCK_UPTIME_RAW`, matching Dispatch uptime. libc++'s `steady_clock` counts
sleep and cannot supply that domain. Absolute sound timers use the wall clock
through `dispatch_walltime`. Configuration, cancellation, receipt
consumption and native expiration handlers share a private serial queue.
Re-arming clears old pending timer data and receipts; retirement waits for the
native cancellation handler before releasing its context or descriptors.
The native callback only emits a receipt. Source-bank writes and source DPCs
still execute on their existing application owner thread. Dispatch scheduling
is best effort; this is not a source timing-precision claim.

The production startup timer, sound event port, timed notifications and codec
event port now use these resources. The shared startup executor is in a separate
translation unit so resource/ABI validation does not require replacing its
unfinished source numerical dependencies.

Two Mac checks passed, one for each universal slice, and the corresponding
Linux platform check passed. They exercise actual expiry, past/future wall
deadlines, a six-hour relative request, cancel/re-arm without stale receipts,
nonconsuming readiness, repeated timer destruction and descriptor closure.
The production startup-timer checks verify its call frame, source stores,
Boolean return/stack cleanup, retained DPC arguments, rejected malformed calls,
missing-executor rejection without consuming expiry, no replay and retirement.
The three production sound owners and shared startup executor also compile for
both Mac architectures. Five existing Linux source checks passed, including 48
startup prefixes, 31,656 original instructions and 13,008 source stores. The
initial two codec checks rejected a forced SDL dummy backend. A real stereo
output preflight and both checks subsequently passed with PulseAudio, completing
all seven source regressions. After the Mac-only clock correction,
Linux startup-timer preprocessed tokens match the version exercised by the
five passing checks, and the latest Linux platform check passed again.

The [event/timer evidence](macos/events-20261020.json) records commands, source
hashes, reports and cleanup. These Mac checks do not execute complete source
sound callbacks or full process startup, which still reach the deferred exact
numerical backend. They do not establish audible output, gameplay, physical
Intel compatibility, live clock changes or sleep/resume behavior. Mac private
builds and both Linux source-regression builds were removed. No large artifact
was retained.

Platform contracts: Apple's [Dispatch timer API](https://github.com/apple-oss-distributions/libdispatch/blob/main/dispatch/source.h)
defines its clock domains, pending-data reset and cancellation fence. The
[XNU timer implementation](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/kern/kern_event.c)
converts absolute calendar `kqueue` timers once, so that facility cannot supply
the required ongoing wall-clock domain. LLVM's [clock implementation](https://github.com/llvm/llvm-project/blob/main/libcxx/src/chrono.cpp)
documents the sleep-inclusive libc++ steady clock used on Apple platforms.

## Darwin startup storage resources, 2026-10-20

The arena and death-startup resource owners now accept Darwin's POSIX file
open, seek, resize, read and close operations. Arena checkpoint/restore also
uses Darwin's nanosecond modification timestamps and real file synchronization.
`src/macos/startup_resources.hpp` owns the Darwin differences; shared dispatch
stays in `src/native_startup_resources.hpp`. Linux keeps its existing nice(-1),
`st_mtim` and `fdatasync` policies.

Worker creation retains an actual Darwin thread ID and pthread handle. The
explicit host priority policy queries that worker's scheduling class and
attempts one priority step, returning the actual host BOOL. Both Mac slices
observed priority 31 becoming 32 while preserving the parent thread's priority.
This is a native scheduling policy; the original constructor ignores its BOOL.
It does not establish Xbox scheduler or timing equivalence. Identity-query
failures propagate through the startup future and worker outcome.

The focused CMake project compiles both production owners for arm64 and x86_64.
Both Mac host tests pass 24 controls covering real worker identity/priority,
parent preservation, changed file size, nanosecond timestamps, synchronization,
invalid descriptors and preserved stored bytes. Linux passes its 20 controls.

The real generated arena executor also passes under Rosetta x86_64: 13 complete
startup calls, 1,049,656 ordered writes, actual worker/file jobs, two 4,239,360-byte
savegame files, source CRC and checked 64-bit storage ownership. The separate
ungated production composition retires all 150 source jobs before handoff and
rejects foreign/released addresses. The 51,265,536-byte physical arena hashes to
`211437447b5b8ab45f1b3279d5b9f06c59a45cf35fb2ef33db74af57e503bb0a`.
The Mac cold-field hashes, source results, write counts and physical hash match
the Linux run authenticated against the unchanged TU1.5 CPU reference. The
priority BOOL remains the declared, ignored host outcome. Linux compares all
ordered stores and all physical bytes with that retained reference; the Mac
test itself uses no executable input.

The first focused runs exposed a stale 20-field/43,562-byte test expectation.
The existing data owner separately declares the opposite-end allocation cursor
at `4F2958`. A SHA-256-authenticated TU1.5 audit confirms its four loader-zeroed
bytes; the corrected 21-field/43,566-byte expectation passes the complete
original-store comparison. The test now keeps its files and snapshot cache
inside the wrapper's owned scratch directory.

Reproduce the universal host checks with:

```bash
python3 -B tools/run_tests.py --source tests/macos/startup_storage \
  --target startup_storage_tests --filter '^startup_storage_' \
  --output build/test-results/macos-startup-storage \
  --retain-report Testing/Temporary/LastTest.log
```

For the retained x86 numerical executor on the Mac, add
`--cmake-arg=-DCMAKE_OSX_ARCHITECTURES=x86_64`,
`--cmake-arg=-DHALO2_STARTUP_ARENA_TEST=ON` and
`--retain-report arena.json`, with a distinct output directory. The focused
closure uses the actual production/generated sources and discards unused
entry points; it supplies no substitute callee or source result.

The [startup-storage evidence](macos/startup-storage-20261020.json) retains
commands, hashes, real failures and bounded reports. The broad Linux root
build was deliberately cancelled after it selected 1,496 targets; the focused
three-test Linux cohort passes. Two Mac root configure attempts stopped on
movie inputs and bundle-license configuration. They provide no full-engine
build acceptance. Concurrent Linux work changed only an unrelated overlapping
region exception message; the report preserves both source hashes and records
that difference instead of claiming identical complete checkouts.

All six Mac and four Linux CMake scratch owners are absent, including their
private crypto SDKs, binaries and generated files. No new large artifact was
retained. The previously authorized original MCC download remains available.
Complete arm64 generated execution, death-startup execution/cache reads,
existing profile-file persistence, broader storage and full world/gameplay
acceptance remain open. The exact numerical backend remains deferred.

## Universal SDL dependency, 2026-10-20

`cmake/macos/SDL.cmake` and `tools/macos/build_sdl.py` now supply the production
build's SDL target. The 2.32.10 source archive is pinned to
`5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165`,
matching the publisher's GitHub asset digest. Each CPU gets a separate CMake
configuration and actual shared-library build. The SDK combines those slices,
checks their architecture and system-only dependency closure, and signs the
result. All 78 common public headers agree. The generated CPU capability
headers differ and remain separately preserved behind an architecture-selected
`SDL_config.h`, so the x86 consumer uses its actual x86 configuration.

The main engine/launcher CMake graph selects this owned SDL target by default.
The SDK and its inputs live under `macos-dependencies/sdl` and `sdl-work` in
the current build. The normal bundle seal includes the generated upstream
license after the SDL dependency completes. External SDL remains an explicit
option with the existing architecture and matching-notice requirements.
The Mac build script requires no separate SDL license input for the default
source build. Linux keeps its existing SDL package selection.

Four focused Mac checks pass. Both architectures use the actual bundled SDL
2.32.10 image and native Cocoa/CoreAudio drivers. A real Metal window renders
and reads back the requested `[32,64,128,255]` pixel from its 640x360 drawable.
The test observes the current display, delivers its queued event, and verifies
axis/button values through SDL's virtual game-controller device. Real 48 kHz
stereo float output consumes four silent callbacks on each relocated slice.
These are real output callbacks, without an audible game-audio claim.

The relocated, signed apps pass with the original SDL SDK renamed out of the
lookup path and dynamic-loader overrides removed. The SDL resource probe and
the actual Vulkan presentation probe run on arm64 and Rosetta x86_64. Both
graphics slices present three frames, and `dladdr` identifies SDL and MoltenVK
inside the relocated app. Source notices match the pinned upstream bytes.

The production root graphics build also passes its four existing checks with
this default source SDK, including signed bundling and architecture rejection.
The complete engine graph configures with the new selection and passes its
portability check; that run builds only the focused portability target. The
Linux root graph configures and passes its existing portability check too.
These ten CTest executions provide dependency/platform evidence. They do not
prove a playable engine, source depth/geometry parity, full game audio,
physical keyboard/controller operation, physical Intel or older macOS support.

Reproduce the focused Mac cohort through the disposable SDK owner:

```bash
python3 -B tools/run_test.py --scratch-root .local/test-scratch -- \
  python3 -B tools/macos/test_port.py --engine-configure-check \
  --source tests/macos/sdl --engine-target macos_sdl_tests \
  --engine-test-filter '^(macos_sdl_|vulkan_portability$)' \
  --output build/test-results/macos-sdl-source \
  --retain-report Testing/Temporary/LastTest.log \
  --retain-report sdl-relocation.json \
  --retain-report macos-dependencies/sdl/manifest.json
```

The [SDL evidence](macos/sdl-20261020.json) retains commands, source/input
hashes, manifests and bounded results. All private builds, source archives,
SDKs and test apps are removed; no test disk image or owned process remains.
The authorized original MCC download is preserved. Other engine dependencies,
the loading PCM admission gate and the deferred exact numerical backend remain
separate checklist work.

Upstream: [SDL 2.32.10 release](https://github.com/libsdl-org/SDL/releases/tag/release-2.32.10)
and its [CMake build instructions](https://github.com/libsdl-org/SDL/blob/release-2.32.10/docs/README-cmake.md).

## Universal MCC decompression dependency, 2026-10-20

The installed Mac libdeflate 1.26 is arm64-only. An actual two-architecture
compiler probe failed with the missing x86_64 allocation/free symbols. The
production graph previously selected that library through pkg-config when MCC
acceleration was enabled. `cmake/macos/Libdeflate.cmake` and
`tools/macos/build_libdeflate.py` now build each requested CPU separately and
combine the real shared libraries. The source archive's SHA-256 is
`125856d4656e0feab660f94842f835923410c9281fedbcee64598c918da42b5a`,
matching its publisher asset digest. Both installed headers match the pinned
source; dependency closure contains only system libraries. The SDK is signed
and includes the unchanged upstream source notice.

`cmake/MccDecompression.cmake` shares the target selection between the engine
and focused decoder project. The engine's existing acceleration option stays
enabled by default; Mac builds select the owned universal SDK. Linux keeps its
pkg-config selection and existing generic fallback. The production decoder's
input validation, zlib fallback, concurrency and byte ownership are unchanged.

Five Mac CTests pass. Each CPU runs the existing 100-case decoder contract,
including 19 malformed-input rejections and eight simultaneous callers. Every
successful result and every rejection agrees with the retained serial decoder
from before the loading optimization. The reference body is pinned separately;
its zlib inflater comes from the production source. The tests compile the real
production decoder and `Bytes` owner, discarding unrelated unused cache entry
points during linking. A test-only forwarding observer records actual
libdeflate calls and returns the real library's result without supplying bytes.

The signed relocated app runs with its original SDK hidden and loader
overrides removed. Both arm64 and Rosetta x86_64 decode the user's acquired
Lockout map and compare all 3,777,920 output bytes with the serial reference.
The identical 1,820,416-byte source hashes to
`97d034dca2109adde65e56c0ab78ee56f72045b56bf9b3200f304d5e1ff4c61e`;
both decoded outputs and the Linux regression hash to
`d137b6511eec0753304be4f72c5047e4462d02dc25bece0fc9f190d7c68e7bd2`.
The fixture and map complete 16 observed successful accelerator calls per
relocated run. `dladdr` identifies the loaded private libdeflate image, and the
bundled notice matches the pinned source bytes. Generated compressed fixtures
may differ between platform zlib versions; their complete decoded bytes agree.

The complete Mac engine graph also configures with the new selection. A
deferred CMake inspection verifies the actual `halo2_cache` compile definition,
link target, imported SDK path, SDK build dependency and registered source
notice. That run builds only its portability test. The Linux root graph and
two decoder checks pass as well, for nine passing CTest executions overall.
These checks validate the loader/dependency path; they do not execute gameplay
or establish full engine, physical Intel or older macOS acceptance.

Reproduce on the Mac with the existing owned map path:

```bash
python3 -B tools/run_tests.py --source tests/macos/libdeflate \
  --target macos_libdeflate_tests --filter '^macos_libdeflate_' --test-jobs 1 \
  --output build/test-results/macos-libdeflate \
  --retain-report Testing/Temporary/LastTest.log \
  --retain-report macos-dependencies/libdeflate/manifest.json \
  --retain-report decoder-sources.json --retain-report libdeflate-relocation.json \
  --cmake-arg='-DHALO2_LIBDEFLATE_TEST_MAP=/path/to/owned/lockout.map'
```

The [decompression evidence](macos/libdeflate-20261020.json) retains the real
host-library failure, pinned input/source hashes, commands, architecture
manifests, decoder comparisons and full-graph selection receipt. Disposable
builds, archives, SDKs and test apps are removed; the authorized original MCC
source remains. No new large artifact is retained. Full engine dependency
closure and playable-runtime requirements remain open, and the exact numerical
backend stays deferred.

Upstream: [libdeflate 1.26](https://github.com/ebiggers/libdeflate/releases/tag/v1.26)
and its [CMake options](https://github.com/ebiggers/libdeflate/blob/v1.26/CMakeLists.txt).

## Universal Rust DSP dependency, 2026-10-20

`cmake/macos/Dsp.cmake` builds the retained DSP core with Rust 1.99.0 and the
existing locked Cargo dependency graph, targeting `aarch64-apple-darwin` and
`x86_64-apple-darwin` explicitly. It combines the static archives with `lipo`
and verifies each requested architecture. Supplied prebuilt DSP archives pass
that same architecture gate. The CMake deployment target is also supplied to
Rust and its C build dependencies.

Upstream's binding generator writes into its source directory. The Mac build
uses a private source copy and compares the generated C header with the retained
production bindings before linking. It leaves the vendored source unchanged.
The two Cargo builds run serially to avoid competing binding-generator writes.

Three focused Mac checks passed: native arm64, x86_64 under Rosetta, and the
prebuilt archive gate. The production C bridge executes real translated DSP
MOVE/JMP instructions and checks PC/cycles, program invalidation, actual mix
memory aliases, a peripheral callback and its exact stop boundary. The archive
check admits a universal static library and rejects either thin library.
An initial `lipo` invocation failed because this Mac rejects both architectures
in one verification call; separate verification calls passed. The full engine
CMake graph then configured with this integration and its portability test
passed. The [DSP evidence](macos/dsp-20261020.json) retains the initial failure,
final commands/reports, toolchain provenance, source hashes and cleanup.

Repeat the disposable validation on the Mac with:

```sh
python3 -B tools/run_test.py -- python3 -B tools/macos/test_dsp.py
```

It downloads a versioned, checksum-verified Rustup installer, installs the pinned
toolchain and both target libraries into owned scratch, then runs CMake through
`tools/run_tests.py`. Every toolchain, Cargo cache, archive, executable and private
build from both attempts was removed. No large artifact was retained. This does
not validate the complete C++ GP owner, original GP image playback, audible
output, hardened-runtime JIT entitlements or gameplay. Other universal engine
dependencies and their complete static license closure remain open.

Platform reference: Rust's [macOS target support](https://doc.rust-lang.org/rustc/platform-support/apple-darwin.html)
documents the target triples, deployment target and cross-compilation contract.

The read-only `tools/macos/audit_dependencies.py` checks installed package
libraries or explicit staged SDK libraries, follows their non-system load
commands and reports missing architecture slices. It shares the bundle
resolver rather than copying or modifying dependency binaries. The current
[host dependency audit](macos/dependencies-20261020.json)
found eight direct libraries and 18 total non-system libraries; all contain
only arm64. The audit's failing architecture gate is expected for these host
packages. It does not describe the earlier universal SDL/MoltenVK SDK or a
configured complete engine bundle.

The installed packages are OpenSSL 3.5.9, Opus 1.6.1 and FFmpeg 9.0.2. FFmpeg's
transitive graph includes additional codecs and OpenSSL 4 libraries alongside
the directly selected OpenSSL 3 libraries. A universal SDK must account for its
actual enabled features and complete dependency/license closure. The small
audit retains real paths, per-slice load commands, hashes and missing slices;
it retains no dependency copies or other large artifacts.

## Universal Opus and shared C++20 compatibility, 2026-10-20

`cmake/macos/Opus.cmake` and `tools/macos/build_opus.py` supply a pinned Opus
1.6.1 dylib to the engine. Each architecture gets its own configure/build and
CPU identity through `cmake/macos/DependencyToolchain.cmake`. The builder checks
the upstream archive hash, compares installed public headers, combines and
verifies the slices, sets the install name, signs the library and retains a
small manifest. Floating-point approximations and fast math remain disabled.
The SDK also supplies relocatable pkg-config metadata and the source license.

Five Mac checks passed. The unchanged production MCC decoder tests cover signed
packet lengths, mono/stereo output, authored tails and malformed extents on both
architectures. Additional nonzero 5 ms packets exercise the production decoder
and verify the actual loaded Opus image. The dependency app passes deep signature
verification after relocation into a path with spaces, runs both slices with its
original SDK directory absent, and retains an exact copy of the Opus license.
This is a decoder dependency app, not a playable engine or an audible-output
test. Physical Intel and older macOS runtime compatibility remain unverified.

The first compile exposed removed C++20 `shared_ptr::unique()` calls. Their
equivalent `use_count()!=1` detachment checks preserve the existing ownership
logic. Three existing material/texture isolation tests passed on Linux and all
six architecture-specific Mac checks passed, including escaped mutable views,
moves, concurrent immutable copies and image equality. Five changed production
material source files also compile for both Mac architectures.

The bundle gate subsequently caught a thin host Opus fallback. Declaring the
generated imported library's install name supplies its correct build rpath.
The project also uses GLM's supported header-only target, avoiding a compiled
host `libglm` dependency. Both CMake graphs configured at the recorded source
snapshots and each portability test passed. Concurrent loading-audio and other
work changed the root CMake file afterwards; those later additions are outside
that configuration evidence. The initial mirror omissions,
compiler diagnostics and failed bundle check remain in the
[Opus evidence](macos/opus-20261020.json), alongside final reports, library and
source hashes, license verification and cleanup.

All private test builds, downloaded sources/SDKs, object files, app copies and
mounted SDK images were removed after successful and failed runs. Only bounded
evidence remains. FFmpeg, complete dependency
license closure, deferred source numerical paths and the other gameplay/bundle
gates remain unfinished.

Upstream references: [Opus source checksums](https://downloads.xiph.org/releases/opus/SHA256SUMS.txt),
[Opus build options](https://github.com/xiph/opus/blob/v1.6.1/CMakeLists.txt)
and [GLM header-only target](https://github.com/g-truc/glm/blob/master/glm/CMakeLists.txt).

## Universal OpenSSL, 2026-10-20

`cmake/macos/OpenSSL.cmake` and `tools/macos/build_openssl.py` build pinned
OpenSSL 3.5.9 with upstream's `darwin64-arm64` / `darwin64-x86_64` targets and
the requested deployment target. Assembly remains enabled. Both real shared
libraries are merged, have their install names/dependencies rewritten for
relocation, and are ad-hoc signed. All 142 public headers match byte for byte
between architectures; a mismatch fails the SDK build. The SDK retains the
exact source license, hashes and configure commands. Source extractions are
published only after completion and are refreshed from the verified archive.

Upstream `no-module` places its legacy provider in `libcrypto`, alongside its
built-in default and base providers. It preserves algorithms while avoiding a
runtime dependency on the builder's module directory. The CLI and upstream
test programs are not installed. A build-tree CMake package redirect admits
later `find_package(OpenSSL)` calls through the same generated SDK targets;
the engine's pinned libdatachannel must not rediscover thin Homebrew libraries
or borrow host headers before the owned SDK has compiled.

Seven Mac checks passed: the unchanged production Steam and Discord identity
tests on both architectures, actual crypto/TLS on each architecture, and a
signed relocated dependency app. The crypto check verifies SHA1/SHA256 vectors,
base64, OS-backed random generation, the actual built-in legacy provider, a
TLS 1.3 application record with a private in-memory certificate/trust store,
and hostname-mismatch rejection. `dladdr` identifies both real loaded images.
The relocated app runs both slices with the original SDK hidden and deliberately
absent config/module paths, then verifies the source license byte for byte.
The identity tests inject their remote services and use a real loopback callback;
no user account sign-in, Steam content entitlement or public-service TLS session
is claimed. Three Linux checks passed for the same identity owners and Vulkan
portability with the existing Linux OpenSSL selection.

The complete engine graph also configured with the generated SDK and compiled
its universal identity and peer transport executables. Five integration checks
passed (both identity tests on both architectures plus Vulkan portability),
while all four direct-peer connection/lifetime checks failed their unchanged
timeouts. The minimized production-owner probe exchanged two descriptions and
eight host candidates (four IPv4, four IPv6), but never initialized DTLS. The
actual loaded crypto image came from the owned SDK. Eight independent native
UDP controls passed across both slices, covering loopback/interface-self and
IPv4/dual-stack sockets. These narrow the open frontier to ICE connectivity;
they do not prove its cause or exonerate application-specific firewall policy.
The Mac application firewall was observed enabled, with block-all disabled and
automatic signed-software allowances enabled. No firewall setting was changed.
The pinned ICE implementation is libjuice
`b89c792e3612faf2f12cf35bcc56857313a06be3`, through libdatachannel
`6b1e2e620f1e37f0eafeee702eaea0043cb305fd`. No loopback-only production binding,
relay fallback or relaxed connection requirement was introduced.

`cmake/macos/DependencyTests.cmake` retains architecture-specific aliases for
the existing production identity/peer tests. Their current peer failures stay
visible. The one-off stage and UDP controls were retired after retaining bounded
state counters, commands and source hashes. Raw signaling/ICE credentials were
not retained. All private SDKs, downloads, archives, object files, apps and
control binaries were removed after both successful and failed validation;
no large artifact was retained.

Repeat the disposable dependency suite on the Mac:

```sh
python3 -B tools/run_tests.py --source tests/macos/openssl \
  --target macos_openssl_tests --filter '^macos_openssl_' --test-jobs 1
```

The [OpenSSL evidence](macos/openssl-20261020.json) records source, SDK and signed
bundle hashes, actual relocated library paths, test commands and cleanup.
This does not establish complete engine gameplay, physical Intel compatibility,
older macOS execution, notarization, or the complete FFmpeg dependency closure.

Upstream references: [OpenSSL source release](https://github.com/openssl/openssl/releases/tag/openssl-3.5.9),
[supported Darwin targets](https://github.com/openssl/openssl/blob/openssl-3.5.9/Configurations/10-main.conf),
[built-in provider implementation](https://github.com/openssl/openssl/blob/openssl-3.5.9/providers/build.info),
and [build options](https://github.com/openssl/openssl/blob/openssl-3.5.9/INSTALL.md).

## Direct ICE connectivity investigation, 2026-10-20

The earlier unchanged full production peer suite failed all four Mac connection
and lifetime checks. `tests/macos/ice` isolates its exact pinned libjuice source
and production POLL mode without OpenSSL or libdatachannel. Linux passes actual
bidirectional payload delivery; Mac runs alternate between passing and failing
on both architectures. Passing isolated runs do not clear the production gate.

During failures, both agents advertise host candidates and send STUN packets.
Actual `sendto` calls return the full 100 bytes, but neither UDP socket becomes
readable and `MSG_PEEK` finds no queued datagram. Plain 100-byte control packets
sent after the verdict through the same sockets and exact destination addresses
also fail to arrive. This places the observed failure below STUN parsing and
DTLS. It does not establish whether the remaining cause is socket setup, routing
or host network policy. An IPv4-only diagnostic passes in a failing dual-stack
cohort; production binding remains automatic dual stack.

Setting Darwin address-length fields did not resolve the failure: interleaved
same-binary trials failed twice with the proposed change and once without it
across eight runs each. The patch was rejected. A shared-socket MUX control also
failed on Linux and differs from the engine's POLL configuration. Temporary
patches, packet metadata instrumentation and standalone socket controls were
retired after retaining bounded evidence. No ICE credential, raw description,
packet capture or large binary was retained.

No firewall, VPN or network setting was changed. Enabled system-extension
registration alone does not demonstrate interception; the process inventory
found Tailscale, which carries the authorized SSH connection. Apple's
[TN3179](https://developer.apple.com/documentation/technotes/tn3179-understanding-local-network-privacy)
states that terminal/SSH command-line programs and their children automatically
receive local-network access. The bundle now declares
`NSLocalNetworkUsageDescription` for direct player connections; actual packaged
GUI permission behavior still needs validation.

Repeat the isolated production-mode check on either host:

```sh
python3 -B tools/run_tests.py --source tests/macos/ice \
  --target macos_ice_tests --filter '^macos_ice_poll_' --test-jobs 1 \
  --retain-report ice-source.json
```

The [ICE investigation evidence](macos/ice-20261020.json) retains commands,
source hashes, minimized diagnostic sources, counter summaries and real failed
results. All owned private builds, SDKs and control binaries were removed.
Those failed cohorts remain evidence; the subsequent prompt-acceptance rerun
below establishes current local connectivity without changing the peer code.

### Connectivity after prompt acceptance

The user reported accepting all firewall/local-network prompts. A fresh build
of the unchanged isolated POLL check then passed on arm64 and x86_64: each
exchanged actual payloads in both directions using automatic dual-stack binding.
The complete engine dependency cohort also passed all nine checks: Vulkan
portability, Steam and Discord identity on each architecture, and all four
production encrypted peer checks. The latter exercise real ICE/DTLS/SCTP,
maximum-size bidirectional payloads, peer admission and packet bounds, denial
of TCP/TURN fallback, independent room lifetimes, shutdown, restart and concurrent
room creation/exchange/destruction. x86_64 runs through Rosetta on this Mac.

No connection timeout, candidate policy, library source or production peer test
was changed for this rerun. This is an observed recovery following the user's
permission action; it does not prove the exact cause of the earlier intermittent
failures. No firewall, VPN or routing setting was changed by the agent.
The local two-peer gate is now verified on both slices. Cross-machine Internet
paths, two-consumer NATs, packaged GUI permission behavior and live gameplay
remain separate acceptance gates.

The user's later reply to the same prompt question was "No prompt observed."
Keep that as the latest observation alongside the earlier acceptance report.
Neither report establishes the cause of the intermittent failures; the actual
passing payload and production-peer results above remain the validation evidence.

The [prompt-acceptance evidence](macos/ice-prompts-20261020.json) retains the
user-reported external-state change, unchanged source/library hashes, commands,
actual isolated payload counters, full production verdicts and cleanup checks.
Both private builds, OpenSSL/graphics SDKs, downloads, archives and executable
outputs were removed. No raw signaling or large generated artifact is retained.

### Mac-to-public-server UDP verification

The existing `tools/online_peer_wan_probe.cpp` also passes on both Mac slices
against the existing isolated aiortc 1.14.0 echo endpoint on `halo2-master`.
The endpoint source hash was checked before execution; no server code, service,
account or firewall configuration was changed. SSH authenticates and forwards
only SDP/ICE setup and final receipts. The production `OnlinePeerTransport`
sends its actual encrypted, unordered, zero-retransmit UDP payloads directly.

| Mac slice | Sent | Server received | Validated echoes received | Reported SCTP RTT |
| --- | ---: | ---: | ---: | ---: |
| arm64 | 50 | 49 | 47 | 54 ms |
| x86_64 via Rosetta | 50 | 49 | 47 | 45 ms |

Each 1,400-byte datagram is validated byte for byte by the server and native
probe. The existing admission threshold of at least 45 valid packets per side
is unchanged. This establishes one Mac-network/public-server path. It does not
establish two-consumer NAT coverage, physical Intel behavior, gameplay, voice,
bandwidth under load or packaged GUI permissions.

The [WAN evidence](macos/wan-20261020.json) retains both actual receipts,
native executable/source hashes, pinned build inputs, commands, disposable
coordinator source and cleanup. The native build runs through `run_tests.py`'s
worker inside an enclosing `run_test.py` owner, retaining it only while both
architecture probes execute. All private builds, SDKs, downloads and executables
were removed afterward; the remote echo processes also exited. No raw signaling,
packet capture or large artifact was retained. The disposable coordinator is
retired; its small source in the evidence can reproduce this exact setup:

```sh
python3 -B tools/run_test.py --scratch-root .local/test-scratch -- python3 -B -c \
  'import json; from pathlib import Path; exec(json.loads(Path("docs/macos/wan-20261020.json").read_text())["coordinator_source"])'
```

This assumes the authorized `codex-mac` source mirror and the existing isolated
`halo2-master` endpoint described in `multiplayer/docs/direct-player-hosted-online.md`.

## Universal Ogg/Vorbis and loading-audio boundary, 2026-10-20

`cmake/macos/Vorbis.cmake` and `tools/macos/build_vorbis.py` build checksum-pinned
Ogg 1.3.6 and Vorbis 1.3.7 separately for arm64/x86_64, then merge and sign all
four libraries. Each slice links its matching Ogg input. Public headers must
match; install names and dependent library paths are rewritten for relocation.
The SDK includes both source licenses, relative pkg-config metadata and a
manifest of archive, library and header hashes. A cancelled extraction is never
reused as complete source. Normal builds own the SDK under their build tree;
validation removes it with that tree.

Three dependency checks passed: actual Ogg bit packing, Vorbis encoder setup
and Vorbisfile callback rejection on each architecture, plus both slices of a
signed app relocated into a path with spaces with its original SDK absent.
`dladdr` verifies all four actual loaded library images. Both dependency source
licenses are copied byte for byte. The latest full engine graph also configured
on Mac, built the real production loading converter as a universal executable,
and passed its portability check. Linux configured, built that converter and
passed the same focused portability check. These checks do not compile or run
the complete engine.

The separate genuine-track check remains **failed**. Production archive reading
authenticates the installed MCC index and selected 3,356,709-byte WEM before
streaming only that selected media to owned Mac scratch. The unchanged
converter and retained ww2ogg codebook decode the full 2,744,320-frame stereo
44.1 kHz extent, including its final overlap, on both slices. Relocated runs
verify the actual bundled Ogg/Vorbis images with the original SDK absent.
Disabling Vorbis floating-point contraction preserves source expression
boundaries and makes both Mac slices byte-identical:
`c38968922521e5f031733ab6de5183df92335741591e283b778e33baa173acf7`.
The existing admission hash remains
`4170c17c50ed19f4fd1fcd292431a78564c139ff6f1324e3ea56fdefcac34d6e`.
The initial default-contraction arm64 output differed from x86_64 in 4,129,129
samples; the final compiler policy reduces that count to zero. The retained
Linux library contains FMA instructions and vector math imports, but the
remaining difference from its canonical output has not been isolated to a
specific operation. No alternate hash, approximate admission, synthetic track
or silence substitution was added. This source-build foundation does not yet
qualify fresh Mac setup or establish audible playback.

A subsequent disposable Linux differential keeps the production converter,
authenticated WEM and codebook fixed. Both Clang 23.1.1 and GCC 16.2.1 builds of
the pinned Vorbis source with contraction disabled reproduce the exact Mac
hash above. With GCC and x86-64-v4 fixed, enabling contraction produces
`2756bb0b05415ffeade0b7f9498f0549ad29f416aca41713bf5f02fd90f05dc7`;
adding fast-math produces
`3cc1cf17c5eea149bf65b12066aa424071ac771ccf8a77e891402fb01ce9ec26`.
Neither matches the admitted reference. This isolates decoder build behavior
in this comparison, without identifying the responsible operation or changing
the admission policy. The [arithmetic diagnostic](macos/loading-arithmetic-20261020.json)
retains commands, source pins, hashes and cleanup checks. All five private
library builds, source archives and WEM/WAV copies were removed; the original
game installation and existing converter were read only.

On the Mac, repeat the SDK/closure checks without game inputs:

```sh
python3 -B tools/run_tests.py --source tests/macos/vorbis \
  --target macos_vorbis_tests --filter '^macos_loading_audio_dependency_'
```

From Linux, repeat the real-track gate using an owned MCC installation (it
currently exits with the exact-profile failure):

```sh
python3 -B tools/run_test.py -- python3 -B tools/macos/test_loading_audio.py \
  --source-halo2 '/absolute/path/to/MCC/halo2'
```

The [Vorbis evidence](macos/vorbis-20261020.json) records all three passing
dependency checks and the failed real-track check, both full-graph component
builds, exact PCM/library/source hashes, the contraction diagnostic and private
storage removal. The shared original x87/SSE backend remains deferred. All
owned WEM/WAV copies, downloads, source extractions, SDKs, apps and build trees
were removed; no large test artifacts were retained.

Upstream references: [Ogg checksums](https://ftp.osuosl.org/pub/xiph/releases/ogg/SHA256SUMS),
[Vorbis checksums](https://ftp.osuosl.org/pub/xiph/releases/vorbis/SHA256SUMS),
[Vorbis CMake libraries](https://github.com/xiph/vorbis/blob/v1.3.7/lib/CMakeLists.txt)
and [source transform arithmetic](https://github.com/xiph/vorbis/blob/v1.3.7/lib/mdct.c).

## Native loading converter delivery and the PCM boundary, 2026-10-20

The inverse-transform trace now compares the real decoder boundary, rather
than compiler flags alone. Both libraries generate identical 256- and
2048-point coefficient tables, and all first twelve inverse-transform inputs
match. The first transform already differs in 184 of its 256 float words.
A diagnostic strict library that delegates only `mdct_backward` and
`vorbis_synthesis_blockin` to the retained Linux reference library reproduces
the full admitted PCM hash. This two-function substitution is an isolated
reference control, not a shipped implementation. Explicit FMA at the visible
product pairs and the tested contraction policies still fail to reproduce it.
The [arithmetic evidence](macos/loading-arithmetic-20261020.json) retains the
boundary hashes, commands and controls. The probe builds, reference wrappers,
source archives and WEM/WAV copies were removed.

A separate delivery gap is fixed: the loading provenance previously described
only the Linux ELF loader layout, while the Mac launcher omitted the native
converter and codebook. `tools/macos/package_loading_converter.py` now stages
a universal executable, its two private Ogg/Vorbis images, exact source notices
and the retained codebook. The generic bundle owner rewrites and signs the
images before the resource sealer records their final hashes. All Mac runtime
files live under `macos/loading-converter` in Resources, with the codebook at
its existing shared path. Native admission uses bounded Mach-O headers and
file hashes without requiring Xcode tools on the user's computer.

The shared Python reader dispatches the explicit Mac provenance schema and
executes its native image directly. The Linux wrapper/loader policy is retained.
The shared C++ receipt validator is a small independently buildable owner in
`src/mcc_loading_converter_metadata.cpp`; Mac layout constants remain under
`include/halo2/macos`. Linux packaging includes the small Mac metadata reader
so an offline receipt retains its original decoder provenance. No PCM hash,
source media profile, production codec arithmetic flag or original engine
x87/SSE path was changed.

The production C++ validator passes 41 controls on native arm64 and Rosetta
x86_64, plus six Python file/CPU/provenance tests. Linux's metadata cohort and
three focused root-graph checks pass, including existing loading-import
regressions; the changed production caller also passes syntax compilation.
A signed converter package passes relocation, private-library loading,
tamper/extra-file rejection and sealed-resource checks with its SDK hidden.
The complete codec cohort still reports its real-track failure: four checks
pass and the unchanged canonical PCM check fails.

The actual universal C++ launcher with bundled Python/Tk, SDL and MoltenVK also
passes after relocation into a Unicode path with spaces, with original Python,
graphics and codec SDKs hidden. Both bundled Python slices open setup/home,
recognize the downloaded MCC source through production preflight, and execute
the native converter with actual private library images. Both outputs retain
`c38968922521e5f031733ab6de5183df92335741591e283b778e33baa173acf7` and
are rejected by the existing admitted profile. Runtime resources and the
signature remain unchanged; the test's user-data roots are not created.
This validates native converter delivery and source preflight, not complete
preparation, native folder-dialog interaction, audible playback or gameplay.
The Steam session was not re-authenticated in this cohort.

To include the converter in a new launcher build, provide both
`--loading-converter /path/to/universal/halo2_mcc_loading_audio` and
`--vorbis-sdk /path/to/owned/macos-dependencies/vorbis` to
`tools/macos/package_launcher.py`. The focused combined launcher check is:

```sh
python3 -B tools/run_tests.py --source tests/macos/python \\
  --target macos_python_tests --filter '^macos_python_relocation$' \\
  --test-jobs 1 --cmake-arg=-DHALO2_MACOS_LOADING_CONVERTER_TEST=ON
```

Add `--cmake-arg=-DHALO2_MACOS_LOADING_TEST_SOURCE=/path/to/MCC/halo2` to
exercise the genuine track and record its still-failing PCM gate. The
[converter evidence](macos/loading-converter-20261020.json) includes all
results and the initial rebuild failure caused by a test PATH that omitted
`/usr/sbin`; the corrected build environment passes. Every owned Mac and Linux
build, app, SDK, archive, probe and WEM/WAV copy was removed. No owned process
or disk-image mount remains. No new large artifact is retained; the previously
authorized 42.2 GB MCC source in Application Support is preserved.

## Universal playback SDK, 2026-10-20

`cmake/macos/FFmpeg.cmake` and `tools/macos/build_ffmpeg.py` build separate
arm64/x86_64 FFmpeg and dav1d slices, then merge and sign six libraries.
The 143 public headers match byte for byte across architectures. The SDK
retains source hashes, configure commands, relative pkg-config files and
upstream license text. Every non-system runtime dependency must be one of the
owned six libraries. Linker header padding permits the longer relocated paths;
the initial packaging failure and its correction remain in the evidence.
The library reports LGPL version 2.1 or later at runtime. The engine's Xbox
ADPCM header check remains mandatory for external SDKs; the generated owned
SDK is validated by compilation and actual production decoding instead.

All six Mac checks passed: production playback and existing movie-routing
tests on each architecture, signed relocation, and Vulkan portability. The
playback check compiles the unchanged `src/movie.cpp` and `src/audio.cpp` owners,
checks all 13 required decoder registrations, and verifies the Xbox ADPCM
64-sample block contract with a zero-valued test block while rejecting
truncation. Explicit synthetic
H.264, HEVC and AV1 inputs each contain six 64x64 video frames and 9,600 AAC
audio frames. Both slices retain their frame timing, decode finite non-silent
audio, and compare every automatic-mode RGBA frame byte for byte against
software decoding. H.264/HEVC report VideoToolbox; AV1 uses software dav1d on
this Mac. This records the selected decoding device, not a separate measurement
of physical hardware acceleration.

The relocated app includes nine real non-system libraries: the six playback
libraries, SDL, Opus and OpenSSL crypto. With the original dependency tree
hidden, both slices pass again from a path containing spaces. `dladdr` verifies
the actual loaded playback images, source licenses match byte for byte and the
complete app signature verifies. The full engine target graph configures with
the generated SDK. Two Linux checks pass for unchanged movie routing and
Vulkan portability using the existing Linux dependency selection.

Repeat the disposable Mac check without game inputs:

```sh
python3 -B tools/run_test.py -- python3 -B tools/macos/test_port.py \
  --engine-configure-check --source tests/macos/ffmpeg \
  --engine-target macos_ffmpeg_tests \
  --engine-test-filter '^(vulkan_portability$|macos_ffmpeg_)' \
  --retain-report macos-dependencies/ffmpeg/manifest.json \
  --retain-report fixtures/manifest.json --retain-report ffmpeg-relocation.json
```

The fixture generator uses the installed host FFmpeg CLI solely to create
declared synthetic inputs in the private test build. Runtime validation uses
the owned libraries. The [playback evidence](macos/ffmpeg-20261020.json) records
commands, source and bundle hashes, actual loaded paths, decoder/device
results, the failed download/packaging attempts and cleanup. All private
downloads, host build tools, objects, SDKs, synthetic media and apps were
removed. No large artifact was retained. These checks do not validate full
retail movie playback, physical Intel hardware, older macOS execution, audible
output or the complete playable engine.

Upstream references: [FFmpeg release source](https://ffmpeg.org/releases/),
[FFmpeg configuration](https://github.com/FFmpeg/FFmpeg/blob/n9.0.2/configure),
[dav1d 1.5.4 source](https://github.com/videolan/dav1d/tree/1.5.4)
and [dav1d build options](https://github.com/videolan/dav1d/blob/1.5.4/meson_options.txt).

## Darwin launcher executable identity, 2026-10-20

`include/halo2/macos/executable.hpp` owns the Darwin executable query. It uses
the size requested by `_NSGetExecutablePath` instead of a fixed path buffer,
then resolves the actual file. The shared launcher dispatches to it for its
home-window relaunch command, selected-release forwarding and the internal
Discord relaunch path. `macos_bundle_contents()` uses this same helper, keeping
shader and MoltenVK resource discovery under the existing macOS folder owner.
Mac precompiled launchers use their actual executable, including when it lives
inside an app bundle, instead of assuming a Unix binary beside the data root.

The current-fixture arm64 baseline fails exactly two launcher checks before
the fix: no Discord relaunch path reaches the runtime, and a selected updated
launcher receives no forwarded command. After the fix, all eight standalone
Mac checks pass. The 34 existing first-run cases run for each of the source,
title-first and onboarding launchers on both architectures. Each tested child
uses the actual extracted and signed arm64 or x86_64 code from the universal
executable, preventing the native Python interpreter from silently choosing
arm64 for the Rosetta cases. A separate test runs the universal path helper
directly with `arch` through a relative Unicode symlink, after relocation and
a change to an unrelated working directory. Both architecture runs pass.
The full project CMake graph also builds its three actual launcher targets
and passes their native first-run checks with the unfinished engine dependency
explicitly disabled.

Four SDL/MoltenVK graphics, portability and bundle checks also pass after
sharing the path helper. Five Linux checks pass for the three launchers and
the existing frontend admission/routing checks. The routing fixture now copies
the helper modules it actually imports and supplies a fresh, explicitly
synthetic V3 loading receipt, matching the unchanged frontend admission gate.
No retained frontend proof or real cache is upgraded by that fixture.

Repeat the standalone Mac checks without game inputs:

```sh
python3 -B tools/run_test.py -- python3 -B tools/macos/test_port.py \
  --engine-configure-check --source tests/macos/launcher \
  --engine-target macos_launcher_tests \
  --engine-test-filter '^(macos_launcher_|vulkan_portability$)' \
  --retain-report launcher-paths.json
```

The [launcher evidence](macos/launcher-20261020.json) records the failed
baseline, source and extracted-slice hashes, path results, checks and cleanup.
These tests exercise real launcher binaries with synthetic setup/runtime
recorders. They do not establish actual MCC preparation, a complete packaged
GUI, a bundled Python/Tk runtime, Steam content authentication, gameplay,
physical Intel compatibility or release notarization. The resource/storage
separation and signed-app checks below build on these executable-path checks.
The subsequent bundled-runtime cohort supplies Python/Tk and native setup/home
widgets; the broader interactive setup and gameplay limits remain separate.

Upstream contract: Apple's [dyld executable-path API](https://github.com/apple-oss-distributions/dyld/blob/main/include/mach-o/dyld.h).

## Packaged launcher resources and user storage, 2026-10-20

`src/macos/application.mm` uses Foundation's user-domain Application Support
lookup for writable data, with the app directory `net.halo2forever`. On the
tested Mac this resolves to `~/Library/Application Support/net.halo2forever`.
An absolute `H2_USER_DATA_DIR` can select another directory. Both the normal
lookup and `--launcher-info` leave that directory uncreated. A data directory
inside the app, or an ancestor containing the app, is rejected after resolving
links. Unbundled source launchers retain their existing installation discovery.

Bundled launchers keep tools, static preparation profiles, shaders and the
Python runtime under `Contents/Resources`; their engine is under
`Contents/MacOS`. Imports, source-selection registries, runtime selections,
settings and relative engine outputs use the separate data root. The launcher
passes a process-local `H2_INSTALLATION_LAYOUT` binding to its children. Shared
`tools/installation_paths.py` applies that binding only to its exact data root;
unrelated roots keep their existing meaning. Static movie, prompt, UI-layout,
shader, correction and loading readers resolve the resource owner before
applying their existing containment, provenance and checksum checks. Setup's
worker uses those same tools while passing the writable root to preparation.

The packaged launcher requires its own Python interpreter and never starts a
source build. Its children disable Python bytecode writes: the writable-app
regression caught `__pycache__` changing sealed resources even though the
read-only-app check passed. With this fix, the real universal C++ launcher
passes eight checks on each architecture from relocated Unicode app paths,
with both read-only and owner-writable app resources. Setup retains the manual
MCC source, home relaunch stays inside the app, and the child runtime receives
the data working directory and imported-map paths. Every app-file hash remains
unchanged and the complete signature verifies after both permission modes.
The pre-fix launcher baseline selected app resources as mutable storage.

The resource-ownership test also exposed Linux-only `renameat2` in bank
publication. `tools/macos/atomic_files.py` now supplies Darwin's
`renamex_np(..., RENAME_EXCL)`. Movie, prompt and loading-bank publishers retain
atomic no-replace behavior, including a concurrent destination created after
preparation begins. Existing customized files are preserved; there is no
copy/replace fallback. The broader persistent-storage checklist remains open.

Validation includes all six source-launcher first-run suites (three launchers
on two architectures, 34 cases each), executable relocation, the signed-app
storage test and the shared ownership test. The full project graph builds its
three universal launcher targets and passes five native checks with the
unfinished engine dependency disabled. Four graphics/bundle checks still
pass. Linux passes nine launcher/frontend/setup CTests, 158 shared preparation
unit tests and 42 setup/home UI tests under Xvfb. On the Mac, 26 ownership,
movie and setup-worker checks pass, followed by 52 movie, prompt, loading and
ownership checks with Pillow installed only in disposable scratch. The first
bank cohort's missing-Pillow error is retained with the passing rerun.
After a concurrent shared onboarding change, six Mac ownership checks and
40 Linux onboarding/ownership checks also pass against the refreshed source.

Repeat the standalone launcher checks without game inputs:

```sh
python3 -B tools/run_test.py -- python3 -B tools/macos/test_port.py \
  --engine-configure-check --source tests/macos/launcher \
  --engine-target macos_launcher_tests \
  --engine-test-filter '^(macos_launcher_|installation_resource_ownership$|vulkan_portability$)' \
  --retain-report launcher-paths.json --retain-report launcher-storage.json
```

The [storage evidence](macos/storage-20261020.json) retains commands, source and
slice hashes, failed baselines, per-architecture app results and cleanup.
The app fixture uses explicit setup/UI/engine recorders and a shim to the host
Python interpreter. It establishes launcher routing and resource ownership;
it does not establish a self-contained Python/Tk app, real MCC preparation,
live GUI behavior, gameplay, physical Intel support or release notarization.
The exact loading PCM admission and ICE connectivity gates remain open.
Private apps, downloads, SDKs, objects and Python wheels were removed. No large
test artifact was retained.

Platform references: Apple's [Application Support directory lookup](https://developer.apple.com/library/archive/documentation/FileManagement/Conceptual/FileSystemProgrammingGuide/AccessingFilesandDirectories/AccessingFilesandDirectories.html)
and [Darwin exclusive-rename declaration](https://github.com/apple-oss-distributions/xnu/blob/main/bsd/sys/stdio.h).

## Bundled Python and native setup UI, 2026-10-20

`halo2_macos_python_sdk` assembles a checksum-pinned Python 3.14.8 runtime from
the signed PSF universal2 installer, Pillow 13.0.0 from its matching two native
wheels, and certifi 2026.10.19. The installer is expanded without installing it
or executing its installer scripts. Its private Tcl/Tk is 9.0.4. The SDK retains
the required Python, Tcl/Tk, Pillow and certificate licenses. It contains 116
universal Mach-O images and 1,067 regular files totaling 101,314,107 bytes;
these are measured test SDK values, not a playable app's download size.

`src/macos/python_launcher.cpp` locates its own runtime, binds Python, Tcl/Tk
and the CA store to that runtime, disables user-site packages and bytecode
writes, and preserves the requested CPU slice through the upstream GUI-capable
Python stub. This avoids depending on a user's Python installation or packages.
All non-system library edges stay inside the runtime, including Python.app,
the Tcl/Tk frameworks and Pillow's native libraries. Nested bundles are signed
from the inside out. Broken upstream development-header aliases and build-only
configuration scripts are excluded; every remaining link resolves inside its
owner. The general bundle helper now checks and relocates nested executables
and extensions as well as ordinary dylibs.

`tools/macos/package_launcher.py` packages an existing universal source launcher
with this SDK, the setup tools and static preparation resources. It requires a
new app output and retains the separate writable user-data owner. An optional
engine must be supplied together with its shaders. The verified app includes
the actual launcher and bundled SDL/MoltenVK, with no engine or game media.
Its build receipt explicitly records that limitation.

Both arm64 and x86_64 under Rosetta pass after moving the app to a Unicode path
and hiding its original Python and graphics SDKs. With host Python paths and
Tcl/Tk environment variables deliberately unusable, the real SDK entry point
checks standard compression, SQLite, Pillow PNG/DDS/TIFF round trips, FreeType,
certificate-checked HTTPS and private Tcl/Tk widgets. It imports the actual
setup/preparation modules and opens and closes the real source-selector and
launcher-home classes. Loaded non-system images all belong to the bundled
runtime; no data directory is created by opening these windows. The actual
C++ launcher's `--launcher-info` also succeeds on each slice. All app-file hashes
remain unchanged and the complete ad-hoc signature verifies after use.

The regression also passes four graphics/bundle CTests, including both real
MoltenVK graphics slices, nested executable relocation and rejection of a thin
embedded extension. The complete engine CMake graph configures with the new
lazy Python SDK target and its portability check passes. This does not compile
the complete engine or clear the deferred numerical backend.

Repeat the disposable Python and UI integration check on the Mac:

```sh
python3 -B tools/run_tests.py --source tests/macos/python \
  --target macos_python_tests --filter '^macos_python_relocation$' \
  --jobs 2 --test-jobs 1 --cmake-arg=-GNinja \
  --retain-report macos-dependencies/python/manifest.json \
  --retain-report launcher-sdk/manifest.json \
  --retain-report python-relocation.json
```

For a normal configured Mac build, select `cmake --build BUILD --target
halo2_macos_python_sdk`; its output is `BUILD/macos-dependencies/python`.
`HALO2_MACOS_PYTHON_INSTALLER` optionally supplies a caller-owned installer,
which must pass the same pinned checksum and PSF signature checks. Packaging
uses `tools/macos/package_launcher.py --root ROOT --launcher LAUNCHER
--python-sdk BUILD/macos-dependencies/python --app NEW.app --arch arm64
--arch x86_64`, with the MoltenVK library and SDL/MoltenVK licenses supplied
through its `--library` and `--license NAME=PATH` options.

The [Python runtime evidence](macos/python-20261020.json) retains upstream
inputs, signatures, SDK file/image hashes, commands, failed build diagnostics,
per-slice results, source hashes and verified cleanup. The temporary installer,
expanded payload, wheels, SDKs, apps, objects and test executables were removed.
No large generated artifact is retained. This closes the bundled interpreter
and native widget runtime milestone. The actual C++ interactive GUI route,
native file dialogs, full user-owned MCC preparation, Steam content sign-in,
playable engine and release notarization still need validation.

Upstream references: [PSF Python 3.14.8 release and installer](https://www.python.org/downloads/release/python-3148/),
[upstream macOS GUI stub](https://github.com/python/cpython/blob/v3.14.8/Mac/Tools/pythonw.c),
[installer dependency recipes](https://github.com/python/cpython/blob/v3.14.8/Mac/BuildScript/build-installer.py),
[Pillow wheel metadata](https://pypi.org/pypi/pillow/13.0.0/json)
and [certifi wheel metadata](https://pypi.org/pypi/certifi/2026.10.19/json).

## Steam setup

The existing server sign-in is Steam OpenID, verified by Valve and exchanged
for the game's one-use S256-bound handoff / opaque multiplayer credential. See
`multiplayer/docs/steam-identity-migration-20261016.md`. Reuse that account flow
for multiplayer in the launcher. OpenID proves identity; it does not grant a
Steam content session, depot key or CDN manifest authorization. Keep those
credentials on the user's Mac, outside the multiplayer service.

[DepotDownloader](https://github.com/SteamRE/DepotDownloader) supports macOS,
QR/Steam Guard authentication, `-os windows`, file selection, checksum validation
and an output directory. The setup helper adapts it for a separate local content
session to acquire owned MCC/Halo 2 content when the Steam macOS GUI does not
offer the Windows install. Do not pass passwords on the command line or log
login/session data. Steam OpenID/API keys cannot replace this authentication.
Select only the necessary owned depots/files, require actual entitlement, show
the download size before starting, and feed the resulting MCC root through the
existing preparation/import pipeline. The dated validation records below
distinguish metadata, sign-in, transfer and preparation acceptance. Real local
ownership, authenticated manifests, payload transfer, cancellation/resume and
file checks now pass. Complete preparation still has the loading-PCM gate.

### Live depot metadata and the private Steam runtime, 2026-10-20

A real anonymous SteamKit session recovered Valve's current PICS app metadata
for MCC (`976730`) and Halo 2: Anniversary (`1064270`). The MCC metadata binds
depot `976735` to that Halo 2 DLC. The other required depot is shared MCC content
`976731`: `mcc_loading_presentation.read_media` reads the genuine loading track
from `MCC/Content/Paks/MCC-WindowsNoEditor.pak` beside the `halo2` directory.
Downloading only the Halo 2 folder would leave the current preparation pipeline
without that required owner. The public metadata's full-depot sizes are not an
estimate for the selected files or proof of ownership.

`tools/steam/content-profile.json` records these inspected relationships and
the required preparation inputs. Shared `tools/steam_content.py` binds them to
fresh public-branch manifest IDs, selects the required core archive and the
Halo 2 asset tree, and retains the existing importer's executable/shader-cache
exclusions. It rejects changed depot ownership/layout, stale manifest bindings,
missing required files, unsafe paths, links and case/Unicode file aliases.
Unknown required depots require another inspected profile update. Its returned
plans explicitly carry `entitlement_verified=false`; an authenticated Steam
session still needs to authorize every required content owner.

`tools/steam/Program.cs` supplies the real SteamKit metadata subprocess. The
helper uses SteamKit 3.4.0 and .NET 10.0.12, with SDK 10.0.112, a checked-in
NuGet lock and checksums for acquired package archives. The Mac-specific
publisher and auditor live under `tools/macos`; the full CMake graph registers
the lazy `halo2_macos_steam_sdk` target. Building this target requires the pinned
.NET compiler, selectable through `HALO2_MACOS_DOTNET_EXECUTABLE`. The generated
helpers are self-contained and need no installed .NET runtime on the user's Mac.

The SDK has a separate private managed/native runtime tree for arm64 and
x86_64. Managed runtime assemblies and hosts are published for their CPU; these
helpers are not described as universal Mach-O images. The engine/launcher's
universal-image checks remain in force. Integrating these explicitly scoped
runtime owners into the final launcher bundle is still open.

Both Mac slices pass from a relocated Unicode SDK path with host .NET lookup
disabled and a deliberately nonexistent `DOTNET_ROOT`. The real host reports
the requested CPU and .NET version, loads its own core library, verifies a
SHA256 vector, and fetches fresh app metadata from Valve. The loaded non-system
images all remain inside that CPU's private runtime. All 30 native SDK images
pass architecture, dependency-closure and ad-hoc signature checks; their Mach-O
minimum target is 12.0. The SDK's 412 regular files total 181,460,426 bytes and
remain unchanged after both runs. Execution was on macOS 27.2, including Rosetta;
older macOS and physical Intel execution remain unverified. Microsoft's current
[.NET 10 supported-OS list](https://github.com/dotnet/core/blob/main/release-notes/10.0/supported-os.md)
also needs to inform the Steam feature's release compatibility gate.

Nine selection tests pass on Linux and the Mac, including the observed public
metadata, required shared archive, existing importer exclusions, changed depot
ownership, wrong manifests, path escape and alias rejection. The updated full
CMake graphs each pass the new policy CTest and Vulkan portability check.
The first SDK build's nullable-result compiler failure and the corrected,
compiler-warning-free builds are retained in the [Steam metadata evidence](macos/steam-20261020.json).
The final restore explicitly targets macOS, keeping its lock independent of the
build host and avoiding an unused Linux runtime. Package and HTTP/plugin caches
stay under the build owner; shared compilation/build servers and development
certificate generation are disabled. Existing shared NuGet caches and user
certificate stores were observed unchanged and preserved.

This milestone supplies the metadata transport, tested runtime and selection
boundary. QR/account authentication, license/depot-key admission, authenticated
file inventories, exact selected-byte/disk-space review, payload transfer,
resume/cancel, setup UI integration and actual import remain open. Existing
manual installation import is unchanged, and the known exact loading-PCM gate
has not been relaxed. All private .NET/package caches, SDKs, transfer archives,
build trees and executables were removed; bounded source/hash/result evidence
remains. No large generated artifact is retained.

### Local Steam setup and packaged QR verification, 2026-10-20

The next milestone adds the real setup route in `tools/macos/steam_setup.py`,
with shared subprocess ownership in `tools/steam_session.py`. It displays a
Steam Mobile QR challenge, checks the locally authenticated account's MCC and
Halo 2 licenses, authorizes the selected depot manifests, and builds a file/size
plan. Download requires a separate confirmation of that exact plan, sufficient
space, and a second path/disk check. The downloader verifies chunks for resume
and checks every selected file's full SHA-1 before offering it to the existing
importer. Real payload and interrupted-download resume are verified below;
complete preparation retains the existing loading-PCM admission gate.

The content helper adapts DepotDownloader revision
`6396afe2e34a6c40459bcb6bdafa43c83c9db8cc`, acquired with per-file hashes. The
SDK includes original and adapted corresponding sources, build inputs and its
GPL-2.0 license, alongside dependency notices. Account tokens and Steam Guard
data use an in-memory store; console account messages are suppressed. QR pixels
stay in the UI's transient image and pipes, and are cleared on cancellation or
ownership confirmation. Protocol failures use bounded diagnostic categories.

Both Mac architectures initially passed 44 content-policy checks and generated actual
Steam QR challenges. A relocated package with the real universal launcher,
Python/Tk runtime, SDL and MoltenVK then displayed its setup/home controls and
real Steam QR window on arm64 and Rosetta x86_64. Cancellation reaped the helper
and its reader in 14 ms and 52 ms, respectively, restored manual import, left no
account/download state, and preserved the sealed app's hashes and signature.
Five Linux CTests passed, including the existing manual-import UI, nine
selection tests, seven subprocess tests and three Steam control tests. The
combined Mac CMake cohort passed relocation plus selection and subprocess checks.

The two CLR trees are explicitly scoped under `Contents/Resources/steam`;
each must contain only its declared CPU and close its own dependency graph.
The bundle rejects thin images elsewhere. A negative packaging check confirmed
that boundary. The helper SDK has 456 files totaling 184,378,843 bytes before
signing; it is an input to disposable validation, not a retained installation.
The feature is offered on macOS 15 or newer; validation here covers macOS 27.2
and Rosetta, not physical Intel hardware or every supported OS version.

Human-approved sign-in exposed two omissions in the initial selection model.
Valve identifies `1097223` as `MCC - H2: Multiplayer`; it owns depots `976736`
and `976737`, separately from the campaign pack. Lockout belongs to `976736`.
The current profile authorizes those manifests as well as `976731` and `976735`,
then selects only the importer's Halo 2 tree and required shared archive.
Campaign/multiplayer manifests repeat some files. Composition accepts an exact
path from multiple authorized depots only when size and full SHA-1 agree;
case aliases or conflicting content remain errors. The transfer uses one
declared owner for each admitted file.

The corrected live plan passes: 883 selected files, 42,179,329,726 source bytes
and 28,103,673,648 compressed payload bytes. Setup conservatively reserves
128,685,472,826 free bytes for source, staging and preparation. Independent
Python policy reproduces the file/byte totals, and all seven required file
hashes and sizes match the existing Linux Steam installation with the same
manifest IDs. The current C# policy passes 50 checks on native arm64 and
Rosetta x86_64. Account identity, tokens, QR pixels and depot keys are absent
from the retained reports.

The user explicitly authorized downloading and retaining this MCC source on
the Mac for port testing. A real transfer was cancelled after 33,973,211
uncompressed payload bytes; the helper exited with cancellation status and was
reaped in 134 ms. The partial source occupied 38,432,768 allocated bytes, with
10,932,130,226 logical bytes including the sparse shared archive and metadata.
The next authenticated session validated and reused all 33,973,211 bytes before
continuing. The full transfer passed, followed by every selected file's size
and full SHA-1. The resumed run received 28,087,785,568 validated compressed
bytes, with the already verified partial chunks reused. Completion reaped the
helper and reader before handing the source directory back to setup. This
actual payload test used native arm64; the x86_64 payload route remains separate
from its passing policy, QR, cancellation and package checks.

A read-only Python audit independently checked all 883 file hashes and
42,179,329,726 source bytes against the admitted inventory. The existing source
directory reader accepts the installation, Halo 2 and maps-directory paths;
the production Lockout reader parses 5,816 tags. The archive reader authenticates
the genuine 3,356,709-byte loading WEM from the downloaded shared archive.
This verifies acquisition and reader handoff, not complete preparation or
runtime gameplay. Preparation was not started by the transfer test.

Persistent game input is explicitly retained at
`/Users/eric/Library/Application Support/net.halo2forever/downloads/steam-mcc/e46c404c25077cdd5c064c12`.
The retained tree contains 896 files, 42,183,170,486 logical bytes and
42,185,318,400 allocated file bytes, including download metadata. It is
separate from the disposable app and compiler owners. Copied SDKs,
installer/expansion inputs and Linux compiler caches were released once the
standalone download app was running. The temporary app and its remaining build
owners were removed after completion. The final audit verifies their paths are
absent, with no owned process or disk-image mount left; the read-only audit's
scratch was removed too. The known loading-audio admission gate still precedes
complete preparation.
The bounded [Steam setup evidence](macos/steam-setup-20261020.json) records the
earlier failures, corrected live plan, architecture checks and current frontier.

Upstream references: [MoltenVK runtime integration](https://github.com/KhronosGroup/MoltenVK/blob/main/Docs/MoltenVK_Runtime_UserGuide.md)
and [Steam authentication](https://partner.steamgames.com/doc/features/auth),
[SteamKit](https://github.com/SteamRE/SteamKit),
[Valve's Halo 2 DLC listing](https://store.steampowered.com/api/appdetails?appids=1064270),
and Microsoft's [architecture-specific self-contained publishing](https://learn.microsoft.com/en-us/dotnet/core/deploying/single-file/overview).


## Exact D24 storage adapter, 2026-10-20

`include/halo2/source_depth_storage.hpp` and the shared shader codec preserve
source integer depth N as the exact binary32 value N / 16777216. Every integer
from zero through 0xFFFFFF has a unique ordered representation. Encoding and
decoding use integer bit operations and do not change the deferred source
x87/SSE backend. The default physical image format remains D24S8.

The production `SourceWeatherDepthAlias` now accepts D32S8 only with an explicit
canonical-integer storage declaration. Its forward pass copies physical depth
and stencil aspects, validates each float word against that declaration and
produces the original packed color bytes. The color writeback pass converts
those bytes back into canonical depth words and unchanged stencil storage.
A noncanonical GPU word makes retirement throw and invalidates the binding;
the caller cannot accept that submission's consumers. Fresh valid production
recovers after the failed submission. Ordinary D32 writes or clears cannot be
admitted by merely changing the image format.

Three Mac CTests pass: portability and real production execution on arm64 and
Rosetta x86_64. Each architecture checks all 16,777,216 source integers on the
M2 Pro: exact forward alias bytes, unchanged source depth/stencil, an actual
color draw and exact physical writeback. All 256 stencil byte patterns occur.
Seven invalid physical words, including negative zero, NaN, infinity and
fractional-grid values, are rejected with successful recovery after each.
Mismatched image-format/encoding metadata is rejected before recording.

Three Linux CTests also pass. The RTX 5080 runs the complete carrier and native
D24S8 domains plus the existing odd-extent/discard/frame lifecycle regression.
Khronos core and synchronization validation are active and report zero errors.
The temporary validation layer came from an authenticated Arch package and was
removed after testing. No validation layer was active in the Mac runs.

The [bounded evidence](macos/depth-storage-20261020.json) records commands,
matching Linux/Mac source hashes, results, the corrected first compiler attempt
and cleanup. All owned build trees, downloaded SDKs, test executables and
validation-layer files are removed. No owned process or disk-image mount
remains; the authorized original MCC source is preserved.

This completes the storage/alias slice only. The renderer still enforces its
D24 requirement. Original clear, generic-Z and W-depth writes, depth comparisons,
bounds/bias, sampling and geometry programs need integration and differential
validation before selecting the carrier for native rendering. Full Metal
rendering, the loading PCM admission gate and playable universal-engine
acceptance remain unchecked.


## Geometry-free retained sky stencil, 2026-10-20

`SourceSkyStencil` can now execute its retained source triangle calculation
without geometryShader. Integer-only strip assembly preserves the Vulkan
primitive order, odd/even parity, degenerates and caller-supplied captured
cyclic rotations. Each instance carries the three original VSOut clip/window
pairs. Three vertex invocations select their corner and run the same shared
precise determinant and reciprocal-W plane calculation. No CPU floating plane
calculation or expected stencil mask is supplied to the producer.

The context and image must agree on native D24S8 or canonical D32S8 storage.
The shared shader encoder keeps the original D24 conversion adjustment and
encodes the D32 source integer exactly, including positive zero after a
signed-zero clamp. Original region rejection, W reconstruction, clamping,
late LEQUAL, disabled depth writes and stencil replacement remain in the
production diagnostic. The existing geometry-shader path stays available as
an independent Linux differential control.

Both Mac architectures pass 16 full-image cases at 640x480. The logical-device geometry
feature is disabled; native D24 is unavailable. The retained VSOut strip and
independent original GSOut paths exactly match every original depth/stencil
word for events 8097, 8102 and 8110, individually and as the actual three-draw
chronology. The first original draw changes 462 stencil pixels; the next two
change none. Empty regions, LEQUAL failure across all stencil byte values,
leading degenerate triangles and signed-zero clamping also pass.

Four Linux CTests pass with Khronos core and synchronization validation active
and zero errors. The original CPU/GPU suite remains exact across its 17 metric
rows. The new cohort passes 172 cases: geometry-shader and instanced paths on
both physical storage formats, all nine even/odd cyclic rotation combinations
and the original/control cases. Both Mac architecture tests and portability
also pass, for seven final CTests overall. No validation layer was active on
the Mac.

The previously derived packed-depth files were absent. The new
`tests/macos/sky_stencil/prepare_references.py` authenticates the retained
original raw readbacks and copies their packed DWORD prefix into owned scratch.
All twelve reference files must match the unchanged CMake size/SHA pins.
`HALO2_SKY_REFERENCE_ROOT` lets reference tests use that private tree without
recreating files in the original evidence directories. The Mac receives the
same authenticated files by a tar stream inside its disposable test owner.

The [bounded evidence](macos/sky-stencil-20261020.json) retains exact commands,
matching source hashes admitted before the final Mac run, reference provenance,
metrics, the excluded stale-snapshot attempt after an SSH transfer timeout and
cleanup. All owned builds, SDKs, packed-reference copies, readbacks and test
executables are removed; no owned process or mount remains. The existing
original references and authorized MCC source are preserved.

This is a retained-geometry stencil diagnostic. It does not supply authored
native sky vertices, sky color/textures, preceding world depth history or live
Renderer activation. Menu-world vertex catalogs, particle and other geometry
programs, generic depth writers and full Metal renderer integration remain
open. The exact CPU numerical backend stays deferred.

Primitive assembly follows the [Vulkan strip ordering specification](https://github.com/KhronosGroup/Vulkan-Docs/blob/main/chapters/drawing.adoc).


## Exact raw vertex fetch prerequisite, 2026-10-21

The geometry-free native world path needs to gather neighboring vertices from
raw buffers. `shaders/source_world_vertex_fetch.glsl` now preserves the hardware
fetch values for the nine existing source formats: float32 position/UV,
SNORM16 position/UV, packed signed normal words and one-to-four UNORM8 lanes.
Byte-addressed reads handle all four alignments and source strides; absent
float XYZ/W components retain the native zero/one defaults. Raw float and
packed integer bits are preserved.

The first GPU differential run found 1,320,960 one-ULP differences in normalized
lanes. Shader division by 255 or 32767 did not always match native vertex input.
The corrected encoder constructs the rounded binary32 ratio through bounded
32-bit integer quotient/remainder operations. It handles signed-normalized
saturation, including both source encodings of -1, without selecting a new
source CPU floating-point backend.

Five final CTests pass: Linux portability and fetch comparison, Mac portability
and the same universal executable on arm64 and Rosetta x86_64. Each execution
compares 36 cases, 2,359,296 vertices and 9,437,184 component values against
actual native vertex fetch, with zero different lanes and no uncovered pixels.
Every 16-bit normalized encoding and every byte-normalized encoding occurs on
each consumed lane at each alignment. Float and packed-word coverage uses
varied deterministic inputs and explicit signed-zero, subnormal, infinity and
NaN controls; it is not exhaustive over the 32-bit domain. All nine native
formats were available on both GPUs, so no padded-format control was selected.
All generated SPIR-V passes the build-time validator. Linux Khronos core and
synchronization validation report zero errors; Mac has no validation layer.

The [bounded evidence](macos/vertex-fetch-20261021.json) retains failed and
corrected comparisons, commands, exact source hashes matching both hosts and
cleanup. All owned builds, downloaded SDKs, validation-layer files, raw
readbacks and test executables are removed. No owned process or mount remains;
the authorized MCC source and Lockout source hash are preserved.

This established the shared graphics fetch prerequisite. The following stage
adds its optional native-world integration. Full Renderer selection and Metal
acceptance remain required; the broad Metal renderer item stays unchecked.


## Geometry-free native world programs, 2026-10-21

The build now generates an optional triangle ABI1 variant for all 161 shipped
world vertex programs: 128 menu ABI2 entries and 33 projected ABI3 entries,
covering classes 0–6. The original helpers, instruction statements, uniforms,
program identities and epilogue remain intact. Only the raw-input interface
and outer entry wrapper change. Three ordered source corners execute the
original program; the selected corner executes last, retaining its outputs.
The shared source depth-plane calculation then runs in the vertex stage.
There is no runtime compiler or CPU floating-point plane reconstruction.

The compiler validates both interfaces, including exact read-only SSBO
bindings/layouts and the absence of a geometry capability in the new variant.
All 161 original and triangle SPIR-V hashes match between Linux and Mac builds.
`MenuWorldSourceStorage` optionally uploads the unchanged source resource bank
with four-byte padding. Integer metadata retains original triangle order,
strip parity, degenerates, source ranges and owner/palette boundaries. Every
resource record must match the immutable source layout, including unused
resources; consumed fetch extents and shader address arithmetic are bounded.

`MenuWorldNativeDraw` has an explicit triangle mode that uses these production
uploads/descriptors and a vertex-plus-fragment pipeline. Its default remains
the existing geometry path. On macOS, triangle mode also requires attestation
that the strict shader compiler policy was actually configured. The fragment
interface now accepts the declared native-D24 or canonical integer-D24/D32
encoding. This does not select a new live depth format or establish the full
renderer's clear, generic-Z, W-depth, sampling and comparison history.

The first Mac comparison against NVIDIA original VS+GS output differed in
10,073 of 492,660 words. An independent same-device control narrowed the
wrapper-specific discrepancy to 46 color words across eight cases. It runs
the unchanged original vertex shader once per native hardware-fetched vertex,
then independently assembles its outputs and source planes in a compute pass.
Linux verifies that this control also exactly matches the original geometry
shader. This separates wrapper equivalence from cross-device arithmetic.

Disabling Metal fast math removes all 46 same-device discrepancies without
changing the program body or introducing a tolerance. The macOS
`SourceShaderMath` owner now attaches `VK_EXT_layer_settings` at instance
creation with `MVK_CONFIG_FAST_MATH_ENABLED=0`, preserving the existing pNext
chain. Renderer instance setup and the signed graphics check use this owner.
The final Mac tests deliberately set the environment value to 1; the instance
policy overrides it and produces the same words as the explicit-zero diagnostic.
The setting names and precedence follow the pinned
[MoltenVK configuration contract](https://github.com/KhronosGroup/MoltenVK/blob/v1.4.2/Docs/MoltenVK_Configuration_Parameters.md#mvk_config_fast_math_enabled)
and [instance implementation](https://github.com/KhronosGroup/MoltenVK/blob/v1.4.2/MoltenVK/MoltenVK/GPUObjects/MVKInstance.mm#L323-L354).

All 322 program/format cases now match every one of their 51 output words per
corner against the same-device original control, on NVIDIA, native arm64 and
Rosetta x86_64. Each execution compares 492,660 words. Both Mac slices produce
identical cases and input identities match Linux. These are explicit arithmetic
fixtures with ordered, overlapping and degenerate triangles, not a live camera,
pose or original framebuffer capture. Strict Mac arithmetic still differs from
the NVIDIA original path in 21,341 words, including clip and some screen values.
Cross-device numerical and pixel parity therefore remain unaccepted.

The production Linux hologram test additionally executes both complete draw
modes with the actual original rigid Regret geometry/material and scene-copy
chain. Their color readbacks match byte-for-byte across all three controls.
The existing diagnostic camera/palette and original pixel-oracle limits remain
explicit. The real storage test verifies four original draws, 12 raw streams,
165,660 bytes and 9,315 ordered triangles, plus 24 malformed-metadata rejections.
At that stage Mac validation compiled the actual draw/storage owners
universally. The following section executes the production Mac fragment path.

Twenty final CTests pass: the production Linux draw/storage pair, Linux and
Mac world-program cohorts, and Mac regressions for depth storage, raw fetch,
retained sky stencil and the relocated signed SDL/MoltenVK graphics bundle.
The graphics bundle presents three frames on each architecture. Four isolated
Python shader-source tests also pass. Linux core and synchronization validation
report zero errors; no validation layer is active on the Mac.

The [bounded evidence](macos/world-triangles-20261021.json) records source and
shader hashes, exact commands, input/output digests, failures, residual numerical
differences and cleanup. All owned builds, SDKs, validation-layer downloads,
temporary reference/oracle banks and executables are removed; no owned Mac test
process or mount remains. Original references and the authorized MCC source,
including the authenticated Lockout bytes, are preserved.

That milestone left the main checklist at 54/66. Live Renderer integration, the other geometry
producers, source GPU arithmetic/pixel acceptance, loading PCM admission and the
playable universal engine remain open. The exact CPU numerical backend stays
deferred as requested.


## Production native fragment draw and depth state, 2026-10-21

The real root-project cache, material animation, texture/model readers, raw
source storage, packet builder, vertex/fragment draw and scene-copy chain now
execute on the Mac in both universal slices. The fixture uses the original
rigid Regret section/material with an explicit diagnostic camera, identity
palette, source clock and raster controls. It exercises the production owners;
it does not supply a live cinematic pose or claim original framebuffer parity.

The Mac build uses the default pinned universal SDL, OpenSSL, Opus and
libdeflate dependencies. `lipo` verifies both architectures in the actual
29,563,608-byte test executable, 46,696,952-byte cache archive and 105,176-byte
raw storage archive. The dependency report contains the owned libraries and
system libraries, with no thin Homebrew runtime dependency. These are disposable
validation products, not an installed engine bundle.

The first attempt correctly rejected a missing retail shader-table executable.
The corrected owned reference tree contains the original 113,031,168-byte
opening map and 4,861,952-byte `default.xbe`; both full SHA-256 hashes are checked
before testing. The original shader-table identity check remains intact.
No additional shared-map input was needed by this diagnostic.

The next attempt exposed a test-harness requirement inherited from the effect
light cohort. The measured Mac reports depth clamp, BC textures, anisotropy
and shader demotion, but `nonSeamlessCubeMap=false` on both CPU slices. The
hologram samples only 2D images and creates no non-seamless cube sampler, so
its triangle mode now enables only the relevant feature set. The legacy effect
light fixture keeps its cube requirement. Original per-face cube filtering
remains a separate Metal renderer gap.

The unchanged hologram blend controls pass on both Mac slices. All three color
readback digests match between architectures. The host-computed maximum blend
error differs slightly between their reports; both stay below one byte against
the existing two-byte bound. Mac and NVIDIA color digests differ, so this does
not establish cross-device numerical or source pixel parity.

An additional 64-case production fragment regression verifies canonical D32S8
depth writes, all eight source comparison functions, both write policies and
clears of integer 0, 1, 2 and 0xFFFFFF. The explicit view row makes source W
exactly 1, with zero source factor/offset. An ALWAYS/write control exposes the
72,968-pixel depth coverage; it is a component control, not an independent
original coverage mask. Every case checks the complete depth DWORD image,
RGBA admission and preservation of the cleared A7 stencil byte. Linux and both
Mac slices pass all 64 cases, with zero reported validation errors. Linux has
the Khronos layer active; no validation layer is active on the Mac.

Six final CTests pass: three Linux production/control tests and the two Mac
architecture tests plus portability. The [bounded evidence](macos/native-world-fragments-20261021.json)
records commands, failed attempts, physical feature queries, source/reference
admission, binary architecture/dependency checks and results. The matching
[compressed source-hash manifest](macos/native-world-source-20261021.json.xz)
preserves the effective 8,812-file Mac snapshot without retaining another tree.

An active `run_test.py` owner allowed the missing-feature correction and
incremental CMake rerun to reuse its private SDKs. The finite command session
was closed after validation. A queued-command/quit lifecycle check also passes
with stdin left open. All six Linux and three Mac test owners are
verified absent, including builds, source clone, SDK downloads, original-input
copies and the feature probe. No owned Mac process or mount remains. The
authorized MCC source and authenticated Lockout bytes are preserved.

At this checkpoint the checklist was 55/67, with twelve broad gates still open. Live
Renderer activation, complete depth history and other writers, nonflat W,
bias/clip coverage, other geometry producers and original pixel acceptance
remain required. Loading PCM admission, the deferred exact CPU numerical
backend and the playable universal engine/launcher also remain open.

## Native sound storage on 16 KiB hosts, 2026-10-21

The actual `NativeSoundStorage` owner rejected a retained constructor's
`0x82e45000` physical alias on native ARM64, although Rosetta and Linux passed.
The same 3 MiB mapping at `0x82e44000` passed on ARM64. This paired control
isolated the host-page alignment guard rather than an allocation or range
failure. The original failing reports are retained.

The shared owner now separates private 4 KiB logical intervals from each
independent host-rounded OS mapping. It preserves allocation cursor increments,
source-visible extents, one-allocation bounds and the actual host value returned
by `page_size()`. `alias_page_size()` exposes the private translation granularity.
Read/write protection expands only within its owned host mapping; unsupported
modes still stop. The adapter supplies no original heap/arena state, source
readiness, device success or playback completion.

Both actual executables contain ARM64 and x86_64 slices. All eight Mac CTests
pass, including the six original constructor cases on each architecture:
172,032 complete output bytes and 4,090 ordered instruction writes match,
including every retained global and physical-limit array. The original fixture
is authenticated against its existing export manifest, then copied only into
owned temporary storage. Additional controls verify adjacent private aliases
within one numeric 16 KiB interval, hidden host padding, middle/tail subpage
protection, the last 4 KiB source-address page, rejected operations preserving
bytes and callback-owned lifetime. Four Linux normal and four ASan/UBSan CTests
also pass.

Reproduce the universal comparison from Linux using:

```sh
python3 -B tools/run_test.py --scratch-root .local/test-scratch -- \
  python3 -B tools/macos/test_sound_storage.py \
  --output build/test-results/sound-storage-original-mac
```

The [bounded evidence](macos/sound-storage-20261021.json) records both the failing
controls and successful comparisons, source/reference hashes, commands and
cleanup. Private source/fixture copies and all test builds are verified removed.
No large artifact is retained. Acquired MCC content remains preserved.

At this checkpoint the checklist was 56/68, with twelve broad gates still open. Complete sound
notifications, codec callbacks, process startup and audible runtime acceptance
remain open, along with the existing loading PCM and deferred numerical gates.

## Full Intel engine compilation and resource layout, 2026-10-21

The complete `halo2_native` x86_64 source graph now compiles and links on the
Mac. This required two shared C++ cancellation fixes in `online_directory.cpp`
and `online_matchmaking.cpp`: libc++ rejects `request_stop()` on a const capture.
The existing copied stop source remains captured before releasing the mutex;
its local copy and callback call operator are now mutable. An isolated universal
probe rejects all three const variants and verifies actual shared-state
cancellation for the corrected form on ARM64 and Rosetta.

The production directory suite passes on both universal slices. The real root
engine graph also passes directory, matchmaking and portability tests under
Rosetta. The entire matchmaking translation unit compiles for both architectures;
its runtime suite was exercised on x86_64 here. The matching Linux directory and
matchmaking regressions pass. These checks preserve the existing cancellation,
failover, configuration, parsing and queue-race controls.

The full build subsequently failed during app signing. A shared post-build
command copied tag JSON into `Contents/MacOS/data/tags`, although the existing
Mac resource reader and bundle owner already use `Contents/Resources/data/tags`.
The signing tool rejected the JSON as an unsigned code subcomponent. The shared
copy now lives in `cmake/RuntimeTagData.cmake`; Mac staging remains owned by
`cmake/macos/Bundle.cmake`. Other desktop platforms keep their executable-relative
tag directory. Linux compares all four staged tag files byte for byte.

A universal fixture calls those actual CMake staging/bundle functions. It first
reproduced the same signing failure, then passed after the correction. Both
architectures read their resources before and after relocation, and strict
signature verification passes. All five bundle regressions pass, including
existing transitive relocation and thin-library rejection. This fixture uses a
synthetic executable/dependency and supplies no game or GPU acceptance.

The Mac briefly went offline during the full build. On reconnect, authoritative
process/report checks showed that compilation had finished and signing had
failed; the original session closed and its entire build/SDK owner was removed.
The [bounded evidence](macos/engine-x86-20261021.json) and matching
[source manifest](macos/engine-x86-source-20261021.json.xz) retain the exact
source refreshes, diagnostics, component results and cleanup observations.

The corrected actual-engine build completed all 1,665 build steps. The
x86_64 app passes strict signature verification and the new
`macos_engine_cli_x86_64` test after relocation into a path containing spaces
and Unicode. Its real CLI parser executes under Rosetta; every loaded
non-system image resolves inside the relocated app, including its actual
SDL and MoltenVK libraries. Both the CLI and portability CTests pass. The app
is ad-hoc signed for development, contains a single Intel slice and has not
been notarized or accepted as playable.

The initial CLI diagnostic inherited `DYLD_PRINT_LIBRARIES` through the
protected `/usr/bin/arch` launcher, which strips that variable. A three-way
probe measured direct launch, inherited launch and `arch -e`; only the inherited
case omitted its trace. The test now sets the diagnostic in the child's
environment with `arch -e`. No executable or signature changed for the passing
rerun. The [final source manifest](macos/engine-x86-final-source-20261021.json.xz)
authenticates all 8,999 inputs and the declared test-only refresh.

A real one-frame preview against the acquired, hash-verified MCC Lockout file
loads its map and geometry but stops at missing prepared native material
metadata. The raw Cache constructor supplies no prepared owner; the source
shader initializer consequently cannot admit the preserved native library or
function tables. This probe stops before Vulkan initialization and does not
establish a Metal render result. Full preparation retains its existing strict
loading-audio gate.

The enclosing session is closed. Its 1,607,392 KiB allocated source/build/SDK
owner and the 156,281,123-byte app were removed; no process remains attached
to that owner. Bounded commands, hashes, CLI/load traces and bundle reports
remain in the evidence. The acquired MCC source and its Lockout hash remain
intact; no large test artifact was retained.

The checklist is now 57/69, with the same twelve broad gates open. Universal
ARM engine arithmetic, full renderer activation, loading PCM admission, complete
setup and gameplay/audio/input/display acceptance remain open.
