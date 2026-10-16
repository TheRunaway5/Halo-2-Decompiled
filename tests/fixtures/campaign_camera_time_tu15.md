# Original TU1.5 scripted camera timing

Generate a fresh JSON with `tools/oracle_campaign_camera_time_tu15.py --xbe`
pointing to the owned English TU1.5 executable and `--output` pointing to a
new path. Copy it to `tests/fixtures/campaign_camera_time_tu15.json`. The image
SHA-256 is `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

The 192 query cases run unchanged whole `2B2930` against declared director and
clock banks. Only `211DA0`, the published-result boundary, is captured. The
comparison checks the published short, including large and negative counts,
variable tick durations, infinities and NaNs. It does not claim complete HS
thread, caller ABI or floating-point status equivalence.

Another 192 cases execute the unchanged game-frame interval
`13AB94..13ABBB`, stopping before `13ABC0`. They compare the complete 0x78-byte
director bank and unchanged 0x24-byte clock bank. This covers the original
wrapping decrement/clamp and float32 FOV elapsed addition. Stack backing and
the popped caller EBX are explicit fixture inputs. This interval comparison
does not establish the surrounding game-frame owners or caller ABI.

`original_campaign_camera_time_tu15` checks these production native value
seams. `native_opening_source_cinematic_camera` separately executes the real scripted
camera actions, authored animation/resource and BSP cluster owners on the
retained source world. It checks source camera matrix motion, scenery timing,
FOV storage, checkpoint repeat and stop. Neither test proves the full opening
VM, rendered camera pixels, audio or converted MCC acceptance.
