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
