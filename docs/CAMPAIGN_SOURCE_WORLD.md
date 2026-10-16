# Campaign source world

`PhysicsLiveWorldTu15` owns one retained world: tags, objects, actors, command
scripts, HS threads, physics, clock and their resource owners share that bank.
Campaign integration must borrow this world rather than allocate a second actor
or object authority.

Initialize the map, then call `configure_campaign_session` with the actual
difficulty, simulation mode, tick rate and random seed. The world invokes the
original HS storage and map lifecycle before AI initialization. Player creation,
machine identity, command delivery and view selection remain explicit native
inputs; initializing a world does not fabricate them.

Use `place_ai(reference)` for authored placement or `place_ai(reference, count)`
for the signed-short overload. References are the original compiled HS AI values,
including packed group and starting-location identities. The original actions
own difficulty counts, authored starts, object creation and actor/script binding.

The overloads intentionally differ. The default overload delegates every value,
including NONE, to the original reference action. The counted overload skips
NONE; it places only plain references with a positive count, then records every
non-NONE pending reference, including packed references and nonpositive counts.
An unavailable reached owner throws before later writes. Callers must not
normalize these distinctions or interpret a pending reference as actor creation.

`CampaignSourceRuntime::place_ai` forwards those same compiled references and
optional signed-short counts, then republishes the existing source world to
Simulation. Simulation admits both original `ai_place` signatures before its
older squad-only host path. It does not strip packed bits, filter NONE, or
replace a nonpositive count with an authored default.

The 2026-10-15 bridge compiles against a coherent captured engine cohort and
the regenerated 256-case original wrapper fixture matches the existing bytes.
The campaign goal has since resumed. Immutable interaction-clip sharing and
earlier release of the copied tag snapshot reduce setup retention. The bounded
Cairo probe now completes source-world and Simulation construction, including
actual player and biped birth. Five noncreating `ai_place` overloads pass actual
source ticks. Counted actor creation and equipment adoption now also pass in
the coherent engine build. The original E558D -> 2039B0 leaf reached by authored
packed starting-location placement is now recovered and passes 108 complete
original/native component comparisons. Its coherent rebuild passes. The next
query fixture reaches missing original callback 1B0195 -> 1C9C90 after its NONE
getters and five noncreating placements pass; it stops before non-NONE getter
validation. The authenticated callback is now in the canonical module and passes
432 original/native component comparisons under the serial memory guard. The
next integration proceeds beyond it and reaches missing original selector
1B0128 -> 1BFE00 before counted-birth completion or non-NONE getter assertions.
The current 256-case overload
differential and default campaign regressions pass; full bridge runtime
validation and campaign gameplay remain unverified. The
[implementation handoff](../analysis/tags/campaign-ai-bridge-20261015/RESULTS.md)
retains the original pins and failures; the
[memory continuation](../analysis/tags/campaign-memory-20261015/RESULTS.md)
records the newer checks without treating asset parity as mission completion.

`source_ai_placement` is the small internal overload seam. The world supplies
its actual actions and pending-reference field; differential tests compare this
same seam with unmodified original wrapper execution. The generated
`source_campaign_runtime_tu15` module retains 55 complete original routine
bodies. Shared AI-frame helpers keep their existing owner. Regenerate with
`tools/generate_campaign_runtime_tu15.py`; the generator rejects duplicate owners
and authenticates the original executable before lowering instructions.

Checkpoint publication replaces the bank's backing. Borrowed `DatumPool` views
must stage replacement bindings with `prepare_source_view`, then commit after
`commit_authority`. Do not keep old actor, command, thread or player spans across
publication. The complete authority transaction retains actual script links;
reconstructing equivalent-looking actor records is not restoration.

Validation is incremental. The public Cairo fixtures pass counted, authored and
packed starting-location placement, actual actor/command/thread binding, whole
authority restoration, and bounded original AI/object ticks. The counted fixture
passes three later ticks. The controlled original overload comparison covers 256
cases. These checks do not establish every campaign mission, frontend integration,
audio, rendering or Xbox parity. The full campaign goal remains open until the
actual mission, BSP and checkpoint paths pass.

The subsequent [physical request increment](CAMPAIGN_PHYSICAL_REQUESTS_20261015.md)
reuses the original allocation/protection wrappers for unrestricted SDK requests
and the reached write-combined pages. Native page ownership survives startup
worker retirement. The complete original physical-stage comparison now streams
its expected arena bytes and passes under the serial simulation memory cap.
This increment does not complete renderer startup or the campaign HS frame.
