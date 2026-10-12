# Promotion output naming erratum

The frozen `sound_submit.hpp` and submit integration document use `sound` / `promoted_sound` for the DWORD output of `128610`. That name is misleading: the output is a **numeric promotion selector**, not a new sound datum. The implemented raw argument and bit transfer remain correct. Preserve the frozen files and interpret that callback output accordingly when providing the complete live owner.

Original `128610` writes its selected ESI integer to caller output at `1286EA`; the `snd!+0C=-1` branch writes `FFFFFFFF` at `12874E`. Its u5 counterpart is `1299D0`, with the same complete normalized body. Original `21F220` passes the original sound in ECX and the address of its saved sound stack DWORD to that owner, then forwards the overwritten DWORD as the fourth `127750` argument. The first factory argument remains the original sound datum.

Original `127750` compares argument four against `NONE` at `1279C2`, selects `21B020` at `1279C9` for a promotion selector or `21B060` at `127A0F` for the ordinary path, and stores its low byte at source `+A0` at `127A31`. A complete native promotion provider must write this selector. It must not return or choose a replacement sound datum.

This correction makes no new admission, factory, playback or hot-controller execution claim. The prior 2,574 cold submit comparisons remain unchanged; their availability gate prevents this provider from being invoked. The new erratum checkpoint separately pins the prior frozen header/document and the whole paired original submit/promotion/factory listings, checked against both original images. No prior immutable file or production behavior was modified.
