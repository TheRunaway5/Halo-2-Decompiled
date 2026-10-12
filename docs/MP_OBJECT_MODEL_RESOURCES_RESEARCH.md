# Complete source object model-resource query

`object_model_resources.hpp/.cpp` implements complete `C0890`, paired uniquely
with update 1.5 `C19D0` across all 49 instructions. Its selected-image body
authority is the existing immutable `unit-pose-requests/body-next-u5` report.
No readiness flag or resource count is substituted for that original body.

The CPU ABI takes a retained object pool and tag table, the object datum and
the actual two output addresses. It reads low16 directly through header `+8`
and body definition, then definition `+38`, model `+4/+C`, and mode signed
DWORD counts `+74/+7C`. It does not introduce a salt check. A missing reference
leaves both outputs untouched. Present references write both original datums
even if a count is nonpositive; defined AL is true only when both counts are
positive. Collision is reread after the first output store, preserving aliasing.
Unowned backing throws, rather than returning a manufactured false or true.

## Owning input and independent execution

A read-only observer reexecutes the exact inherited four-stage PlayerTrace
preparation and genuine biped/weapon construction. Its two natural calls have
valid authored references but both counts zero; both return AL zero and write
the actual mode and collision datums. The player references are `E9F2086D` →
`E9F3086E` → mode `E9070782` / collision `E9170792`. The weapon references are
`E57B0406` → `E57C0407` → mode `E57D0408` / collision `E5850410`.

The fresh owning repeat is byte-identical, capture SHA
`d3ba9d84b9db0ffc188664f39911b49bed70b7c6b18b512a446054a04ab227bf`.
Captured complete input fields and caller outputs are evidence for the query;
they are never restored as a final native Unit or complete live world.

The direct oracle executes unchanged whole old/update 1.5 bodies on those
retained inputs, plus separately declared NONE, signed-count and output-alias
controls. Positive counts exist only in declared query controls and do not
establish positive resource loading. Update 1.5 uses the old owning raw backing
against the authenticated corresponding accesses, rather than a full update
1.5 world. Source private ABI PUSH/POP stack storage is excluded from the
native CPU contract; all caller-owned output stores remain compared.

Independent direct reports match exactly:
`bf1f4bd253cf5075f1cec9007da0725863db8ee94956f76f661f06341bc66330`.
The 60 controls compare all 96 ordered output stores and 21,120 full retained
input/output bytes. The original-only fixture SHA is
`ddd113f08aca3b4889582d9381a581606061a6d40cdee39e8aa0ac57575c035a`.
The initial exploratory comparison included the source's private PUSH slot;
that failed assertion and its oracle are preserved separately in history.

## Validation and remaining ownership

Copied-source normal and ASan/UBSan builds pass with warnings as errors.
Registered production normal and actual ASan/UBSan builds pass the same
fixture. Binary, archive, flags, commands and before/after CMake observations
are retained. This is a focused query comparison, not unrelated archive
freshness, complete physical resource admission or launcher verification.

The generic constructor must call this owner with actual pool/tag and caller
storage. Complete type callbacks, physical-component allocation and world
lifecycles remain required. Canister response/explosions, live Q and reload
playback remain unfinished. No runtime, renderer, shader or launcher is changed
by this checkpoint.
