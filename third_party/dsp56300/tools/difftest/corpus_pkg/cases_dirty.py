"""Dirty-state cases: run under a deterministic affine memory fill.

These cases only make sense in a bank whose Bank.fill is set (see
banks.py): the harness pre-fills X:$0000-$0BFF and Y:$0000-$07FF with
`(K*addr + C) & $FFFFFF` (per-space K/C) and seeds the 15 hardware
stack slots with a ramp, instead of the historical zero-fill. Reads of
never-written cells are then deterministic AND address-unique, so a
wrong-address/wrong-space read or write diverges instantly (against a
zero fill, wrong reads of zeroed cells are self-consistent).

Case bodies deliberately read memory they never wrote - the exact
pattern the zero-fill era could not check. Everything they execute is
silicon-proven elsewhere in the corpus; only the VALUES are new.
"""

THEME = "dirty"

# (name, [body lines], max_steps)
CASES = [
    # Pin the fill itself: direct reads across the X window, including
    # both edges. Self-contained (safe under --cases on hardware).
    ("dirty_probe_x", [
        "move x:$0600,x0", "move x:$0601,x1",
        "move x:$0bff,y0", "move x:$0005,y1"], 100),
    # Same for the Y window.
    ("dirty_probe_y", [
        "move y:$0700,x0", "move y:$0701,x1",
        "move y:$07ff,y0", "move y:$0009,y1"], 100),
    # Pin the stack-slot ramp: point SP at slot 3 and pop the seeded
    # SSH/SSL pairs back down to an empty stack (ends balanced at
    # sp=0). The nop after the SP write is load-bearing: a direct SP
    # write refreshes the silicon's SSH/SSL top-of-stack views with
    # one instruction of latency, so a stack read in the immediately
    # following instruction returns the stale pre-write view (probed,
    # see ARCHITECTURE-NOTES; the JIT models only the
    # settled views, and the authoring rules ban the shadow shape).
    ("dirty_stack_ramp", [
        "movec #$3,sp", "nop",
        "movec ssl,x0", "movec ssh,x1",
        "movec ssl,y0", "movec ssh,y1",
        "movec ssl,r2", "movec ssh,r3"], 100),
    # Read-modify-write on dirty cells: bit ops fold the fill value into
    # both the stored word and the carry flag; add consumes a dirty read.
    ("dirty_rmw", [
        "bset #3,x:$0620", "bchg #21,y:$0320",
        "clr a", "move x:$0621,x0", "add x0,a"], 100),
    # Modulo walk that reads five dirty cells and wraps (base $0628 is
    # 8-aligned for the mod-5 buffer).
    ("dirty_mod_wrap", [
        "movec #$4,m0", "move #$0628,r0",
        "move x:(r0)+,x0", "move x:(r0)+,x1",
        "move x:(r0)+,y0", "move x:(r0)+,y1",
        "move x:(r0)+,x0", "movec #>$ffffff,m0"], 100),
    # Bit-reverse walk reading dirty low-X cells (classic base-0 FFT
    # walk: addresses stay in 0..$F).
    ("dirty_bitrev_walk", [
        "movec #$0,m1", "move #$0,r1", "move #>$8,n1",
        "move x:(r1)+n1,x0", "move x:(r1)+n1,x1",
        "move x:(r1)+n1,y0", "movec #>$ffffff,m1"], 100),
    # Dual parallel reads pull dirty X and Y words in one word.
    ("dirty_pm8_read", [
        "move #$0648,r0", "move #$0348,r4",
        "move x:(r0)+,x0 y:(r4)+,y0",
        "move x:(r0)+,x1 y:(r4)+,y1"], 100),
    # Long moves read the dirty X:Y pair at one address.
    ("dirty_lmove_read", [
        "move l:$0650,a10", "move l:$0651,b10"], 100),
    # Writing zero must show up against a nonzero fill (the deviation
    # dump emits it; against a zero fill this write was invisible).
    ("dirty_write_zero", [
        "move #>$000000,x0",
        "move x0,x:$0658", "move x0,y:$0358"], 100),
    # Off-by-one detector: write one cell, read back its neighbor
    # (which must still hold fill, not the written value).
    ("dirty_partial_overwrite", [
        "move #>$123456,x0", "move x0,x:$0660",
        "move x:$0661,y0"], 100),
    # Loop count harvested from a dirty cell (masked to 0-7): the fill
    # value steers control flow, then accumulates a visible product.
    ("dirty_do_dyn", [
        "clr b", "clr a", "move x:$0668,a1",
        "move #>$000007,x0", "and x0,a",
        "move #>$111111,x1",
        "do a,_dd_end", "add x1,b", "_dd_end"], 100),
    # Control-register load from a dirty cell (LC is dump-visible and
    # inert while LF=0).
    ("dirty_movec_lc", ["movec x:$0670,lc"], 100),
    # Indexed read whose offset comes from a dirty cell (masked to the
    # sanctioned window).
    ("dirty_indexed_n", [
        "clr a", "move x:$0678,a1",
        "move #>$00003f,x0", "and x0,a",
        "move a1,n6", "move #$0520,r6",
        "move x:(r6+n6),y1"], 100),
]
