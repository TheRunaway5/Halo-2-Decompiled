# Original four-player sign-in presentation

Screen30 previously painted its complete authored pane at once. An occupied
player therefore showed the disconnected controller and join/connect prompts
alongside the selected profile; empty slots retained Profile/Gamertag captions.

The authority is `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`,
and `ui\screens\game_shell\4way_multiplayer_signin\4way_signin_main` in
`mainmenu.map`. Original screen update `252554` is the third vtable method at
`45A490`. It selects each controller's widgets by their original child indices.
Text indices begin at2 after the header/button-key widgets; model and bitmap
indices begin at0.

The original policy is:

- A controller is occupied only when connected and player-slot flag4 is set.
  Its profile name, Profile caption, emblem and selected species model become
  visible. Its disconnected controller and join/connect prompt become hidden.
- An occupied controller's separate continue widget binds `1300028C`,
  `PRESS A TO CONTINUE`. Gamertag fields require the occupied slot's flag5.
- An unsigned controller shows `0F00028A`, `PRESS A TO JOIN`, when connected,
  or `1100028B`, `CONNECT CONTROLLER`, when disconnected. Profile/continue,
  emblem and model widgets are hidden.
- Every disconnected controller uses bitmap frame0. Connected unsigned
  controllers use frame1 for alternate gamepad type2, or frame2 for standard
  type1. Original `2B0A14` checks the real bitmap's three-image count.
- Original `2B10A3` selects the Spartan or Elite model from the profile species
  byte. It hides both when no occupied profile is supplied.

`tools/oracle_frontend_signin.py` executes unchanged `252554`, `22EE47`,
the visibility prefix of `2B10A3`, `2B0A14` and `190262`. Widget lookup,
controller/profile retrieval, text loading, model appearance rendering and
base animation are declared environment seams. The supplied profile occupies
the original0x1E0-byte buffer. The oracle freezes256 cases covering all four
slot indices, connected/signed/Live flag combinations, both model species,
and gamepad types0 through3. Its TSV SHA-256 is
`7cc0424c3d1ad12da1d7985497b519eb92921f6f5b9d612c929932e3d962ced1`.

The production regression enters Title→Choose Profile→Profile Name→profile
customization→main menu→Split Screen. Before the fix it failed because player1
still drew an empty-controller bitmap. After the fix it verifies three
disconnected-controller quads, one signed-in Spartan scene and all24 authored
text visibility decisions. The individual text observations supply an ordinary
glyph and isolate each widget's bounds while retaining its font and animation,
so unbound private profile-name fields cannot conceal an incorrect visibility
decision. Logs are `signin-red-tests.log` and `signin-green-tests.log`.

The original normal-entry screenshot `original-model-fourway.png` independently
shows the occupied first slot's profile name and continue prompt, with three
disconnected controller slots. The native implementation currently has one
signed-in local profile. It uses that actual state rather than the existence of
saved preferences; a direct screen30 preview shows no invented occupied model.
Additional physical controller joins, Live identity state, profile emblem
rendering and a complete framebuffer comparison remain separate work.
