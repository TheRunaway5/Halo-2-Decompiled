# Normal-player bloom screen context

The multiplayer player-view bloom threshold now comes from the original screen
context producer. It produces `0x20ACACAC` for the default threshold pass, matching
original Zanzibar capture20 event8941. The previous native absent-context value
was `0x20E5E5E5`. Blur weights and final bloom strength are unaffected by this
correction. This closes one measured effect-input difference; it does not prove
whole-frame equality.

## Original producer and call state

The pinned retail executable is
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.

1. `2B5D0` selects `2B790` for a normal player view whose first word is zero.
2. `2B790` passes the inline view's `+0xF4` screen context to `2BA10`.
3. `2BA10` writes that pointer to its `0x298`-byte render context at `+0x290`.
   `132D0` copies the complete context to `485600`, placing the pointer at
   `485890`.
4. `13870` reads that optional pointer, writes the sign-extended player index
   from `485600` to context`+0x1C`, and supplies **EBX zero** and EAX`51ECF0` to
   `245C0`. EBX here is explicitly cleared by the caller; it is not inherited
   exposure mode or the current player's viewport ordinal.
5. `245C0` takes its normal reset branch, writing zero, `.75`, `.4` to both
   three-word ranges, from original executable constants `45DC2C=3F400000` and
   `45DD30=3ECCCCCD`. It clears the counter and then calls complete `24EE0` in
   mode zero. That LUT calculation does not modify these nine context words.
   The final counter is one.
6. `13870` computes `1 - (1 - context[+0x10])`, with two separate rounded SSE
   subtractions, then multiplies by `45DC84=3F666666` (`.9`). Without a context
   pointer it retains the original `.9` threshold. The sky bloom blend remains
   `(sky_threshold - base_threshold) * sky_blend + base_threshold` when the
   blend is positive.

The helper returns the literal nine-word state:

```
00000000 3F400000 3ECCCCCD 00000000 3F400000 3ECCCCCD
0051ECF0 signed-view-index 00000001
```

`0051ECF0` is retained source diagnostic data, never dereferenced as a native
resource address. The helper is explicitly the normal-player call. It does not
port other `245C0` modes or their framebuffer measurements/history.

## Native boundary

`source_player_bloom_screen_context(view_index)` produces the exact normal
player state. `BspPostprocessView::bloom_screen_context` is optional and defaults
to absent. The normal multiplayer Renderer caller explicitly supplies it for
each player view; generic postprocess callers retain the null-context branch.
`BspPostprocess::prepare` forwards its literal `+0x10` factor to
`bsp_bloom_frame`, which now preserves the two original subtractions before
multiplication. No Camera, ModelView, Material or vertex ABI changed.

This correction does not derive cinematic bloom timelines, alternate view
contexts, arbitrary exposure modes, or framebuffer-based exposure state. The
existing amount/brightness defaults remain the original initialized `.5/.5`
inputs and the fixtures explicitly vary those inputs independently.

## Independent evidence

- `tools/differential_source_bloom_context.py` executes unchanged
  `13887 -> 214F0`, including **complete `245C0 -> 24EE0` and its math callees**.
  No instruction patches or function hooks are installed. It starts after
  the shader-context reset at `16B10` and stops at the actual GPU blur submission
  entry, retaining its threshold/strength arguments. Controlled raw-factor
  cases separately start at `138EE`; they are arithmetic controls, not normal
  player context production.
- **526 cases, 3,180 exact output words**, including 178 complete reset/LUT calls,
  normal and null contexts, signed view indices, explicit sky/cinematic values,
  and small factors that distinguish the two-subtraction expression from an
  algebraic simplification. The fixture is
  `tests/fixtures/retail_source_bloom_context.tsv` with a separate provenance
  manifest and complete function hashes.
- The immutable `ivory-live-view.bin` and `zanzibar-live-view.bin` each retain
  the exact inline nine-word normal-player state at `+0xF4`. They were read
  paused after capture; they are not relabeled as same-frame original20 RAM.
- Independently, original20 event8941's actual 944-byte fragment uniform block
  has C6 at byte640: RGB `172/255`, alpha `32/255`, packed `20ACACAC`. The
  fixture generator pins the raw uniform block hash and these literal words.
- The CPU regression asserts the captured packed threshold and independently
  retains the null-context `20E5E5E5` negative control. It asserts that the
  blur program and final-strength program remain unchanged.
- Focused production-source compilation and execution passed normally and with
  ASAN/UBSAN and leak detection enabled. No GPU or guest operation was used in
  this producer recovery. The registered production-linked CTest is
  `retail_source_bloom_context` (`source_bloom_context_tests`), which passed
  1/1 normally and 1/1 against the complete private ASAN/UBSAN library build
  with leak detection enabled.

A fresh production GPU replay is needed to verify the forwarded context at the
actual submitted threshold draw and measure the resulting image residuals.
