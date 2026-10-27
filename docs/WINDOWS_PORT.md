# Native Windows Vulkan build

The Linux x86_64 desktop build profile produces both Linux ELF and native
Windows x86_64 PE executables from the same engine and Vulkan renderer.
Windows uses MinGW, SDL2, Winsock, native Windows virtual memory, file-backed
snapshots, events and waitable timers. The Windows application runs directly
on Windows; Wine is used only for execution checks on the Linux build host.

```bash
./build-linux.sh
# Or use the desktop CMake profile:
cmake --preset desktop
cmake --build --preset desktop --parallel 6
```

Use `--platform linux` or `--platform windows` for an explicit single-platform
build. The Windows dependency SDK is downloaded from MSYS2 with the package
versions and SHA-256 hashes in `tools/windows/sdk-lock.json`. The build also
prepares the Windows standard library for the pinned Rust toolchain.
See the prerequisites and output locations in the root README.

The Windows output is `dist/builds/windows-x86_64/halo2_native.exe` with its
required DLLs, shaders, small tag catalogs and `windows-package.json` inventory.
Keep that directory together. `Run-Halo2.cmd` sets its working directory and
forwards arguments to the engine. MCC content is supplied separately through
the existing import/preparation workflow. The vendor EOS library additionally
requires the [Microsoft Visual C++ x64 runtime](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist).

An asset-free presentation check is available directly on Windows:

```bat
Run-Halo2.cmd --vulkan-smoke
```

It creates an SDL window, Vulkan instance, presentation surface, graphics and
compute device, and swapchain, then submits and presents three frames. It
prints a JSON receipt with the device name and frame count before exiting.
This checks startup and presentation; game/menu acceptance is separate.

On Linux, execute the delivered Windows image with an owned Wine prefix:

```bash
python3 -B tools/run_test.py --scratch-root .local/test-scratch -- \
  python3 -B tools/windows/test_runtime.py \
  --executable dist/builds/windows-x86_64/halo2_native.exe \
  --report build/test-results/windows-vulkan-runtime.json
```

## Validation

The focused Windows resources CTest passes under Wine, covering reserve,
commit, protection, reset, events, timer expiry/rearm/cancellation and exact,
empty and truncated UDP packets. Linux UDP and virtual memory regressions
also pass. The build-profile CTests pass, including a real MinGW executable
and recursive dependency packaging fixtures. Reports are retained under
`build/test-results/windows-platform-*` and `build/test-results/windows-profile`.
Those test builds, probe executables and private Wine prefixes were removed.

Full executable build and Vulkan execution receipts are recorded after the
deliverable build completes. Physical Windows gameplay, controller, audio,
online session and rendering parity are not established by the focused checks.
