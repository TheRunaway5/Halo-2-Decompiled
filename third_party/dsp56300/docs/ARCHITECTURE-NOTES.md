# DSP56300 Architecture Notes

Reference knowledge derived from the DSP56300 Family Manual (Rev. 5) that has been
verified during code review rounds. This file exists to avoid re-investigating
settled questions in future reviews.

## Addressing Mode Categories (Manual Section 4.4, Table 4-1)

The DSP56300 has four addressing mode categories:

1. **Register Direct** (4.4.1): Data/control register, Rn, Mn, Nn
2. **Address Register Indirect** (4.4.2): (Rn), (Rn)+, (Rn)-, (Rn)+Nn, (Rn)-Nn, (Rn+Nn), -(Rn), (Rn+displ) - MMM 0-5, 7, and displacement sub-mode of 6
3. **PC-relative** (4.4.3): Short/long displacement, address register
4. **Special** (4.4.4): Immediate data, **absolute address** (mode 6 RRR=0), absolute short, short jump, I/O short, implicit

### Mode 6 in MMMRRR encoding

Mode 6 (MMM=110) is overloaded:
- RRR=000: **Absolute address** (Special mode) - address from extension word
- RRR=100: **Immediate data** (Special mode) - value from extension word
- Other RRR: reserved/don't-care (normalized by decoder's `norm_all`/`norm_lo`)

### Which instructions support mode 6 (absolute address)?

**"All addressing modes" / "all memory alterable"** - YES, supports mode 6. Use `emit_calc_ea_ext`:
- JMP/JSR/Jcc/JScc ea, MOVEC/MOVEM/MOVEP ea, BCLR/BSET/BTST/BCHG ea, PLOCK/PUNLOCK ea, VSL

**"Address register indirect addressing modes"** - NO, mode 6 is a Special mode, excluded. Use `emit_calc_ea`:
- DO/DOR/REP ea, JCLR/JSET/JSCLR/JSSET ea, BRCLR/BRSET/BSCLR/BSSET ea, LUA ea

The decoder marks the second group's table entries `no_mode6`, so mode-6 words
never decode as these instructions: those bit patterns belong to other
instructions (jclr-family rows = MOVE (Rn+xxxx)) or are unallocated. See
"MMM=110 Code Points..." below.

## Manual Errata / Inconsistencies

- **ROR CCR table** (p.13-166): Says "C: Set if bit 47" - copy-paste from ROL. Description correctly says C = old bit 24. Code follows description.
- **MERGE operation** (p.13-108): Says `S[7-0]` but description/example use bits 11-0 (12 bits). Code matches description.
- **ENDDO** (p.13-67): Operation says `SSL(LF) -> SR` (only LF), but BRKcc says `SSL(LF,FV) -> SR`. All code paths restore both - necessary for DO FOREVER.
- **DIV operation header** (p.13-52): Says `D[39] XOR S[15]` - these are 16-bit compatibility positions. Description says bit 55/bit 23. Code uses 55/23.
- **NORMF V flag** (p.13-147): Says "Set if bit 39 is changed" - same DSP56000 40-bit holdover as DIV. The equivalent NORM instruction (p.13-146) says bit 55. Code uses 55.
- **BTST bit field width** (p.13-41): Encoding diagrams show 4-bit `bbbb` with bit 4 fixed to 0. But the instruction fields table (p.13-40) says "Bit number [0-23]" (which requires 5 bits), and the official Motorola asm56300.exe encodes `btst #23` using all 5 bits (bit 4 = 1). The diagram is wrong; the field is 5 bits (`bbbbb`) like BCHG/BCLR/BSET. Confirmed by cross-referencing with the DSP56001 manual which also uses 5 bits. **The same 4-bit diagram error appears across the BCHG/BCLR memory forms and the JCLR/JSET/JSCLR/JSSET rows**; following those diagrams left the decoder unable to decode bit numbers 16-23 for half the family until real MCPX silicon caught `bchg #$17,y:$40` executing where the JIT skipped it (hardware difftest). All decode templates now use 5-bit `bbbbb`.

## MCPX Silicon vs Manual Discrepancies (hardware difftest)

Findings from running the difftest corpus and targeted probes on the MCPX
APU GP core (real Xbox silicon, xbtest oracle). Each claim below was
isolated in its own single-case boot so no result is tainted by an earlier
case's misbehavior.

### REP with LC = 0 executes the target zero times

The manual (p.13-160) says REP with LC=0 repeats the target 65,536 times.
On MCPX silicon the target is **not executed at all** (flags from prior
instructions survive untouched; LC reads back unchanged). Verified with a
legal spelling (single-word arithmetic target). Note the asymmetry with
DO: **DO #0 annuls the body** (skipped, matching the manual's annul
description) - verified on the same silicon.

### Hardware stack slots persist

Stack slot contents survive pops and DSP reboots (the bootstrap reloads
P memory only). A push via `movec #imm,ssh` writes only the SSH half of
the new top slot; the SSL half retains whatever an earlier push (JSR's SR,
DO's LC, a previous session) left there, and `movec ssl,xN` reads that
residue back. The manual is silent on this. The xbtest hardware runner
zeroes all 15 slots (both halves) in its boot preamble so the base state
is deterministic and matches the emulator's zero-initialized stack.

### ADDL/SUBL carry comes from the add/sub stage only

Carry-edge probes (isolated boots, operands chosen to separate the
destination shift's carry-out from the add/sub stage's carry): silicon
sets C from the 56-bit add/subtract alone - (asl_out, add_carry) of
(1,0) gives C=0, (0,1) gives 1, (1,1) gives 1. The shift-out does not
fold into C, unlike sim56300's XOR rule (re-check the sim when it runs
again). The shift still contributes V and L (bit-55 change), which
silicon confirms. ADDR/SUBR (the shift-right variants) match the
emulator on the same edge patterns.

### A2/B2 move reads sign-extend

Reading A2/B2 through a move sign-extends the 8-bit extension register
to 24 bits (`move a2,x:` with a2=$FF stores $FFFFFF; $7F stores
$00007F). Documented read path, now silicon-golden via bank b4 and
modeled in the emulator's move-read.

### BRKcc follows the manual - when spelled legally

BRKcc restores loop state exactly as the manual describes, but only
when spelled legally. An arithmetic instruction immediately before the
brk violates restriction A.3.4 ("Every arithmetic instruction ...
should not appear immediately before a BRKcc instruction") and
produces deterministic-looking undefined behavior that can be mistaken
for an architectural quirk (see "Restriction-Violation Behaviors"
below for the by-placement table). With a legal spelling (NOP between
the arithmetic and the brk, brk at LA-3 or earlier), silicon restores
LF/FV, purges the frame, and restores LA/LC exactly as p.13-28
describes - for both the taken and not-taken cases. The emulator
models the legal behavior; the corpus uses only legal spellings.

### NR/NN condition formula

NR (normalized) is Z + Ū·Ē per manual table 12-18; silicon takes JNR
for a normalized accumulator (Z=0, U=0, E=0), matching that formula.

### SSH as a bit-operation operand

The manual documents SSH pop only for the BR*/BS* bit-test branches and
is silent (or, for BTST, ambiguous) elsewhere. Silicon: **every bit-test
read of SSH pops** - BTST and the JCLR/JSET/JSCLR/JSSET jump variants
included - while the modifying ops **BSET/BCHG/BCLR rewrite the top
stack slot in place without touching SP**. Details and probe design in
"SSH Register Access Semantics" and "Bit-Test-and-Branch SSH Pop
Semantics" below. Found via bank b6's first capture (three emulator
defects fixed; goldens/b6/PROVENANCE.md).

### Out-of-range register shift counts: probed points match

The manual leaves shift counts >= 24 (LSL/LSR) undefined-ish; the
emulator's carry position for LSL n>24 / LSR n=0 falls out of shift
wrap-around rather than a deliberate rule. An isolated probe (LSL by
register counts 0, 24, 25 and LSR by 0, data $C00001) matched silicon
on all points - the accidental behavior coincides there. Bank b8
extended the probe to LSL/LSR register counts {31,32,40,55,56,63} on
the same $C00001 data - all match. Still unprobed: other DATA
patterns at out-of-range counts, and ASL/ASR-by-register with counts
>= 24 (the immediate forms cap at 23).

### Bit-reverse mode (M=0) is uniform rev-domain arithmetic

Every bit-reverse address update follows ONE rule on silicon:

    r' = rev24(rev24(r) +- rev24(|N|))    (mod 2^24)

where the +-1 plain forms use N=1. Probed via LUA across
r = $001234/$000F0F/$000010/$800001/$000250, N = 0/1/3/8, both
directions, and through a real (Rn)- memory access. Consequences, all
emergent from the rule and individually observed:

- Plain `(Rn)+` / `(Rn)-` toggle bit 0 (rev(1) = $800000, add or
  subtract at the reversed MSB).
- `(Rn)+Nn` with Nn = 0 is a no-op.
- Power-of-2 +Nn reproduces the classic FFT walk (a
  revbits = trailing_zeros(N)+1 model coincides there).
- **`(Rn)-Nn` is a true reversed borrow**, NOT the +Nn walk:
  -N=8 from $001234 gives $001238; -N=3 gives $001236, which a
  trailing-zeros model cannot express. Non-power-of-2 +Nn also follows
  the rule (+N=3 verified).

The emulator's `update_rn` implements the rev-domain rule directly,
with no special cases.

### SM saturation: rounded-grid clamp and post-clamp flags

Two SM-mode details pinned by the b9 (SR-modes) capture:

- **Rounding instructions clamp to the rounded grid.** Under SM, a
  saturating RND/MPYR/MACR (and the immediate/shift variants) leaves
  the low 24 bits ZERO: the positive rail is $00:7FFFFF:000000, not
  $00:7FFFFF:FFFFFF. Non-rounding ops (ADD/SUB/MAC/ASL/ABS/NEG...)
  clamp to the full-precision rail $00:7FFFFF:FFFFFF (probed both
  ways).
- **CCR N/Z/U/E come from the post-saturation value.** A MAC whose
  sum crosses the rail reports the clamped value's flags (E=0, U from
  the rail pattern), not the unclamped sum's. V and L still record the
  saturation event.

### Multiply-with-shift immediate counts 0 and >= 24 multiply by zero

The QQ-mulshift family (`mpy/mpyr/mac/macr S,#n,D`, 5-bit count field)
computes S * 2^-n for n in 1..23, but **n = 0 and every n >= 24 yield
a zero product** on MCPX silicon. Probed with sentinel accumulators
(#0, and #24/#25/#31 in bank b11): `mpy`/`mpyr` store 0 and set
zero-result flags (Z, U); `mac`/`macr` leave the accumulator unchanged
and compute flags from D+0. The minus (k=1) forms behave the same.

### Modulo wrap correction is direction-gated

Standard modulo addressing corrects an out-of-range result only in
the direction of travel: an ADD wraps only past the high bound, a
SUBTRACT only below the low bound. With the pointer starting OUTSIDE
the buffer (offset >= modulo, reachable for non-power-of-2 moduli),
silicon gives (M=5, r=$0247): `(r)+` -> $0242 (wrapped) but `(r)-` ->
$0246 (plain decrement, still outside). N > modulo (non-multiple)
matches the bufsize-stepping pre-wrap model in both directions
(M=5, r=$0242, N=20: + -> $0250, - -> $0234). Reserved M values
($4000-$7FFF, $C000-$FFFE, and values with bits above 15 set) behave
exactly as the emulator's decode fall-through: the $4000/$7FFF and
$C000/$FFFE bands run as standard modulo M+1, and $018000/$010003
follow the multi-wrap/modulo split with no high-bit masking surprises
(bank b11 agu_reserved_m).

### More register-operand semantics (banks b11/b12)

- **SSL bit ops rewrite the top stack slot in place, SP untouched**
  (like BSET/BCHG/BCLR on SSH); SSL reads never pop.
- **SP bit ops are plain register rewrites** (no SC recompute, no
  stack traffic).
- **Full-accumulator bit-op targets (a/b) go through the MOVE path**:
  limited (scaled) 24-bit read, RMW on the limited value,
  sign-extended write-back that clears the low word, L set by the
  read. `bset #23,a` on a = $00:7FFFFF:FFFFFF yields
  $FF:FFFFFF:000000; `bchg #0,b` on b = +1.0 yields $00:7FFFFE:000000.
- **The S flag's scaling pairs move WITH the mode**: no-scale 46^45,
  scale-down 47^46, scale-up 45^44 (the emulator had down/up
  swapped).
- **SM does NOT clamp the unsigned-mode multiplies** (mpysu/mpyuu/
  macsu/macuu, dmacsu/dmacuu) - silicon confirms the emulator's
  ss!=0 no-clamp model, including $800000 x $800000 into a near-rail
  accumulator.

### DO with SP as count source loads LC = SP, not SP+1

Manual p.13-56: "For the DO SP, expr instruction, the actual value
loaded into LC is SP before DO, incremented by one." MCPX silicon loads
SP **unmodified**: with SP=1 the body runs once, with SP=3 it runs three
times (isolated probes, iteration-counting body). The emulator's SP+1
special case is removed.

### Immediate-move space bit is a don't-care

The long-immediate parallel move (`move #>xxxxxx,D`, MMM=110 RRR=100)
carries the X/Y space bit like any other pm4 word, but the destination
comes solely from the register field. Assemblers emit the bit as 0
("X"); an isolated probe of the hand-encoded Y-flavored word
($4EF400 ext, i.e. `move #>$123456,y0` with bit 19 set) executes
identically on silicon - the space bit is a don't-care alias for the
immediate form. The corpus cannot carry this case (the assembler never
emits it); this note records the probe verdict.

## Restriction-Violation Behaviors (MCPX silicon, undefined per manual)

The manual's instruction-sequence restrictions (Appendix A.3) are real:
violating them on MCPX silicon produces deterministic-looking but
undocumented behavior, or wedges the core outright (execution never
reaches subsequent code; only a reboot recovers). Observed by isolated
probes, one boot per probe:

**BRKcc with an arithmetic instruction immediately before it (violates
A.3.4), by position relative to LA:**

| Placement | Condition | Observed |
|---|---|---|
| LA-3 or earlier | taken | branches to LA+1 **without** popping or restoring anything (LF/LA/LC/SP all left live, LC not decremented) |
| LA-1 | taken | **wedges** |
| LA-1 | not taken | loop corrupted: exits after the current iteration at LA+1 without popping; LC decremented once |
| LA | taken | branches to LA+1 without popping; LC decremented once (the fetch of LA still triggers the end-of-loop decrement) |

The bank-b2 goldens were first captured with the violating spelling and
reproduced the no-restore behavior 100% deterministically across two
capture runs - restriction UB can look perfectly stable, which is why it
masqueraded as an architectural quirk.

**ENDDO inside the loop-tail guarantee zone.** The manual guarantees
proper loop operation only if no instruction at LA-2, LA-1, or LA writes
the program-controller registers or PC (DO description, p.13-62 area).
Observed: ENDDO at LA or LA-1 **wedges**; ENDDO at LA-2 happened to
execute correctly (restore + continue) - inside the no-guarantee zone but
benign in this instance. The corpus keeps ENDDO at LA-3 or earlier.

**REP of a two-word instruction** (violates A.3.8's single-word rule,
e.g. `rep #4` + `add #>xxxx,a`): **wedges**.

**The cache instructions wedge**: pflush, pflushun, pfree, and the
plock/plockr/punlock/punlockr group are all fatal on the MCPX GP core
(isolated probes; presumably no P-cache is present/enabled). The
emulator treats them as NOPs, which diverges only for programs that are
already dead on hardware.

**DEBUG wedges** (halts the core awaiting an OnCE session). DEBUGcc
with a false condition executes as a NOP and is safe.

**ENDDO outside a loop** (LF=0): **wedges** (the frame pop underflows
the empty stack).

**Reading SSH with the stack empty** (SP=0): **wedges** (consistent with
a stack-error exception vectoring through VBA=$FF0000 into the bootstrap
ROM). The emulator survives this (posts STACK_ERROR, sets the underflow
bits); the divergence is acceptable since the program is already dead on
real hardware. The embedder's interpreter hard-asserts on the same
operation.

**Unallocated immediate-move code points $40F400-$43F400** (the ddddd =
00000-00011 rows of the mode-6 immediate encoding): execute
deterministically as a **three-word** sequence - the two following words
are consumed, x0 is loaded with the word at pc+2, pc advances by 3, no
flags or other registers change. $40F400 and $43F400 behave identically.
The decoder treats these words as Unknown; corpus programs never
contain them.

## SSH Register Access Semantics

SSH has **pop-on-read / push-on-write** side effects for **move instructions** (MOVEC, MOVEM, MOVEP). The manual describes SSH pop semantics on page 13-130 (MOVEC): "If the System Stack register SSH is specified as a source operand, the Stack Pointer (SP) is post-decremented by 1 after SSH has been read."

Hardware verdicts for the non-move register-operand accesses (isolated
MCPX probes):

- **BTST #n,SSH pops** (SP-1), like a move-source read. C is computed
  from the popped value.
- **BSET/BCHG/BCLR #n,SSH do NOT change SP**: they rewrite the **top
  stack slot in place** (verified via bchg/bclr toggling the TOS value
  that a later pop returns; a bset probe with the bit already set
  matches trivially). C is computed from the old value.
- **BSET/BCHG/BCLR on SSL rewrite the top stack slot in place too**
  (verified by the stack-walk slot dump: silicon's
  `bchg #0,ssl` residue persists in the slot across pops and reboots;
  the JIT's mirror-only store was invisible to register dumps because
  the mirror read back correctly until the next reload). SP never
  moves, matching the SSH rule. Bit ops on SP route through the SP
  write path (views recomputed, settled semantics).

Behavior at sp=0 (isolated MCPX probes):

- **Stack slot 0 is real storage for SSL.** `movec #imm,ssl` at sp=0
  reads back immediately AND persists across an SP round-trip
  (sp 0→1→0 returns the written value); `bset #n,ssl` at sp=0 works in
  place on slot 0 (readback = old|bit, C from the old value). The
  emulator stores slot 0 like any other slot (`jit_write_ssl`). Slot 0
  boots as 0 (plain SSL read
  and `btst #n,ssl` at fresh-boot sp=0 return 0, benign, SP
  untouched).
- **SSH accesses at sp=0 wedge silicon, except pushes.** `movec
  #imm,ssh` at sp=0 is an ordinary push (SP 0→1, slot 1 written,
  matches the emulator). But `btst #n,ssh` at sp=0 (a popping read →
  stack underflow) and `bset #n,ssh` at sp=0 (in-place op on the
  empty-stack slot) both hang the part: the boot never reaches its
  end PC because the stack-error exception parks in the unobservable
  boot-ROM vectors (see "Stack-Error Exception Delivery" below for the
  VBA-redirect characterization). The emulator continues benignly
  through both shapes (it posts STACK_ERROR for the underflow but
  delivers no interrupts in difftest); this is a documented divergence
  in an error state, not a corpus-reachable shape.

## Stack-Error Exception Delivery (VBA-Redirect Probes)

The sp=0 SSH shapes that hang an un-redirected boot are ordinary
stack-error exceptions: with VBA redirected to an in-image vector
(`movec #>$000200,vba`, `jsr >handler` at VBA:$02), silicon vectors and
the program continues. The hang is the exception parking in the
unobservable boot-ROM vectors at VBA=$FF0000.

Measured delivery model (MCPX GP, isolated probes, five VBA-redirect
probe rounds; branch classes pinned round 2-5):

- **Shadow window**: after a faulting instruction, a per-class budget
  of **FETCH-STREAM WORDS** executes; the first instruction whose word
  count exceeds the remaining budget is **annulled** (no side effects)
  and its address becomes the exception frame's saved PC.
  - The stream FOLLOWS BRANCHES: a `jmp` inside the window executes,
    consumes its 2 words, and the shadow continues at its target
    (probe: btst fault + jmp at window word 4 -> one target word
    executed, annulled at target+1).
  - Budgets in words after a faulting instruction of length `len`
    (equivalently, delivery boundaries in the stream):
    - register-destination pops / in-place SSH writes: **6**
      (`movec ssh,rN`, `btst/bset #n,ssh`: deliver at F+len+6);
    - memory-destination pops: **7** (`movec ssh,x:aa` delivers at
      F+8 - the store stage reads the stack later);
    - push overflow: **9 - len** (start+9 flat: 1w and 2w `movec
      #,ssh` deliver at F+9; **JSR at SP=15 completes its branch**
      and delivers at target+7, frame in slot 1);
    - SP writes that set the **SE bit**: **6 - len** (start+6 flat;
      `movec #$10,sp` and the 2-word form both deliver at start+6 -
      writing SE is itself the stack error; a **UF-only write does
      not fault** at the write, the poisoned SP faults on the next
      stack operation);
    - RTS/RTI underflow at sp=0: **3 - len** - the branch executes
      with the **slot-0 SSH storage** as target (RTI also pops SR
      from slot-0 SSL), then 2 more stream words run; pinned with
      frame-seeded probes (a phase-1 fault plants a known saved PC
      in slot-0, RTI resumes, the RTS/RTI then demonstrably branches
      to it). On a fresh boot the residue target is bootstrap
      leftovers - the unseeded round-1 probes rode the runner's
      preamble at P:$0002 (the `clr b / clr a` there is what set the
      mystery U|Z flags).
- **Frame**: long interrupt (JSR-family at the vector) pushes
  (saved PC, pre-fault SR) at the post-fault SP. Underflow faults
  (SP $00 -> $3F) push at nibble 0 -> **slot 0 holds the frame** (SP
  ends $30; slot-0 storage is real - the frame reads back). In-place
  SSH write faults set SP $00 -> $30 (UF|SE, no push/pop of their
  own); the dispatch push then lands in **slot 1** (SP $31).
- **SR on handler entry**: S1/S0 cleared (S0 probed), pre-fault SR
  preserved in the frame SSL. SC counts the frame push (0 -> 1, or
  wraps $1F -> 0 after an underflow's decrement).
- **SE latch**: with SE set, further stack ops do not re-fault - a
  deliberate handler-side `movec ssh,rN` pop at SP=$30 completed and
  returned the slot-0 frame PC (SP -> $3F).
- **Fault classes verified**: `btst #n,ssh` / `movec ssh,rN` at sp=0
  (underflow pop), `bset #n,ssh` at sp=0 (in-place on empty stack).

The emulator models this exactly (stream-budget rewrite):
fault-posting sites record the per-class word budget (a compile-time
opcode property - the opcode-cached single-instruction path needs no
runtime PC), arbitration arms an `Armed` state, and the step loop
decrements the budget per executed instruction (branches followed
naturally, since the budget rides the PC) and annuls-and-vectors when
the next instruction would exceed it. The inline JIT stack push/pop
paths (JSR/RTS/RTI/DO/ENDDO) arm the model too - ENDDO/DO-annul pop
budgets are extrapolated from the RTS/RTI class (unprobed).
Fault-capable instructions (SSH/SP movec/movem/bit-op forms, and all
branches) terminate JIT basic blocks so block mode reaches the same
instruction-granular timing (the run loop single-steps while an
interrupt is in flight). All 13 redirect probes match silicon in BOTH
difftest modes, except two acknowledged non-comparables: the unseeded
RTS/RTI probes (hardware slot-0 boot residue is unknowable) and the
UF-write probe (the poisoned SP makes the hardware runner's snapshot
walk fault mid-capture). Known gap: an SSH op inside an inline
REP/DO body does not split its block; corpus authoring keeps fault
shapes out of loop bodies.

## Core-Fault Map: ILLEGAL, TRAP, DO/ENDDO, Fast Vectors

Twelve further VBA-redirect probes (probe_ill_*, probe_trap*,
probe_se_fastvec, probe_sp_bset_se*, probe_do_overflow,
probe_enddo_underflow, probe_jsr_ovf_slot0) extend the delivery model:

- **ILLEGAL vectors at VBA:$04; TRAP and taken TRAPcc at VBA:$08**
  (Table 2-2 is right for the MCPX; the ILLEGAL page's "P:$3E" is a
  DSP56000 holdover - handlers planted at VBA:$3E/$3C are never taken).
- **Zero stream-word budget** for ILLEGAL/TRAP/taken-TRAPcc: no shadow
  words execute; the next instruction is annulled and its address
  (F+len) becomes the frame's saved PC. An RTI resume therefore SKIPS
  the faulting instruction (probed for ILLEGAL: resume exact, no
  re-execution). TRAPcc with a false condition is a plain nop.
  Long-vector frames are (F+len, pre-fault SR) at the post-dispatch SP.
- **Fast-vector shape honored**: with two plain words at the vector,
  ILLEGAL executes both words, pushes NO frame, and resumes at F+len.
  A fast-vectored STACK ERROR still annuls at its budget boundary,
  executes the two vector words, and resumes at the ANNULLED address
  (which then re-executes; SP keeps its UF/SE error bits since no
  frame is pushed - cases must still clean SP before their end PC).
- **DO push overflow budget = 3** stream words after the 2-word DO
  (delivery at start+5) - NOT the JSR push class's 9-len. LA/LC load
  before the fault and stay loaded. Slot residue: push2
  (loop-start,pre-LF SR) lands in slot 1, dispatch frame (annulled PC,
  LF-set SR) in slot 2, handler sees SP=$12; push1's wrap-write to
  slot 0 is consistent with the JSR finding below (slot 0 not dumped).
- **ENDDO pop underflow budget = 5** (start+6 flat): the double pop
  takes SP $00->$3F->$3E, LA/LC restore from slot-14 contents, and the
  dispatch frame lands in slot 15 (handler sees SP=$3F).
- **JSR overflow at SP=15: the overflowing push itself LANDS in slot
  0** (a seeded slot-0 SSL of $abcd reads back as the pushed SR
  afterwards) - the wrap-write is not annulled. The dispatch frame
  lands in slot 1, as probed earlier.
- **Bit-ops on SP behave exactly like movec SP writes**: `bset #4,sp`
  (SE bit) faults with the SpWrite class (start+6 flat, frame slot 1,
  emulator already matched); `bset #5,sp` (UF only) does not fault at
  the write.

Emulator model: ILLEGAL/TRAP/TRAPcc post their interrupt AND arm the
Armed state with budget 0 (they also terminate JIT blocks); DO/ENDDO
inline push/pop paths carry their own per-caller budgets; and the step
loop does not tick the interrupt pipeline during the delivery step (an
extra tick there makes fast (non-JSR) vectors miss the vector+2 saved-PC
restore and fall off the vector page).

## Direct SP Writes Refresh the SSH/SSL Views One Instruction Late

The SSH/SSL registers a program reads are top-of-stack **views** that
silicon reloads from the stack slot addressed by SP. Pushes, pops, and
in-place SSH/SSL writes update the views immediately, but a **direct SP
write** (`movec #d,sp` or a bit op on SP) reloads them with **one
instruction of latency**. Probed on MCPX GP silicon with all
15 stack slots seeded to a known ramp (slot j: SSH=$a0+j-1,
SSL=$c0+j-1; sp=0, views=slot0=0 before each probe):

- `movec #$2,sp` / `movec ssl,x0` / `movec ssl,x1` → x0=**0 (stale
  pre-write view)**, x1=$c1 (fresh slot-2 value).
- `movec #$2,sp` / `nop` / `movec ssl,x0` → x0=$c1: any intervening
  instruction closes the window (time-based, not read-triggered).
- `movec #$2,sp` / `movec ssh,x0` → x0=**0 (stale)** and the pop's SP
  decrement is **lost** (final sp=2); the following `movec ssl` reads
  fresh $c1.
- `bset #1,sp` / `movec ssl,x0` / `movec ssl,x1` → x0=0, x1=$c1:
  bit-op SP writes behave exactly like movec writes.
- `movec #$3,sp` / `movec #$2,sp` / `movec ssl,x0` → x0=$c1 (fresh):
  an SP write inside the shadow does not restart the window - the
  in-flight reload samples the newest SP.
- `movec #$2,sp` / `movec #$77,ssl` → the write lands in **slot 2**
  (the new SP) despite executing in the shadow; only *reads* see the
  stale view. (Verified by popping the slot back later in the run.)

Shadow × RMW and shadow × DO corners (isolated MCPX probes,
seeded-ramp fill, sp=0 before each probe):

- `movec #$2,sp` / `bchg #5,ssh` → C=0 (bit of the **stale** view, 0),
  and slot 2 ends `$000020` = stale-value ^ bit: the RMW **reads the
  stale view but its write lands at the new SP's slot**, consistent
  with the movec-write row above. `bchg #6,ssl` behaves identically
  (slot 2 = `$000040`). Settled expectation would be `$81` from the
  ramp value; the JIT models settled semantics only.
- `movec #$2,sp` / `do #n,...` → DO's **first push (LA,LC) samples the
  stale SP** (frame lands in slot 1, and that push's SP increment is
  lost to the in-flight SP write, matching the shadowed-pop rule); the
  **second push (loop-start,SR) samples the fresh SP** (frame in slot
  3, slot 2's ramp untouched; exit leaves sp=1 and LA/LC restored from
  the displaced slots). With one nop spacer before the DO, hardware
  matches the settled model exactly.
- A direct SP write as the **last body instruction of a DO** (loop-back
  and exit-pops land in the shadow): silicon completes the loop, the
  first loop-back reads the stale (correct-by-accident) view, but the
  exit pops **lose their SP decrements** (final SP stays at the
  written value) while still consuming displaced frames for the LA/LC
  restore. Emu block mode happens to match everything except the lost
  decrements; emu step mode diverges further (it follows the settled
  SSH view at loop-back). All within the banned-shape family.

The JIT models only the settled semantics (SP writes recompute the
views immediately); the one-instruction shadow is treated as a
restricted shape by the corpus authoring rules (put any instruction
between a direct SP write and the next SSH/SSL read). Real firmware is
not expected to hit it; revisit with a pipeline-shadow model only if a
program does. Found by the first dirty-state bank (b15) capture: under
zero-fill the stale view (slot 0 = 0) is indistinguishable from the
freshly reloaded value (also 0).

## DO Loop-Back Reads the SSH View; SSH RMW Inside a Body Wedges

Hardware's end-of-loop fetches the next iteration's PC from the SSH
top-of-stack view (directly evidenced by the shadow probes above: a
loop-back executing in an SP-write shadow uses the stale view). The
JIT's two modes model this differently - step follows the live SSH
register, block compiles a static loop-back - which only matters if a
program makes TOS != loop-start at an LA fall-through. Probing the
one silicon-legal-looking way to do that (`bchg #0,ssh` inside the
body, sp>0, outside any shadow) **wedges silicon** (boot never reaches
its end PC), the same signature as the sp=0 SSH ops. So no
corpus-eligible program can distinguish the two models, and the
step/block split stands as a documented non-issue on real code.
Authoring rule: inside a DO body, no direct SP writes and no SSH
writes of any kind (movec-push included) unless the stack is
rebalanced before the LA fall-through - balanced JSR/RTS pairs are
fine because TOS is the loop frame again by loop-back time.

## QQQQ Register Encoding (Table 12-16, Encoding 4)

All 16 values (0x0-0xF) are valid. Used by DMAC, MPY(su,uu), MAC(su,uu). The full mapping:

| QQQQ | S1,S2 | QQQQ | S1,S2 |
|------|-------|------|-------|
| 0000 | X0,X0 | 1000 | X1,X1 |
| 0001 | Y0,Y0 | 1001 | Y1,Y1 |
| 0010 | X1,X0 | 1010 | X0,X1 |
| 0011 | Y1,Y0 | 1011 | Y0,Y1 |
| 0100 | X0,Y1 | 1100 | Y1,X0 |
| 0101 | Y0,X0 | 1101 | X0,Y0 |
| 0110 | X1,Y0 | 1110 | Y0,X1 |
| 0111 | Y1,X1 | 1111 | X1,Y1 |

## NORMF V Flag: ASL vs ASR Paths

NORMF shifts the accumulator left (ASL) or right (ASR) based on the sign of the source operand. The V flag ("Set if bit 55 is changed during the shift") only applies to the **ASL (left shift) path**. During ASR, bit 55 is the sign bit and is always replicated - it can never change. V must be hardcoded to 0 for the ASR path.

The same uniformity check used for multi-bit ASL (checking whether bits [55:(55-n)] are all-same) is correct for the left-shift path, but must NOT be applied to the right-shift path.

## Parallel Move Duplicate Destinations

The manual (p.13-120) explicitly states: "duplicate destinations are not allowed within the same instruction." This means the ALU-wins-on-conflict question (when both the ALU and parallel move write to the same register) is **undefined behavior** for illegal instruction encodings. Our pm_0 has a defensive check; other move types do not, since the input is architecturally invalid.

## Long Interrupt SR Clearing (Section 2.3.2.5)

When a long interrupt is formed, the following SR bits are cleared: LF, S1, S0, I1, I0, **and SA** (Section 2.3.2.5). The SA bit (bit 17) is easy to miss since SA mode is otherwise unimplemented, but the clear must still happen to maintain correct SR state across interrupt boundaries. **FV is NOT cleared** - it is not listed in the manual's enumeration of cleared bits.

## Hardware Stack Overflow Behavior

When SP is at 15 and a push occurs, SP wraps to 0 (P[3:0] = 0000) with SE bit set, and a STACK_ERROR exception is posted. **The manual does not explicitly state whether the push data is written to stack slot 0 or silently dropped** (Section 5.4.3.1, Table 5-2).

The implementation treats the hardware stack as a circular buffer that overwrites slot 0 on overflow. **Hardware-verified** (probe_jsr_ovf_slot0 / bank b21 fault_jsr_ovf_slot0): slot-0 SSL seeded with $abcd before a JSR at SP=15 reads back as the pushed SR afterwards - the overflowing push's wrap-write LANDS in slot 0 and is not annulled. This matches the circular-buffer reading: P[3:0] wraps to 0 and SP "always points to the top of stack" (manual p.5-19).

Programs that overflow the hardware stack without stack extension (SEN=0) are already in an error state - the STACK_ERROR interrupt fires and the data at slot 0 is stale regardless.

## RND V Flag: Positive-Addend Overflow Formula

The RND instruction adds a positive rounding constant to the accumulator. The standard overflow formula `V = (sign_A XOR sign_R) AND (sign_B XOR sign_R)` simplifies to `V = (sign_A XOR sign_R) AND sign_R` when sign_B = 0 (positive constant). This means overflow is only possible in the positive-to-negative direction - adding a positive value to a negative value cannot overflow (it moves toward zero). A simple `sign_A XOR sign_R` (any sign change) is incorrect because it false-positives when a small negative value rounds to zero/positive.

## Bit-Test-and-Branch SSH Pop Semantics

The manual only documents SSH pop for BRCLR/BRSET/BSCLR/BSSET (p.13-26,
13-29, 13-32, 13-35) and is silent for JCLR/JSET/JSCLR/JSSET. Isolated
MCPX probes (jclr and jsset with the branch not taken) show
the jump and jump-subroutine variants **pop SSH as well**: every
bit-test read of SSH pops, regardless of variant. `read_bit_test_operand`
therefore pops unconditionally for an SSH register operand; see the SSH
access-semantics section above for the pure bit ops (BTST pops;
BSET/BCHG/BCLR modify the top slot in place).

## DO FOREVER Cycle Count

DO FOREVER = 4 cycles per Table A-1, distinct from all other DO variants (5 cycles). DOR FOREVER timing is not separately listed in Table A-1.

## MMM=110 Code Points in Bit-Test/Loop ea Rows Belong to Other Instructions

The MMMRRR ea field of jclr/jset/jsclr/jsset is bit-identical, at MMM=110,
to the long-displacement move `MOVE X:/Y:(Rn + xxxx),D` / `MOVE D,X:/Y:(Rn +
xxxx)` (opcode `0000101s01110RRR1WDDDDDD`, manual p.16-618/16-814). The MOVE
owns those code points; the bit-test-and-jump instructions have no
absolute-long form ("All address register indirect addressing modes...
Absolute short and I/O short addressing modes can also be used", p.13-85).

Ground truth (verified with Motorola's official tools under wine):

- asm56300 6.3.15 emits `0B70C4 000C08` for `move y:(r0+$c08),x0`, and
  rejects `jsclr #4,y:$0c0c,$0c0c` with "Absolute address must be either
  short or I/O short".
- sim56300 disassembles `0B70C4 000C08` as `move y:(r0+$c08),x0` and
  executes it as that move: with r0=5 and y:$c0d=$123456 it loads
  x0=$123456, pc advances by 2, and it never branches or pushes the stack
  regardless of the bit value at y:$c08.

The MMM=110 rows of brclr/brset/bsclr/bsset ea, DO/DOR ea (extension word =
loop address), and REP ea (no extension word at all) collide with nothing:
sim56300 disassembles them as `dc`, i.e. they are unallocated. The decoder
marks all of these entries `no_mode6` so the colliding MOVE (or Unknown)
decodes instead, and the assembler rejects the corresponding source forms
like the official assembler does.

## SC (Stack Counter) Tracks Every Push/Pop

The 5-bit SC register "monitors how many entries of the hardware stack are
in use" (manual 5.4.3.2) and is updated implicitly by every stack push and
pop — DO (which pushes twice: LA/LC then PC/SR), JSR/BSR/JScc, RTS/RTI,
ENDDO, and MOVEC accesses to SSH. Verified against sim56300: mid-DO-body
the sim shows sc=2; balanced jsr/rts returns sc to 0; an unbalanced
movec-to-ssh push leaves sc=1. Writing SP directly does NOT recompute SC.
SC drives the stack-extension full/empty thresholds (sc=14 full, sc=2
empty, manual A.2.5), so it matters even before stack extension is
implemented.

## No Immediate Form for L-Space Parallel Moves

The Pm4/Pm5 L: move encodings with ea mode 6 RRR=100 (immediate data,
e.g. $40F400 = the bit pattern "move #>xxxx,a10" would occupy) are
unallocated: sim56300 disassembles them as `dc`, and asm56300 rejects
`move #imm,a10` with "Illegal X field destination register specified".
X:/Y: long-immediate moves ($44F400 = move #>xxxx,x0) remain valid.

## Immediate Moves to M/Control Registers Use MOVEC Encodings

`move #imm,<M or control register>` has no PM3/PM4 encoding (those
register fields are 5-bit; M0-M7 and the control registers are 6-bit
codes $20-$3F). asm56300 assembles these to the MOVEC immediate forms:
`move #$4,m0` = $0504A0 (short), `move #>$ffffff,m0` = $05F420 + extension
word (MOVEC ea form with MMMRRR=110100). The MOVEC ddddd code is the low
5 bits of the register index.

## Arithmetic Mode Bits: DM, SA, SC (silicon-probed)

Probes `tools/difftest/out/probe_dm`, `probe_sa`, `probe_sc` — isolated
boots on MCPX GP silicon, zero wedges. All three modes are still
unimplemented in the emulator; these are the characterization results.

### DM — Double-Precision Multiply mode (SR bit 14)

The four-op algorithm (Figure 3-8) semantics are **exactly pinned**
(operands x1:x0 = MSP1:LSP1, y1:y0 = MSP2:LSP2; verified bit-exact
against the true 96-bit product `2*X*Y`):

| Op (in algorithm order) | DM-mode behavior |
|---|---|
| `mpy y0,x0,a` | `A = uu(y0,x0) << 1` (both operands UNSIGNED) |
| `mac x1,y0,a` | `A = (A asr 24) + (su(x1,y0) << 1)` |
| `mac x0,y1,a` | `A = A + (us(x0,y1) << 1)` (no shift) |
| `mac y1,x1,a` | `A = (A asr 24) + (ss(y1,x1) << 1)` |

Signedness rule: MSP registers (x1, y1) are signed, LSP registers
(x0, y0) unsigned. The 24-bit arithmetic right shift of A is built
into the two "shifted(a)" macs (stages 2 and 4) — it does NOT depend
on the algorithm's parallel `a0` stores (probe used separate stores).
Final A holds bits 95-48 of the 96-bit product: `(2*X*Y) >> 48`.

Off-algorithm combo under DM: `mpy y1,y0,a` left A **completely
unchanged** — no signedness combination (uu/ss/su/us) matches, the
write to A simply did not happen. One data point; the manual calls
this UB ("do not use"). Plain moves of a2/a1/a0 are unaffected by DM.

Mode entry/exit: `ori #$40,mr` / `andi #$bf,mr` + 2 nops, proven.

### SA — Sixteen-bit Arithmetic mode (SR bit 17)

Everything probed matches manual §3.4 (move placement Tables 3-3/3-4,
16-bit add with 8-bit EXT, C flag, `mpy` 16×16<<1 into
`b1[23:8]:b0[23:8]` — `$3456²·2 = $156619c8` exact, `asl` as 32-bit
double, `rnd` clearing the 16-bit LSP at the b0[23] rounding position)
**except one deviation**: on reads to the bus, **bits 23-16 are ZEROS
where the manual claims sign extension**:

- A2/B2 → bus: a2=$ff stores `$00ffff` (sign-extends bit 7 only
  through bit 15; manual's "next 16 bits are sign extension" would
  give `$ffffff`).
- Full accumulator → bus (single store): negative A stored `$00fe00`,
  not `$fffe00` (manual: "sign extension placed in eight MSBs").

Uniform silicon rule: every SA-mode read to the bus has a zero top
byte. Register residue after mode exit confirms writes land at SA
placement in the register itself (`move #>$123456,x0` under SA leaves
x0=$345600, readable as such in normal mode). S flag observed sticky
on acc-to-bus stores as in normal mode. Entry `movec #>$c20300,sr`,
exit `movec #>$c00300,sr`, 2 nops each, proven.

### SC — Sixteen-bit Compatibility mode (SR bit 13)

All probed behavior matches manual §4.2 / SR bit description:

- AGU register **write** clears the 8 MSBs **in the register** (r0
  written $aabbcc under SC reads back $00bbcc after mode exit); a
  **read** clears only the bus copy (r1 seeded $aabbcc before SC
  stores $00bbcc but still holds $aabbcc afterward).
- Address-calc results are MSB-cleared: `lua (r2)+` with r2=$123456
  gives $003457.
- N high byte ignored in calc: `lua (r3)+n3` with n3=$818000,
  r3=$000100 gives $008100. Note the "N sign bit is 15" rule is
  unobservable in the plain-update result: signed and unsigned
  readings converge after the MSB clear (only modulo wrap direction
  could distinguish; unprobed).
- PCU moves: `movec #imm,lc` clears MSBs in the register
  ($fedcba→$00dcba persists); `movec la,x:` puts $00cdef on the bus
  while LA keeps $abcdef; `movec sr,x:` under SC reads $002300 (top
  byte, CP bits included, cleared on the read).

Mode entry/exit: `ori #$20,mr` / `andi #$df,mr` + 3 nops, proven.

## Nested Core Faults: Window Truncation and Parked Delivery (silicon-probed)

Probes `probe_ill_in_shadow` / `_pad` / `_noclean` / `_noaccess` /
`_earlywrite` plus fresh-fault controls `probe_ill_after_spread` /
`_spwrite` (ILLEGAL planted in a stack-error shadow window; VBA
redirect, distinct long-vector handlers for SE and ILLEGAL, jmp-out,
residual slot-1 frame read back). All seven match the emulator in both
difftest modes after the fixes below.

**Window truncation.** A fault that arms inside another core fault's
shadow window OVERWRITES the outer fault's stream budget: an ILLEGAL
(budget 0) as shadow word 1 annuls every following shadow word, and
the outer stack error delivers at the ILLEGAL's boundary. The
emulator's unconditional `fault_budget` store is exactly this behavior;
guarding it breaks the probes.

**Parked delivery.** The nested fault stays pending through the outer
fault's dispatch and handler ("parked") and delivers later:

- Base: **6 instruction completions after the outer fault's
  dispatch** (the outer vector's jsr + 5 handler-stream completions
  in the probes).
- Any guest **SP WRITE** (movec class) during the countdown defers
  delivery to **at least 3 completions after the writing
  instruction**. SP *reads* and SSL/SSH accesses do not defer. The
  fitted rule `max(dispatch+6, last_sp_write+3)` reproduces all five
  timelines exactly.
- A **fresh** fault (nothing else in flight) is unaffected: ILLEGAL
  immediately after an SP read or write still delivers with its
  normal zero budget (control probes matched pre-fix).
- Delivery itself is immediate at countdown zero: the saved PC is the
  next unexecuted instruction, no additional shadow words; the frame
  pushes onto the stack as-is (a still-poisoned SP takes the frame
  with SE|UF intact — probe_ill_shadow_noclean recorded sp=$31 in the
  ILLEGAL handler).

Emulator model: `InterruptState::Parked` + `parked_countdown`, seeded
with 3 at the pipeline stage-0 completion arm (which runs on the 4th
completion after delivery), decremented per completion, bumped to
`max(cd, 3)` after a completion whose instruction wrote SP
(`parked_sp_write` flag set in `jit_write_sp`), delivering via
`deliver_parked_fault` at zero. Unprobed corners, documented:
delivery order among multiple parked faults follows arbitration index
order; a parked budget-bearing fault (stack error) delivers with no
window; faults arming during pipeline stages 3-0 (rather than in the
shadow window) also park.
