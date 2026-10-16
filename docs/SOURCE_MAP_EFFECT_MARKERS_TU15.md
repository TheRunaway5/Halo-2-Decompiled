# Original map effect marker selection

Elongation's authored spark, steam and fire effects use the real model marker
named `marker`. The old native lookup replaced that name with the attachment
row's marker, which is empty for these placements. The effects were created,
but their emitters received no location. `NativeWorldEffects` now aliases only
the original reserved StringID `0700054E`, which is `replace` in every supplied
Xbox map. Ordinary marker names retain their actual model frames.

`tools/oracle_effect_attachment_alias_tu15.py` executes the unchanged TU1.5
instructions at `17B2EC..17B2FF`. Its 143 cases cover all 21 Xbox caches,
including 10 actual `marker` inputs and 50 declared reserved-alias inputs. There
are no patched instructions or replaced services. This proves the name
selection prefix; it does not prove the whole `17B1F0` location resolver.
The original executable SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

The minimized actual-cache fixture had two started effects and zero particles.
Correcting only the marker alias produces a 51-particle peak. The spark graph
also contains a separate `lens` part. The bridge now starts its supported
authored components while retaining an explicit incomplete-graph diagnostic;
it does not count that graph as fully prepared. The combined fixture starts
all three Elongation effects, reaches 111 particles, and requires actual draw
submissions from each spark, steam and fire definition. The 21-map fixture
totals 93 fully prepared graphs out of 96, 69 initial native starts and 2,908
summed particle peaks. It checks duplicate synchronization, repeated views,
immediate retirement and salted recreation with declared bind WORLD inputs.

The staged coherent runtime has positive one/four-player spark captures at
tick 130: 2,363 and 1,000 changed pixels, with exact enabled repeats and identical
simulation/RNG records. These use ordinary spawn and native look inputs,
without camera or root overrides. The final submission control hides all
native map particles; the inspected close view shows the blue ceiling sparks.
Tick 130 lies within the observed authored spark draw window, 110..156; the
original event delays and duration bounds are unchanged.

The marker delivery's combined normal runtime has separate positive one/four-player
comparisons: 1,591 and 905 changed pixels. Its enabled repeats and
simulation/RNG records are exact. Both launchers select the tested binary;
the accepted bank runs its byte-identical pinned copy with the unchanged
compiled shader paths. [Normal launcher delivery](SOURCE_WORLD_LAUNCHER_DELIVERY.md)
records that historical17-capture receipt and the newer lens continuation.
The current selected build retains six corresponding images and simulation/RNG
records exactly. The staged results
above remain separate evidence.

## Remaining lens owner

The simulation part dispatcher `17D730` returns for `lens`, but the separate
lens request routine `17C710` does process it. Its original `17CAF9` branch computes
the current lens fade, calls the owner/view test `3E9B0` at `17CBA6`,
then requests the lens through `2DB80` at `17CBB3`. The corresponding direct
object-attachment render path calls those same helpers at `41B0A/41B13`.
`2DB80` performs view, tag, scale, flags and distance gates before its retained
request construction and retained request-pool services. These original branch
and complete helper disassemblies identify the owner to integrate; they do
not establish its native request owner, occlusion, graphics state or rendered pixels.
No substitute flare is submitted.

[The native request continuation](SOURCE_MAP_LENS_REQUESTS_TU15.md) now registers
nine original bodies and passes 72 whole original request comparisons. Actual
pool scheduling, occlusion and final lens rendering remain separate work.

`tools/research_map_lens_owner_tu15.py` retains six complete original bodies,
810 instructions and ten authenticated call sites. It also records the one
authored lens part inside the surveyed `effe` graphs and the 12 direct `lens`
attachment rows outside that graph count: Headlong 3, Backwash 4, Elongation 1
and Terminal 4. These rows are authored records, including potentially
inactive placements; they are not simultaneous visible flares. This is source
recovery only, with no original execution or native lens acceptance claim.

Evidence:
`analysis/map-world-features/swarm-lifecycle-20261014/live-swarm/marker-alias-fix/`.
The accepted normal capture/delivery receipts are in `normal-385fefbd/`.
`normal/` retains the 21-map fixture and earlier bank interrupted by a
concurrent runtime replacement. The refreshed original prefix oracle is in
`original-prefix-normal-20261015/`; final lens recovery is in
`lens-owner-audit/recovered-v3/`, with additional authenticated callers in
`analysis/map-world-features/lens-request-20261015/recovered-callers/`. Source-created
Elongation particle history, complete lens/light graphs, independent client
PVS/particle histories, reset and Xbox framebuffer acceptance remain open.
