# Ordinary scoreboard name, color and text providers

The new native providers execute the ordinary FFA name/color contract from the authenticated later-update image. The original name sanitizer is not an identity copy: it substitutes the executable's eighteen literal emoticon mappings, and some private-use glyphs call dynamic profile/menu providers. The color reader consumes four signed appearance bytes and the actual `matg` RGB palette. No player-slot colors, title string, connection bars or team presentation are invented.

[Original instruction oracle](../tools/oracle_mp_scoreboard_providers_1_5.py), [complete API inputs, outputs, stores and source bodies](../analysis/multiplayer/title-update-1.5/scoreboard-providers/measured/report.json), [native providers](../include/halo2/full_scoreboard_providers.hpp), [native differential](../tests/full_scoreboard_providers_tests.cpp).

The original image SHA-256 is `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`; the source cache is actual disc Lockout, `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`. Earlier scoreboard, KBM, player/world and other research freezes remain unchanged. All new fixture/report artifacts are separate. No instruction patch or supplied gameplay, graphics, readiness or clock return is used.

## Names

Actual row builder `162E70` copies at most255 UTF16 code units, appends NUL, calls complete `233CF1` with capacity256, copies the result, then creates the20-unit name widget. The widget retains at most19 code units. `233CF1` scans code units in order; it does not case-fold, strip arbitrary punctuation or replace player names with slot labels. At each position it checks the private-use interval `E000..F8FF`, then the literal token table when no nonzero dynamic handler was selected. Every matching token appends its literal mapped glyph and updates the consumed token length. Replacement is admitted only when the resulting complete length is strictly less than256. The cursor advances one unit after the source memmove/copy, including a rejected replacement. The native provider translates that visible-string behavior while avoiding the original scratch-buffer overcopy outside the resulting NUL-terminated text.

The importer must preserve these **literal executable tables**, rather than embedding replacement text in native code:

| Image | Token table | Glyph table |
| --- | --- | --- |
| TU1.1 | `451548`, eighteen8-byte records | `4776EC`, 13112-byte records |
| Later update | `455E48`, eighteen8-byte records | `47C91C`, 13112-byte records |

Token record+0 is a pointer to NUL-terminated UTF16; +4 is the mapped unsigned16 glyph. Glyph record+0 is its DWORD key, +4 is the original callback address, +8 is preserved data not read directly by `233CF1`. The complete TU1.1 `22F40E` and later `233CF1` bodies match119 normalized instructions and branch topology. All eighteen token strings/glyphs and131 glyph keys are compared from untouched original bytes; relocated callback addresses remain image-specific. The provider requires the complete18/131 mappings and rejects malformed host tables.

The separate oracle executes43 complete original sanitizer calls: plain/empty/Unicode/bar/unlisted-private names, each actual token alone and embedded, and the255-unit source capacity. Dynamic glyph `E408` selects actual callback `233FAA`; the bounded stop retains its exact ABI and table record. An additional unchanged call executes that handler and returns empty in the image's cold profile/menu context. That result is **not promoted to a live-profile callback rule**: it reads profile/menu owners outside this ordinary name leaf. Native code reports `UnavailableGlyphCallback` unless an explicit owning replacement provider is present. It does not silently drop or preserve a dynamically substituted glyph as a recovered live result.

[Complete sanitizer and string callees](../analysis/multiplayer/title-update-1.5/scoreboard-providers/measured/instructions/00233cf1.asm), [row's original copy/sanitize order](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-00162e70.asm).

## Appearance and palette

Complete ordinary color helper `830C0` receives EDI=four appearance bytes, ESI=48-byte output, and two stack arguments. The ordinary FFA caller passes teamNONE and overridefalse. It sign-extends each byte and invokes `1AA2B0` four times, copying twelve raw bytes for each result. The scoreboard uses the first RGB triplet. `1AA2B0` starts with the original fallback RGB pointed to by `[474C70]` (`44AADC` in this image). It uses `matg+160` count and `matg+164` pointer only when the loaded scenario pointer `[4ECC20]` is nonzero and the signed index lies in `[0,count)`. The fallback words are three `3F800000`; their source address/data are retained, rather than interpreted as an invented per-slot white default.

`4ECC20` is a **scenario pointer**, not a palette readiness byte. The source cache loader `1252E0` owns its store. The new suite executes unchanged `125434..125482` with EBP=the actual Lockout header scenario datum and the original130-byte saved-register/local frame. It resolves both scenario and `matg` through the explicit imported tag table; `[556D90]` references the literal original tag header, whose+10 contains the `matg` datum. Original stores produce scenario pointer`80152498` and `matg` pointer`80145000`. No pointer/readiness value is written at either output slot by the fixture. **The preceding complete file loader is not executed**: literal cache backing, imported tag records, the header pointer and owning-tail register/frame are the declared provider boundary. This is enough to execute the palette reader without claiming full title/map startup.

Actual Lockout has eighteen12-byte RGB rows. Twenty-two original signed-index cases execute every authored row plus the negative/upper-bound fallbacks. An independent cold-scenario case reads the original fallback. Full `830C0` consumes the source-default appearance and produces four RGB triplets; primary index10 is raw words `3E828284 3ED8D8CB 3F0F8F8A`. Native code copies these source bits; it does not quantize through8-bit colors.

[Complete palette reader](../analysis/multiplayer/title-update-1.5/scoreboard-providers/measured/instructions/001aa2b0.asm), [complete four-color helper](../analysis/multiplayer/title-update-1.5/scoreboard-providers/measured/instructions/000830c0.asm), [original loader and pointer stores](../analysis/multiplayer/title-update-1.5/scoreboard-providers/measured/instructions/001252e0.asm).

The complete full-profile default constructor `1AA360` clears1E0 bytes, sets appearance bytes at+118..11B to`10,0,0,10`, and writes the other recorded defaults. It executes to a real original return. Its complete19-instruction body matches base `1A0750` and TU1.1 `1A40B0`. Native `full_scoreboard_default_appearance()` ports exactly these four owning stores. The source default's provenance is separate from the unexecuted copy into a player's current profile. Earlier TU1.1 `14D600` proves that a supplied90-byte profile is sanitized/copied to player+44, hence its appearance+40 lands at player+84; the earlier fixture did not produce that supplied profile from the full1E0 default record. No automatic claim that all live/saved profiles use index10 is made.

Current `MultiplayerMatch::player()` contains the actual native name and full source identity, but no four-byte appearance record. `MenuState` stores explicit appearance choices under keys100+part and its `profile_option()` returns a caller-provided fallback when missing. It does not itself execute or import this original constructor. Root may initialize a supported native default appearance using the proved constructor, then carry actual selected profile values as an explicit host preference adapter. Preserving those bytes on the match player is required; deriving them from slot or pretending the existing match already contains an original profile is unsupported. The complete full-profile→wire/local-binding owner remains a separate startup boundary.

[Executed constructor and stores](../analysis/multiplayer/title-update-1.5/scoreboard-providers/measured/instructions/001aa360.asm), [earlier current-profile copy](../analysis/multiplayer/integration/spawn-source/player-visibility/instructions/0014d600.asm), [native preference access](../src/frontend_state.cpp).

## Exact text renderer seam

The full scoreboard uses the same complete downstream text algorithms as the existing original frontend text renderer. The suite compares untouched complete bodies, including normalized image operands and internal branches:

| Base owner | Later owner | Instructions | Meaning |
| --- | --- | --- | --- |
| `13EC70` | `141750` | 66 | Bright RGB normalization |
| `13EB60` | `141640` | 68 | Clamp/store foreground state |
| `13F0E0` | `141BC0` | 216 | Bounds, alignment and line measurement/delivery |
| `13F470` | `141F50` | 81 | Packed foreground/shadow state |
| `13F700` | `1421E0` | 216 | Glyph bearings, kerning, clipping and advance |
| `1F490` | `1F470` | 85 | Clipped shadow/foreground graphics callback |

Thus reusing the actual font glyph/kerning atlas importer and these translated metrics has source authority. The **private menu `text()` wrapper is not a drop-in scoreboard caller**: it applies menu animation, centered-coordinate projection, authored definition flag remapping and optional menu pulsing. A scoreboard entry must use the following source caller state instead.

`163EA0` writes font selector6 at`1640F6` (actual `font_table.txt` slot6 is `conduit-13`). It sets `4F3C78=FFFFFFFF`, `4F3C74=0`, and `4F3C7C=2` only for a right-justified widget, otherwise0. It calls `141750(widget.foreground)`, then `1FA30` with the widget UTF16 text and exact signed16 text rectangle, other arguments`0,0,0,1.0,0,0`. No additional guessed baseline/glyph offset is needed: original font ascent, glyph bearings and kerning own those positions. Text edges are used verbatim; adding1 to bottom/right changes the source clipping. `1FA30` also processes `|` control sequences through actual `1426D0`; the menu painter's simple UTF8 codepoint decoder does not implement that stage. Ordinary names/labels without those controls can use the shared glyph path, while control-sequence delivery remains explicit.

Foreground runs the original all-channels>.68 normalization (`46A438=3F2E147B`) and clamping. It is not raw ARGB. The source renderer's shadow pointer `[474C34]` initially points to literal `44AAF8={1,0,0,0}`. `163EA0` enables and copies this shadow at`164109..16412C`; `141F50` limits shadow alpha to foreground alpha. `4F3C74=0` is retained as caller state; the original `142140` classifier preserves eligible private-glyph atlas coloring. Shadow uses the already-clipped glyph and a draw-scale offset in both axes, before the foreground. The new oracle executes three complete original `141750` calls and retains the global output words.

Background rectangles use source `36870` with raw widget background ARGB and the signed16 rectangle corners **without+1**: top-left, top-right, bottom-right, bottom-left. A native quad consequently uses width=`right-left`, height=`bottom-top`. This renderer draws the valid widgets' background cells; it does not introduce a new full-screen backdrop. Original GPU command-buffer construction/submission remains unexecuted here. Current-player pulse `162A00` emits four1-pixel strips through `36870`, using alpha `((sin(tick*literal46A2EC/rate)+1)*.5)*name_foreground_alpha`, with original x87 ordering. The literal phase word is `40490FDB`; this is not the frontend menu's pulse timing or an arbitrary solid highlight. Root can retain this source listing as the border contract without claiming complete graphics parity.

[Full source caller and draw order](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-00163ea0.asm), [exact color state](../analysis/multiplayer/title-update-1.5/scoreboard-providers/measured/instructions/00141750.asm), [UTF16/control delivery](../analysis/multiplayer/title-update-1.5/scoreboard-providers/measured/instructions/0001fa30.asm), [shared frontend metric/clip proof](FRONTEND_TEXT_RESEARCH.md), [background command construction](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-00036870.asm), [pulse owner](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-00162a00.asm).

The source font cache/readiness and final graphics submission are not supplied as successful owning services in this new suite. Complete instruction correspondence plus the existing declared-font frontend metric oracle establish the reusable leaf, not complete original font startup, D3D admission or framebuffer parity. Team rows, quality bars, LAN profile replication, title delivery and live original input readiness remain outside this extension.
