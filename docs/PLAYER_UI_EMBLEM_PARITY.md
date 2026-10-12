# Player emblems on scoreboards and frontend rows

The original-asset scoreboard now allocates its authored player-emblem and death
materials independently of optional UI artwork. Previously the constructor
allocated them only inside the artwork-pack branch, so stock held, death and
final scoreboards omitted every icon. All four local views retain independent
per-player material banks, appearance colors, selected foreground/background
pages, enabled toggle and row fade. Optional Vista/MCC artwork replaces the
images after the original material ownership is established.

Lobby rows also require independent banks. The original repeated grid cells
construct separate type9 widgets; sharing a material by skin-definition pointer
made several rows display the last player's appearance. Each cell now reserves
its own bank before renderer construction. Saved profile-picker and header
emblems retain their existing independent appearance/focus banks.

The framebuffer probe found a further death-icon omission after the constructor
fix: its quad was submitted but invisible. Original Xbox1.1 `22DCCA` copies
RGB(1,0,0) through `[46FBBC] ->447064`; Xbox1.5 `2325CA` copies the same bytes
through `[474C7C] ->44AB0C`. Vista `62265C` agrees through
`[8127A4] ->79E3AC`. The `waypoint_icon_meter` shader consumes that color as
pixel extern33. The scoreboard now binds it while preserving the authored
opacity overlays. It uses the actual `mulg` runtime's emblem/death shader and
bitmap references, rather than a new icon or composition formula.

Original references and captured data:

- [Xbox1.1 image owner](../analysis/scoreboard-emblems/20261012/scoreboard-image-owner-1.1.asm)
- [Xbox1.1 color bytes](../analysis/scoreboard-emblems/20261012/death-color-original-1.1.json)
- [Xbox1.5 color bytes](../analysis/scoreboard-emblems/20261012/death-color-original.json)
- [Vista image owner](../analysis/scoreboard-emblems/20261012/0062265c.asm)
- [Authored death shader](../analysis/scoreboard-emblems/20261012/death-shader.json)
- [Original scoreboard research](MP_FULL_SCOREBOARD_RESEARCH.md)
- [Original frontend shader and placement research](FRONTEND_EMBLEM_RESEARCH.md)

## Verification

The real scoreboard regression first failed with
`Original scoreboard omitted its authored emblem/death materials`.
The three-profile lobby regression independently failed with
`Lobby rows share a mutable player emblem material`.
The native GPU probe then failed with
`Submitted player icon produced no framebuffer pixels` for the death row.
All three failures were corrected at their production consumers.

`scoreboard_vista_gui_tests` checks original shader/image ownership, independent
player/view banks, selected pages and colors, original red death-color extern,
and 40 held/final layouts across one to four local views and both aspects. It
also retains the 6,208 Vista bitmap bindings and 27 original connection-strip
cases. `retail_frontend` checks three distinct saved profiles through normal
profile-picker/controller/lobby routes, as well as the existing profile rows
and header. `retail_frontend_emblem` retains the original program/input oracles.

The [native Vulkan regression](../tests/player_ui_emblem_gpu_tests.cpp)
constructs three distinct saved profiles through the real frontend. Its captures
compare each UI with the same draw after removing only icon quads. All twelve
icon cases produce visible pixels: three lobby emblems, two held-scoreboard
emblems and one death icon, three final-scoreboard emblems, and three lobby
emblems after the scoreboard. The two complete lobby captures are byte equal.
No Vulkan validation errors were reported.

The same regression runs against original artwork and the selected MCC menu/HUD
packs. The permanent CTest entries are `retail_player_ui_emblem_gpu` and
`retail_player_ui_emblem_gpu_mcc`.

Production gameplay checks also passed actual F3 hold/release with four local
views using both original and MCC artwork, automatic final results on all four
views, and a real UDP SMG death followed by the automatic death scoreboard and
salted respawn/HUD restoration. These checks assert the actual paint consumer
has its emblem/death artwork; they do not substitute a debug overlay.

[GPU results](../analysis/scoreboard-emblems/20261012/gpu-results.json),
[lobby and held-scoreboard captures](../analysis/scoreboard-emblems/20261012/gpu/lobby-and-scoreboard.png),
[CPU regression results](../analysis/scoreboard-emblems/20261012/frontend-tests.log),
[scoreboard GUI results](../analysis/scoreboard-emblems/20261012/gui-final.log),
[final focused tests](../analysis/scoreboard-emblems/20261012/verified-tests-final.log),
[artwork-pack GPU tests](../analysis/scoreboard-emblems/20261012/verified-ui-packs-final.log),
[live gameplay results](../analysis/scoreboard-emblems/20261012/runtime-results-isolated.json).

The final build also reran the production armor framebuffer regression. Both
Spartan and Elite outputs remain byte equal to their original 640x480 RGBA
fixtures (1,228,800 bytes each).

## Delivery

Both root launchers select `build-scoreboard-emblems-20261012/halo2_native`.
The Release build uses the retained `verified-source` snapshot under this audit
directory so concurrent work does not change the tested cohort. The emblem
production files and regression tests match the workspace copies. Unrelated
ongoing physics/effect-light changes are outside this snapshot.

[Delivery, source and framebuffer hashes](../analysis/scoreboard-emblems/20261012/delivery.json).

## Remaining boundary

The Vista PC16/185 post-carnage frontend still lacks its native emblem pass.
Its actual `waypoint_emblem` implementation has a zero Xbox combiner region
and separate D3D pixel programs. The original ps_2_0 instructions were recovered
in this audit, together with `45315B`'s appearance resolver, but their complete
native live bindings and vertex path have not been verified. The painter still
reports this gap explicitly; no Xbox shader is substituted for it.

[Original PC pixel instructions](../analysis/scoreboard-emblems/20261012/pc-waypoint-ps0.asm),
[PC appearance resolver](../analysis/scoreboard-emblems/20261012/0045315b.asm),
[post-carnage bridge boundary](VISTA_PCR_SCREEN_BRIDGE.md).

These UI checks prove source-backed ownership, placement and actual native GPU
output. They do not establish an original-game framebuffer match for every UI
screen. The separate armor-pass framebuffer comparisons remain documented in
[PLAYER_ARMOR_EMBLEM_PARITY.md](PLAYER_ARMOR_EMBLEM_PARITY.md).
