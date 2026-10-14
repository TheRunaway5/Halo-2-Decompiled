# TU1.5 combat action and attachment oracle

Generate the ignored local fixture with an owned English TU1.5 executable:

```sh
.tools/python/bin/python tools/oracle_combat_action_tu15.py \
  --xbe '/path/to/owned/u5English/default.xbe' \
  --fixture tests/fixtures/combat_action_tu15_original.json
cmake --build build --target combat_action_tu15_tests
build/combat_action_tu15_tests tests/fixtures/combat_action_tu15_original.json
```

The executable must have SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The oracle executes unchanged complete F0550/D23B0 bodies in 20 declared
movement/parent cases, then unchanged complete120740 in 64 quaternion,
translation and signed/zero/nonunit scale cases. It supplies input backing,
not child-service results or gameplay admission. All functions finish through
their original RET; an unexpected callee or incomplete path fails.

The native comparison checks instruction order, every GP and SSE output,
defined arithmetic flags, and every byte of the retained input/output banks.
This is arithmetic and ABI evidence. It does not establish Xbox framebuffer,
complete gameplay, animation timing or particle visual parity.

The native combat module is regenerated independently from the pinned image:

```sh
.tools/python/bin/python tools/generate_combat_action_tu15.py
```

The generated native C++ contains complete authenticated bodies, with explicit
required-owner boundaries. Production does not decode or execute Xbox code.

