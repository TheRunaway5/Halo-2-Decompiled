# Original TU1.5 opening dialogue routing

Generate the ignored raw fixtures with the owned TU1.5 executable and original
opening map present. Use a fresh output directory and the required memory guard:

```bash
python3 tools/tag_match_memory_budget.py \
  --output-root analysis/opening-routing-original-NEW-guard --timeout 90 -- \
  /tmp/halo2-fov-reference-env/bin/python \
  tools/oracle_opening_scripted_routing_tu15.py \
  --output analysis/opening-routing-original-NEW
```

The generator authenticates both SHA-256 hashes, executes unchanged original
instructions, and exports `native/` with the original inputs, output bytes,
registers, flags, ordered stores and external boundaries. Copy that directory
to the previously absent `tests/fixtures/campaign_audio_routing_tu15` to enable
`original_campaign_audio_routing_tu15`. Existing evidence must be preserved.

The 685 cases use actual metadata for 38 opening cinematic-dialogue tags and
declared world points at 1, 17.5 and 25 units. They include complete spatial-gain
and channel-routing calls, request packing to the original effect boundary,
and the mixer's metadata-copy prefix before its first SDK call. They do not
establish live game startup, admission, DSP output, audible gains or timing.
The `--omit-dialogue-routing` test-reader control must fail at the original
cinematic routing write. These controls supply no original callee returns.
