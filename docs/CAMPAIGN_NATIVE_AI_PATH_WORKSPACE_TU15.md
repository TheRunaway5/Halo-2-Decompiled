# Native TU1.5 path-workspace initialization

`src/campaign/ai/native_campaign_ai_path_workspace_tu15.cpp` implements complete
`27B010` initialization of an actual retained path workspace. It initializes
request fields, optional policies, the actual BSP navigation-data binding and
the original count/marker fields. It does not lease storage, search a graph or
supply a successful path result.

The typed API is declared in
`include/halo2/native_campaign_ai_path_workspace_tu15.hpp`. Its main 20-word and
optional 7-word parameter records are lazy semantic data interfaces. A retained
memory adapter supports the existing layout and aliasing; later native callers
may supply typed host records without constructing an original private stack.
These are data records, with no executable selectors, virtual registers,
instruction interpreter or CPU flags machinery.

All initial marker stores precede the input reads. Policy copy/default fields
retain their exact widths and publication order. The kernel reads actual
`4ECC18`, signed BSP+C4 count, and BSP+C8 pointer only when that count is
positive. The main record is copied forward one word at a time; no snapshot or
host memmove substitutes for observable overlapping input/output behavior.
Context and transport fields are then published at their original phases.
Missing optional policy only clears two WORDs; unused bytes remain untouched.

`cmake/CampaignNativeAiPathWorkspace.cmake` supplies the library source and
focused test. No numeric router or live activation was added. The typed copy
contract corresponds to ordinary DF-clear callers; arbitrary DF-set CPU entry
is not admitted or emulated.

`tools/oracle_native_campaign_ai_path_workspace_tu15.py` runs unchanged physical
i386 original code from English TU1.5, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
No original instruction, dependency result or resumed fault outcome is supplied.
Only declared pre-entry data, registers and FP inputs are installed. REP MOVSD
is observed at its real before-read and completed-store boundaries; no
fictional stop between that instruction's source read and store is claimed.

The required compressed fixture is
`tests/fixtures/native_campaign_ai_path_workspace_tu15.json.gz`. Its 303 cases
include 53 completed calls and 250 read/store/observer prefixes, exercising all
53 original sites. Cases include all optional-record combinations, negative/
zero/positive BSP counts, context words, unaligned retained records and input/
workspace aliases that distinguish forward copying from snapshot/memmove.
An independent physical repeat produced byte-identical evidence. Tests compare
all retained bytes, ordered reads, store widths/sites/order, original FP and
unrelated host FP preservation.

GCC normal and ASan/UBSan each passed 1/1 CTest. Clang passed 6/6, including
five actual mutations rejected by the original fixture: reversed main copy,
wide default policy write, unsigned BSP count, repeated transport source word
and wide completion field. Exact source hashes, fixture size/hash, commands,
original read/store totals and verified scratch cleanup are in
`analysis/campaign-native-ai-path-workspace-20261028/proof.json`. Only the
required compressed fixture and bounded receipts remain; original executable,
observer executable, raw banks and compiler products were removed.

The real workspace lease/release owners `1D2D90`/`1D2E30` have two descriptors
at `50208C`/`502098`, with actual optional allocator teardown/reconstruction owners `1498E0`/`149680`.
These owners release and rebuild the scratch-arena allocator; they are not
mutex operations. Their storage allocation already exists but their semantics are not established
by this initializer. Query start `27B100`, real graph search `27B340`/`27BD30`,
the complete `1C9770` selector, `200D30` update and full native campaign runtime
acceptance remain open. Prior character/filter/geometry/count proofs were
preserved.
