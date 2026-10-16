# Original TU1.5 unit-time result conversion

Generate into a fresh path with `tools/oracle_campaign_unit_animation_time_tu15.py`
and the owned English TU1.5 executable. The SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Copy the JSON to `tests/fixtures/campaign_unit_animation_time_tu15.json`.

The192 cases execute whole unchanged `2AD170`. Declared completed boundaries
supply parsed arguments and `11DD90`'s query seconds, then capture `211DA0`'s
published result. Original float32 multiplication, x87 nearest-even conversion,
32-bit subtraction/clamp and short publication execute without patches.
Cases include ties, negative/large values, signed zero, subnormals, infinities
and NaNs. The native comparison checks the published16-bit value, not the
whole evaluator ABI, floating-point status or source HS thread state.

`original_campaign_unit_animation_time_tu15` compares the production native
post-query projection with those values. The separate actual-world controller
test executes real unit lookup and custom-time query on an authored unit.
Neither test establishes all camera/visibility scheduling, full scenes,
GPU/audio or converted MCC acceptance.
