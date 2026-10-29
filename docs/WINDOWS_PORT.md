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

## Onboarding launcher

Open `Halo2-Launcher.exe` or `onboarding/Halo2-Launcher.exe` to use the same
1040-by-720 base setup and home screens as Linux. Both entry points use the
relocatable onboarding binary. The package includes native Windows
Python/Tk/Pillow, the shared setup tools, artwork/layout, approved custom
assets and the loading-audio converter inputs. No source checkout or separate
Python installation is required. Keep the complete package folder together.

The launcher binds its interpreter and Tcl/Tk libraries relative to the
package, preserves Unicode and spaced MCC paths, and starts its GUI helpers
without console windows. Setup progress uses bounded native Windows pipe
reads. EOF cancellation unwinds the import cleanup, and Windows file locks
retain cross-process importer exclusion. A retained Linux gameplay runtime
selection is ignored by the Windows launcher and stays intact; it grants no
Windows runtime or gameplay qualification.
The same frontend/gameplay readiness checks still govern Play.

Read-only Windows file admission compares kernel file IDs, sizes, last-write
and change times for both open descriptors and named paths. This avoids the
different `ctime` meanings exposed by Windows CRT/path stat APIs while
retaining changed-file and replacement rejection.

MCC donor enumeration takes fresh no-follow Windows file metadata instead of
the incomplete file IDs cached by directory entries. The original inventory
snapshot remains authoritative through copying and final publication; actual
source changes are still rejected. This avoids falsely reporting unchanged
`game.cfg` as changed during import.

MCC bitmap preflight and conversion have separate retained-memory lifetimes.
After preflight releases its cache/parser objects, conversion carries only the
bounded partition plan into its metadata allowance. The original 512 MiB
allowance and shared two-million-reference work limit remain in force across
the import; moving to a new memory phase does not refill the work counter.

The Windows loading-track converter statically links a private, checksum-pinned
Vorbis 1.3.7 decoder. Its inverse MDCT and window overlap use explicit binary32
operation order, including `fmaf` at the reference rounding boundaries. This
preserves the existing Linux PCM profile with a baseline x86-64 build. The
remaining decoder code uses strict floating-point compilation; the standard
Windows CRT supplies `fmaf`. Source notices and the preparation manifest are
staged under `licenses/Halo2-Loading-Vorbis`. The ordinary Windows build prepares
this dependency automatically in its own build tree.

Fresh graphics preparation also preserves the resource addresses authenticated
by the retained native PC map policies. Identical corrected native texture
chains share storage as in the retained COW producer. An explicit, unreferenced
296,250-byte address reservation preserves the historical preview allocation
span after V3. The original bank and exact V3 prefix must match their pinned
size and SHA before any write. Every real target partition remains checked and
read back; the reserved span owns no image. This makes fresh Needle map bytes
match the existing authority while the texture bank records its actual new
hash. The original bitmap ranges and projected menu previews stay below this
reserved address space.

## Vulkan startup

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

Add `--menu-maps /absolute/prepared-cache/halo2/h2_maps_win64_dx11` to also
check the MCC frontend. On Windows, the equivalent frontend invocation is:

```bat
Run-Halo2.cmd --menu-screen 9 --skip-intro --menu-maps "D:\prepared-cache\halo2\h2_maps_win64_dx11"
```

The Linux runtime remains a local development build at `build/halo2_native`,
beside its EOS worker and runtime libraries. Linux `dist/builds/linux-x86_64`
contains binary update copies; it is not a standalone runtime directory.

## Validation

The ordinary `./build-linux.sh` command completed both builds and refreshed
their local output manifests. The delivered native Windows PE executable
passed SDL/Vulkan startup and three presented frames on an RTX 5080 under
Wine. Its real prepared MCC main menu also reached intro-clear and presented
three qualified frames, with both processes exiting successfully. The exact
executable hash, commands, input map hash and results are in
`build/test-results/windows-vulkan-runtime.json`. The full build command log
is `.tools/windows-sdk/desktop-build-final.log`.

The same-hash Linux executable in `build/` also passed Vulkan presentation
and three qualified MCC menu frames on the RTX 5080. Its receipt is
`build/test-results/linux-vulkan-runtime.json`. The binary-only Linux `dist`
boundary is recorded separately in `linux-vulkan-runtime-dist-local-staging.json`.

The focused Windows resources CTest passes under Wine, covering reserve,
commit, protection, reset, events, timer expiry/rearm/cancellation and exact,
empty and truncated UDP packets. Linux UDP and virtual memory regressions
also pass. Build-profile and packaging CTests pass, including real MinGW
executables, recursive DLL resolution and authenticated SPIR-V inventories.
The Windows thread-local registry regression covers multiple translation
units and thread isolation. Windows map-owner tests accept valid separator
forms while retaining traversal, repeated-separator and ownership rejection.
Reports are retained under `build/test-results/windows-platform-*`,
`windows-profile-final`, `windows-profile-output-final`, `windows-package-final`,
`windows-tls-fixed-runtime`, and `windows-*-owner-separator-*`.

The onboarding comparison exercises setup, busy setup, available home and
unavailable home on Linux and native Windows Python/Tk under Wine. It checks
the actual artwork geometry, text, palette, fonts, controls and widget bounds;
native font rasterization and focus painting retain their platform behavior.
The native launchers are also checked after relocation, with Unicode/spaced
installation and MCC paths, quoted arguments, cancellation and foreign-platform
selection preservation. Detailed receipts use the
`build/test-results/windows-onboarding-*` prefix.
The Windows MCC-copy regression also checks the installed `game.cfg` and all
883 selected source-file metadata records without making another 29 GB cache
copy. Receipts use `build/test-results/windows-mcc-import-*` and
`windows-mcc-source-inventory-*`; this check does not qualify a complete import
or conversion.

The final fresh Windows setup also passes the complete native Python importer
under Wine: all 883 source files (29.10 GiB), all 43 bitmap preflights and map
conversions, the exact loading PCM profile, final native map-owner publication,
custom-map installation and private-cache selection. A second actual setup
verifies the same 883-file cache and original inputs, retains its registry and
cache directory, and performs no asset recopy. Existing UI/library maintenance
can update provenance during reuse. Results are in
`build/test-results/windows-mcc-full-import-final-v2.json`.

The paired Linux/Windows Needle check additionally reproduces the retained
9,902,592-byte map exactly, and the fixed decoder reproduces every sample of the
retained Linux loading track. Eleven focused CTest groups pass across metadata,
source inventory, preparation, onboarding, audio arithmetic, packaging and
resource layout. All owned CLI test trees and Wine processes were removed,
including the final 39,482,396,585-byte disposable import. The cleanup receipt is
`build/test-results/windows-onboarding-validation-cleanup.json`. This fresh
cache was removed after validation; the earlier same-engine Vulkan presentation
check remains separate evidence.

This launcher-only refresh retains the already authenticated engine and shader
bank. Its explicit `--shader-package` packaging route verifies that the engine
hash is unchanged, authenticates the bank against that executable, and records
the original bank provenance. Full builds still stage a fresh bank from their
build inventories.

These receipts qualify the recorded executable hashes. Later changes in the
shared checkout require their own build and validation. Physical Windows
gameplay, controller, audio, online sessions and rendering parity remain
unqualified.

## Retained storage

The requested deliverables and reusable build inputs remain available:

| Path | Retained size | Purpose |
| --- | ---: | --- |
| `dist/builds/windows-x86_64` | approximately 3.39 GB logical / 2.29 GB allocated | Windows runtime, 2,150 Vulkan modules, onboarding interpreter and setup assets; custom assets share local hardlinks |
| `dist/builds/linux-x86_64` | 2,452,346,968 bytes | Local Linux binary update copies, including debug information |
| `build-windows` | approximately 1.94 GB | Incremental Windows build |
| `build` | approximately 20.9 GB | Existing incremental Linux build and small reports |
| `.tools/windows-sdk/mingw64` | 1,631,551,801 bytes | Pinned Windows dependency SDK |
| `.tools/windows-sdk/onboarding-python` | 20,627,919 bytes | Separately pinned Pillow dependency input; original SDK remains intact |

Disposable test builds, shader staging directories, probes and private Wine
prefixes were removed. No large test captures or raw memory banks were
retained. Existing assets, reference evidence and selected launcher runtimes
are separate from these local build outputs.
