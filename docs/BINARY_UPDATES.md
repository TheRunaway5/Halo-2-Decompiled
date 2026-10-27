# Binary builds and launcher updates

Run `./build-linux.sh` from a Linux x86_64 clone after installing the dependencies listed
in [README.md](../README.md). The script resolves its repository from its own
location. `HALO2_BUILD_DIR`, `HALO2_PYTHON` and `CMAKE_BUILD_PARALLEL_LEVEL`
configure the Linux build directory, interpreter and job count. The default
profile builds both Linux and native Windows x86_64 Vulkan executables.
`--platform linux` and `--platform windows` select one platform; Windows uses
`HALO2_WINDOWS_BUILD_DIR` (default: the Linux build directory plus `-windows`)
and `HALO2_WINDOWS_SDK` (default: `.tools/windows-sdk/mingw64`).
The pinned Windows SDK is prepared automatically before compilation.

`cmake --preset desktop` and `cmake --build --preset desktop` select the same
platform pair. The default Linux CMake `ALL` build also builds `halo2_windows`;
set `HALO2_BUILD_WINDOWS=OFF` to disable that target. Focused validation excludes
it automatically.
The script and presets enable local output staging even when an older build
cache disabled it. Staging does not change launcher runtime selections.

Successful Linux x86_64 links stage these files without changing the original
build outputs or their compiled shader bindings:

| CMake target | Latest local output |
| --- | --- |
| `halo2_native` | `dist/builds/linux-x86_64/halo2_native` |
| `halo2_launcher` | `dist/builds/linux-x86_64/Halo2-Launcher` |
| `halo2_title_launcher` | `dist/builds/linux-x86_64/Halo2-Title-Launcher` |
| `halo2_onboarding` | `dist/builds/linux-x86_64/onboarding/Halo2-Launcher` |

`build-output.json` records each file's original path, size and SHA-256. It
updates after successful publication under a platform lock. Unchanged binaries
are reused. A failed copy or manifest update preserves the prior output.
These are local binaries; they still use the original runtime's shader and
library bindings. Publishing a downloadable update requires a relocatable
release package.

Windows x86_64 outputs use the corresponding `.exe` names under
`dist/builds/windows-x86_64/`. The build profile packages the full DLL import
closure, executable-bound Vulkan shader bank, tag catalogs and license files
there. `windows-package.json` records package contents and system-provided DLLs.
The Vulkan loader comes from the installed Windows graphics driver. The EOS
online SDK additionally requires the
[Microsoft Visual C++ v14 x64 Redistributable](https://aka.ms/vc14/vc_redist.x64.exe).
The manifest lists those prerequisite DLLs separately from bundled DLLs;
see [Microsoft's supported runtimes](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist).
Use
`Run-Halo2.cmd` with the native engine arguments and a prepared MCC cache.
Windows launcher setup needs Windows Python and source setup resources.

`linux-arm64`, `windows-arm64` and `macos-arm64` remain reserved platform
identifiers whose local output staging is skipped.
Disposable test builds default `HALO2_STAGE_BUILD_OUTPUT` to `OFF`, preventing
private validation binaries from accumulating in `dist/`.

The launcher Update action uses the official
[GitHub release feed](https://github.com/TheRunaway5/Halo-2-Decompiled/releases).
It does not pull source, invoke Git or compile the engine. The feed currently
has no published releases; Update reports that and preserves the installed
build. Source compilation remains available through the root script.

`runtime-selection.json` and `frontend-runtime-selection.json` are local
installation records and are ignored by Git. They bind exact runtime and
capability hashes in the current checkout. A source clone does not include
those compiled files; importing MCC prepares the asset cache only. Install a
qualified precompiled runtime or build the source before selecting a runtime.
Do not copy a development selection into another checkout without installing
and validating its runtime, shader bank and cache resources there.

A published release must include `halo2-runtime-update.json`, schema
`H2_PREBUILT_RUNTIME_UPDATE_V1`, with the actual GitHub numeric `release_id`
and a `platforms["linux-x86_64"]` entry. That entry specifies layout
`linux-packaged-slot-v1`, runtime `halo2_native`, the authenticated padded
`compiled_shader_default` literal and a `files` object. Each relative file
entry names an exact release asset and its `bytes`, `sha256` and `executable`
mode. The manifest itself must match GitHub's SHA-256 asset digest.

The supported release slot has a canonical wrapper, a relocated
`bin/halo2_native.real`, its loader/libraries, and the same 97 bundled shaders.
It may include the native launchers and separately validated frontend/gameplay
capability sidecars. Sidecars do not become accepted merely by being packaged.
No game assets, imported caches, shaders from MCC, source checkout changes or
user saves are installed by Update. Existing shader bytes must match the
release's preserved bank.

Downloads and unchanged-file copies are staged privately under
`bin/releases/`. After verification, an atomic `runtime-selection.json`
activates the complete release. Previous release slots remain available for
rollback. Root launchers forward to the verified updated launcher when one is
present, and select its runtime without overwriting development builds.
