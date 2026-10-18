"""SSH/SSL/SP/SC semantics, including bit ops on SSH.

Case bodies are verbatim from the original bank lists; bank membership
and order live in banks.py. See registry.py for the authoring rules.
"""

THEME = "stack"

# (name, [body lines], max_steps)
CASES = [
    # --- authored in bank b0 ---

    # --- Stack ---
    ("stack_push_pop", ["move #>$012345,r0", "movec r0,ssh", "movec ssh,r1"], 100),
    # Unbalanced push: pins SP/SC counting (SC monitors entries in use,
    # manual 5.4.3.2). Keep last: it leaves sp=1/sc=1 behind.
    ("sc_unbalanced_push", ["move #>$054321,r2", "movec r2,ssh"], 100),
    # --- authored in bank b2 ---
    # Stack via movec
    ("stack_two_deep", ["movec #>$aaa001,ssh", "movec #>$bbb002,ssh", "movec ssl,x0", "movec sp,x1", "movec ssh,r0", "movec ssh,r1"], 100),
    ("stack_ssl_rewrite", ["movec #>$ccc003,ssh", "movec #>$123321,ssl", "movec ssl,y0", "movec ssh,r2"], 100),
    ("stack_sc_write", ["movec #>$3,sc", "movec sc,x0", "movec #>$0,sc"], 100),
    ("rep_ssh_pops", ["movec #>$000003,ssh", "clr a", "move #>$000001,x0", "rep ssh", "add x0,a", "movec sp,y0"], 200),
    # --- authored in bank b4 ---
    # Deep stack: fill all 15 slots, pop them all back
    ("stack_fifteen_deep", ["movec #$1,ssh", "movec #$2,ssh", "movec #$3,ssh", "movec #$4,ssh", "movec #$5,ssh", "movec #$6,ssh", "movec #$7,ssh", "movec #$8,ssh", "movec #$9,ssh", "movec #$a,ssh", "movec #$b,ssh", "movec #$c,ssh", "movec #$d,ssh", "movec #$e,ssh", "movec #$f,ssh", "movec sp,y0", "movec sc,y1", "movec ssh,r0", "movec ssh,r0", "movec ssh,r0", "movec ssh,r0", "movec ssh,r0", "movec ssh,r0", "movec ssh,r0", "movec ssh,r1", "movec ssh,r1", "movec ssh,r1", "movec ssh,r1", "movec ssh,r1", "movec ssh,r1", "movec ssh,r2", "movec ssh,r3"], 200),
    # --- authored in bank b6 ---
    # SSH as a bit-test operand pops the stack; exactly one push before it
    # keeps the stack balanced (sp returns to 0).
    ("btst_ssh_pop", ["movec #>$abc123,ssh", "btst #$1,ssh",
                      "movec sp,y1"], 100),
    # Bit-test-branch reads of SSH pop the stack for ALL variants (the
    # manual documents only BR*/BS*; isolated probes pinned the jump and
    # jump-subroutine forms too). Both cases stay balanced: push, popping
    # test, push, popping movec.
    ("jclr_ssh_pops", ["movec #>$abc123,ssh", "jclr #$1,ssh,lbl_jsp",
                       "nop", "lbl_jsp: movec sp,y1",
                       "movec #>$654321,ssh", "movec ssh,x1",
                       "movec sp,y0"], 100),
    ("jsset_ssh_pops", ["movec #>$abc121,ssh", "jsset #$1,ssh,lbl_jss",
                        "nop", "lbl_jss: movec sp,y1",
                        "movec #>$654321,ssh", "movec ssh,x1",
                        "movec sp,y0"], 100),
    # BSET/BCHG/BCLR on SSH rewrite the top stack slot in place without
    # touching SP. Leaves sp=1 behind - keep last in the bank.
    ("ssh_rmw_inplace", ["movec #>$abc0f1,ssh", "bset #$3,ssh",
                         "bchg #$0,ssh", "bclr #$4,ssh", "movec sp,y1",
                         "movec #>$654321,ssh", "movec ssh,x1",
                         "movec sp,y0"], 100),
    # --- authored in bank b11 ---
    # SSL bit-op semantics: after the SSH verdicts (bit-test reads pop,
    # modifiers rewrite TOS in place), SSL is the top errata-adjacent
    # suspect. Same disambiguation pattern; balanced. Probe-first.
    ("ssl_rmw_semantics", ["movec #>$abc0f1,ssh", "bchg #0,ssl", "movec sp,y1", "movec ssl,x1", "movec ssh,r0", "movec sp,y0"], 100),
    # SP bit ops: modifying SP without an SC recompute. Wedge-suspect;
    # probe-first; restores sp=0.
    ("sp_bitop_probe", ["movec #$2,sp", "bset #0,sp", "movec sp,y0", "bclr #0,sp", "bclr #1,sp", "movec sp,y1", "movec sc,r4"], 100),
    # --- authored in bank b13 ---
    # SSH pops INSIDE a REP body, SP read after the loop: regression
    # shape for the stale-promoted-register-after-helper-in-loop JIT bug
    # (block mode read the block-entry SP). Balanced: 3 pushes, 2 body
    # pops, 1 closing pop.
    ("rep_ssh_body_pops", ["movec #>$00a001,ssh", "movec #>$00a002,ssh", "movec #>$00a003,ssh", "rep #2", "movec ssh,x0", "movec sp,y0", "movec ssh,r1", "movec sp,y1"], 200),
]
