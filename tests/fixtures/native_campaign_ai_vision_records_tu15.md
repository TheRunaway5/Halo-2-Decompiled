The required `native_campaign_ai_vision_records_tu15.json.gz` fixture is an
independent physical i386 execution of unchanged English Xbox TU1.5 instructions,
SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
It supplies declared retained inputs and actual caller-private output storage.
It patches no instructions, supplies no callee outcomes, and is offline only.

The four call-free owners are26EEF0,1ECCC0,1D0400 and272970 (99 sites).
404 completed calls and53 lazy read/store rejection prefixes compare results,
ordered reads, write interleaving, complete retained banks and unchanged FP.
Coverage includes all flag bytes, signed offsets/counts, inherited first-positive
blocks, duplicate/player NONE cases, unaligned output aliases and clock aliases.
The typed attachment value is qualified on completion; private output/fault
transport and complete26E040 engine behavior are outside this component.

Proof and source pins:
`analysis/campaign-native-kernels-20261028/ai-vision/records-component-proof.json`.
Independent repeats match every compressed fixture byte. GCC and Clang pass,
Clang ASan/UBSan instruments the actual semantic object, and seven semantic
mutations are rejected. The unsigned-offset mutation genuinely reaches missing
backing; the test reports that exact read failure instead of aborting.

Regenerate through `tools/run_test.py` with
`uv run --with capstone --with unicorn python tools/oracle_native_campaign_ai_vision_records_tu15.py`
and its explicit `--output`/`--report` paths; use `--verify` for exact repeat.
Run `tools/validate_native_campaign_ai_vision_records_tu15.py` through the same
wrapper for disposable official CMake GCC/Clang/sanitizer/mutation validation.
Original binaries, raw banks and compiler products are removed with scratch.
