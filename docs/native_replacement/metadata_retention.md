# Frozen-cohort metadata retention

`tools/native_metadata.py` stores the source manifest, runtime counter reports
and their summaries losslessly as deterministic gzip. Existing plain JSON
reports remain readable. The coordinator's source and runtime hashes continue
to identify the original decoded bytes, so compression does not change source
identity, counters, ownership or checkpoint comparisons.

Every retained metadata file remains limited to 1 MiB. A compressed stream
that exceeds that bound is split into bounded parts and a small authenticated
index. The index records each part's hash and size, the complete compressed
stream hash, and the original byte count and hash. Runtime reports retain
their original 1 MiB decoded limit. Source manifests have a separate bounded
64 MiB decoding limit. Ambiguous plain/compressed inputs, corrupt or unsafe
parts, oversized decoding and retention-budget excesses are rejected.

The aggregate limit remains 8 MiB, including indexes and compressed parts,
with 128 KiB reserved for subsequent coordinator metadata. Checks also run
after component and engine stages and before successful completion. No
component target, test selector, runtime admission or capture control changes.

`native_metadata_storage` exercises byte/hash preservation, chunk corruption,
unsafe names, decoded limits and atomic budget refusal.
`native_metadata_retention_replay` borrows the historical R14/R17 reports
read-only through `HALO2_TEST_ASSET_SOURCE_ROOT`, compresses disposable copies,
and checks both logical old filenames and new physical compressed paths with
the actual aggregate, checkpoint and runtime readers.

R17 remains a failed retention checkpoint. Its six persisted runtime reports
cannot establish the two missing workloads. The eight-report budget fixture
explicitly uses the two corresponding completed R14 reports for those slots;
it proves storage capacity only. Separately, all eight authentic R14 reports
prove that compression leaves the complete reader results unchanged. Neither
fixture launches an engine or supplies missing R17 counters.

The initial replay reduced R17's 2,619,162-byte source manifest to 968,457
bytes while preserving its SHA-256
`87757b203cfa9c403bcb7032cf0830456788a5c9974003e95353d5be93e4330b`.
The eight-report storage fixture uses 5,851,271 bytes and leaves 2,406,265
bytes beyond the unchanged reserve. Historical reports, original references
and installed runtimes are preserved; disposable copies are removed by
`tools/run_test.py`.

The registered official gate passed both tests in 22.10 seconds, including
the eight storage unit checks and actual historical metadata replay. Its
bounded evidence is
`build/test-results/native-metadata-retention-official-20261109/cmake/evidence/native-metadata-retention.json`;
the final receipt is
`build/test-results/native-metadata-retention-closure-20261109/report.json`.
All owned source/build/replay copies were removed. This repair does not
change R17's failed status or establish a new runtime checkpoint.
