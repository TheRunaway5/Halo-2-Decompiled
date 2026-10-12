# Private physics snapshot asset boundary correction

The physics source snapshots under ignored `analysis/` are private validation trees, not code-only distribution packages. Source-v9 inherits binary test fixtures from its immutable parent, including `tests/fixtures/frontend_ui_dialog1_original.bc3` and `.rgba`. The earlier statement that all original assets were excluded from that snapshot was too broad: the snapshot excludes the top-level `assets/` and `analysis/` directories, but it preserves those parent test fixtures.

`tools/oracle_frontend_ui_bc3.py` establishes the two files' provenance: it reads the actual `ui\\global_bitmaps\\dialog1` bitmap from the owned `mainmenu.map`, then writes the compressed page and its decoded pixels. They are now explicitly ignored by Git. Their local files are retained so existing frontend tests and other work continue to run.

Sealed physics source manifests, test logs and binaries remain unchanged. Their hashes still identify the exact private trees that were tested. They must not be treated as redistributable code-only bundles. Moving original test resources to the external local import store and regenerating them from user-owned data remains part of the broader asset-isolation goal.
