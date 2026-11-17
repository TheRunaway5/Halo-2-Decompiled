# TU1.5 codec row copy owners

The original table at `48C47C` points to `2963D0`, `296410` and `2CEC60`.
Primary and ModelRender contain the quaternion and translation copies.
ModelRender alone contains the scale copy. These three childless routines have
18, 17 and 10 original instructions respectively. The next table triple
(`296240`, `2962F0`, `296620`) owns interpolation and is qualified separately.

The typed row views retain the factory and row index reads, wrapped arithmetic,
streamed forward aliases and original return read. Quaternion captures its
output before the row reads. Translation reads its first source word before
the factory addition and late output pointer. Scale also reads its source
before acquiring the destination and writes the dword at output + `1C`.

Both sanitizer compilers passed 1,066 unchanged-original paths for the pair
and 508 for scale. Each compiler compared 4,264 pair and 2,032 scale transfers,
including 53,704 and 17,400 complete original pre-I/O packets and four
compiled defects per test. The tests exercise dense x87 and all 256 opaque
MMX masks. Raw callbacks use the declared readonly GP/XMM and Before policy
with isolated incidental host FP effects.

The direct retained view passed 884 comparisons per compiler against the five
unchanged source copies, including 560 provider feedback comparisons, 186
provider prefixes and all 58 source-copy retirement boundaries. The tagged
source smoke passed 29,808 comparisons and 480 untouched refusals per compiler
across eight complete producer contexts and 20 source routes. The own-reference
run executed 145,488 retained statements and no native calls. The generated
body, resolver, dispatcher and LiveState tuples are pinned together. The old
ModelRender dispatcher uses selected-address branches; fresh generation uses
direct numeric cases. Both resolve to the same authenticated TU1.5 owners.

The binder rejects a direct bridge or continuation label outside its exact
owner. The accepted-before counterexample remains a separate receipt. Full
Clang validation of the installed proposal passed 406,128 source comparisons
over default, own-reference and unrelated-control modes. GCC passed 135,376
comparisons in each independently completed control mode. Its default
comparator also passed 135,376 before the combined wrapper's finite timeout;
that historical wrapper is not counted as terminal acceptance. Both compilers
passed the complete bounded guard and seven compiled defect tests.

The exact prefixes and generator hooks are installed in Primary and
ModelRender. Reversing the six-file binding recovers every prior byte,
including the newer Device and Hull neighbors. The guarded specializer
authenticates each complete incoming group before retagging prepared entries.
Raw callbacks use the retained body; callback-free raw and qualified prepared
routes can enter the native view.

The final frozen installed cohort passed all five registered CMake tests,
with no skips. The bounded GCC source check passed 31,728 comparisons across
all 20 routes and three modes, 480 untouched refusals and seven compiled
defects. Its own-reference run executed 155,728 retained statements and no
native calls. Independent installed Clang passed the full 135,376-comparison
default matrix and both physical gates. The exact catalog, copied pure
helper and specialization checks passed too. All owned scratch was removed.

The retained view uses the authored source write/observer protocol and resumes
the exact successor after completed I/O when its live policy changes. These
checks leave arbitrary FP metadata/control mutation, signals, MMIO, partial
writes, other images and the complete `2835B0` parent outside this scope.
