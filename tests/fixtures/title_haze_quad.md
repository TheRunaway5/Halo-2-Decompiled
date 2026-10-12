# Original haze quad fixtures

These records execute unchanged `default.xbe` callbacks `00026CA0` (noise) and
`00027520`, including its `000350E0` call (direct composition). No instructions
or uploads are intercepted. Each callback executes modes 0..4 at the four
authored corners. Both sets have 512 cases spanning six viewport sizes.

`title_haze_quad.inputs.bin` has a little-endian record count followed by 37
float words per record: eight affine float4 rows, vertical scale, noise width,
noise height, near clip and far clip. Its expected records are twenty float4
attributes, position first and then four texture-coordinate streams. The first
eight row sets come from original persistent haze calculation fixtures; the
remaining cases use reproducible finite inputs to exercise float ordering.

`title_haze_direct_quad.inputs.bin` uses the same record size. Words 0..3 are
the two fog distances and noise-target dimensions; words 33..36 are viewport
dimensions and near/far clip. Other input words are unused. The title's verified
direct branch uses the default 24-bit maximum depth argument and enables both
depth-gradient attribute streams. The expected record layout is identical.

Both native constructors match every expected attribute byte. These fixtures
do not reconstruct the live camera or random history, other sorting branches,
or complete frontend pixels. Unused depth/clip fields are outside this check.

XBE SHA-256: `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.

Noise input SHA-256: `fbb3329c88baac06fd82b1fe81bf048f135257588123a59f8d123b617bd692da`.

Noise expected SHA-256: `9efc7d0fc8bab7ccc337b275ff2376b8c1d90c77898d68c1b878c0ea9f83f8c3`.

Direct input SHA-256: `4c240073d40b58e6e68c22c41d5ece7d5529c33f0cfd53b838b0e12e68039c35`.

Direct expected SHA-256: `56f55e5b852d846b757e3696f461b82113eb08ccf634b1c749f17661d06d4496`.
