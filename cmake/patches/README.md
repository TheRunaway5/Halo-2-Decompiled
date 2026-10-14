The native peer stack remains pinned to libdatachannel
`6b1e2e620f1e37f0eafeee702eaea0043cb305fd` and its usrsctp submodule
`fec583d54493f879d2ae44a743423bf8a04371ab`.

`usrsctp-freed-window-atomic.patch` makes all userspace accesses to
`freed_by_sorcv_sincelast` atomic. The native encrypted UDP ThreadSanitizer run
observed `sctp_sorecvmsg` reading this counter concurrently with
`sctp_send_sack` clearing it under different locks. The existing increment is
already atomic. The patch uses an atomic exchange for read-and-clear and atomic
stores for the remaining assignments; no association layout or transport mode
changes. GCC/Clang use relaxed compiler atomics, and MSVC uses Interlocked.
The kernel-only branches retain their original implementation.

`tools/apply_online_peer_patches.py` applies the patch idempotently during
CMake configuration and rejects conflicts without resetting dependency edits.
The original usrsctp BSD-3-Clause and libdatachannel MPL-2.0 notices remain in
their source distributions and must accompany packaged dependencies. Native
Linux runs qualify the implementation; Windows/macOS validation stays separate.
