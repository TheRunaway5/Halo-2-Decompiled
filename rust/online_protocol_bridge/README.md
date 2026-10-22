# Owned CXX bridge for the ONLINE protocol

This crate is the single native static-library interface to the
[unsafe-forbidden parser](../online_protocol/README.md). CXX and cxx-build are
pinned together at 1.0.205, with transitive versions/checksums in Cargo.lock.
Handwritten Rust is denied unsafe operations; the exception is the CXX-generated
schema/shims. CXX and its generated/native dependency code remain in the S13
review surface. The bridge borrows packet bytes synchronously and returns owned
fields/Opus bytes and native request controls/profiles. These parser interfaces
carry no callbacks or native-world owners into Rust. The shared crate now also
contains a separate EOS SDK adapter with native callbacks/ownership; parser
qualification does not accept that transport or its unsafe dependency surface.

Authority decoding additionally returns an opaque Rust owner of a flat value
tree. Its views stay with the immediate native materializer/hash call, and its
native allocation claims share a synchronized reservation budget. See
[archive qualification and limits](../../multiplayer/docs/online-security-authority-archive.md).

Malformed frames return typed failure status. Unexpected Rust unwinding is
caught before the extern shim; allocator/OOM abort is a separate limit. The
C++ adapter owns its returned vector; it cannot retain a borrowed Rust vector or
the caller's packet storage. Parsed data grants no sender or speaker authority.

`cmake/OnlineProtocolRust.cmake` builds this mandatory library and generated
header inside the selected CMake build tree. The build helper verifies the
actual Rust version, uses the locked manifest, reads rustc's actual native
link requirements and never links an rlib as a native archive. Supported-platform
target selection is implemented, including separate macOS slices. Windows GNU
cross-build/Wine component controls pass in the
[native-request qualification](../../multiplayer/docs/online-security-native-requests.md);
MSVC, physical Windows, macOS and full packaging acceptance remain pending.
The release runtime will link
the static library; it does not require an end-user Rust installation.

For local validation with an existing `stable` alias whose actual compiler is
exactly the pinned 1.96.0, use `-DHALO2_ONLINE_RUST_TOOLCHAIN=stable`. A compiler
version mismatch fails; the alias cannot select an arbitrary newer/older compiler.
Automatic toolchain installation is disabled. Rustup users can install the
explicit 1.96.0 toolchain before building. This setting never selects a C++
fallback or relaxes receive-path validation.

See [interface and qualification details](../../multiplayer/docs/online-security-rust-bridge.md).
