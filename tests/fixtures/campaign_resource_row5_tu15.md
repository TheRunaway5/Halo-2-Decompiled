# Original TU1.5 campaign resource row 5 release

Authority: unchanged English OG Xbox TU1.5 executable, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The complete `2255F0` routine contains five instructions and 20 bytes. Its
registration is the release column of row 5 at `45F940 + 5*8`. It reads
`48B8FC`, clears that word when nonzero, and returns with the original EAX and
defined flags. It neither frees storage nor supplies resource readiness.

`tools/generate_campaign_activation_tu15.py` authenticates the complete bytes
and registration before generating this owner. The shared module now contains
50 complete bodies / 3,143 instructions; the previous 49 bodies are preserved.
The current object-attachment include is retained from the shared generator.

The local original fixtures are ignored assets. Regenerate into a fresh path
through `tools/run_test.py`, setting a fresh, absent `PYTHONPYCACHEPREFIX` inside
`HALO2_TEST_SCRATCH`, with:

```text
tools/oracle_campaign_resource_row5_tu15.py --xbe ORIGINAL_TU15_XBE
  --helpers tools/reference/halo2-decompiled/tools --output FRESH_OUTPUT
  --execute-original-after-grant
```

The Python process requires `-B`. Explicitly retain generated reference inputs
only when needed for a later native comparison; report their paths and sizes.
No original executable bytes belong in version control.

Eight initial words, including zero, signed extremes and alternating bits, are
each called twice while retaining the first call's resource bank. The original
16 calls execute 71 instructions and seven ordered stores without external
fixtures or instruction patches. Native observed/unobserved comparison passes
32 calls: 14 clear paths and 18 skip paths. The caller checks full declared
memory banks, stack, GP/XMM registers, dynamic instruction order, store order and
before/written bytes, defined flags mask `0xCC5`, FP controls and host FP restore.
The five controls pass with exits 0/1/0/77/1: missing baseline capability,
baseline rejection, candidate comparison, missing fixture and corrupt fixture.

Evidence remains under
`analysis/opening-cinematic-20261015/campaign-resource-row5-release-owner-v1`.
The original receipt is `original-stage-v1/result.json`; native controls are
`native-controls-stage-v1/result.json` and `native-controls-terminal-v1.json`.
The standalone CTest is `original_campaign_resource_row5_tu15`; it uses the
existing supported GNU/Clang x86 guard and strict FP options, with no World or
Halo archive dependency. Component evidence does not prove full cache loading,
map activation, sound scheduling, camera timing or Vulkan visual parity.
