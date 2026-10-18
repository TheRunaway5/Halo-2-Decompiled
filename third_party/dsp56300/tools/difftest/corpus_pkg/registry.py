"""Case registry.

Rules for cases:
  - initialize every register you read (prologue only clears CCR)
  - keep memory operands in X:$0000-$0FFF / Y:$0000-$07FF, unique per case
    (but X:$0400-$04FF is reserved for the hardware runner's register
    snapshot area - see run_corpus_hw.py in the xbtest repo - and GP
    X:$0C00-$0FFF is the sound-engine mixbuffer, rewritten every frame
    while the SE runs)
  - no peripheral ($FFFF80+) accesses: sim peripherals differ from ours
  - branch targets stay inside the case; label names unique per case
  - leave the stack balanced
  - M-register writes use movec spellings (the historical plain-move
    truncation bug is fixed - plain spellings now emit the same MOVEC
    words - but bank 0's image is golden-bound, so its staged form stays)
  - movem aa-form can only address P:$00-$3F (below the corpus; on
    hardware, inside the runner's already-executed preamble). Write
    before reading so content is deterministic on both engines - the
    p:$07C0+ scratch rule protects live image/snapshot code, which
    this zone is not
  - in a bank with Bank.fill set (dirty-state banks, see banks.py and
    cases_dirty.py), reading X/Y cells you never wrote is deterministic
    (the affine fill) and encouraged; everywhere else it reads zero.
    Fill values are full 24-bit words - mask anything used as a loop
    count or address offset so it stays bounded and in-window
  - never read SSH/SSL (movec or bit-test) in the instruction
    immediately after a direct SP write (movec #d,sp or a bit op on
    SP): silicon refreshes the top-of-stack views one instruction
    late, so the read returns the stale pre-write view (and a
    shadowed SSH pop loses its SP decrement). Put a nop (or any
    instruction) between; the JIT models only the settled views
    (probe matrix in ARCHITECTURE-NOTES)
  - inside a DO body: no direct SP writes and no SSH writes of any
    kind unless the stack is rebalanced before the LA fall-through
    (loop-back reads the SSH view; unbalanced shapes wedge silicon -
    balanced JSR/RTS pairs are fine)
  - fault cases (cases_faults.py, banks with a vector-page aux blob):
    trigger stack errors only with the silicon-pinned shapes, keep the
    delivery shadow window free of SSH/SP writes (branches are OK -
    the stream-word budget model follows them - but branch-class
    faults must flag-guard their re-entry paths, since RTI re-executes
    the annulled instruction), and end the case with the cleanup tail
    (SP, SC, VBA, and SR if touched) so the next case starts clean
"""

from . import (cases_agu, cases_alu, cases_bits, cases_dirty, cases_faults,
               cases_flow, cases_logic_shift, cases_loops, cases_moves,
               cases_multiply, cases_pmove, cases_srmodes, cases_stack,
               cases_valsweeps)

THEME_MODULES = [cases_agu, cases_alu, cases_bits, cases_dirty,
                 cases_faults, cases_flow, cases_logic_shift, cases_loops,
                 cases_moves, cases_multiply, cases_pmove, cases_srmodes,
                 cases_stack, cases_valsweeps]

# name -> (body, max_steps, theme)
REGISTRY = {}
for mod in THEME_MODULES:
    for name, body, max_steps in mod.CASES:
        assert name not in REGISTRY, "duplicate case name: %s" % name
        REGISTRY[name] = (body, max_steps, mod.THEME)


def resolve(name):
    """Look up a case; sweep cases are generated lazily by gen.py."""
    return REGISTRY[name]
