# TU1.5 32A660 stack probe

The complete CPU stack reservation now defaults to the shared typed DATA
kernel in `include/halo2/native_tu15_stack_probe_kernel.hpp`, with the retained
ABI adapter in `src/physics/runtime/native_tu15_stack_probe_abi.hpp`.

The adapter touches each crossed 4 KiB page through the real Memory owner,
preserves the duplicate final touch for a zero remainder, and relocates the
genuine caller return word. It reacquires the saved ECX word after the probes.
No guard-page result, allocation handle or operating-system success is supplied.

All three complete TU1.5 source copies are bound: AI frame, shadow frame and
player departure. The Vehicle copy, canonical 326AF0 and actual TU1.5 32A660,
is also bound. It retains its existing canonical instruction cursor and PUSH
labels by an explicit 0x3B70 offset; comparisons map only those declared labels
to physical TU1.5 addresses. GP, flags, values, addresses and storage are compared
without normalization. MapPack and unknown observation protocols retain their
reference routes.

The primary live-support dispatcher has a separate authenticated actual-owner
entry because it has no complete local `run_32a660` body. Its
`native_stack_probe_actual_owner_entries` receipt, `stack_probe_entries` address
map and `probe_entries` activity remain separate from Raw, Tick, Epoch, BSP,
polygon and DatumDelete evidence. `HALO2_REFERENCE_PAGE_ALLOCATOR` forces the
reference route; the other allocator-family controls remain independent.

Whole original image: 32A660, 65 bytes, SHA-256
`b77ad187f6f3db2a5ae4f8e214b023d7c62ce25a38ce769a993d2a8c81643760`.
The binder authenticates the complete body and owning selected-address resolver
for every copy. Common and Vehicle generators preserve these bindings.

Qualification on 2026-11-09:

- 168 unchanged-original i386 hardware fixtures cover all 27 instruction sites,
  all 16 caller-stack alignments, subword return-word overlaps, 4 KiB boundaries,
  the page-parent's 0x1054 reservation, both DF values and saved-counter memory
  feedback. The original computes every probe and return word.
- Direct, four source-copy and actual-owner routes pass 16 default/reference
  and control-isolation modes. Each mode compares complete caller banks, 728
  explicit reads, 248 stores, final GP/material flags, XMM, FP controls/status
  and cursor. Direct modes compare 976 physical pre-I/O GP/flags/XMM/FP packets
  at all 11 memory boundaries. Source default routes deliver zero instruction
  callbacks; admitted native routes execute zero translated fallback bodies.
- Each mode also compares 60 retained baseline scenarios, 259 Memory fault
  prefixes and 29 throwing-observer prefixes. These compare complete partial
  GP/full flags, patterned XMM, live 80-bit values, FP state, ordered accesses,
  stores, banks and cursor. Ten image/observation admission refusals per mode
  preserve state without I/O.
- Eight complete-body/resolver mutations and two whole-dispatch mutations are
  rejected. All four complete production units pass Clang syntax checks.
- Official sanitizer CMake gate passes 3/3: stack probe, dependent physical
  allocation and ten specialization tests. The allocation fixture's 104 cases
  now compose 176 native stack calls in both default and parent-reference modes;
  its other qualified child counters remain zero.

Official receipt:
`build/test-results/native-stack-probe-abi-20261109/official-cmake/evidence/native-stack-probe-abi.json`
(13,481 bytes, SHA-256
`79930783debedc7ec0c6076a938456b478dee32a980e711672ca84776d9720aa`).
The dependent allocation receipt is in the same directory (9,578 bytes,
SHA-256 `ca20e7b3e77c6020a0836247377edbe4866cf11b00a78566922fa4229e7ef575`).
The private CMake tree `halo2-test-yfixl6j5`, nested test owners and all private
probe/syntax owners were verified removed. No large generated artifacts remain.

The retained Memory/observer exception protocol is qualified. Physical
instruction/page-fault atomicity, operating-system guard-page behavior and
whole page-allocation/physical-allocation closure remain separate and open.
The legacy scratch-flags page allocator CPP and its private stack helper remain
byte-identical to the preceding qualified source
(`6a5b593db0f7a82cd4f7d74655bcefd82b04d52191f0a1338601ec1359840f8d`).
The next parent is 13FE50; no parent-default or launcher promotion is claimed by
this component proof.
