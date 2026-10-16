# Original TU1.5 timed screen-effect banks

Generate into a new path with `tools/oracle_campaign_screen_bank_tu15.py --xbe`
pointing to the owned English TU1.5 executable and `--output` pointing to the
new JSON. Copy it to `tests/fixtures/campaign_screen_bank_tu15.json`. The image
SHA-256 is `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

The 864 cases compare complete 0x3FC-byte banks and 16 target identity/flag
bytes across start, reset, set, set2, update, capture acknowledgement and stop.
Whole original HS command wrappers execute with declared parsed-argument and
published-result boundaries. Whole original `1FD00`, `1FCE0` and `1FCB0` execute
without external callees. No original instruction is patched. The supplied
double clock, bank backing and target identity are explicit fixture inputs.
These comparisons do not establish a complete HS thread, renderer frame, GPU
allocation, copy or pixel result.

The production native command-store seams and three complete compiled original
owners are compared by `original_campaign_screen_bank_tu15`. The separate
`native_opening_source_screen_bank` check uses actual original arena storage,
update, acknowledgement, stop and source checkpoint/repeat. Its acknowledgement
is a declared completed-capture boundary; production calls that owner after the
Vulkan consumer reports its copy. The source-integrated graphics run records
that actual callback separately.

The original complete timed update retains its other screen-control branches;
this fixture exercises crossfades with the other timed controls clear. It does
not establish authored DOF, all effects, all scenes, sound/subtitles, skip,
handoff, Xbox pixels or converted MCC acceptance.
