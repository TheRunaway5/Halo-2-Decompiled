# TU1.5 WORLD effect location admission

`effect_location_world_tu15.original.bin` contains 65,536 bytes, one boolean
result for each unsigned node word in ascending order. The unchanged English
Xbox TU1.5 `17DCE0` iterator produced every result, with mode0 as pushed by
the original `17D210` part-dispatch caller. Input pools, cursor and stack are
declared fixtures; no instruction or callee is replaced. Only iterator/stack
writes are admitted. Geometry, the audio backend and full sound dispatch are
outside this isolated fixture.

Image SHA-256: `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Fixture SHA-256: `5fa3e101c6ad2c19a4232c1053990262486cc7a4f00aaa5a92aa6c513b04bec0`.
WORLD mode admits 32,769 words: unbound `FFFF` and nodes without the FP bit.
Separate WORLD nodes sharing a marker name remain separate locations.

Regenerate under owned scratch with:

```sh
python3 tools/run_test.py -- .tools/python/bin/python tools/oracle_effect_location_admission_tu15.py \
  --xbe /path/to/English/TU1.5/default.xbe --output '@HALO2_TEST_SCRATCH@/original'
```

The tool checks the image, both original caller `push 0`/call instructions,
all node words in modes0/1/3, mixed linked chains and the write boundary. Its
JSON/stdout retain counts and hashes; generated binaries disappear with scratch.
The separate native UDP/viewport late-audio regression exercises the real
registered weapon's WORLD/FP split and actual SDL sound submissions.
