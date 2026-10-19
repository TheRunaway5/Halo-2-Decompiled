# Campaign text control fixture

`campaign_text_control_tu15.tsv` contains the results of unchanged English
Xbox TU1.5 function `1426D0` for its six recognized escape characters and the
NUL fallback. Columns are input byte, returned code point and recognition byte.

Executable SHA-256:
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The original execution, instruction-byte checks, command and memory receipt are
in `analysis/opening-cinematic-20261015/title-control-original-v607` and
`title-control-original-guard-v607`. The aggregate comparison limit is60MiB,
the measured ceiling64MiB, with zero swap. No instructions or callee returns
were substituted. The nonzero unknown-character diagnostic/timer branch was
not executed.

The opening map's High Charity and Cairo titles use `|n`, which returns CR.
This fixture establishes control decoding; it does not certify inline
justification state, full font metrics, screen placement or Xbox pixels.
