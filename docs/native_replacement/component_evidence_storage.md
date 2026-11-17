# Component evidence storage

`tools/native_component_evidence_storage.py` compacts component JSON copies in
an unfinished frozen cohort. It changes storage only. It does not rerun tests,
change their results, or complete an engine/runtime qualification.

The coordinator calls `compact_component_evidence(output)` after the component
runner returns and before its whole-output metadata budget check. It requires
`HALO2_TEST_SCRATCH`, that same live scratch owner in the cohort receipt,
`passed: false` and the exact `components` phase. Passed,
failed, cancelled and other historical/terminal phases are refused. Historical R21
evidence remains untouched.

Every input JSON must match its original `cmake/report.json`
`retained_small_reports` size and SHA-256 receipt. The runner report stays
byte-identical. Beneficial gzip payloads, originals and a provenance manifest
are staged in owned scratch, with exact decoded byte equality checked before
any output changes. The projected whole-output budget includes the new
manifest. A failed publication restores original component copies with
exclusive creation. If another owner recreates an original or replaces a
published target, rollback preserves that external file, stores any displaced
original in a bounded `cmake/component-evidence-recovery-*` directory and fails
the cohort with its recovery path. It deletes only published files whose inode
and content hash still match this operation.

Ordinary failure restores the original raw metadata and removes this call's
published files. External-conflict recovery instead retains the external files
and authenticated copies of displaced originals, with explicit per-file bounds,
the failed output's budget status and a manifest path/hash in the exception.
An initially over-budget raw state can remain over budget after failed rollback;
it is failed evidence, never passing retention acceptance. No successful call
can bypass the unchanged aggregate check and reserve.

The independent `cmake/component-evidence-storage.json` manifest records each
logical path, encoded path, original receipt index/hash, raw byte count/hash,
stored byte count/hash, and unchanged limits. A repeated call while the cohort
is unfinished validates the existing manifest and all encoded and decoded
identities. It does not recompress the files.

The limits remain 1,048,576 bytes per stored metadata file, 8,388,608 bytes in
aggregate, with 131,072 bytes reserved. The CMake runner still applies its own
original per-report and aggregate limits before compaction; this helper cannot
recover a report rejected by that earlier limit.

Consumers must resolve retained component JSON as a logical path through
`native_metadata.read_bytes`, `load`, or `digest`. The unchanged runner receipt
still names the original plain path, which may now resolve to `.json.gz`.
Direct `Path.read_text()`, `Path.exists()`, or raw-file SHA-256 calls on that
plain path do not provide this compatibility. The aggregate and checkpoint
readers already use logical resolution; the coordinator's coverage read also
needs it after compaction.

Focused tests cover exact receipts, ordinary logical reads, nonbeneficial
payloads, an initially over-budget batch, unchanged limits, corruption, unsafe
paths, input changes during staging, historical refusal, idempotence and
publication rollback. The separate replay uses all eight genuine completed
R21 metadata records in disposable scratch and compares both production
readers before and after compression. It establishes storage/reader equality,
not new gameplay, rendering, timing or native-closure evidence.
