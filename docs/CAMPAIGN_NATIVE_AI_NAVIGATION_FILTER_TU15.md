# Native TU1.5 navigation-filter construction

`src/campaign/ai/native_campaign_ai_navigation_filter_tu15.cpp` implements the
complete `201500` navigation-filter builder. The algorithm derives physics,
action/order and vehicle restrictions from actual retained definitions. It
contains no CPU interpreter, private guest stack, captured dependency results
or guessed pathability result. `1ECD00`, `1ED700` and `27C6C0` use the separately
proved semantic shared-character module, unchanged by this work.

The public API is
`native_campaign_ai_navigation_filter_tu15(memory,actor,destination,observer)`
in `include/halo2/native_campaign_ai_navigation_filter_tu15.hpp`. The module is
`cmake/CampaignNativeAiNavigationFilter.cmake`. No numeric caller binding or
live route is added.

The builder preserves lazy tag/actor/action/squad/order lookups, mask shifts and
intersections, the full DWORD squad sentinel, signed order indices, source
read/field publication order and output aliases. The base physics-character
mask distinguishes NONE/null, absent blocks and real authored flags. A positive
action restriction bypasses the order restriction; otherwise the actual order
list or its scenario character supplies the restriction. The low flag bit is
then set through the actual retained destination.

Navigation kind counts are signed16, capped at six for the bit mask and reread
before the >=7 flag decision. Vehicle mode uses the real inherited B4 records
and their B0 field. Bytes 0D,16,17 and every other untouched output byte retain
their input values; no private padding is fabricated or cleared.

The original has no null-definition guard on the action restriction path.
Two declared invalid action-definition cases execute actual physical faults
at `2015A3`, source DWORD address 4C, after their earlier mask publication. The
native builder fails on that same missing source operand and preserves all
preceding bytes. It does not silently replace the missing definition with a
successful fallback. Other prefixes stop before actual reads/stores or after
completed stores, with no supplied outcome or resumed reference execution.

`201620` is a single original `OR [flags],1` read-modify-write instruction.
PTRACE can prove its before-read and committed after-write boundaries, but
cannot independently authenticate an intervening bus operation. The fixture
omits three fictional before-store midpoint labels. No intra-instruction MMIO
or concurrent atomicity claim is made; all other recorded scalar operand and
publication ordering remains exact.

`tools/oracle_native_campaign_ai_navigation_filter_tu15.py` runs unchanged
physical i386 original routines from English TU1.5, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
All three dependency routines execute for real. The required compressed input
and result fixture is
`tests/fixtures/native_campaign_ai_navigation_filter_tu15.json.gz`.
An independent repeat produced byte-identical evidence. Its 496 cases include
291 completed calls, 203 injected source/publication prefixes and two actual
missing-source faults. They execute all 247 closure sites: 159 in `201500` plus
88 in its actual dependencies. Exact masks, fields, retained padding, source
aliases, ordered reads/stores/sites, original FP and host FP are checked.

GCC normal and ASan/UBSan each passed 1/1 CTest. Clang passed 6/6, including
five actual source mutations rejected against the physical reference: omitted
action mask, wrong vehicle field, wrong kind cap, omitted kind reread and
cleared padding. The independent original repeat and all compiler runs use
owned scratch through the repository wrappers.

`analysis/campaign-native-ai-navigation-filter-20261028/proof.json` retains
source hashes, fixture hash/size, commands, read/store totals and verified
cleanup. Only the bounded required compressed reference and small receipts
remain; raw banks, original executable/observer and compiler products were
removed. Existing character, count/fraction and point-geometry proofs remain
unchanged. `2013B0` cached-position construction, the full `1C9770` selector,
`27B340`/`27BD30` actual search, `200D30` path update and full native campaign
runtime acceptance remain open.
