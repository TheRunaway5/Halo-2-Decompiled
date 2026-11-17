# Actual TU1.5 geometry leaves: private qualification

These two childless owners remain translated in the generated World/Contact
routes. No new default or public CPP delegation is installed.

| Canonical World/Contact owner | Actual TU1.5 owner | Sites | Shared value behavior |
| --- | --- | ---: | --- |
| `2E0CC0` | `2E47B0` | 35 | Transposed direction, with Z repeated in lane W |
| `2E0BA0` | `2E4690` | 26 | Position associated as X+Y+(Z+translation) |

`native_tu15_geometry_leaf_kernel.hpp` extracts the existing public geometry
algorithms into a lazy typed storage policy. The private raw Frame owns the
original caller stack, flags, full XMM publication and I/O phases. It preserves
Position's intermediate Z/Y SIMD state before the next matrix-column read,
Direction's early stack alignment, Position's late stack alignment and the
saved-BP/RET8 aliases. Model independently reviewed these phases against both
complete source owners without finding a defect.

The physical authority is unchanged TU1.5 image
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
`native-geometry-leaf-mutations-private-20261110/report.json` records 476
physical paths, all 61 sites, and 952 comparisons/9,696 complete pre-I/O
packets per GCC/Clang sanitizer compiler. It includes finite and exceptional
values, paired NaNs, aliases, rounding/DAZ/FTZ controls, alignment faults and
read/write/completed-observer prefixes. Four compiled defects are rejected
per compiler: wrong transposed axis, reassociated translation, missing
intermediate SIMD publication and the wrong source formal.

The raw proof explicitly uses ordinary atomic 16-byte vector backing. It does
not qualify the abstract four-lane provider, retained split-store transport,
arbitrary GP/XMM-mutating providers, pure-CPU Before stops or other images.
Physical alignment-fault saved flags remain distinct from execution flags.

`native-geometry-leaf-public-value-private-20261110/report.json` compares the
complete existing public CPP and a private two-body shared-kernel facade.
Both variants pass 758 unchanged-original value/FP/ordered-I/O/bank cases per
GCC/Clang sanitizer compiler. The unchanged 40-site affine-compose owner is
included as a preservation control. All unrelated CPP bytes, the public FP
environment and storage adapter remain unchanged in that overlay. This value
proof does not establish hostile-provider FP mutation behavior or install the
overlay.

Remaining work is source-faithful transport/admission, exact raw/prepared
owner/resolver/dispatcher authentication, actual default/reference and
continuation controls, public integration, fresh full generation and registered
acceptance. The neighboring actual `303BA0` 29-site record copy and `30BAD0`
two-site invalid-contact sentinel have a separate private typed DATA/raw Frame
proof at `native-contact-copy-leaf-full-private-20261110/report.json`: 168
physical paths, 336 comparisons and 7,294 complete pre-I/O packets per GCC/Clang
sanitizer compiler. Their padding, narrow-register upper bits and RET4/RET0
transport are included. Model's independent review is clean for readonly
GP/XMM/policy and atomic-vector backing. Their existing public DATA functions
remain unchanged and their generated routes remain translated.

The private copy/sentinel source adapter has a separate comparison against
the two complete source bodies extracted from both actual World and Contact
units. `native-contact-copy-leaf-source-r3-private-20261110/report.json`
pins 45 inputs, including both State definitions, and the generated reference
include. Each of GCC and Clang under sanitizers passed 2,130 cases, 108,185
pre-event packets and 732 explicit provider, observer or callback stops.
Four compiled defects per compiler caught incorrect direct-callback elision,
an atomic store in place of the source's two writes, stale SIMD data after a
callback and a lost selected-image World PC. Contact's prepared source uses
literal cursor operations; World still delivers its selected or canonical PCs
directly even after operand specialization. The adapter calls each route's
actual vector provider and split-store helper, including live callback and
observer changes. This is an extracted source comparison, with no installed
default, whole-World producer admission, other-image or physical MOVAPS claim.
No result here closes the surrounding `304350` or `30E930` parents.

All completed compiler/reference owners were removed. Only bounded reports,
commands, source hashes and diagnostics are retained; original images/assets
and the launcher-selected runtime are unchanged.
