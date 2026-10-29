# Native TU1.5 scratch-resource retirement

`src/campaign/ai/native_campaign_ai_resource_retirement_tu15.cpp` implements
`130030`, `140310`, `2DB4CA`'s page-protection contract and the unassociated
`12FD10` callback. It calls the existing semantic `16E8A0` owner; its guarded
library is linked once. The public API uses typed allocation/container/datum
identities and a required void `NativeCampaignAiResourceProtectionTu15` service.
It contains no virtual registers, guest stack, instruction loop, supplied game
outcome or lifted fallback. `cmake/CampaignNativeAiResourceRetirement.cmake`
adds the production source and focused original-reference target.

`130030` does **not** unmap the physical allocation. It caches its initial
resource record, reads the real byte range, and for nonzero length reads import
`41B648`. Authenticated selector `800000B6` invokes the actual native page owner
with protection `404`; a zero length skips both selector read and operation.
Only afterward does the original clear the resource active bit, publish header
and neighbor links, and remove list metadata. The two neighbor branches have
different datum/container read order. The implementation preserves it at
lines78–107, including a container read before the next-neighbor publication.

`140310` caches the initial24-byte record address, loads its actual delete
callback, executes `12FD10` when selected, and then reads links from that cached
record. Neighbor writes and final deletion reload the actual pool fields
(lines44–73). The callback independently resolves its40-byte resource record
and reloads the resource pool before native deletion (lines32–39). These are two
distinct deletions, with actual poisoning, bitmap, salt, cursor, high-water and
live-count effects supplied by the existing native data-array owner.

The profile is supported by authenticated producer code, recorded separately in
`analysis/campaign-native-ai-resource-retirement-20261028/producer-profile.json`.
`149930` zeroes ESI at149939 and calls `12FE00(size,0,0,0)` at149984. Successful
`12FE00` initializes all38 record bytes after the salt to zero at12FED9–12FEF3,
then sets the active flag. The association DWORD+10 is therefore zero for the
scratch allocation. Actual resource-container construction registers callback
`12FD10` at12EC71/13FCA4; no NULL callback is assumed. This receipt authenticates
the producer statically; it is not a claim to have executed its unconverted
allocation/clock path. The native retirement still reads the actual association.

Associated records stop at real12FD40 with the record identity. GPU/fence
queries/waits `4057F0`/`404E30` and semaphore work remain required owners.
Unknown import or delete-callback selectors likewise stop at their real entry
and argument. No event, fence completion or successful retirement is invented.
The whole lease/search/campaign feature is not promoted by this component.

`tools/oracle_native_campaign_ai_resource_retirement_tu15.py` runs unchanged
physical i386 TU1.5 code, image SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The only platform adaptation is an authenticated-import ABI bridge to real
Linux `mprotect` over the actual original pages. It supplies no game result;
its real syscall return is overwritten by the original caller before use.
Declared read-only inputs are installed by that same real platform operation.
Original instruction bytes and every source callee remain unchanged. Native
comparisons use coupled `SourceRuntimeMemory` and
`SourceRuntimeHostPhysicalMemory`, whose actual backing is protected by
`mprotect` and source write guards. Page permissions and retained allocation
identity/extent are compared as well as data. Xbox cache/device coherency is
outside this unassociated host-page profile.

The327-case fixture executes169 reachable source sites and35,834 original
instructions, with7,667 ordered reads,4,381 stores and143 actual page-protection
operations. It includes226 lazy fault prefixes, six required-owner boundaries,
and a true original unmapped read at44 after prior unlink publications.
Complete interleaved read/store/protection traces and FP preservation match.
Cases cover both link directions, absent/present callbacks, salted IDs across
bitmap words, generic delete strides/tails, poisoning, high-water changes,
byte protection boundaries, aliases that replace container/pool globals, and
GPU/import/callback refusal. Zero-count REP instructions contribute no phantom
memory access. No artificial read/store midpoint inside an original RMW
instruction is claimed.

GCC and ASan/UBSan pass1/1; Clang passes7/7 with six actual-source mutations
rejected: omitted native protection, omitted delete callback, cached pool,
late container read, fabricated GPU completion and wrong terminal neighbor.
The original repeat is byte-identical. The current composed module also
passes all327 cases on GCC, ASan/UBSan and Clang after its include guard was
added. See `analysis/campaign-native-ai-resource-retirement-20261028/proof.json`
and the composed arena proof it references for exact current pins/commands.

The required fixture is
`tests/fixtures/native_campaign_ai_resource_retirement_tu15.json.gz`,828,821
bytes, SHA-256
`d7a4839d7636db6b16217721289f4c00feb67bce280a5f811f4e49f27ef3abfd`.
All original ELF, raw banks, native test binaries and compiler builds were
removed through the official storage wrappers; only this required reference
input and bounded reports remain. No world/router/launcher state was changed.
