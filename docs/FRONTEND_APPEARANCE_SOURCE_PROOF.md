# Frontend profile appearance binding

The original customization preview takes its appearance from the edited profile,
not the biped's authored constructor colors. The verified source path is
`2C8D7C → 2B10A3 → 7F790(NONE,false) → 1A06A0`; `2B0F76` writes the four
resolved colors into each visible biped. `2B10A3` tests the appearance's signed
species byte before making those writes. `2B102F` separately copies the emblem
dword at appearance+5. Held weapons retain their own object appearance.

`FrontendAppearance` retains the four signed color indices, species, foreground,
background, flags and the final byte consumed by that emblem dword. New profiles
use original `1A0750`: colors `[10,0,0,10]`, species0, foreground/background0 and
flags0. The 18 RGB triples come directly from mainmenu's `matg+160/+164`; they are
not fitted to an emulator screenshot. `1A06A0` returns literal white for invalid
signed indices. The UI path disables team and default-color overrides.

The appearance menu edits a temporary settings copy. Original color-page update
`2CB350`, species update `2CB4E2`, and emblem update `2CB174` write the focused
choice into that copy while browsing. Their animation bit1 gates those writes.
Original child constructor `2C87BE` saves a settings snapshot and `2C87FE`
restores it for B/back. Foreground-page X (`2CB1D4`) toggles bit0 of flags;
background-page X does not. Accept handlers retain the selected edit and return
the child window. The native state now keeps the same distinct working and saved
appearance banks, snapshots each child entry, and commits appearance when profile
editing ends. Preferences remain the host's own file format; this is not an Xbox
save implementation. Standalone diagnostic47 entry commits on leaving its page;
the normal profile38 route commits on leaving profile settings. Successful
profile rename also ends the working edit, following original `238742`; accepted
appearance fields move with the renamed profile rather than remaining under the
previous host preference key.

The original flags3 profile-name group transforms the primary color through
`1318D0`, replaces saturation/value with original constants45E19C/45E198, then
runs `131A00`. `frontend_appearance_name_color` uses the existing source color
conversion helpers with those exact constants. It does not use armor draw colors
that have already passed through the byte-packing boundary.

`UiDraw::Scene` carries the appropriate appearance to the native runtime. Screen193
uses the focused saved profile; Create New has no preview. Screen30 uses the
signed-in slot's profile, independently of an unrelated profile selected for
editing. Screens47/50 use the edited appearance. The runtime applies colors only
to that biped's material range and flushes the resulting shader constants before
presentation. Shared `apply_object_colors` retains the original
`131ED0 → 131BB0` byte-packing boundary, copy-on-write pass banks and the existing
shader-animation rebind behavior.

## Evidence and verification boundary

`tools/oracle_frontend_appearance.py` executes the pinned original Xbox
instructions unchanged. It retains:

- 60 species/color cases, including hidden unknown species and invalid indices;
- three actual appearance47 parent updates with the retained mixed palette;
- 192 focused-choice update cases, including animation bit1;
- 24 original cancel/toggle cases and 30 accepted color/species writes;
- all 18 original name-color conversion cases;
- literal palette words and the original new-profile appearance bytes.

The supplied seams are ready tag-global pointer relocation, widget lookup, list
focus, unrelated base update/text expansion, the emblem renderer, window-return
virtuals and final object-color sinks. They are listed in the fixture, and are not
claimed to execute in this oracle. Fixtures are
`analysis/frontend/profile-regression/frontend-appearance-original.{json,tsv}`.
The original XBE SHA-256 is
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.

The focused native test compares all color/name/palette words and preview bytes
with those original outputs, then follows normal title → profile creation →38 →47
and child pages to check focus, cancel, accept, flags, persistence and signed-in
binding. It also checks both authored preview model material banks and color
reapplication. The recorded red regressions were the native primary selector
focusing 0 instead of original default 10 and successful rename discarding a
pending accepted appearance edit. The focused appearance test passes after
both corrections, including save/reload of the renamed appearance.

These checks establish appearance data and shader-bank propagation. They do not
establish a complete framebuffer match, all UI transition timing, original save
file compatibility, every multi-controller profile-edit lifecycle, lighting/pass
selection, emblem shader composition or arbitrary model consumers outside the
explicit menu contexts above. Those rendering and presentation paths have their
own evidence and tests.
