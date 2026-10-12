# Full scoreboard draw leaf

The native painter translates the ordinary scoreboard's actual draw caller and downstream font6 glyph contract. It draws the imported layout's cells, the clock-driven current-row border, and actual `conduit-13` text. It does not borrow the menu painter's coordinate projection, animation, word wrapping or definition flags. No new fullscreen backdrop, title text, slot colors, emblems or connection-quality graphics are invented.

[Native API](../include/halo2/full_scoreboard_painter.hpp), [implementation](../src/full_scoreboard_painter.cpp), [original instruction oracle](../tools/oracle_mp_scoreboard_draw_1_5.py), [executed inputs/outputs and complete instructions](../analysis/multiplayer/title-update-1.5/scoreboard-painter/measured/report.json), [independent native check](../analysis/multiplayer/title-update-1.5/scoreboard-painter/native-check.cpp).

Authority is the authenticated later-update XBE, SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`, the actual original font, and the unchanged frozen scoreboard layout. The earlier scoreboard, providers, KBM and world suites remain immutable. Only the new painter header/implementation are production edits in this extension.

## Caller and geometry

Complete original `163EA0` renders player emblems first, then header backgrounds, each player row's backgrounds/quality/current pulse, then title/header/player text. The native supported order follows these instructions. The title receives text but no invented background. For ordinary player rows, the icon cell's background is emitted only when source icon-kind is2; place/name/wins/score/connection backgrounds follow. Source `36870` takes signed16 corners verbatim: width is `right-left`, height is `bottom-top`, with no extra inclusive-edge pixel.

The new oracle executes unchanged post-begin fragment `163EF5..1640DD` with the frozen layout in the original stack offsets and EBX0. It records nineteen ordered rectangle requests for the actual four-row fixture, including the four current-row strips. **This is a declared post-`36570` stack/register fragment, not a successful replacement for GPU begin or complete display admission.** Void `36870` submission records the rectangle/color and returns with EAX unchanged. The first genuine outer UTF16 draw attempt independently stops at original `404BCD` reading NULL+8 through `1F050→35B80→D3D` context construction. That stop remains in the report; no successful context/graphics return is substituted.

Current pulse `162A00` uses the name rectangle. Define `left=name.left-45`, `top=name.top-1`, `bottom=name.bottom+1`, and `right=name.right+(connection-valid?7:0)+(score-valid?54:0)+(wins-valid?50:0)`, with source16-bit wrapping. The original requests, in order, are:

| Strip | top | left | bottom | right |
| --- | --- | --- | --- | --- |
| Top | top | left | name.top | right |
| Right | top | right | bottom | right+1 |
| Bottom | name.bottom | left | bottom | right |
| Left | top | left-1 | bottom | left |

Source alpha is the ordered x87 expression `((FSIN(tick*phase/rate)+one)*half)*name.foreground.alpha`, with only the final binary32 store. The suite executes twenty original rate/tick inputs, retaining literal twelve-byte clocks, complete current-row records and all four outputs.

## Text and font ownership

`163EA0` selects font6 and supplies `4F3C78=FFFFFFFF`, `4F3C74=0`, and alignment `4F3C7C=(widget.byte8?2:0)`. **Alignment2 is CENTER in `141BC0`; alignment0 is LEFT.** The frozen layout API member `right_justified` names literal widget byte8 but is misleading for this caller. Its bits remain unchanged; the painter correctly centers those ranks/scores. The original place `1` begins at x194.5 within its20-pixel text rectangle.

The text rectangle is already in source pixel coordinates. Original font ascent, horizontal offset, bearings, kerning, descent and leading own placement. The caller supplies the actual selector6 font/atlas material, untextured rectangle material, current viewport clip, font header+0A horizontal offset and text globals `4F3CCE/4F3CD0` first/next-line indentation. Actual conduit-13 header+20A is zero. These explicit states do not establish original font-cache startup or global text-state reset ownership.

Flag `4F3C74=0` disables word wrapping. `141A20` scans the complete line, but rejects an overflowing glyph's contribution to measured width after the first admitted glyph; drawing still traverses every glyph and clips its ink. This matters when an overlong numeric/centered field is measured. CR starts the next line; LF is ignored by `142080`. The painter preserves those rules and the strict baseline-before-bottom gate.

Foreground executes `141750→141640` all-channel bright-RGB normalization/clamping, then `141F50` x87 multiplication/truncation into packed ARGB. It is not raw widget ARGB. The imported shadow uses alpha `min(shadow.alpha,foreground.alpha)` and is submitted before the glyph, shifted one draw-scale pixel from the already-clipped position without another clip. Unit-scale `1F470` truncates clipped ink dimensions and bitmap crop offsets before `1F380`, including a degenerate admitted quad when a positive fractional width truncates to zero. Eligible private glyphs retain atlas RGB through the same classifier as the existing frontend renderer.

Fifteen unchanged original text leaves use real header/glyph records: the twelve actual ordinary row fields, a nineteen-unit wide name, an overlong centered field and literal LF input. There are98 original shadow/foreground quad records. The ready glyph lookup/pixel-residency cache is a **declared provider**, as in the earlier frontend text proof; original color, measurement, kerning, clipping, callback geometry and traversal instructions execute. The direct `141BC0` leaf receives literal DWORD character delivery and the positive widget clip rectangle; complete outer `1FA30/1F050` GPU admission remains stopped. Native atlas packing/sampling and framebuffer equivalence are not claimed from these records.

`|` controls, tabs and private alignment-control codes require the original `1FA30/1426D0` parser/context. The painter reports those widgets as unavailable rather than drawing guessed controls. Emblems and connection-quality drawing are also explicit result flags. Supported text remains drawable through `FullScoreboardPaintResult::draw`; `Partial` means one of these declared consumers is absent.

## Imported words and complete image correspondence

The painter accepts raw words from `OriginalData`; it embeds no text/font payload. Importer ownership remains with root. All entries below are read from untouched images and their complete callers are compared:

| Field | TU1.1 | Later update | Bits |
| --- | --- | --- | --- |
| bright_rgb_threshold | `465398` | `46A438` | `3F2E147B` |
| pixel_scale | `4650A8` | `46A148` | `437F0000` |
| pulse_phase | `46524C` | `46A2EC` | `40490FDB` |
| pulse_one | `465030` | `46A0D0` | `3F800000` |
| pulse_half | `46502C` | `46A0CC` | `3F000000` |
| shadow pointer/data | `[46FB74]→447050` | `[474C34]→44AAF8` | `3F800000,0,0,0` |
| palette fallback pointer/data | `[46FBB0]→447034` | `[474C70]→44AADC` | three `3F800000` |

The new complete TU1.1→later pairs are `1403E0→141750`, `1402D0→141640`, `140850→141BC0`, `1406B0→141A20`, `140BE0→141F50`, `140E70→1421E0`, `140D10→142080`, `140DD0→142140`, `1F470→1F470`, `1F380→1F380`, and `161620→162A00`. Full normalized instruction/branch topology and relocated data operands are retained, not just matching function labels.

The x86 host pulse adapter executes the original hardware x87 instruction sequence under the caller's masked control word, then restores the host control word. Other architectures report an unavailable provider. **Host FSIN is not labeled Xbox/Unicorn bit parity:** at rate30/tick45 the original Unicorn run produces alpha word `279A0000`, while this host's x87 produces `279AE3C0`. The other nineteen declared cases match. The difference is retained in native logs. Painter rectangle/color tests receive the exact source pulse output through the explicit pulse provider; hardware adapter measurements remain separate.

## Validation and remaining live inputs

The independent native harness passes2,059 checks: all twenty source pulse geometry/raw-color cases, fifteen text cases, actual atlas-coordinate bounds, the complete nineteen-cell/current-pulse order followed by the actual row text order, and invalid-input/unavailable-control gates. Independent source report/listing and GCC build/output repeats, plus Clang ASan/UBSan, are recorded in the separate freeze verification artifact. This is a supported draw-leaf comparison, not live GPU or complete in-match display proof.

The previous providers freeze supplies source name sanitization, palette lookup and full-profile constructor defaults. It **does not supply the missing full1E0-profile→90-byte multiplayer-profile producer**. Earlier `14D600` proves the supplied90-byte record is copied to player+44 and its+40 appearance bytes land at player+84; that final copy alone cannot assert a live default profile. Actual profile storage/selection and the bridge still need owning evidence before root can populate live appearance. No slot-derived colors or borrowed frontend preference fallback are introduced here. Live roster membership is signed player+C0, local view is signed short player+28, and input user is the separate player+24 field; the engine-active `15DD10` gate depends on actual engine table/lifecycle/options state, not native alive/finished guesses.
