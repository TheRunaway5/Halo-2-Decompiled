# Selected profiles through native match setup

The actual signed-in native frontend profile now supplies its name, stored
appearance and complete144-byte source configuration to the launch record,
local match and native UDP session. The HUD and full scoreboard consume those
profile fields. This closes one configuration-delivery gap in the
[all-variant goal](GAME_MODE_PROGRESS.md); it does not establish original
platform identities, the complete player-creation callback chain or retained
Carnage Report publication. No additional variant gains admission.

## Original contracts and native providers

The implementation uses the recovered
[shared profile constructors, converter and sanitizer](MP_PLAYER_PROFILE_RESEARCH.md).
The16 appearance bytes begin with the whole original full-profile constructor's
defaults; the first9 bytes are replaced with the actual signed-in user's stored
native appearance. The native UTF8 name is converted to UTF16 and passed through
the original31-code-unit, NUL-terminated copy contract. Team selection uses the
actual signed32 local-profile field and its original signed-byte conversion.
Account metadata remains absent and follows the original offline clear branch.

Owning input-member initialization matters. The standalone144-byte constructor
sets7E/7F and84..8F to NONE. The original fresh local activation instead calls
`65690`, which clears aD0-byte member record; the converter copies that present
member's144 configuration bytes before applying the profile fields. Whole
`192380 ->1AA360 ->192931 ->192BAA` execution independently retained this
[owning bridge](MP_SCOREBOARD_PROFILE_BRIDGE_RESEARCH.md). Both cold and
owning-initialized cases produce the same complete90-byte output. The durable
`tests/fixtures/local_profile_initialized_default.hex` copies that output from
the measured report, whose SHA256 is
`ed51be6ed8cdd89bdb4c81902c996c34ce47f3aa32b98232bc8a9e90142660c8`.
The new native local provider owns a fresh cleared input configuration and is
compared against every byte of that output. It does not claim an executed
original platform/input allocation callback or a genuine machine identity.

Native UTF8/UTF16 conversion validates Unicode scalar boundaries and rejects
unavailable encodings. Original truncation remains byte-exact even when it
leaves a high surrogate in the last retained UTF16 slot; such a name receives
an explicit native encoding error before leaving the frontend. The host does
not rewrite the original configuration to repair it.

At native join, the unaltered request and sanitized live configuration remain
separate, corresponding to original player+D4 and player+44. The sanitizer
uses the actual matg+138 representation count and the admitted Slayer engine's
team contract. Retail retains its original sanitizer policy; later executable
imports use their recovered multiplayer representation mapping. The native
body path selects authored Spartan/Elite representations2/3 from resulting
profile values0/1/2/3; see [Elite integration](MP_ELITE_MULTIPLAYER_INTEGRATION.md).
Actual native FFA/team membership is published
back to live configuration+7C. No callback execution is inferred from these
copies: original live profile changes, pre-kill/unit callbacks and complete
participant creation still need their owners.

## Native delivery and consumers

Normal Start supplies exactly the one local user currently selected by the
frontend. It does not invent additional Split Screen users. The runtime copies
the selected records into local joins or the client hello. Legacy callers that
provide only a name/team keep an absent optional source configuration.

The profile handoff introduced native UDP protocol15 with an explicit presence
byte and every configuration byte in both hello and snapshot. Current
protocol16 additionally carries the [original life fields](MP_PLAYER_LIVES_RESEARCH.md)
in authoritative snapshots. One hello supports four actual local views;
existing bounded snapshot fragments carry the complete16-player roster.
Repeated hello matching includes the full requested records. All views are
validated before any player allocation or sticky team-mask mutation. A bad
fourth profile therefore cannot leave three partial participants or a retained
team behind. Snapshot decoding checks live membership against configuration+7C.
This is the native transport contract, not original Xbox wire compatibility.

The HUD primary palette query uses configuration+40. The full scoreboard reads
the original UTF16 name field, applies its recovered name provider and uses
the selected appearance through the original scenario/matg palette provider.
Team palette overrides remain source-owned. Body armor colors, emblems and
other appearance consumers are still separate unfinished paths.

## Verification and limits

The actual saved-profile test signs in `Zoë`, selects Split Screen and invokes
normal Start with four stored appearance colors. It checks the complete
initialized source fixture, original sanitizer and preserved request bytes,
local FFA membership, actual HUD palette output and font-backed scoreboard
layout. Three actual localhost UDP peers with four views each join four local
participants, replicate90 ticks and compare every144-byte configuration for
all16 players on every peer. It also checks malformed Unicode, source name
truncation, profile/team mismatch and atomic rejection of an unsupported fourth
player body.

All30 rebuilt focused regressions pass, including original profile/retention,
HUD/scoreboard providers, game end, native match/UDP and frontend owners. A
fresh strict audit executes78 normal menu Start actions and admits2/39 variants,
blocking37 (expected exit1). A fresh production four-local-player Team Slayer
run on Lockout reaches300 ticks with `finished=1`; those are not300 active
combat ticks. The CLI run has no selected-profile input and is regression
coverage, not verification of the new profile handoff.

The saved-profile test exercises native menu state, actual match/UDP and the
font-backed painter. It does not render a GPU frontend-to-match session, prove
physical LAN, bind original machine identities, construct complete retained
participant records or render/navigate the Carnage Report. General weapons,
objective engines, edited-rule flows and complete matches remain unfinished.
The goal remains active. Current commands, source/binary pins and logs are in
`analysis/multiplayer/game-variants/native-player-profile-verification.json`.

ASan, UBSan and leak detection also pass for the integration test with the
complete native profile provider, original shared profile core, frontend state,
match, network, HUD and scoreboard units instrumented. Other cache/systems and
platform library objects retain their ordinary production build. The retained
sanitizer script records exact compile/link commands and this scope.

```sh
cmake --build build --target native_player_profile_tests -j2
ctest --test-dir build -R '^native_player_profile$' --output-on-failure
```
