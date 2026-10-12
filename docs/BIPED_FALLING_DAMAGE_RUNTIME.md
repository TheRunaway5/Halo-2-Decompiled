# Original falling and moving-support runtime

2026-10-11. This increment compiles complete paired original bodies for falling
admission (`E4D30→E5C50`), airborne action naming (`E5700→E6620`), support motion
(`10DF90→10EEB0`), current object position (`BACC0→BBE00`), request initialization
(`D7C60→D8BB0`) and attribution (`D7CD0→D8C20`). The 557 original
instructions become native arithmetic, explicit branches and static calls. No
original code execution or instruction decoder runs in production.
[Body generator](../tools/generate_biped_falling_damage_runtime.py),
[generated manifest](../analysis/damage-shields-goal-20261011/falling-producer/generated/manifest.json),
[retail producer handoff](FALLING_DAMAGE_PRODUCER_HANDOFF.md),
[support and point correspondence](../analysis/damage-shields-goal-20261011/falling-producer/support-motion/final-manifest.json).

The paired raw API retains the complete register, SSE, flags, x87, floating-point
control and source memory ABI. The TU1.1 joined API executes existing native
`BB0C0` velocity, `DEF10` buoyancy and animation/vector owners when reached, on
the same caller-owned bank. `source_call_biped_runtime_1_1` now routes the two
source-defined falling calls exposed by the biped prephysics phase. Source
player notification, current-action lookup, complete damage delivery and
retirement services remain required at their actual call frames.
[Interface](../include/halo2/biped_falling_damage.hpp),
[native joins](../src/biped_falling_damage.cpp),
[phase adapter](../src/biped_phase_runtime.cpp),
[prephysics callers](../src/biped_prephysics.cpp).

`E4D30` retains options, object/definition immunity, depletion, parent type and
parent definition gates. Its comparison is the original ordered strict test
against `0 − globals+5C`; equality and unordered NaN return. The accepted request
uses `globals+28`, which the owned Lockout cache identifies as
`globals\distance` (`EAE90964`). It does not read the schema-labelled harmful
fall-height fields. The source flag store precedes the actual player or damage
request owner. Kill-volume admission remains a distinct `DEFC0` path.
[Original and cache authority](../analysis/damage-shields-goal-20261011/falling-producer/manifest.json).

`10DF90` follows the real parent chain, tests root+C0 bits1/2 and reads the
support bank only on the admitted branch. It scans the actual signed count and
full datum IDs, checks the optional definition filter and current sample E5,
and calculates angular cross-product plus linear velocity in original SSE
order. Later matching samples overwrite earlier output. No match leaves the
output untouched. The point owner uses object+64 directly or the actual parent
node matrix, including its conditional scale and translation. No stationary
support, fabricated point or successful lookup is returned.
[Complete resolver and lifecycle evidence](../analysis/damage-shields-goal-20261011/falling-producer/support-motion/HANDOFF.md).

`D7C60` clears exactly0x88 request bytes, then performs the original ordered
datum, invalid-identity, current-BSP, scalar-one and flag stores. The live BSP
word is read after zeroing; the static image default cannot substitute for it.
`D7CD0` follows the original redirected source object, chooses current unit or
saved object attribution, and writes DWORD+4, DWORD+0, WORD+8 in that order,
preserving the remaining two padding bytes. A NONE source copies the original
pointed twelve-byte literal. There is no attribution-success callback.
[Request leaf correspondence](../analysis/damage-shields-goal-20261011/falling-producer/damage-request-owners/pairing/report.json).

## Original diagnostic evidence

[Measurement tool](../tools/measure_biped_falling_damage_runtime.py) executes
both unchanged owned executables on explicitly declared standalone input banks.
The authored Spartan/Elite definitions and global damage block are verified
against the complete Lockout cache hash; every controlled field overlay is
listed per case. CPU, velocity, object, hierarchy, node matrix and support banks
are diagnostic inputs. Original `BB0C0`, `10DF90` and `BACC0` actually execute;
other reached callees stop before their first instruction with the full pending
ABI and 32-byte stack snapshot. No native expectations, callee returns, damage
amounts or instruction patches are supplied.

Each image has 137 cases: 112 return and 25 stop at the actual next owner. Each
executes 22,390 original instructions with 4,618 ordered stores. Coverage includes
both authored definitions, velocity values immediately around the exact
threshold, signed zero, infinities, NaN, mode/options/immunity/depletion and
parent gates; negative, empty and 33-row support counts; duplicate, inactive and
full-datum mismatch rows; definition filters; attached points and node scales;
airborne action naming; request extent/guard bytes and varying BSP/tag inputs;
and all attribution branches and untouched padding. Every instruction in the
support, point, request initialization and attribution bodies executes in this
suite. Falling requests continue through both real leaves to `D9180` with their
actual full request and pending frame. The complete damage delivery and tumble RNG/trig
branch remain outside this dynamic proof. Both complete bodies are compiled,
but compiling a branch does not establish its live admission.
[Original captures](../analysis/damage-shields-goal-20261011/falling-producer/original-measured-v4/report.json).

The repeated original fixture files/report and repeated generated C++/manifest
are byte-identical. Regenerating the existing complete process-heap/startup
module through the extended shared lowerer also produces the identical existing
C++ SHA256 `9561e7c59e8c7ceedaec6db662700b92bb6ae88800021dd93bd6bc1eecdc4c61`.
[Repeated capture](../analysis/damage-shields-goal-20261011/falling-producer/original-with-requests-repeated/report.json),
[repeated generation](../analysis/damage-shields-goal-20261011/falling-producer/generated-with-requests-repeated/manifest.json),
[existing generator regression](../analysis/damage-shields-goal-20261011/falling-producer/heap-generator-regression/manifest.json).

The native comparisons pass for137 TU1.1 cases,137 TU1.5 cases and136 TU1.1
joined cases (the separately captured buoyancy owner is excluded from the raw
paired stop-boundary join). They match67,083 instruction visits,13,839 ordered
stores and688,498 retained-state bytes, plus8 defined-input biped phase calls
and24 absent-owner prefixes. Six invalid logical ABI forms fail before memory
in each comparison. Floating-point controls are checked; this does not claim
Unicorn proves hardware exception-status or the unexecuted tumble trig branch.

All24 focused/regression checks pass without skips, and the full native build
and CLI help check pass. The existing SMG integration fixture initially exposed
the newly implemented positive player+194 spawn-protection multiplier0.1. Its
unprotected original references now run after30 real production countdown ticks;
no production damage scaling was changed. All6 local/UDP hit and7-tick feedback
trajectories pass. This preserves the distinction between supplied-contact SMG
arithmetic and naturally admitted gameplay.
[Player damage contract](MP_PLAYER_DAMAGE_NATIVE_BINDING.md),
[corrected fixture](../tests/smg_damage_tests.cpp),
[final checks](../analysis/damage-shields-goal-20261011/falling-producer/delivery-tests.xml),
[validated source and report](../analysis/damage-shields-goal-20261011/falling-producer/validation.json).

The report pins45 stable core implementation/dependency inputs, the actual core
object dependencies and compile commands, the original/generated repeats, and
the bounded frozen source. Other chats are editing this shared checkout. The
observed configuration and full build do not assert a stable entire checkout or
equivalence of every concurrently edited compiled header.

## Remaining owners

The ordinary `Simulation::move_unit` path still uses its native body/collision
state. These diagnostic banks do not establish an admitted live airborne biped,
support attribution, persistent support registration/sample lifecycle, or the
complete damage/request/health/effect pipeline. `10D490/10D4F0/10D520/10D590/10D7F0`
and the support query have separate retained original evidence; they must use
real arena, object-header, tag, component, rigid-body and current-pose owners.
The inspected earlier prephysics suites contain no falling calls and cannot be
relabelled as falling proof. Shield vitality, hit shimmer and rendered damage
effects remain required by the overall active goal.
[Capture census and lifecycle](../analysis/damage-shields-goal-20261011/falling-producer/support-motion/HANDOFF.md),
[live movement](../src/simulation.cpp),
[coverage goal](DAMAGE_SHIELD_COVERAGE_GOAL.md).
