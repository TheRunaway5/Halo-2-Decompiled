# Edit Profile model visibility

Screen193 previews the saved profile under focus. Moving between profiles does
not commit the focused profile. Create New Profile hides both player-model
widgets, even after an Elite profile was focused.

The original screen193 constructor `2B1A1C` creates screen `0xC1` with vtable
`45B0B8`. Its update slot `+0xC` resolves to `24A886` in the original executable,
SHA-256 `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.

The [offline oracle](../tools/oracle_frontend_profile_preview.py) executes the
unchanged update and its focus/visibility callees:

- `24A855` reads the focused list datum and treats index `NONE` as Create New.
- `24A763` finds the focused saved profile in the list's profile records;
  `24A828` copies that profile into the preview cache. Create New retains the
  previous cached appearance but does not display it.
- `24A886` passes the focused appearance to model children `(7,0)` and `(7,1)`,
  with types0 and1 respectively.
- `24A7EC` clears visibility when Create New is focused. Otherwise `2B10A3`
  sets each model's visibility by comparing its type with the appearance's
  signed species byte at `+4`.

The [frozen fixture](../analysis/frontend/profile-regression/profile-preview-original.json)
contains six executed cases: Spartan, Elite, Create New after Elite, Spartan
again, an unsupported species, and Create New after that unsupported species.
It records both original child calls, resulting visibility bytes, cached
species, and function-entry counts. Its SHA-256 is
`e8b949577074d576e1d71842ca1c4fba6f106bdaf4f40f9995b32137f02b9e59`.

The supplied boundaries are the record-pool lookup, list-record virtual getter,
child lookup, unrelated base/list/text updates, and appearance-color setters.
The focused-profile and visibility branches execute unchanged. This fixture
does not establish profile appearance colors or a pixel match for the model.

The native `MenuState::focused_profile_species` accessor reads the focused
screen193 entry without changing the active profile, signed-in profile, or
preferences. `FrontendPainter` uses that value only for screen193 player model
submissions. Create New submits neither model. Other screens retain their
existing species policy.

The real MenuState/painter regression creates Spartan and Elite profiles through
the title/profile-name/customization flow, signs into the Spartan, and reaches
screen193 through Main Menu -> Settings -> Player Profiles. It checks focused
species, Create New hiding, returning focus, unchanged committed profile and
exact preference-file bytes, and the existing commit-on-accept behavior.
The initial native implementation failed when the Elite gained focus, still
submitting the committed Spartan. The retained red and green logs are
[here](../analysis/frontend/profile-regression/profile-preview-red-tests.log) and
[here](../analysis/frontend/profile-regression/profile-preview-green-tests.log).

A later, separate [placeholder visibility proof](../analysis/frontend/profile-row-appearance/PREVIEW_PLACEHOLDER_SOURCE_PROOF.md)
executes the original seven bitmap factories and `24A886 → 22EE47` consumer.
Direct bitmap6, `E5EE03CE`/`profile_picker\unknown`, is visible only for Create
New and hidden for every saved focus, independently of model-species admission.
The native bitmap6 binding is corrected after semantic RED6.50s and GREEN5/5;
the other six authored panel/decorative bitmaps and this model policy remain
unchanged. That slice does not extend the proof to full193/194 transition
chronology, mixed widget/scene sorting or synchronized framebuffer parity.
