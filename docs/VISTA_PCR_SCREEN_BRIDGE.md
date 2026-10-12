# Vista PCR screen bridge

The PC postgame consumer is now connected to the native painter through
`VistaPcrScreenOwner`. It accepts an explicitly retained `VistaPcrResults` bank;
the painter never creates a report or ends a game. Screen16 and lobby-review185
keep their separate input and draw rules. All six panes bind their actual PC
list skin and group-zero player skin, including source column formatting,
opponent selection, medal visibility, appearance colors and rank pages.

The authority is the supplied `Halo 2 Vista installer/halo2.exe`, SHA-256
`33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109`, and its
`maps/mainmenu.map`, SHA-256
`eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44`.

Original consumers were executed unchanged with explicit external services:

- `oracle_vista_pcr_screen.py`: 1,195 row/input/draw controls.
- `oracle_vista_pcr_appearance.py`: 568 descriptor, color, rank, frame and
  visibility controls; 1,530,220 original instruction visits.
- `oracle_vista_pcr_chrome.py`: 1,242 heading, active-pane footer, empty-report
  and voice controls; 204,572 original instruction visits.
- `oracle_vista_pcr_appearance_assets.py`: 146 calls and 90,381 original
  instruction visits using actual imported tag records. This resolves the
  nine team colors through matg+16C, mulg universal[0]+10, and the 50 signed
  rank ranges through the selected matg interface, wgtz+4 and wigl+158.

The native source consumers match these fixtures. Appearance and chrome suites
also pass ASan/UBSan with leak detection; the three CMake PCR targets pass
CTest. The transition asset test passes 69 checks covering all authored fields,
219 bitmap pages, complete mip hashes, palettes and rank records.

`frontend_vista_pcr_painter_tests` loads the actual PC assets and the unchanged
original retained output fixture12. It verifies all six panes of both screens,
each reached row appearance owner, retained-bank preservation, name glyph
submission, and separate Done/Close actions. The optional final argument is a
native Vulkan capture directory. `tools/capture_vista_pcr.py BINARY OUTPUT`
records stable input hashes, twelve 1280x720 captures and Vulkan validation
results. Current captures and reports are under
`analysis/frontend/vista-session-flow-audit/pcr-native-20261011/captures-verified`.
The capture executable directly compiles the changed bridge objects and links
the existing read-only native renderer libraries; it is separate from the
application and uses explicitly declared identity/voice fixture services.

Two boundaries remain visible. The actual PC emblem definition and its complete
16-byte appearance survive, but its native `spas` pass remains unresolved; no
Xbox emblem is substituted. PC16 voice icons require the genuine retained-XUID
voice owner. Missing owner data is reported through `UiDraw::rendering_gaps`;
PC185 independently hides that bitmap, as its original routine does. The
screenshots establish native asset/consumer composition and Vulkan delivery,
not original framebuffer equality, a live game result, a complete session
transition, or Windows/Xbox execution.
