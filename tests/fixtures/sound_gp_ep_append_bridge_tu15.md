# Original EP scratch to GP table bridge

Original TU1.5 routine38F5F3 contains41 instructions/119 bytes. It writes the
EP scratch metadata at offsets438 and43C, builds three16-byte descriptors on
its real stack, then calls38850A to append their pages to the GP table. The
original38F6F4 caller uses offset10000 in the13000-byte EP allocation; the
fourth extra GP page comes from the parent's separate391410 descriptor.

`tools/oracle_sound_gp_scratch_tu15.py` authenticates every executed x86
instruction against the owned original English TU1.5 executable. Its three
new cases use offsets0,1000 and10000. Original input/output banks, complete
GP/XMM registers, defined flags and every ordered store are retained. They
supply no kernel return, device read, request clear or readiness result.
EP/GP software caller contexts and a write-only MMIO sink are declared inputs.

The combined fixture preserves all67 previous cases byte-for-byte and adds
these three. `campaign_dsp_image_tu15_tests` runs all70 twice, with and without
an observer:140 comparisons/196590 ordered stores. The existing reader and
its coverage minimum remain unchanged. The new cases contribute111 stores.
Generate a fresh combined fixture, without replacing old evidence:

```sh
python3 tools/tag_match_memory_budget.py --output-root NEW_GUARD --timeout 120 -- \
  /tmp/halo2-fov-reference-env/bin/python tools/oracle_sound_gp_scratch_tu15.py \
  --upload-fixtures tests/fixtures/sound_gp_upload_tu15 --output NEW_FIXTURE
```

Keep the owned fixture in the ignored local directory
`tests/fixtures/sound_gp_ep_append_bridge_tu15` to run the registered comparison:

```sh
python3 tools/tag_match_memory_budget.py --output-root NEW_GUARD --timeout 120 -- \
  ctest --test-dir ABSOLUTE_BUILD --output-on-failure \
  -R '^(original_sound_gp_ep_append_bridge_tu15|native_tu15_sound_gp_scratch_publication)$'
```

The native resource test runs38F5F3 against an actual19-page allocation from
the descriptor allocator. It checks both EP metadata words, each of the three
physical SGE pages/high words, reservation513/count11, then the separate page's
cursorB000/reservation512/count12. The existing prepared-image byte, pool
growth and destructor checks still execute. The composed case reaches403
original instruction addresses and verifies823296 backing bytes. The software
contexts, immutable prepared image and full parent device/PIO startup remain
separate inputs or gates.

Current-root Clang/GCC v528 builds and nine registered checks per compiler
v529/v530 pass. Omitting the metadata store or changing descriptor stride10
toC fails both scalar and native-resource checks (v532/v535). The unchanged
strict source audit also rejects the latter at original instruction38F64E
(v539). Previous failed coverage/diagnostic receipts remain retained.

Fresh v533 regeneration matches the DSP module's entire emitted source:
18 complete bodies/731 instructions/1979 original bytes. Full selection v537
passes all eleven modules after synchronizing the generator's header include;
no instruction comparison or helper-text equality requirement was weakened.
All eleven selected modules compile with both Clang/GCC and strict FP flags
(22 successful commands, v538).
These checks establish selected source/resource behavior, not the complete
parent factory, physical MCPX behavior, live audio clock or finished cutscene.
