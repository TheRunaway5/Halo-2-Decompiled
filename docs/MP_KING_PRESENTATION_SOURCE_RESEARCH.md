# King presentation source contracts

Research snapshot: 2026-10-11. This note establishes original event production,
actual authored response rows and bounded original PC local audio delivery. It
does not admit King into normal startup or certify any of its five variants as
operational. The active goal still requires complete normal local/network flows
for all39 variants. Native implementations and builds are concurrent parent work
and are not validated by this research.

Primary evidence is the unchanged owned Xbox executables/cache and supplied
Vista executable/shared cache. [The existing callback research](MP_KING_NATIVE_CALLBACK_RESEARCH.md)
pins retail, TU1.1 and TU1.5 Xbox images. The new original-only
[Xbox producer generator](../tools/oracle_king_event_producers.py) and
[PC delivery generator](../tools/oracle_king_presentation.py) save their inputs,
complete outputs and instruction evidence under
[king-presentation-20261011](../analysis/multiplayer/game-variants/king-presentation-20261011/report.json).

## Original King event producers

Registered King slot6/+18 is retail`2BD260`, TU1.1`2C35F0`, TU1.5`2C7070`.
It takes one player datum on the stack and returns`ret4`. The complete original
callback builds a34-byte defined event prefix:

| Offset | Start event value |
| --- | --- |
|`00`|Category6|
|`04`|Subtype0|
|`08`|Supplied recipient player datum|
|`0C`,`10`,`14`,`18`|NONE cause-player/cause-team/effect-player/effect-team|
|`1C`|Zero|
|`20`|FFFF,16-bit|

The callback suppresses delivery in world mode4. Other tested modes execute the
actual network-submission guard followed by local delivery. The upstream caller
owns the real player activation transition; calling this callback in an oracle
does not prove that native activation dispatch exists.

[producer-report.json](../analysis/multiplayer/game-variants/king-presentation-20261011/producer-report.json)
contains54 whole calls: six modes0/1/2/3/4/255, three recipient values and all
three images. Actual registered pointers are checked. Every defined event byte
agrees across the images; two complete cross-image instruction shapes agree.
These calls use actual Lockout response tags and nonlocal player bindings, so
the reached original response/output-user gates return naturally. No original
return is substituted and no instruction is patched. Mode5's external network
provider is outside this probe.

The existing complete
[King world oracle](../tools/oracle_king_world.py) observes original broadcast
events from slot18 with category6, recipient NONE, cause-player at`0C`, signed
cause-team at`10`, effect fields NONE, word`1C`zero and dataFFFF. Subtypes are1/2
for FFA control/contest,5/6 for team control/contest and4 for relocation. This is
the direct mapping for `SourceKingNotice.type/player/membership`.

Start requires a separate mapping because its player belongs in recipient`08`,
not cause-player`0C`. The existing Vista`510C2B` activation producer explicitly
constructs category2/Slayer0; reusing it unchanged would announce Slayer during
King. A King publication must preserve the registered King callback's category6
and targeted recipient bytes.

## Authored response content and precedence

Xbox TU1.1`1A1780` and PC`45BFCC` resolve selected `matg+16C` through the actual
tag table to `mulg` runtime. Category6 selects the count/pointer at runtime+C8/CC
with A8-byte response stride. PC's literal category6 jump-table entry points to
`45C092`, whose instruction is`lea edi,[eax+C8]`.

Both inspected actual banks contain these eight rows:

| Ordinal | Subtype | Audience | Xbox authored English caption | English cue suffix |
| --- | --- | --- | --- | --- |
|0|0|4 general|King of the Hill|`games/names/king_of_the_hill`|
|1|4|4 general|Hill moved|`games/king/hill_moved`|
|2|1|0 cause player|You control the hill|`games/king/hill_controlled`|
|3|5|0 cause player|You control the hill|`games/king/hill_controlled`|
|4|5|1 cause team|#cause_player controls the hill|`games/king/hill_controlled`|
|5|2|0 cause player|You are contesting the hill|`games/king/hill_contested`|
|6|6|1 cause team|#cause_player is contesting the hill|`games/king/hill_contested`|
|7|6|0 cause player|You are contesting the hill|`games/king/hill_contested`|

The raw Xbox rows, actual tag references, string IDs and literal English file
backing are retained in
[xbox-lockout-response-bank.json](../analysis/multiplayer/game-variants/king-presentation-20261011/xbox-lockout-response-bank.json).
The PC raw rows and all nine actual sound references/names are retained in the
delivery report and `responses.king` fixture bank. Xbox and PC datums/string IDs
are distinct namespaces: for example start's Xbox caption is`13000EEC`, while
PC's is`13002379`. Matching event categories does not authorize borrowing a datum
from the other cache.

The PC rows all have announcer flag bit0 set, literal delay0 and weighted count0.
Language fields0..7 reference actual localized `snd!` tags. Field8/Portuguese is
NONE in every King row; the original path does not substitute English. Subtype3
has no authored row in the inspected banks, so it does not acquire an invented
tick cue.

Original Xbox`1A16F0` and PC`45DB5E` try recipient audiences in this order:
cause player0, effect player2, cause team1, effect team3, general4. Each attempt
scans the bank in authored row order, matching subtype, audience, participant
requirements at+10 and exclusions at+12. The first successful audience ends the
search. Consequently team-contested cause player selects ordinal7 even though
the cause-team row6 precedes it in the tag bank. Team members can select the
different text row while selecting the same actual sound.

## Original PC local delivery proof

`45DC77` suppresses mode4, invokes network notification, then calls`45DBF0`.
Broadcast recipient NONE iterates occupied source players and skips flags bit1;
an explicit recipient directly calls`45DB5E`. The actual delivery owner`45D893`
requires player output-user at+24 to be non-NONE and suppresses game-over events
except category0/subtype19. Its caption paths and audio path are separate: audio
continues at`45C1E8` after applicable text handling.

[king_presentation_original.json](../tests/fixtures/king_presentation_original.json)
contains3,584 whole delivery/queue calls and2,039,823 executed original
instructions. All eight authored King response ordinals are reached. Cases
cover all9 language fields, every source subtype0..6, cause/effect/team/general
precedence, negative signed membership, explicit recipient and broadcast,
ordinary/playback mode and game-over guards, local flags/output-user gates,
30/60Hz, seven original FP contexts, recent-event suppression boundaries and a
real queue sequence containing start, relocation, control, contest and game-over.

Each fixture case preserves input, full before/after queue and recent-event
banks, actual row-match/audience attempts, selected raw response and original
playback requests. It reuses the existing Vista event fixture schema, with the
additional `responses.king`, `matches`, `attempts`, `recipient_bindings` and reset metadata. Native
comparisons must consume the independently captured state, not generate source
expected output from native code.

The original instruction bodies execute unchanged through actual iterator,
response lookup, language choice, hashing and queue operations. The inherited
explicit services are network notification, auxiliary event notification,
output-user count1, host language input, CRT memset/cookie handling, caption
lookup/formatting/HUD sinks and playback handles. Caption services return empty
text, and playback returns declared handles: this establishes response/audio
queue contracts, not caption pixels, real formatting, PCM, SDL completion,
device output, transport or split-screen presentation. Native real PCM/audio
verification is additional work.

Two independent processes reproduced the complete fixture, report and executed
instruction listing byte-for-byte. See
[reproduction.json](../analysis/multiplayer/game-variants/king-presentation-20261011/reproduction.json).
Fixture SHA256 is`d287c35546c50e80879df32dbe5939c59093a161852e34b1583c53fbc33710f4`.
The report pins PC image`33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109`,
shared.map`6c0afbcf46ecd3a70ef5106e4f51d68414d5a389c1e0b1d14c47a2766abddfe0`
and generator/helper hashes.

The fixture records2,744 actual `45DB5E` recipient bindings, including336
explicit targets of the declared zero-backed unused slot3. An explicit event
recipient bypasses occupied-player iteration: original delivery consumes that
target's actual low16-indexed record, not the configured slot0's flags, team or
output user. For example case444 reaches datum12340003 with flags0, team0 and
output-user0, even though its input `user=-1` applies to configured slot0. Each
binding includes the full204-byte original record and its declared provenance.
This tests the literal direct-recipient contract; it does not establish natural
delivery to an allocated or authenticated remote player. Per-recipient native
comparisons must use these observed bindings in original delivery order.

```sh
.tools/python/bin/python tools/oracle_king_event_producers.py
.tools/python/bin/python tools/oracle_king_presentation.py
```

## Visual dependency boundary

Static complete TU1.1 listings and actual authored constants are saved separately
in [visual-source-inspection.json](../analysis/multiplayer/game-variants/king-presentation-20261011/visual-source-inspection.json).
These are inspected original instructions, not executed renderer proof.

Registered slot13/+34`2C35D0` admits polygon count>=4 and even, then calls
`2C49B0`. That owner resolves actual `mulg` constants block runtime+530/534 and
loads its tag datum+C4. In supplied Lockout this is actual `shad`
`multiplayer\hill\hill`, datumF2C81143. It submits original callbacks`2C4910`
and`2C4990`, center state+164 and its literal renderer arguments to`41480`.
Native hill rendering therefore needs this authored material and original
geometry/draw owners; a colored substitute polygon would not establish parity.

Registered slot14/+38`2C3BB0` first calls hill-marker owner`2C4A10`. It then uses
the actual selected local player's current unit, occupied-player iterator and
engine friendliness slot27/+6C before opposing-player marker construction
`163BF0` and marker submission`25473F`. The hill-marker owner consumes the
actual local unit's world position via`BACC0`, retained hill center/radius and
source marker/color records. Full marker geometry/color and projection/render
consumers require separate recovery and runtime validation.

## Native integration implications

The source-backed audio extension can preserve category6, load the exact+C8 bank,
retain its sound namespace, inventory its actual cues, and join the existing
source recent/queue logic to genuine native audio. World notices and targeted
start events need distinct faithful constructors. A local output binding and
actual simulation tick/mode/game-over state remain real caller inputs.

These contracts do not close native normal activation dispatch, authored hill
rendering/markers, actual caption formatting/submission, live trait effects,
enabled recording, objective replication, later-round reset or complete local
and network matches. The all39 goal and King startup gate remain appropriate
until those consumers are implemented and validated through their actual flows.
