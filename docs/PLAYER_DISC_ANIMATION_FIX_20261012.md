# Default-disc player animation playback, 2026-10-12

The default disc executable was recognized by `OriginalData`, but its player
animation operands were absent. `NativeLocomotion` therefore threw on creation,
and the runtime permanently disabled that player's gait owner until its binding
changed. Walking, crouch/sneak, airborne and landing consequently presented the
fallback idle pose. Prior TU1.1/TU1.5 playback checks did not cover this default
disc configuration.

The actual disc image matches SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The reproduction logged `Native locomotion requires original animation/deadzone
operands` for every local player. The new loader regression failed with
`Supported gameplay image disables player walk/crouch/turn/blend animation`.

`tools/research_base_player_animation_operands.py` independently pairs 23 complete
original disc/TU1.1 routine instruction streams, internal branches and jump
tables, and imports 21 operand fields from their actual use sites. The complete
biped motion owners differ; only their actual squared-axis deadzone block is
paired. Original selector RNG, facing, channel clocks, landing, transition blend,
melee, grenade and immutable basis data now reach the default disc presenter.
The existing Needler constructor, which shares the one/RNG words, is also paired.
No update executable or update balance patch is selected for the user.

The proof packet is
`analysis/player-animations-20261012/disc-player-operands-complete/report.json`,
SHA-256 `1d0799491845ce53f256dab827a3edf3193abec44e4bbb8d0fdc4934a847a416`.
The tool authenticates both input files and preserves original encoded listings.
This is static operand/consumer evidence, not whole-game Xbox equivalence.

The startup fire/reload warnings came from eagerly trying optional third-person
actions for every resident weapon and both species. The aim eligibility consumer
now uses the same cached, stance/dual-aware optional action lookup as body
composition. Legitimate missing actions are handled there; malformed resources
still throw. Required idle lookup no longer swallows a decoding failure.

## Validation

- The main `build/halo2_native` builds successfully. Both main-folder launchers
  select that runtime. Main asset-selection registries were not changed.
- Original-data validation passes for disc, TU1.1 and TU1.5, including missing and
  corrupted source rejection. All three real-cache body runs pass 84
  species/weapon/dual/stance combinations, 336 directional cycles, airborne and
  landing poses, optional actions, and native contact/SDL footstep delivery.
- The new `native_disc_player_animations` CTest passes. Metadata-store, complete
  blend operand and immutable basis provider checks now cover all three images.
- Original arithmetic checks pass: 512 overlay masks, 112 local transitions,
  3,840 landing writes and 1,920 melee arithmetic cases.
- Disc melee admission applies actual target damage at its authored impact
  event. Disc grenade delivery passes 448 actual SDL Q/native UDP checks.
- Two-view Vulkan tests use actual SDL inputs and the default adjacent disc
  executable, without `--gameplay-xbe`. Both Spartan and Elite submit advancing
  walk legs, all four sneak directions, a lower crouched stance, jump/landing
  frames and recovery, fire/reload, moving grenade and ordinary melee poses.

| Default-disc two-view run | Walk frames / distinct leg poses | Crouched / standing body height | Jump frames / leg poses | Landing frames / leg poses |
| --- | ---: | ---: | ---: | ---: |
| Spartan, SDL audio enabled | 9 / 26 | 0.304704 / 0.487978 | 42 / 129 | 4 / 13 |
| Elite, SDL audio enabled | 9 / 27 | 0.212065 / 0.450781 | 42 / 129 | 4 / 13 |
| Spartan, native UDP loopback, SDL audio enabled | 8 / 28 | 0.304715 / 0.487967 | 42 / 141 | 4 / 16 |

The leg signal uses vertical bone-basis components, excluding world translation
and root yaw. Physical movement alone cannot pass it. Logs and submitted-node
traces are `analysis/player-animations-20261012/disc-default-{spartan,elite}-final.{log,tsv}`.
The two-view UDP run also passes the same submitted-pose checks with SDL audio
enabled (dummy output device), including gait/footstep audio delivery. Its files
are `disc-default-lan-audio-final.{log,tsv}`. Physical network and speakers are
not established by this loopback/device check.

The first LAN input sequence threw a genuine damaging grenade before its later
stance/melee checks, and the participant died before completing them. The final
sequence throws after those checks. It still verifies the actual moving throw
overlay and retained gait; the dedicated grenade test separately verifies
detonation and damage. No health, death or grenade gameplay behavior was altered.

All source edits and build outputs are in
`/home/eric/Developer/Halo-2-Decompiled`. The mounted checkout was inspected
read-only. Retail framebuffer/Havok parity and full action admission remain
separate. The existing ready-effect lifecycle warning is outside this fix;
the intro movie's `wmav2` skipped-sample message concerns audio timestamps.
