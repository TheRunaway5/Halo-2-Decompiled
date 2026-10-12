# Source master output gain

`sound_driver_master_gain.hpp/.cpp` implements the complete original `2230E0`
owner and its `32909C` low-DWORD integer conversion. Update 1.5 counterparts
are `2279E0` and `32CC0C`. Independent pairing covers all 146 and 37
instructions, respectively. The paired and repeated report SHA is
`dec64b3042c91f34913b2622f8aa37e4ffad93549cb58affb3f3e31a11433a13`.

The routine takes retained driver and output-global backing and the actual
24-byte input address. It clamps the four raw gain words, preserves the last
two raw words, computes the original logarithmic volume and source integer
conversion, writes the global mixing parameters, invalidates the source
static-voice flags and sets the update byte. Every store is observed in source
order. Missing or incomplete backing stops execution.

The GNU x86 backend uses the actual caller's masked x87/SSE context, including
its precision and rounding controls, then restores the host environment. It
does not substitute the standard library logarithm or integer truncation.
Unsupported backends or unmasked exception delivery remain unavailable. No
x87 stack or sticky-exception parity is claimed.

## Independent original execution

The original inputs compose the genuine allocated driver prefix from
`sound-driver-startup-native/measured/report.json` with the actual incomplete
looping-sound reset state from `grenade-sound-pool/measured/report.json`.
This is an explicitly bounded composition of retained owning outputs, not
natural completion of driver startup or looping-sound reset. The driver
constructor still has no supplied device return. The original reset input is
`[1,1,1,1,8000,0]`; the allocation's static-voice count is genuinely zero.
Positive static-voice invalidation therefore has complete instruction evidence
but no positive execution coverage in this checkpoint.

Complete old and update 1.5 calls cover 18 raw vectors under four x87 rounding
modes, for 144 controls. They include the original reset vector, signed zero,
subnormal and normal minima, finite fractional values, clamp boundaries,+infinities and quiet/signaling NaNs. The caller-selected input controls are
declared; no driver count, readiness, listener or device result is assigned.
Independent original repeats are byte-identical:
`838fb4a200823975dc932338434a061238bbd7095d8b351177e2eea84f2227b3`.

The native comparison matches all 2,304 ordered stores and 20,734,272 complete
output bytes. The fixture stores both before and after backing, totaling
41,468,544 bytes, and never obtains expected output from native code. Fixture
SHA is `ff4c6b9d6eed212764971ad62da7a13c31221e7676e5eac520c1cf9a40d9184a`.

## Immutable import and validation

`OriginalData::sound_driver_master_gain_constants` imports only three immutable
DWORD operands for each authenticated update, totaling 24 bytes. These are
float one, the logarithm scale 2000 and float one quarter. Base-disc support is
explicitly absent. Driver pointers, output globals, input state, voices and
readiness are not imported as constants.

Copied-source strict-warning normal and ASan/UBSan comparisons pass. Both
registered production builds pass `original_sound_driver_master_gain` and
`original_sound_driver_master_gain_constants`. The sanitizer report checks
the actual compiler flags. Source, archive, binary, flags, link commands and
CMake observations are retained; these focused checks do not assert unrelated
archive freshness or a new launcher build.

The owner closes the CPU routine at which the measured looping reset stopped.
It does not itself complete that reset, create a device, admit a sound, connect
reload animation events, construct/release a grenade or fix live gameplay.
Those owning lifecycles must still compose using genuine runtime state and
real platform operations.
