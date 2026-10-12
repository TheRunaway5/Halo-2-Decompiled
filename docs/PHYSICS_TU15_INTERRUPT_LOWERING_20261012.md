# Original TU1.5 timer interrupt-lowering prerequisites

The owned modified Complex4627 reference supplies two complete native bodies:
export163 at `80018BA0` and the internal lowering helper at `80014368`, with67
sites. The actual timer call at `800195C7` now dispatches to these bodies through
`source_tu15_interrupt_services` and the public startup composition
`source_run_tu15_process_startup_with_interrupts`. Original IRQ masks are authenticated against
the complete retained kernel text and mounted read-only. Production does not
decode or execute the original image.

Context and clock reads, scheduler callees, dynamic interrupt handlers and PIC
outputs require explicit owners. CLI/PUSHFD/POPFD preserve the retained guest
IF and flags; they do not change host CPU interrupt state. Missing owners stop
at the original instruction, preserving partial ABI and writes.

The independent original corpus has55 cases,5,298 instructions,2,021 ordered
stores and24 PIC outputs. Nine cases run original queue/timer constructors,
then original149/150 and actual lowering through their real returns. All use
explicit procedure context/clock inputs. They are not a live context owner or
a claim that process startup has completed. Missing scheduler and dispatch
owners receive no fabricated callee returns. The corpus covers44 of67 new
sites;23 sites after unavailable scheduler/dispatch calls remain unverified.

The public composition also compares all48 original fresh startup prefixes
through the retained missing context read. It preserves that boundary until a
real context owner is available.

Fresh Release and ASAN/UBSAN/leak builds each pass10 selected checks. They use
the pinned prior source foundation and its unchanged world archives, since the
shared checkout's cache header has changed independently. The initial main
project configure failed while tag sources were absent; its log remains. Once
those files appeared, both new targets built from the actual root project and
both production checks passed. Their binaries/archive/test results are retained
in an immutable snapshot. This is component validation, not a rebuilt playable
runtime or new multiplayer acceptance.

Two isolated owned xemu boots also stop at PE entry `8005504C`, before the first
kernel instruction, using an execution breakpoint without guest memory writes.
They establish loader-initial section metadata, INIT bytes and named zero
interrupt/clock records. These inputs are distinct from a running scheduler
snapshot. Original initialization subsequently raises/lowers IRQL, initializes
threads and updates clocks; loader zeros alone do not establish the context at
title startup. The first capture attempt failed because its UNIX socket path
exceeded108 bytes; its failed log is retained. Successful captures use the
short runtime temporary directory and private HDD/EEPROM copies.

Evidence lives in
`analysis/multiplayer/sandbox-source/tu15-interrupt-startup-20261012/`.
The BIOS is modified and the reference runs under xemu; physical Xbox behavior
remains unverified. Fresh native context/clock construction, timer expiry/DPC
servicing, process heap installation, source world/body/contact/controller
creation and actual `Simulation::step` integration remain required. Visible
multiplayer lift/vehicle acceptance remains red. This component completes no
player, vehicle or object physics system.
