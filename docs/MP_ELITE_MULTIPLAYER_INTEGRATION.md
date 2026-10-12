# Elite multiplayer selection

Saved Elite profiles can start the admitted native Slayer session through the
normal frontend. The match, local views and native LAN consumers select the
authored representation for each participant, including mixed Spartan/Elite
rosters. This removes the pregame error “The selected player representation has
no admitted native biped path.” It does not admit additional game variants.

## Original authority

- Retail `14F040` maps multiplayer appearance0/1 to representation2/3 and
  retains2/3. [Recovered selector](../tools/reference/halo2-decompiled/src/unknown_14b560.cpp#L959).
  `tools/oracle_multiplayer.py` independently executes the unchanged selector.
- Lockout's `matg+138` table has four `BC`-byte records. Record2 references
  `objects\characters\masterchief\masterchief_mp` and Master Chief FP arms;
  record3 references `objects\characters\elite\elite_mp` and
  `objects\characters\elite\fp_arms\fp_arms`. Both records are retained in
  `LevelContent`; spawning uses the selected record's biped data.
- Original `169D8A` selects the second weapon FP record for an Elite, falling
  back to record0 only when no second record exists. The existing recovered
  binding importer supplies each species' actual arms, weapon model, graph,
  node mapping and primary clock clips. [Binding research](SMG_FP_REGISTRATION_1_1_RESEARCH.md).
- Elite gait actions contain signed inherited-graph identifiers. Locomotion
  now retains the original packed action word through the recovered `29C6A0`
  fallback iterator and existing metadata/variant/channel owners. A resolved
  clip index alone cannot reconstruct that inheritance word.
- HUD `13A163..13A18F` sets shield availability and Elite appearance separately.
  `22ACE7` supplies the `hud_elite` function; `22AE21` selects sprite1 for Elite
  when the sequence contains a second entry. [HUD source](MULTIPLAYER_HUD_RESEARCH.md).
- Grenade attachment, throw graph, marker, permutations and speed come from
  the throwing participant's biped. The common grenade/material definitions
  remain shared. [Native grenade scope](MP_NATIVE_GRENADE_SANDBOX_RESEARCH.md).
  The Elite body graph contains the optional `throw_overheated` action, while
  both authored SMG FP graphs lack it. Retail `166992` retains the current
  channel when animation selection fails; the native clock now retains that
  channel for this absent optional action, without inventing a substitute clip.

## Native integration

The frontend passes the selected profile unchanged into match setup. Original
profile sanitization still runs before join. Match spawn and respawn select
the correct biped, collision dimensions and camera data. Local and replicated
first-person definitions are keyed by representation. Local hand feedback
ownership remains separate from dedicated presentation clocks.

Rendering retains both species' body/arms models, original material programs,
held-weapon markers, first-person poses/IK, gait and footstep metadata. Each
viewport selects its participant's camera and HUD. Remote bodies use their own
profile rather than the local participant's representation.

Native UDP is protocol25. Profiles and ordinary fire/grenade radar controls
cross the wire; updated peers are required. Older species consumers always
interpreted participants as Spartans. This
is the port's transport, not the Xbox System Link wire format.

## Verification

`multiplayer_species_tests` uses actual Lockout metadata and TU1.1 operands by
default; an optional XBE argument selects another verified gameplay image. It
checks all four source selections, a full16-player mixed roster, FFA/team
membership, actual localhost UDP delivery, per-species first-person fire/reload
clocks, Elite respawn and Elite-to-Spartan slot reuse. The respawn case enters
through the existing depleted-body lifecycle seam; it is not a damage oracle.

`multiplayer_species_runtime_tests` creates a disposable saved Elite profile,
signs in through `MenuState`, invokes normal Split Screen Start and runs the
actual Vulkan multiplayer runtime. Its single/mixed/LAN modes verify the
selected profiles, finite posed body/arms geometry, authored graphs and cameras,
plus real SDL firing/reload/grenade input and projectile preparation/release.
The LAN mode uses a separate native match thread and actual UDP socket. It is
localhost validation, not a physical multi-machine network test.

`native_authored_footsteps` additionally checks Elite inherited gaits and actual
grounded walking with one/four participants, original timing events, material
selection and SDL footstep submission. `retail_multiplayer_hud` checks the
actual species sprite UV/image selections alongside its existing layout tests.

The focused18-test regression set passes. Solo, mixed four-view and localhost
LAN rendered scenarios pass with both TU1.1 and the configured TU1.5 image.
Both root launchers were rebuilt against `build-frontend/halo2_native`; the
normal root launcher smoke check also passes using its configured gameplay
source and imported Vista HUD assets. Local logs, source-selector output,
captures and artifact hashes are retained in
`analysis/multiplayer/integration/elite-multiplayer-20261010/`.

```sh
cmake --build build-multiplayer --target halo2_native multiplayer_species_tests \
  multiplayer_species_runtime_tests native_locomotion_tests hud_tests -j8
ctest --test-dir build-multiplayer --output-on-failure \
  -R '^(native_multiplayer_species|native_authored_footsteps|retail_multiplayer_hud)$'
# With an available X display (or xvfb-run), all three rendered tests:
ctest --test-dir build-multiplayer --output-on-failure \
  -R '^native_multiplayer_species_(single|mixed|lan)$'
```

Rendered checks use an NVIDIA GeForce RTX5080 on Linux. The original species
checkpoint ran without a validation layer; the subsequent Elite HUD/radar
framebuffer check uses the locally installed validation layer with no errors.
`HALO2_TEST_VALIDATION=1` enables it for gameplay runtime tests. These checks do
not establish full original Xbox gameplay,
animation blending, Havok, audio or network parity. Existing native sandbox
limits and unfinished appearance/color consumers remain in scope of their
separate reports.

The subsequent HUD correction selects the original purple Elite shield palette,
uses original bitmap edge rounding, and connects ordinary-player radar pulse,
moving/action contacts and fading to each local view and native LAN snapshot.
Both species'160-position fixtures pass. Vista framebuffer checks cover1–4
views at640x480 and1280x720; configured TU1.5 solo, mixed and localhost LAN
runtime checks also pass with validation enabled. Root launchers were refreshed.
Current logs, captures and artifact hashes are local in
`analysis/multiplayer/hud-functionality/elite-radar-runtime/verification.json`.
See [the detailed HUD scope](MP_HUD_FUNCTIONALITY.md) for the host radar rasterizer
and remaining original-world/GPU parity boundaries.

The follow-up radar correction preserves the original `00010101` RGB-only
channel mask after its zero clear. Writing pulse alpha into that intermediate
had incorrectly darkened the background. The regression now checks every phase
of the source sweep, original zero alpha, and unchanged square corners, including
the actual Vulkan final shader. Source clear/write-mask traces and captures are
local under `analysis/multiplayer/hud-functionality/radar-graphics-alpha*.json`
and `radar-alpha-render/`.
