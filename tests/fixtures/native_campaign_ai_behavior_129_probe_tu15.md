# Native behavior129 direction probes

The required reference contains288 unchanged physical TU1.5 calls:84 genuine
no-target returns and204 prefixes ending at the actual1CA560 entry. No original
instruction is patched and no result is supplied for that unowned callback.
The source image SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

1CA900 selects the nearest eligible tracked perception and constructs a planar
direction, reusing native2673A0 and30BD0.1CAA20 constructs eight source-ordered
directions from the actor facing and stops at its first1CA560 call. Native
refusals identify the exact original call sites1CA9FF and1CAC7D. No later loop
iteration or successful post-child return is admitted.

Original coverage is78/84 sites for1CA900 and138/147 for1CAA20; the15 missing
sites are all after1CA560. The selected perception branches cover20/27 sites
of the pre-existing2673A0 owner, and normalization covers all38 sites of30BD0.

The native replay compares saved registers/AL on whole returns and all eight
GP registers at the exact refusal boundary, plus FP state. It checks all four
call-frame words and the complete generated direction buffer, including all
eight vectors before the fan's first call. These boundary inputs total10,752
bytes across the204 prefixes. Retained banks compare completely. No retained
stores are expected: the helpers build private inputs for their child. Dead
private intermediates, volatile XMM, ordered reads, hardware FP traps and
post-child code are outside this contract.

Eight mutations were rejected. Scoped ASan/UBSan passed for the probe and
driver; existing child libraries were borrowed without instrumentation in that
pass. An independent original repeat was byte-identical, and a separate source
review found no actionable mismatch. A final baseline confirms the same x86
algorithm after adding an explicit unsupported-platform compilation guard.

Fixture:`native_campaign_ai_behavior_129_probe_tu15.json.gz`,1,293,783 bytes,
SHA-256`9580742c982d0f650fa985ca99199ef23862f50dd48376b60a9b97fcd608d335`.
Evidence:`build/test-results/armory-native-progress-20261113/behavior-backing/probes-closure.json`.
All five owned original/replay/control scratch roots were verified removed;
only this required reference and small reports remain.
