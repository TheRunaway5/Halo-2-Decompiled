# Original TU1.5 object shader-function slots

Generate a new JSON with `tools/oracle_object_function_slots_tu15.py --xbe`
pointing to the owned English TU1.5 executable and `--output` pointing to a
fresh path. Copy it to `tests/fixtures/object_function_slots_tu15.json`.
Image SHA-256: `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

381 cases execute whole original `10FC30`, `10FB70`, `10FD50` and their direct
callees with no stubs or instruction patches. Each compares the complete
0x300-byte slot/link bank and both0x200-byte declared object bodies; lookups also
compare the found flag, value and slot index. Cases cover duration rounding,
negative elapsed/durations, missing/cleared values, more than32 keys, tied and
distinct creation times, two full object identities and invalid sentinel inputs.

Object/header/global storage, clock and stack are explicit fixture inputs.
This comparison does not establish original birth, complete HS threads or
rendered pixels. The actual-world regression separately executes the original
constructor/map reset, creates the authored Banshee, advances real source phases
and verifies interpolation, object flag, renderer samples, coupled checkpoint,
clear and32-slot replacement.

The native renderer borrows the original getter's values. Its sample map has
zero native duration and is refreshed from the source owner; no second native
timer or eviction policy advances the shader input. Complete cinematic
playback, every material/shader pixel and converted MCC acceptance remain open.
