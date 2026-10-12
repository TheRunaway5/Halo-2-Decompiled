# Original small-text setup and glyph layout

The seven Mjolnir diagram labels in Settings screen19 pane0 author flags9 and
font9. Standard text setup `253765` treats flag8 specially: it retains runtime
font1, copies the shared scale and disables the shadow. The actual shared
`mainmenu.map` fields at wigl+48/+4A/+4C are `3200DB009A99993E`: signed50ms
interval, cursor U+00DB and float scale word `3E99999A` (0.3). Button setup
`253CFF` instead retains its authored font, unit scale and shadow. No font or
scale was fitted to the user's screenshots.

The independent setup fixture `tests/fixtures/frontend_text_setup_original.tsv`
contains92 unchanged-original executions: all eight screen19 pane0 text
definitions, all52 direct standard flag8 definitions and32 explicit standard
text/button flag controls. Its original executor and report are
`tools/oracle_frontend_text_setup.py` and
`analysis/frontend/settings-typewriter-original/text-setup-original.json`.
`frontend_text_setup_tests` checks the actual loaded shared fields and exact
runtime font, scale word and shadow decisions.

`tools/oracle_frontend_settings_text_layout.py` separately executes the complete
original layout and final-quad corridor for the seven actual labels:
`13F0E0 → 13EF40 → 13F700 → 1F490 → 1F3A0`. It uses the actual localized
strings, actual RGB and bounds, real conduit-12 font/glyph records and actual
shared0.3 scale. The canonical projected bounds, full reveal and settled alpha1
are explicit inputs. Ready font-cache lookup/pixel services and a fixed atlas
origin `(8,8)` are supplied. The final graphics call records vertices. The
original instructions in this corridor are unchanged.

The result is seven cases,440 glyph callbacks and440 final foreground quads.
The report retains raw callback words, raw final vertices, each glyph header,
executed routine counts and input identities:
`analysis/frontend/settings-text-source-audit/glyph-layout-original/original-layout.json`.
`tools/freeze_frontend_small_text_layout_fixture.py` exports the final corners
to `tests/fixtures/frontend_small_text_layout_original.tsv`. Atlas UVs are
retained as integer offsets from the supplied original glyph origin so the
test can map them into the independently packed native atlas without using a
captured address as a production input.

Original13F700 places the glyph origin with scaled bearings and kerning, but
clips its **unscaled** source width/height. It first trims right/bottom, then
trims left/top while retaining the source offsets. Only positive float source
width/height reaches the callback. Original1F490 uses `cvttss2si` to truncate
the source offsets and extents to signed integers;1F3A0 takes signed16-bit
source U/V offsets and signed32-bit dimensions. It then scales those integer
dimensions to produce the final screen endpoints. The clipped screen origin
is unchanged. This differs from clipping a previously scaled screen rectangle
and dividing the clip distance by scale to obtain fractional source UVs.

Eight actual diagram glyphs discriminate the clipping rule. For example,
label1 glyph32 (`o`) has original screen X541.69970703125 and source width
5.30029296875. The original truncates that width to5 and emits a1.5-pixel
screen width at scale0.3. A scaled-rectangle clip would preserve the full
seven-texel glyph and incorrectly emit2.1 pixels. Labels1,3,4,5,6 and7 contain
these discriminating glyphs.

Original13F0E0 recomputes each baseline from the unscaled line index:
`top + (ascent + (leading + descent + ascent) * line_index) * scale`.
Repeatedly adding the scaled line height can change later float words.
Original submission also requires `baseline < bottom`; merely testing the
line's top is insufficient. The seven retained labels all fit vertically and
therefore do not independently discriminate that final guard.

The exact final-quad test also discriminates an iterator detail in label7's
`Nanotech`: original13F5A0 assigns `previous_token` from the old token, so
the first **two** characters of a fresh iterator use previous-character0.
The serialized conduit-12 `N,a,-1` pair therefore does not apply to this
line's second glyph. Its original X is536.0 (float word `44060000`), while
the conventional previous-character loop produced535.70001220703125
(`4405ECCD`). Original13F700 constructs a fresh iterator per drawn line.
`tools/oracle_frontend_small_text_iterator.py` independently records original
EDI/EBX at122570 for all seven labels, including `(0,N),(0,a),(a,n)` at
original caller13F8C5. Its unchanged-original evidence is
`glyph-layout-original/iterator-trace.json` in the same evidence directory.

Measurement13EF40 uses a different iterator retained by13F0E0 across its
line scans. A fresh-span native measurement helper does not model that
complete state merely by applying the draw iterator's second-character rule.
The final-quad fixture checks the seven actual labels' wrapping and emitted
geometry, but does not prove arbitrary measurement widths, justification or
boundary-sensitive wrapping. No claim of a general text-iterator replacement
follows from this bounded draw correction.

The production test `frontend_small_text_layout_tests` takes a maps directory
and the original TSV. It uses the real loaded screen19 content and repeatedly
draws at cumulative50ms clocks until the source prefix helper reports all
seven labels fully revealed. It then checks the440 glyphs in source order,
all2640 triangle vertices, exact float words for corner positions and mapped
UVs, and source packed color channels. Source metadata and glyph counts are
also checked. Compilation and execution are performed separately by the
parent task; the existence of this test is not a passing result.

The fixture does not prove widget projection, typewriter allocation/lifecycle,
font-cache residency, complete atlas packing, GPU raster precision or full
framebuffer parity. Other glyph strings, positive-delay text and arbitrary
wrapping/clipping contexts require their own original comparisons.

Pinned identities at fixture freeze:

| Input | SHA-256 |
| --- | --- |
| Original XBE | `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d` |
| Original mainmenu.map | `1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b` |
| Original final-quad report | `a254430b2627829220f11df7d25cd7897485d9b32d72f36335af107c371fe05b` |
| Final-quad TSV | `e0c0c0287310aba7bc87b2d8bd421201e927004a1ad02281140499ea6da5f346` |
| Setup TSV | `fddba8da68ff9dd29676d539b64c3241d330c64822f08ec2a58c8e23b3bd118b` |
| Original iterator trace | `9fcfbb30bc17627c9f0db97c2f19990700ff01a930041f3dd4e422f304fa589e` |

Native delivery, 2026-10-09: all eight focused frontend tests passed in
`analysis/frontend/settings-typewriter-original/native-tests-second.log`.
The comparison covers 527 original draw prefixes, 92 original text setups,
and all 440 glyphs / 2640 triangle vertices in the seven real Settings labels.
The first run's `Nanotech` second-glyph kerning failure remains in
`native-tests.log`; production iterator handling was corrected against the
unchanged fixture before the passing run.

Seven native captures in `analysis/frontend/settings-typewriter-original/native/`
passed with stable pinned inputs and zero Vulkan validation errors: partial
typing at 16:9, complete Settings at 4:3/16:9/32:9, live resize from 32:9 to
1000x600, keyboard at 16:9 and Appearance at 32:9. `runs.json` contains exact
commands, source/asset/binary hashes and capture clocks. These show the actual
native runtime; they are not original-framebuffer equality measurements.
