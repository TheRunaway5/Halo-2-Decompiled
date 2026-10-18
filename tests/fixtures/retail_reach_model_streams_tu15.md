# Reach model stream fixture

`retail_reach_model_streams_tu15.json` is generated from the owned original
opening cache and English TU1.5 executable. It remains ignored under the
repository's fixture policy. Regenerate it with:

```sh
python3 tools/generate_reach_model_stream_fixture.py \
  --map assets/disc/maps/00a_introduction.map \
  --xbe "/path/to/u5English/default.xbe" \
  --output tests/fixtures/retail_reach_model_streams_tu15.json
```

The generator verifies both complete input hashes, the original stream
declarations and format/size dispatch, the authored FLOAT3/packed-UV descriptor,
compression flags, positions and triangle-strip indices. Expected output SHA-256:
`95884971dcf454a2d7e137813c362e22f851f38680d0685401c2eb38efaf844f`.

`reach_model_float_positions_tests` checks preview and all six section LODs,
including exact position/local-position and packed-UV bits, topology, origin and
rigid skin binding. Its six other model digests are comparison evidence; the
caller does not independently assert a golden digest for those controls. The
fixture is a CPU decoding reference, not a rendered-frame parity proof.
