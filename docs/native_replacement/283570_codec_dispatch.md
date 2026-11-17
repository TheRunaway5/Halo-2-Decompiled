# TU1.5 codec dispatch candidates

`283570` loads the format selector at `510F30` and tail dispatches to one of
five field selectors. Those routines (`2840D0`, `284140`, `2841B0`, `284220`,
`284290`) share one control shape with distinct tail targets. The audit matches
182 original instructions and 50 genuine tail targets against nine complete
source copies in Primary, AI and CombatAction.

The shared selector reads the field kind lazily. Kind 0 or 1 tests the wide
field before the late `510F80` byte read, replaces only AL, then tests AL to
choose a tail. Kind 2 selects its own tail; other values leave the final CMP
flags and execute the actual return read. Tail selection keeps the live caller
stack. The private proof stops before descendant instructions and supplies no
child result.

The expanded unchanged-i386 proof passed 1,040 paths per sanitizer compiler:
702 dense inputs, 256 opaque MMX masks and 82 read or before-jump prefixes.
It includes 447 composed outer-to-field paths. Both compilers passed 4,060
comparisons, 14,558 complete packets and five compiled defects. All owned
source, builds and banks were removed. An earlier expanded fixture accidentally
set the pending x87 exception-summary bit; the integer FP admission correctly
refused that unsupported input. That receipt is retained separately.

The private retained view preserves the authored source read and RET helpers
and can stop at the next instruction after a completed provider read. Both
sanitizer compilers passed 395 comparisons against the nine complete retained
bodies, including 221 provider feedback/exception cases, 46 read prefixes,
37 copy-specific retirement checks and 185 genuine tail stops. The test-only
first compile failure and its removed owner remain recorded separately. No
source/default binding, descendant owner closure, other image or engine result
is claimed.
