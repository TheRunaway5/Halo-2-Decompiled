# Original scoreboard variant and viewport input contract

The later original parameter metadata and its actual reader establish the native variant IDs needed by the ordinary FFA scoreboard. Parameter0 is the round kind, parameter2 is Slayer score limit, and parameter16 is life limit. The display mode is a separate serialized engine-kind DWORD; original default construction maps engine selector0 to kind2. The original later rasterizer prefix produces full outer `(0,0,480,640)` and safe `(36,48,444,592)` rectangles. Its unchanged grid helpers produce the20 participant/view/aspect cases needed by the existing native `hud_viewport` adapter.
[Independent oracle](../tools/research_mp_scoreboard_live_inputs_1_5.py),
[report](../analysis/multiplayer/title-update-1.5/scoreboard-live-inputs/measured/report.json).

This is a new independent freeze. The default-profile bridge and all its provider/painter pins remain unchanged. The authenticated later XBE SHA-256 is `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`. Both independent runs reproduce30 files, including their full instruction listings. No original instruction is patched; no game readiness, graphics or clock result is supplied. The default variant constructor uses one explicit inherited XGetLanguage provider returning language1, followed by the actual unloaded-cache-locale branch. It does not supply a locale string or loaded flag.
[Verification](../analysis/multiplayer/title-update-1.5/scoreboard-live-inputs/freeze-verification.json),
[default-profile freeze](MP_SCOREBOARD_PROFILE_BRIDGE_RESEARCH.md).

| Artifact | SHA-256 |
| --- | --- |
| Oracle | `d5aeeea8bf65b2605b09a942132aa4ab4a2c58a50e1abd6e92909ba7fce4396b` |
| Report | `0e2e344391e64e0a2f688135848477ebfc7bc2cc465acec89ac00bf65a03e435` |
| Instruction manifest | `52e893d4a22bb7356e70524d4b22b41531cc91133f91cbab7dfd25be7802ec09` |

Reproduce:

```sh
.tools/python/bin/python tools/research_mp_scoreboard_live_inputs_1_5.py \
  --output analysis/multiplayer/title-update-1.5/scoreboard-live-inputs/reproduction/report.json
```

## Exact variant and score fields

Whole base `2374F0` and later `23DEE0` have equal complete instruction and branch shape. The later reader uses the original112-entry table at `47CF40`. The report executes the owning metadata calls themselves, preserving their literal output buffers, type, offset and multiplier; it then constructs the original default Slayer variant with whole `1A6D40(output, engine_selector0)` and reads all three parameters through whole `23DEE0`.

| Native input | Original reader contract | UiVariant value ID | Actual default |
| --- | --- | --- | --- |
| `display_mode` | DWORD Game+180 = serialized variant+44, separately assigned by the default engine-selector switch | **No parameter ID**; engine selector0 constructs kind2 | 2 |
| `round_limit` | DWORD Game+188 = variant+4C; metadata `23D4D6`, type5, multiplier bits3F800000 | 0 | 0 |
| `score_limit` | DWORD Game+18C = variant+50; metadata `23D4FD`, type5, multiplier bits3F800000 | 2 for Slayer; IDs1..9 alias this field | 25 |
| `life_limit` | DWORD Game+1B8 = variant+7C; metadata `23D623`, type5, multiplier bits3F800000 | 16 / 10 hex | 0 |

The Game variant base+13C is an actual `LEA EBX,[EAX+13C]` at `13A8DF`, before the original normalization call in whole `13A8B0` (TU1.1 `139540`). This is the owning placement contract, not an offset inferred from names. `UiVariant.engine` is the cache/default constructor's selector; `UiVariant.values` contains the parameter IDs above. Its engine selector0 must not be used as scoreboard display mode0. The source default constructor switch itself yields mode2 for ordinary Slayer. Parameters are source signed integer values, rather than inferred from native match completion or unit death.
[Actual parameter reader](../analysis/multiplayer/title-update-1.5/scoreboard-live-inputs/measured/instructions/u5-0023dee0.asm),
[default constructor](../analysis/multiplayer/title-update-1.5/scoreboard-live-inputs/measured/instructions/u5-001a6d40.asm),
[options application](../analysis/multiplayer/title-update-1.5/scoreboard-live-inputs/measured/instructions/u5-0013a8b0.asm).

The full roster builder `163360` uses Game+1128 as its mode selector: ordinary zero chooses signed16 Score at `engine+304 + slot*1C +4`; nonzero chooses signed16+6. Wins reads signed16+12. The ordinary ranking comparator uses signed16+4 descending after its original record/membership/eligibility gates. There are no kill/death columns in this layout. `15BB50` uses time text only for serialized display modes3,4,8; source ordinary Slayer2 uses decimal score. The column gates read actual score limit !=1 and round kind !=0. Life exhaustion is Game+1B8>0, current player+2C==NONE and signed16 player+1AC==0; it does not use `Unit.alive`.
[Complete original builder](../analysis/multiplayer/title-update-1.5/scoreboard-live-inputs/measured/instructions/u5-00163360.asm),
[score formatter selection](../analysis/multiplayer/title-update-1.5/scoreboard-live-inputs/measured/instructions/u5-0015bb50.asm),
[life predicate](../analysis/multiplayer/title-update-1.5/scoreboard-live-inputs/measured/instructions/u5-00160570.asm),
[frozen ranking/output contract](MP_FULL_SCOREBOARD_RESEARCH.md).

`game_over` in the portable leaf means the supplied source Game+1128 record mode, not a recovered equivalence to native `finished`. Engine active reads the selected original table entry, signed16 engine+6C==1, then Game+0C==4 or engine+C04==1. Those session/lifecycle producers remain open to the live native adapter. It may display the explicitly requested ordinary roster with actual published names/identities/score shorts and explicit adapter gates without claiming original whole world/session readiness.

## Source viewport output and render conduit

The executed original fragment `12560..12591` starts at the rasterizer constructor, retains its actual saved frame and stops before its subsequent GPU configuration. It calls whole `169D0`, loads literal scale1 at `1257A`, sets ECX=36 and EDX=48 at `12582/12587`, and calls whole `16A10` at `1258C`. The source writes base outer at `4922AA` and safe at `4922B2`; no rectangle or margin field is supplied. This actual later prefix always requests these margins. The earlier base display-flag conditional for alternate32/24 margins is not asserted as a later prefix branch.
[Complete constructor listing](../analysis/multiplayer/title-update-1.5/scoreboard-live-inputs/measured/instructions/u5-00012560.asm),
[source margin writer](../analysis/multiplayer/title-update-1.5/scoreboard-live-inputs/measured/instructions/u5-00016a10.asm).

Complete base `2F600/2F640/2F6B0` correspond to later `2F5E0/2F620/2F690`. The report executes these unchanged later helpers with the source-created margin banks. Native participant count1..4, view index and aspect-selected grid mode are declared request inputs. One participant uses mode0; multiple participants use mode1 standard/mode2 widescreen. It records the literal grid, span, position, outer and safe outputs for every view, including the original three-participant spanning view and four-pixel internal safe gutters. The owning full view/display-mode scheduler is not executed here.
[Grid helper](../analysis/multiplayer/title-update-1.5/scoreboard-live-inputs/measured/instructions/u5-0002f5e0.asm),
[span and position](../analysis/multiplayer/title-update-1.5/scoreboard-live-inputs/measured/instructions/u5-0002f620.asm),
[outer/safe rectangles](../analysis/multiplayer/title-update-1.5/scoreboard-live-inputs/measured/instructions/u5-0002f690.asm).

The complete descriptor owner `223080→229A90` also matches. Its gameplay branch calls these three helpers, writes outer at descriptor+B0 and safe+B8, then builds the camera at descriptor+80. The original74-byte camera copy to descriptor+C retains those rectangle fields at camera+30/+38. Original `2B9F0` copies its incoming74-byte camera to globals `4C65C0`; thus camera+30/+38 become **4C65F0/4C65F8**, the actual full-scoreboard outer/safe readers. This is a static source render conduit; the GPU/world renderer is not supplied success or claimed executed. The portable layout and painter can receive the existing native `hud_viewport` outer/safe outputs as explicit native viewport delivery, using the corresponding original grid geometry.
[Descriptor/grid owner](../analysis/multiplayer/title-update-1.5/scoreboard-live-inputs/measured/instructions/u5-00229a90.asm),
[current render-camera store](../analysis/multiplayer/title-update-1.5/scoreboard-live-inputs/measured/instructions/u5-0002b9f0.asm),
[existing viewport adapter](../src/multiplayer_hud.cpp:311).

An independent GCC-O2 harness compiles the actual unchanged `src/multiplayer_hud.cpp` and `src/source_camera.cpp`, then compares every outer/safe coordinate to this original later report: **160 actual native viewport words pass** across all20 cells. Function/data sections and linker garbage collection omit unrelated HUD code; no replacement viewport implementation is tested. This establishes the requested native geometry reuse, without proving original live display-mode ownership or a rendered frame.
[Native differential harness](../analysis/multiplayer/title-update-1.5/scoreboard-live-inputs/native-viewport-check.cpp),
[verification and source hashes](../analysis/multiplayer/title-update-1.5/scoreboard-live-inputs/freeze-verification.json).
