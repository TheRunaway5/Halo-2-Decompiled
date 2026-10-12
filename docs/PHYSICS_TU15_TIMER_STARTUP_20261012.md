# Original TU1.5 timer callers and fresh queue producers

The public startup composition now calls the statically translated original
timer149/150 bodies immediately after the completed settings return. It stops
at original129's first unavailable interrupt-context read, `8001434A` from
`80035C00`, before returning a value or selecting context flags. No clock,
captured scheduler state, timer success, heap, world or physics result is
supplied. This is connected prerequisite work; live scheduling and physics
integration remain incomplete. English TU1.5 remains the only title target.

The named kernel reference is the owned modified Complex4627 BIOS in xemu.
Isolated boots retain the two timer exports, six direct dependencies and the
complete immutable153,136-byte `.text`. No mutable scheduler/clock data or
private keys are captured. The text SHA-256 is
`15b8e78cb768146c1d469cda8c39482f545227b6a3c63d6f0e19ea30de08dbfb`.
These are modified-BIOS original-code proofs, not physical Xbox execution.

The generated native graph contains nine complete bodies and304 sites:
149/150,129, the timer/ready-queue initializer `8001CD34`, DPC initializer107,
timer initializer113, due-time helper `8001C4CF`, insertion helper `8001C3CF`
and reciprocal index helper `8001CCAC`. Production uses native control flow,
with no opcode fetch/decoder or original code execution. Unknown lowering,
waiter/DPC/expiry and multiplication helpers retain full required-owner calls.
All absolute data reads require explicit ownership; missing context remains a
typed exception before the original load. The data callback binds/validates
memory and never returns an instruction value.

The constants loader authenticates the whole captured immutable text and
mounts only the nine original reciprocal/shift bytes, read-only. It supplies no
clock value, table result or mutable kernel state. The queue initializer and
record initializers execute their original stores over fresh raw backing.

The original-only procedure corpus constructs both timer and ready queues,
the expiry DPC and two timer records before every dependent arithmetic or
insertion call.37 fresh cases execute15,536 original instructions and6,317
stores. They cover flags, stack alignment, empty x87 TOP, timer types,
maximum64-bit/index-carry inputs, relative and absolute due times, expiry and
multiple fresh timers in the constructed queue. Clock records are explicitly
declared stable procedure inputs; they are not a live clock owner. Native
comparisons retain exact instruction order, ordered stores, final memory,
GP/XMM, applicable flags and FP state. A discovered ADC/SBB lowering error is
fixed; the earlier failed build/corpus and diagnostic trace remain historical.

The unchanged original TU1.5 startup corpus has48 fresh prefixes. Each now
executes the actual timer-call prologues through the required129 data read,
retaining2,760 title instructions,1,800 title stores,8,832 kernel instructions,
720 kernel stores and1,080 primary record/query/list writes. No context byte is
read or injected. Source/native final stack and CPU state compare exactly at
this boundary. Together the two corpora exercise210 of304 new sites; the
remaining94 are explicitly unverified. Both original corpora and generation
repeat byte-identically.

Evidence is under
`analysis/multiplayer/sandbox-source/tu15-timer-startup-20261012/`.
Production registration and fresh Release/ASAN/UBSAN/leak cohorts include
prior startup regression, both new comparisons and the TU1.5 image loader.
The unchanged world archive is reused only with matching dependency pins.
Production artifacts are retained in an immutable snapshot. The source and
evidence validator records the exact compilation inputs and results.

Actual native interrupt-context construction, live clocks, timer resource
lifetime, expiry/DPC servicing and the rest of startup remain required.
No timer is armed by the connected startup cases. Process heap/global
installation, source world/BSP/body/contact/controller creation, original tick
ordering and solved readback in `Simulation::step` remain open. The visible
multiplayer Lockout lift and Coagulation vehicle failures remain acceptance
failures. No completed scheduling system or full physics parity is claimed.
