# Native actor detachment from an AI clump

The Armory's tram placement reaches `272825 -> 273290`, followed by
`27282F -> 2731E0`. The latter entry already existed as the private
`detach_actor` implementation used by native clump teardown. Its new public
entry reuses that implementation and its `274970` perception-list helper.
No algorithm, actor position, placement result or script completion is supplied.

`2731E0` takes the clump datum in EAX and the exact actor datum in EDX. It
unlinks the actor only when its full datum matches, detaches the actor's
perception associations, and decrements the clump's 16-bit actor count. The
native dispatcher consumes the return word and preserves the saved registers.
The two actual callers overwrite volatile registers before consuming them;
there is no scalar result or private-frame equivalence claim.

The unchanged physical i386 reference covers 756 complete calls and all
56 parent and 32 child instruction sites. Native comparisons check complete
retained banks and 4,212 ordered stores, saved GP/ESP and unchanged FP/XMM.
Cases include empty lists, head/middle/tail removal, absent actors, salt
mismatches, linked perceptions and 16-bit count wrap. The existing clump
retirement and callback regressions also pass. The focused ASan/UBSan/LSan
run passes outside the tracing sandbox; the first run could not initialize
LeakSanitizer under ptrace.

Evidence: `build/test-results/armory-native-progress-20261113/clump-detach/qualification.json`.
The required compressed original fixture is
`tests/fixtures/native_campaign_ai_clump_detach_actor_tu15.json.gz` (67,401 bytes).
The runtime uses compiled C++ and imported MCC data; the original executable
is only an offline test authority. Enclosing tram acceptance is recorded
separately in the Armory report.
