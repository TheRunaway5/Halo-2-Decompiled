# Native TU1.5 authored jump-link feasibility

`src/campaign/ai/native_campaign_ai_jump_arc_tu15.cpp` implements the actual
278E60 jump-link predicate and the FF1A0 mathematical profile that it calls.
The original caller fixes gravityScale=1, highArc=true and all optional output
pointers absent. The semantic API makes this contract explicit and returns
private direction data as a native value. It does not admit arbitrary FF1A0
callers with different options. No numeric/live route is added by this module.

The predicate reads the actual signed jump kind, scenario block/count and
per-kind speed/rise fields. It preserves both primary/secondary endpoint gates,
the repeated height/config reads, ordered maximum selection, zero/NaN threshold
behavior and short-circuiting between the two actual arc computations. Missing
or invalid kinds retain source behavior; no new navigation policy is supplied.
Lazy point interfaces support real retained fields or native projected vectors,
so subsequent expansion can pass private geometry without a guest stack.

The solver uses the actual gravity binding52BE68 and computes horizontal
length, minimum launch speed and the quadratic high-arc flight time. It retains
all original REAL/extended rounding boundaries, operand priority, discriminant
and time validity comparisons, minimum-trajectory selection and normalization
fallbacks. Scalar SSE and real x87 own arithmetic and status. Optional-output
arithmetic that still affects FP is executed even though its pointer is absent.
The straight-trajectory branch calls the proved FF580 mathematical owner; the
normalizations call the shared native30BD0 value owner. No captured trajectory,
pathability result, CPU/register interpreter or lifted callee is supplied.

Native arithmetic scopes marshal FP control/status between actual semantic
owners, preserving sticky flags and source TOP. A rejected source read carries
its real pending mathematical quantity: horizontal squared distance/distance,
discriminant root, flight-time squared or launch magnitude. The half-gravity
read can owe both distinct horizontal velocity squares; these have named X/Y
fields. The typed error retains exact80-bit values and the actual failure cause
while preserving pre-fault state. It neither hides a discarded quantity nor
creates a virtual FP stack. Unrelated live host FP state is restored.

`tools/oracle_native_campaign_ai_jump_arc_tu15.py` executes unchanged physical
i386 English TU1.5, image SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The410 cases execute440 source sites and69,187 original instructions, with
8,001 ordered reads,124 semantic fault prefixes, two actual unmapped operands
and48 pending-quantity boundaries. The284 completed cases also run through
native private point inputs. Return booleans, private directions, FP, source
reads, retained bytes, caller values and precise failure quantities match.
No original instruction, dependency outcome or resumed failure is supplied.

Three separately labeled adversarial data-constant cases exercise otherwise
unreachable zero-magnitude/default-direction and negative-time branches. They
are not production constant changes. The48 uncovered sites are exactly45
FF1A0 branches for optional outputs/alternate speed inputs absent from the
actual caller, plus three FF580 optional stores absent from this profile.
Those three stores have independent complete FF580 proof. Other FF1A0 option
contracts remain open; this is not an entire FF1A0 generic-entry qualification.

The inherited original vector profile allows0–3 live caller x87 values,
leaving five temporary slots. An original-only five-live-value probe caused
real original30BD0 stack overflow and caller corruption; the bounded receipt
is `analysis/campaign-native-ai-jump-arc-20261028/pressure-boundary.json`.
That higher-pressure/unmasked behavior is not sanitized, emulated or admitted.
Ordinary callers must establish this contract before any future numeric route.

GCC and ASan/UBSan pass1/1 each. Clang passes8/8 with seven actual-source
mutations rejected: low-arc sign, ignored authored rise, missing pending Y
square, prematurely rounded flight time, an early gravity read, a cached
quarter constant and a minimum secondary rise. Independent original repeat
bytes are identical. Exact current pins/commands and verified cleanup are in
`analysis/campaign-native-ai-jump-arc-20261028/proof.json`.

The required fixture is
`tests/fixtures/native_campaign_ai_jump_arc_tu15.json.gz`,91,909 bytes, SHA-256
`6a5992e310dedf2f594aac70ab749c5abb90d7db40c4a1b90c1e25f086f411df`.
All owned original ELF, raw banks and compiler/build files were removed. Only
the required reference and bounded reports remain.
`cmake/CampaignNativeAiJumpArc.cmake` adds the native source and focused test;
production also requires the existing fallback/vector modules. Actual gravity
storage is consumed without a new mount or successful initializer substitute.
Full selector/expansion/search/update, other arc callers, startup ownership and
generic native campaign acceptance remain separate open work.
