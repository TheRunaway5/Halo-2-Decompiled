# TU1.5 patchy-fog depth fixture

Generate the private TSV and provenance manifest from an owned, unmodified
TU1.5 executable:

```sh
.tools/python/bin/python tools/oracle_bsp_patchy_depth_tu15.py \
  --xbe /absolute/path/to/TU1.5/default.xbe \
  --output tests/fixtures/tu15_bsp_patchy_depth.tsv
```

The executable hash is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Each row contains six hexadecimal words: start, end, near, far, original
quantized-program success, and original minimum W (zero when depth testing is
disabled). The four input words and final W are binary32 bit patterns.

The 265 cases execute unchanged TU1.5 `3A5B0`, its output tail, and
`27500 -> 350D0`. GPU submission services are suppressed; no arithmetic or
guest instruction is replaced. Cases cover range rounding, low-byte rescale,
near-plane disable, finite clip distances, and rejected quantized ranges.

The generated TSV SHA-256 is
`3006d5f8ecf15447bd87ffd9987ecf738d3adf0f4a3700f61771978ad6011334`.
`tu15_bsp_patchy_depth` checks the production producer against every row and
skips only when this private fixture is absent. The independent
`tu15_bsp_patchy_depth_gpu` always runs and exercises the production fragment
shader's near/equal/far admission and disabled-depth branch.
