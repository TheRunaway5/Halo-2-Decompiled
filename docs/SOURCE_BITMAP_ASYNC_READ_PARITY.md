# Source bitmap file routing and owned reads

`source_bitmap_async_read.hpp/.cpp` ports complete `213760` file selection and
the read-request construction it supplies to `1A0F10`. A separate native
adapter opens explicitly bound files read-only and feeds real seek/read
outcomes into the independently proved `1A0E70` arithmetic. It allocates no
image, writes no done byte, changes no resident bitmap state, and reconstructs
no original scheduling or completion timeline.

## Literal producer and file bank

`213760` reads the active cache index at `55ACA8`, then its current file token
at `557C90 + index*804`. Location bits 30 and 31 select the following bank:

| Location | File-token producer | Offset passed to read task |
| --- | --- | --- |
| `0` | Current active cache token | `location & 3FFFFFFF` |
| `1` | `558C98`, mainmenu | `location & 3FFFFFFF` |
| `2` | `557C90`, shared | `location & 3FFFFFFF` |
| `3` | `558494`, single-player shared | `location & 3FFFFFFF` |

The immutable restored Zanzibar bank has active index 3 and tokens
`76/72/64/68` in this order. These are source-observed caller inputs, not
generated native handles. `SourceBitmapFileBank` requires the actual tokens
from its owner. `Cache::raw_data` independently uses the same authored
current-map/mainmenu/shared/single-player-shared filename mapping.

The producer passes file token, size, low thirty-bit offset, buffer token,
optional byte-output token, done token, category and priority unchanged into
the read-task constructor. The callback is literal `1A0E70`; all five unused
task words start at zero. `1A0F10` increments the requests-added counter and
clears the actual done storage before `1208B0` tries to acquire a free node.
The wrapper returns no file-I/O outcome at this point.

`tools/oracle_source_bitmap_async_read.py` executes the complete unchanged
`213760 → 1A0F10` producer to the `1208B0` entry. It does not make the allocator
return. Four separate whole-caller diagnostics continue to actual
`NtWaitForSingleObjectEx` entry `80020F5A`, with mutex token 4, WaitMode 1
(`UserMode`), alertable false and null timeout, and stop before the first
kernel instruction.

## Native-owned file service

`SourceAsyncOwnedFiles` opens each explicitly supplied token/path pair as a
read-only native stream. It rejects unknown or duplicate tokens and files
that cannot actually be opened. Token bindings are opaque caller mappings;
they are not host pointer truncations or fabricated original handle values.
The source has one worker executing read callbacks serially. The owner must
supply that serial ordering; this adapter does not start or schedule a worker.

Every read first computes the source step with the actual chunk-limit input.
It validates that the logical source buffer/range is represented by the
actual owned native span, seeks to the unsigned source offset, and obtains the
actual native position and transferred-byte count. A genuinely failed seek
returns the source low-word sentinel `FFFFFFFF` and performs no read.
A successful seek whose low word is `FFFFFFFF` also skips the read, matching
the source callback's literal sentinel test. It does not infer failure from
an EOF position: a regular-file seek past EOF can succeed and the subsequent
read then returns zero bytes.

The complete original caller passes a nonnull pointer to a zero high DWORD:
`1A0E8B` forms its local address, `1A0E95` pushes it, and `1A0EA0` zeros the
local before calling `2D2488`. Therefore a high-bit low DWORD still means a
positive 64-bit position. The wrapper's `CDQ` sign-extension path is used
only when that pointer is null. `tools/audit_source_async_seek_wrapper.py`
executes the unchanged complete caller and wrapper for ten offsets to actual
`NtSetInformationFile` entry `80016F61`, recording the requested `[low,0]`
position without supplying a kernel result. Twenty separate wrapper controls
prove the distinction: four high-bit/null-pointer cases reject in title code
before the kernel. No original file-position result is claimed. Native tests
also require three high-bit regular-file seeks to remain positive, reach EOF
without transferring bytes, and preserve the owned buffer.

Zero-size requests still seek and perform a native zero-count read. Short
reads modify only the bytes actually transferred; the unused owned-buffer
tail is preserved. EOF/failure does not cause a fabricated full transfer.
Native stream error state is cleared before the next seek, so a previous EOF
does not prevent a later request on the same actual file.

The returned `SourceAsyncReadExecution` contains the source step, real native
I/O values and source callback arithmetic. It does not execute the worker's
completion decision. In particular, callback result one can mean complete,
short or failed-seek processing. An owner must not interpret this helper as
having produced a valid whole image or a captured original ready timeline.
The worker's exact done/unlink/free protocol remains in
[SOURCE_ASYNC_READ_PARITY.md](SOURCE_ASYNC_READ_PARITY.md).

## Independent validation

The original executable is `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The oracle reads the immutable
`analysis/multiplayer-shading/registration-snapshot-restore-20261008` bank,
verifies every reached title instruction against the executable, and restores
and checks sixteen loaded original pages. Supplied FPCW `023F` and MXCSR
`1F80` are explicit execution controls, not inferred live thread state.

There are 1,216 producer cases and 21,888 checked construction words: 168 actual
allocator requests, 1,024 generated caller controls, and 24 location/offset
edges. Of these, 20,672 words are read from executed prefix outputs or actual
`1208B0` boundary inputs. The remaining 1,216 callback words come from the
literal, independently checked `1A0F90` store of `1A0E70`; that assignment lies
after acquisition and is not executed by this prefix oracle. Its complete
owned binding is covered separately by the request/read protocol proof.
The boundary metadata is read from stack `+44/+48` and EBX, rather than copied
from the test inputs. Controls cover all four locations, token selection, offset masking,
zero size, priority/category/raw payload values, request-counter wrap and
done clearing. The proof verifies 102 title instruction addresses, replaces
no services and patches no instructions.

Separate immutable-file rows cover the 42 original requested resources and
twelve header/partial/EOF requests over all four actual files. The native
adapter performs 54 reads totaling 1,454,108 transferred bytes, with byte
hashes checked against independently read authored data. These are actual
native-file outcomes, not claimed original kernel outcomes. Fourteen Linux controls
cover ownership/mapping failures, unrepresentable source buffer ranges,
zero-count I/O, the exact low-word sentinel and a real nonseekable owned pipe.
No source map or shared file is written. The normal and fully instrumented
AddressSanitizer/UndefinedBehaviorSanitizer tests run with leak detection.
Both builds also reject missing, empty, comment-only and last-row-truncated
fixtures, rather than accepting an incomplete source corpus.

The mandatory frozen corpus is
`tests/fixtures/retail_source_bitmap_async_read.tsv`, with adjacent provenance
and `analysis/multiplayer-shading/source-bitmap-async-read-original.json`.
Counts, byte totals and hashes are checked independently of the helper's
control flow. These new files remain standalone pending production review.
Physical allocation/pressure, owned-image binding, resource fence state,
manager maintenance and native residency activation remain distinct services.
