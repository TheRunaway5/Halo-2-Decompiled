# Native generic object creation request

The complete original `B8820` initializer and `BBCB0` salted typed lookup are
ported in `object_creation_request.hpp/.cpp`. They produce the original
196-byte request from actual pool/header/body backing. They do not allocate,
attach, release or launch a projectile and are not a working native Q path.

## Original authority and owning input

`research_object_creation_request_u5.py` independently pairs all 87 initializer
instructions with update 1.5 `B9960`, and all 35 lookup instructions with
`BCDF0`. The complete bodies, internal branches, called lookup and referenced
operands match uniquely. Both independent static reports hash to
`49f0ffd3722252559a15ac9e23b0b5ee143617a70c8e01b2dd26bbf99747673c`.

`oracle_object_creation_request_native.py` first reproduces the frozen original
admitted grenade begin and ordinary continuation without changing its memory,
instructions or service results. An observer at the actual `E8E64` call captures
the complete pool header, entire record backing, allocated unit/weapon bodies,
caller request and location. Complete `B8820` naturally returns to `E8E69`.
The owning preparation later stops inside the unavailable looping-sound owner;
no completed projectile constructor is inferred from its allocated prefix.

The measured and independently repeated captures and direct-call reports are
byte-identical. Report SHA:
`2e454f51fcc53b20af9e34f1bdb0fad9e8570bd9707dd93694f9b3cf1fd993a4`.
Only explicitly declared ABI owner/location/type-mask arguments vary in the
additional controls. Pool, header, type, salt, body and request backing are
literal original outputs. Update 1.5 controls reuse this original 1.1 raw
backing against the authenticated corresponding accesses; they are not a
claim of executing a complete update 1.5 world.

The immutable forward/up vectors and float-one operand equal the previously
verified `OriginalData::sound_driver_startup_operands`. The request interface
takes those 28 immutable bytes explicitly, independently of mutable object
storage. It neither imports pool pointers as constants nor fills a native Unit
with a captured final body.

## CPU contract

`source_object_body_from_datum` follows the original signed high-water
comparison, pool stride/backing pointer, nonzero signed salt comparison,
masked x86 type shift and header body pointer. NONE and actual failed lookups
return zero. Missing backing throws; it is never converted into a missing
object. Type and owner flags are read from actual backing, not native health,
render visibility or guessed role.

`source_object_creation_request_initialize` records the original 49-DWORD
clear and every later byte/word/DWORD store in order. It preserves the source
defaults and sentinel widths, including the absent-location WORD at `+70`.
For a valid owner it copies owner datum and bit 2 of owner `+C0` into request
bit `20`; only original biped/vehicle types propagate `+13C` and sign-extended
WORD `+138`. Optional location is read at the original point after owner
processing. No allocation return or generated object identity is supplied.

Native fixtures cover 62 complete old/update 1.5 direct calls, including
actual unit and weapon owners, NONE, salt mismatch, high-water rejection,
present/absent location and five type-mask controls for each lookup owner.
They compare all 902 ordered stores and 1,975,940 full output bytes, including
unchanged pool/table/unit/weapon backing. Initializer incidental EAX is not
treated as a return value; the original routine's interface is void.

## Validation and remaining work

Copied-source normal and ASan/UBSan builds use warnings as errors and pass the
literal original fixture. Focused production normal and actual ASan/UBSan
targets pass the same test. Observed source, archives, flags, link commands and
before/after CMake snapshots are retained; this is not a whole-tree freshness
or launcher build claim. The initial equivalent implementation with a less
explicit read-expression order remains separate preserved history, excluded
from the canonical freeze.

The native grenade path still needs constructor-owned full unit/action
storage, original player command dispatch, complete generic/type constructors,
marker/attachment/release owners and an advancing match/world lifecycle.
Live canister collision, explosions, reload playback and all-map sound parity
remain separate unfinished work. This checkpoint changes no runtime,
renderer, shaders or launcher.
