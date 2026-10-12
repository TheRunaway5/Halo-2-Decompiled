# TU1.5 startup with an observed kernel hardware data owner

The shared native loader now resolves English TU1.5's actual data import322
before mounting its read-only section. The public startup wrapper checks that
the import still belongs to the supplied immutable eight-byte resource and
continues the unchanged compiled startup caller. This is startup integration
work toward fresh world creation; it is not a completed live physics system.

The owned English TU1.5 XBE SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
No TU1.1 executable is executed for this component.

## Actual input and its limits

An isolated xemu boot reads PE export metadata and actual kernel data through
GDB. It uses the user's owned Complex4627 BIOS and MCPX, a private EEPROM copy,
and a new qcow2 overlay with the user's HDD as read-only backing. No DVD/title
is selected. The capture tool patches no guest instruction, memory or register
and supplies no platform return or physics result. Source resource hashes and
the HDD size/mtime are checked before and after the repeat boot.

Both successful fresh boots, and a repeated read of the first boot, retain:

| Export | Actual address | Bytes | SHA-256 |
| --- | --- | --- | --- |
| XboxHardwareInfo322 | `8003B150` | `41000000a3b20000` | `dc358fc8fd6118a673d0ebb0b1b46a28daa7650810c875585fa02176962f8fc2` |
| XboxKrnlVersion324 | `80010A10` | `0100000013120100` | `813dc2bd5baa4d8563998a7663b9a6b9fe8c05e07c3292434841f91d8384f1de` |

The captured metadata also repeats byte for byte. xemu is version0.8.136,
commit `fc24584ce88f0915ad7f04775bb7712c2e3f49ee`. Complex4627 is a modified
BIOS. These observations describe this named emulator resource; they do not
establish physical Xbox hardware equivalence or title/gameplay execution.
The hardware record differs from the compatibility library's default. The
loader takes actual external resource bytes and address, not a chosen flag
profile, heap pointer, settings result or recorded physics output.

The first automated repeat failed during SDL display initialization before
booting. Its `boot-v2` log is retained. The successful `boot-v3` explicitly
selects the existing X11 desktop and terminates only its own emulator process.

## Runtime wiring and comparison

`load_source_runtime_image(path, TitleUpdate15, hardware)` authenticates the
complete original file before changing the local section copy. Only the four
bytes at data cell `41B66C` change from encoded ordinal `80000142` to the actual
resource address. The imported record and original section remain read-only.
The old two-argument loader continues to import literal sections unchanged.

`source_tu15_hardware_import_owner` validates the actual request at `2E00D4`
(canonical `2DC624`), ordinal322, import cell, extent, pointer and resource bytes.
The public startup wrapper now forwards record observations from the current
generic platform dispatcher. The former wrapper interception was unreachable
because that dispatcher already consumed DPC107 and timer113 calls. The actual
generic initializer and shutdown owners now remain in the observed source
call order. No initializer return or store is replayed in production.

The new independent oracle executes unchanged original TU1.5 startup bytes
under Unicorn, with literal header/section data, loader-zeroed storage and a
patterned stack. It authenticates all169 instruction sites in the three startup
bodies. The shutdown list and DPC/timer expressions are independently compiled
from pinned primary compatibility source into i386 code and executed by the
oracle. Their explicit native void stdcall contract preserves other live CPU
state; this does not recover Xbox kernel register clobbers.

The 24 cases cover both actual startup/setup entries, flags2/CD7, three stack
alignments and empty x87 TOP0/3. They retain744 original instructions,492 source
stores and408 primary platform stores. Native comparison checks the exact
source trace, ordered writes, final banks, pending settings frame, GP/XMM,
defined flags, FS, FP control/status and MXCSR. The original oracle repeats
byte-identically. The settings callee is reached at actual `2E0FE2` from
`2E00FC`; execution stops before its first instruction, with its complete
24-byte call frame retained. No settings result is supplied.

The older sealed no-hardware fixture is preserved. Its current regression
derives the earlier pre-load boundary from the independently captured timer
call entry and declared preserving ABI. Missing ownership now throws the
explicit `SourceProcessHeapDataUnavailable` before `2E00D4`; it does not attempt
to read encoded ordinal `80000142`. All record/list stores still compare.

Tests also compare every header/section byte against the literal image, allowing
only the one four-byte import binding, check immutable resource ownership and
reject mismatched requests/resources or invalid loader addresses/owners.

## Evidence and remaining work

Evidence, frozen source, fresh Release/sanitizer builds and main-project test
results are under
`analysis/multiplayer/sandbox-source/tu15-kernel-hardware-20261012/`.
`validation.json` seals their hashes and scope. `boot-v1` and `boot-v3` retain
the actual resource reads; `original-v1` and `original-repeat` retain the new
startup comparisons. Earlier sealed startup and callback evidence is unchanged.

Settings `2E0FE2`, timer scheduling and the full startup caller remain required.
The process-heap global is not installed by these cases. Fresh world/BSP/body
and contact creation, controller construction, original tick ordering and
solved readback still need integration into `Simulation::step`. The recorded
multiplayer lift and vehicle failures remain open; the live mover still uses
host terrain collision. No new live physics or full1:1 parity is claimed.
