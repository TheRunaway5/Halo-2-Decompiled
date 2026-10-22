# MCC Halo 2 Classic: concrete performance reference mechanisms

Research date: 2026-10-22. This extends the earlier
[renderer investigation](MCC_CLASSIC_RENDERER_ARCHITECTURE_20261018.md) and
[high-refresh research](MCC_HIGH_FRAMERATE_RESEARCH_20261022.md) with actual native
backend callsites. OG Xbox Halo 2 TU1.5 remains the behavior and visual authority.
MCC supplies implementation comparisons, not permission to change that authority.

## Result

The installed MCC `halo2.dll` contains considerably more than a D3D11 import:
there is concrete code for deferred-context creation, command-list completion
and playback, redundant resource-binding suppression, indexed instancing, an
explicit three-buffer vertex/index upload ring, and immutable GPU buffer
creation. Native gameplay getters also use ordinary native pointer loads after
native object lookup. These are useful reference directions for this port.

**None of these static findings measures a speedup, establishes which paths are
active in a particular Classic scene, or proves parallel command recording.**
No MCC game session, API/GPU capture, or matched MCC/port benchmark was performed.
The user's 4K120 observation remains a useful target, not a hardware-qualified
measurement made by this investigation.

The most urgent local opportunity is still the port's measured translated CPU
work. Its installed-runtime profile attributes 7.63% exclusive CPU samples to
skinning, with near saturation of one core and substantial translated memory /
instruction / observer work. Moving all that skinning off the CPU would have an
idealized speedup ceiling of `1 / (1 - 0.0763) = 1.083x`, even before transfer and
synchronization costs. Native source execution and repeated per-view work need
attention to close the much larger gap. See the version-qualified measurements
and their host-contention limits in [frame-rate performance](FRAMERATE_PERFORMANCE.md).

## Pinned reference and method

Inspected existing input, preserved in place:

```text
/home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll
size: 15,807,960 bytes
SHA-256: de65b4f4fdbf3f0a5eab7431fe530da17dd815599182dfd6ae9b7e21cf171946
PE machine: 0x8664, AMD64
VS_FIXEDFILEINFO file/product version: 1.3528.0.0
preferred image base: 0x180000000
PE timestamp field: 1755377906
```

Version and timestamp are fields in this file; the timestamp alone is not a
release date. The hash matches the prior investigations. All addresses below
are preferred-image VAs; live ASLR changes the image base. Subtract
`0x180000000` for an RVA.

The bounded [binary evidence report](../build/test-results/mcc-classic-performance-reference-20261022/binary-evidence.json)
contains complete selected disassembly, exact command arguments, per-range byte
hashes, PE `.pdata` function bounds where available, native vtable entries, the
DLL identity, inspected local source hashes, and D3D11 method offsets. PE delay
imports identify `D3D11CreateDevice` at IAT RVA `E6EDA0` and
`D3D11CreateDeviceAndSwapChain` at `E6EDA8`; a simple ordinary-import scan misses
these delay-loaded entries.

COM vtable offsets were decoded from the actual C-interface method order in
[Microsoft's Windows SDK header](https://raw.githubusercontent.com/microsoft/win32metadata/main/generation/WinSDK/RecompiledIdlHeaders/um/d3d11.h).
The report records the fetched header SHA-256. An eight-byte slot is used for
this AMD64 file. Matching a vtable offset by itself is insufficient; the
observations below also follow the retained receiver field, surrounding API
arguments and constructor/backend table.

## Actual mechanisms found in the native backend

### Deferred command lists exist in code

The renderer initialization at `1800F2250` supplies the native context
constructor `180103CC0`, whose vtable is written as `180BE9900`. That vtable's
second entry is the initialization method `180103EA0`. The constructor records
its mode byte at context `+328` and the underlying D3D context at `+D58`.
Initialization has a conditional branch at `180103F1C`; in its deferred mode,
the device at renderer `+7510` supplies a new context:

```text
180103F25  mov  rcx, [r14+7510h]   ; retained native D3D11 device
180103F2F  lea  r8,  [rdi+D58h]    ; context output
180103F36  xor  edx, edx           ; ContextFlags = 0
180103F38  call qword ptr [rax+D8h] ; ID3D11Device::CreateDeferredContext
```

The same native context class contains:

| Operation | Callsite | Observed receiver / arguments |
| --- | --- | --- |
| Create a deferred context | `180103F38` | Device vtable `+D8`, flags zero, output context `+D58`. |
| Finish recorded commands | `180107B7C` | Loads context `+D58`, calls vtable `+390`, `RestoreDeferredContextState = FALSE`, output pointer retained at owner `+828B8`. |
| Play recorded commands | `180107BBD` | Its receiver loads context `+D58`, calls vtable `+1D0`, receives a command-list pointer, `RestoreContextState = FALSE`. |
| Clear context state during teardown | `180104178` | Loads context `+D58`, calls vtable `+370` on the conditional context path. |

The initialization loop at `1800F2675..1800F2714` constructs additional native
context owners according to a runtime count at renderer `+130`; it supplies
deferred mode to the constructor and associates a current-thread identifier
with each owner. The count, scheduling, and the threads that later record work
were not measured. A thread identifier during initialization does not prove
simultaneous worker recording.

The shared `+D58` offset identifies a field in a native class, **not the same
object instance**. The final callers and live receiver identities of the
finish/play methods were not recovered, so this report does not claim a
deferred context executes its own list. The SDK permits command-list merging
onto another deferred context; ultimate GPU execution requires an immediate
context. Which receiver/mode is used here remains an explicit capture task.

Microsoft documents these methods as
[deferred-context creation](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11device-createdeferredcontext),
[command-list completion](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-finishcommandlist),
and [command-list playback](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-executecommandlist).
This upgrades the earlier import-only boundary to **compiled command-list
machinery is present**. It does not establish that MCC Classic's draw preparation
is spread across CPU workers, that all of these modes are active on the user's
platform, or that command recording is what explains its performance.

### Redundant resource bindings are explicitly suppressed

At `1801066AA..1801066DB`, the backend indexes a cached resource-pointer bank
at owner `+810B8`. It compares the requested pointer with the cached value and
branches to the return path when equal. Otherwise it calls D3D context vtable
`+40`, `PSSetShaderResources`, then stores the new pointer. The neighboring
branches implement corresponding stage-specific resource banks. This is
concrete state-change suppression, not an inference from API choice.

The indexed-draw routine similarly checks the cached topology at owner
`+828C0` before calling `IASetPrimitiveTopology` at `180107C36` or `180107C60`.
Its exact code is in the retained report. These examples justify auditing our
redundant binding, state-selection and material-preparation work; they do not
establish how many changes MCC eliminates in a live frame.

### Indexed instancing has an explicit submission branch

The native draw routine `180107BE0..180107CCA` reads a 16-bit field from its
draw record at `+26`. When zero, it calls `DrawIndexed` at `180107C90`. When
nonzero, it passes that value as `InstanceCount` to `DrawIndexedInstanced` at
`180107CA8`; index count is three times the retained triangle count. The
routine accumulates submitted instance/triangle counts afterward. This is
actual code accepting variable instance counts, not just an instancing name.

[Microsoft's API signature](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-drawindexedinstanced)
identifies the count argument. The observed call does not identify the scene
objects grouped by the producer, prove a live count greater than one, or imply
that every authored BSP instance or skinned object is batched this way. The
producer's grouping and ordering constraints remain unrecovered.

### A three-buffer vertex/index upload ring is explicit

`18010FC80..18010FDAD` creates three D3D11 buffers. Descriptor construction and
the `CreateBuffer` call at `18010FCF5` show caller-supplied byte capacity,
`Usage = DEFAULT (0)`, `BindFlags = VERTEX_BUFFER | INDEX_BUFFER (3)`, and zero
CPU-access/misc flags. The buffer pointers remain in three 16-byte records.

`18010FE30..18010FE6B` advances the current record with `(index + 1) % 3` and
resets its cursor. `18010FE70..18010FEE8` aligns and suballocates ranges in a
retained CPU staging allocation. `18010FEF0..18010FFB2` sends the relevant byte
range to the selected GPU buffer through `UpdateSubresource` at `18010FF80`.
The context setup supplies a capacity of 4 MiB for the conditional deferred
mode or 32 MiB for the other mode at `180103F6E..180103F81`.

This is direct evidence for retained buffer allocation, range updates and
three-way rotation in this allocator. It is **not** proof of three frames in
flight: the caller cadence of the rotation method, synchronization and final
presentation queue depth were not recovered. It is also not proof that all
world meshes are streamed through this allocator.

A separate buffer-creation method, `180110090..18011010B`, supplies initial
bytes and chooses `Usage = IMMUTABLE (1)` or `DEFAULT (0)` from its owner flag,
with the same vertex/index bind bits and no CPU-access flags. This establishes
support for persistent immutable native geometry storage, without assigning
that allocation to any particular Classic mesh. Descriptor semantics are
documented in [CreateBuffer](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11device-createbuffer)
and [D3D11_USAGE](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_usage).

## Native CPU execution is a concrete comparison

The script-name string `object_get_health` has a descriptor reference at RVA
`B1E278`, whose function entry points to `180783D50`. That wrapper calls the
native getter at `1809098F0`. The getter first calls a native object lookup,
tests the result, reads a flag byte directly from `[rax+10A]`, and returns the
health float with `movss xmm0, [rax+EC]` at `18090991A`. The adjacent shield
getter uses the same pattern and loads `[rax+F0]` at `1809099BA`. The lookup
function `1808DBE10` also uses native table pointers, field loads, a type-bit
check and native calls. These are ordinary native ABI operations.

This is a representative native-data-access example, **not** a claim that those
MCC offsets or routines match Xbox TU1.5. It does not remove MCC's own handle
checks or imply every MCC operation is unchecked. There is no per-instruction
emulated-register / source-address / write-observer bridge in the inspected
getter code itself.

By comparison, our documented source-memory contract retains 32-bit original
addresses as data tokens. [The interface](../include/halo2/source_runtime_memory.hpp)
states that contract at lines 32–35 and exposes checked owner boundaries at
lines 80–100. [The implementation](../src/runtime/source_runtime_memory.cpp)
locates the source region, checks commitment and access, runs platform/write
guards and transfers bytes: `locate` at line 339, `validate` at line 355,
`read` at line 398, and `write` at line 427. That work preserves real ownership
and diagnostics, but its repeated per-operation cost is a local implementation
choice that needs whole-owner optimization where the measured profile points.

The useful transfer is to execute proven hot owner operations natively on
validated spans, retaining required side effects and exact TU1.5 arithmetic,
rather than copying MCC field offsets or globally removing checks. An oracle
comparison must cover output memory, registers/flags where relevant, faults,
callbacks, source write observers and owner/lifecycle behavior. Existing
literal-operand specialization is already installed; the performance report
does not isolate its FPS gain, so do not count it again as an available gain.

## What can be adopted and what remains unknown

| Candidate for this port | Evidence and priority | Acceptance needed |
| --- | --- | --- |
| Reduce translated CPU memory/instruction work in hot completed owners | Highest priority from our measured CPU profile; MCC native getters demonstrate a concrete native access model. | Exact original-owner differential; real caller integration; matched unsampled frame timing. Preserve diagnostics and write observers where required. |
| Retain immutable geometry; upload only changed ranges/constants | Actual MCC immutable/default resource and upload-ring code; our raw GPU model path already has retained geometry storage. | Identify which local families still repack/upload; exact geometry/program/pass/ordering and framebuffer comparison. |
| Reuse complete per-owner/per-view results and suppress redundant state | Actual MCC pointer/topology cache checks; repeated local per-view preparation is measured. | Complete cache identities, lifecycle invalidation, per-view visibility/lighting/source side effects and command-order checks. |
| Parallel read-only preparation / command recording | Actual MCC deferred command-list code provides a concrete reference seam. Live scheduling and benefit are unproved. | Separate immutable inputs from source-mutating owners; preserve final order; measure task synchronization and CPU/GPU overlap. Renderer recording alone is too small in the present profile to explain the full gap. |
| Instance compatible repeated geometry | Actual native variable-instance submission branch. Producer policy and active scene use are unproved. | Recover compatible inputs/material/pass/lighting boundaries; preserve authored instance identities and ordering; measure draw reduction. |
| Move admitted model skinning to vertex shaders | Already supported by original TU1.5 vertex-program evidence and parts of our raw GPU path, not newly proved for MCC by this task. Present exclusive CPU share bounds the upside. | Complete original declaration/palette/program/material admission and actual transformed-vertex/framebuffer comparisons. |

No new evidence here establishes compute skinning, GPU culling, indirect
submission, exact CPU portal/cluster changes, animation-palette reuse, the
active worker count, per-frame allocation rates, or GPU pass costs in MCC.
The earlier [original GPU vertex processing and CPU/GPU seam inventory](MCC_CLASSIC_RENDERER_ARCHITECTURE_20261018.md)
remains relevant. A native `render_model_skinning_disable` string or an absence
of embedded compute shaders is insufficient to answer those questions.

A decisive next reference capture should record actual Halo 2 **Classic** mode,
MCC/DLL/build identity, map and camera, player count, resolution, FOV and quality,
and both CPU-thread and GPU/API timing. Check the static callsites above during
that run to resolve which are active. Then compare the same scene on this port
with source hashes, runtime identity, visible output and frame-time percentiles.
The native backend machinery alone cannot supply a reliable percentage gain.

## Storage and validation

All one-off binary/header probes ran through `tools/run_test.py`; generated
temporary files used `HALO2_TEST_SCRATCH`. The retained evidence consists of this
Markdown document and the bounded 89,791-byte JSON report. The downloaded SDK
header, inspected disassembly working output and private scratch owners were
removed. The companion [storage receipt](../build/test-results/mcc-classic-performance-reference-20261022/storage-receipt.json) records post-exit cleanup and report
hashes. No DLL, binary, object/archive, shader extraction, frame, raw memory
bank, or render capture was retained. Original inputs, installed runtimes and
unrelated active work were preserved. Production code was not edited and no
CMake or gameplay acceptance test was performed by this research task.
