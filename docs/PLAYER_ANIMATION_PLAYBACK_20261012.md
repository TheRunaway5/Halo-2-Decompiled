# Player animation playback, 2026-10-12

The default multiplayer body presenter now selects the authored airborne and
crouch clips, retains directional locomotion while dual wielding, and composes
grip, pitch aim, and admitted weapon actions before skinning. Grenade throws
retain their authored overlay masks while the run/foot-event clock continues.
The continuation adds source-timed landing, ordinary kind1 melee animation
requests, and ready publication after the new hand acquires its binding.
This closes
playback defects in the native mover adapter; it does not establish that every
retail player action is reachable in the native gameplay implementation.

## Source evidence and changes

- `NativeLocomotion` retains the existing original classifier, signed inherited
  bindings, variant RNG and channel clock. Airborne uses E5700's `0800001E`.
  Crouch uses the cache's actual `06000087` mode. Dual uses `0400054B` in the
  same original fallback iterator instead of replacing locomotion with idle.
- `NativePlayerAnimation` maps model-order locals using the decoded authored
  components. Additive composition follows 287B90/28B6B0; type2 replaces only
  authored components. Weapon actions therefore preserve unauthored leg bones.
- Upper aim uses 114BD0's standing/moving activities. The inherited clip's
  screen is resolved as in 1DE920, and pitch selects rows using 2800F0's authored
  step angles and counts. Relative yaw is zero because native world yaw already
  owns the body root. This is a native presentation boundary, not a recovery of
  the full retail aim/body facing constraints or aim transition weights.
- Fire/reload start ticks remain snapshot-owned. Left-hand actions resolve
  against the left weapon. Other overlays require an actually published action
  and an authored binding; observing an equip or a button does not fabricate
  successful source action admission.
- `E5960`'s literal biped+1FC..20C thresholds/durations and `E5A80`'s retained
  landing timer select soft/hard landing. The native capsule solver supplies
  incoming downward support-contact speed explicitly; this is a native contact
  adaptation, not reconstruction of DEFC0's full Havok/velocity record.
- The native ordinary melee animation adapter uses the selected executable's
  four EB000 candidate tuples, one shared LCG consumption, EB080's actual FP
  frame/event durations, and EB700's ordinary timer/expiry. Actual F and mapped
  controller edges reach this consumer; UDP serials preserve a short F tap
  under coalescing/loss. Body overlays retain gait and first-person bindings
  include each authored melee_strike_1..4 candidate. Native non-reloading
  contexts are supported. D0830's impact grid/damage, targeted attacks, sword
  combos, player cooldown/target predicates and weapon interruptions remain
  separate; no shot ray or invented damage is used as their substitute.
- The existing native ready consumer now publishes its real activity after
  rebuilding the selected hand's FP binding. A positive pickup test verifies
  the new hand receives ready and the existing hand keeps its channel.
- Non-looping hard landing clip 139 retains its real 29 frames/loop 30 metadata.
  The decoder no longer rejects that unused loop marker. The explicit legacy
  seconds-loop sampler still rejects an invalid loop when looping is requested.
- Codec8 uses float4 quaternion keys and 16-byte frame steps, following original
  292970, 2929B0 and 2CB1E0. These copy float channels without normalization.
  Previously the loader rejected these resources.
- Body skinning, the right weapon, and the left weapon now consume one composed
  world node bank. Split views reuse that bank. Authoritative source world poses
  and explicit reference-player replay retain their own paths.

## Verification

The frozen build is `.local/player-animations-20261012/build`, with its exact
source under the adjacent `source` directory. It uses Release with `-O1`.

Seven focused CTest checks pass: native body overlays, native authored footsteps
and player poses, retail locomotion, original animation channels, and the
Xbox/Vista asset adapter, retained inventory and native weapon gameplay. Coverage includes:

- 512 bitwise differential additive/mask cases against recovered original
  287B90, plus type2 component replacement checks.
- 3,840 E5960 timer/force-flag writes compared with the compiled original body,
  plus retained expiry/moving half-duration checks.
- 1,920 original EB000/EB700 selection/RNG/timer cases. Availability is an
  explicit test input; D0830 records a request boundary without supplying a
  hit or damage. The fixture generator preserves the original executed bodies.
- 126 real cache codec8 component samples compared with the original component
  decoder entries.
- 84 species/weapon/dual/stance combinations: cycle advancement, airborne and
  soft/hard landing selection, positive/negative aim pitch, and each authored grip, fire, reload,
  ready, put-away, grenade, melee and left-hand overlays available in those
  combinations. The separate actual-input test below verifies ordinary native melee playback;
  it does not establish complete retail melee gameplay admission or damage.
- 154 original footstep material/sound selections and actual native contact,
  timing events, fractional sampling and SDL sound submissions.
- All 33 FP SMG clips decode identically through Xbox and adapted Vista caches,
  including the two formerly rejected codec8 clips.

The TU1.5 four-view GPU run uses actual SDL run, jump/contact landing, crouch,
aim up/down, firing, reload, moving grenade throw and ordinary melee inputs.
It verifies submitted authored base clips, continuing throw gait, changing
melee body geometry, FP strike submission/expiry, finite vertices and independent
views. The same actual-input test runs against the native UDP host on loopback
with four participants. A single-view actual-input run also passes with 746
first-person pose submissions. The verified local run submitted 3,776 poses and the
final UDP run 4,992 poses, including fractional FP and remote-body samples. Logs and
submitted-pose traces are under `analysis/player-animations-20261012`.

The independent 16-player UDP regression passes 21,304 sampling checks across
30/60/120/144/240Hz, loss/reordering/duplication/late snapshots, and new F-tap
coalescing/loss/single-delivery checks. Native UDP protocol 48 carries the
retained landing and melee presentation state.

Delivery runtime SHA-256:
`58a8f7492dac7b8d101640118c550d0df7f58ca19b8d3e96b458f533d6686d35`.
The previous launcher binaries are preserved under the frozen build parent
in `previous-launchers`; the newer pre-install pair is also retained in
`before-final-delivery`. Both installed launchers were checked with
`--launcher-info` and select this exact frozen build.

The host has no `VK_LAYER_KHRONOS_validation`; GPU runs use normal Vulkan.
Hardware Xbox arithmetic, framebuffer parity and physical-network behavior
are not proven by these checks.

## Remaining scope

Full stationary turning/body-heading ownership and lower aim/facing/transition
weights remain unresolved in the default native presentation adapter. Native
view yaw still owns the world root; zero relative aim yaw is not a complete
retail turning implementation. Unarmed/sword/targeted melee and the complete
impact/damage/interruption path remain outside the ordinary visual adapter.
This report therefore does not declare every retail player animation/action
reachable or complete 1:1 gameplay parity.

The user's asset-cache reset removed root `assets/` during this task. Read-only
reference tests use the existing backup under
`/home/eric/.local/share/halo2-native/cache-reset-backups/20261012T201413Z/assets`;
only the private test-source symlink points there. The reset was not reversed.
The installed launchers correctly require source selection on launch while the
root asset setup remains empty. The delivered frontend retains the newer
PCR/lobby double-click sequence fix present in the user's installed launcher.
