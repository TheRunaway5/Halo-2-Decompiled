"""Project one complete vanilla MCC instant-charge effect to original raw flags.

This is a preparation-time data profile, never a gameplay service or runtime
Xbox dependency. A full160-byte or separately qualified164-byte definition,
root marker and salted jpt!
identity must agree before the exact stored flag word2 may become6. The
original cache already stores6 before any postprocessor; no part-derived bit
heuristic is used. The caller owns transactional serialization of MCC v13.
"""
from dataclasses import dataclass
import hashlib
import struct

from audit_rendering import FormatError

PROFILE = 'H2_MCC_INSTANT_PISTOL_CHARGE_REFERENCE_V1'
NAME = r'objects\characters\sentinel_aggressor\weapons\charged_bolt\damage_effects\sentinel_charging'
ORIGINAL_CACHE_SHA256 = '14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9'
ORIGINAL_XBE_SHA256 = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
# Original Lockout's complete160B payload: only pointers, the independently
# validated local damage datum, and the selected flag word are normalized.
# Root/event/part placement is fixed30/68; the external location is "root".
NORMALIZED_ORIGINAL_SHA256 = '4dc1c38dda9e4831240a1a85c3650faab330e493679ce9263f039677b8c93445'
# Same-map original Floodlab carries the root StringID inside its164B owner:
# locations30, event34, part6C. This separate whole-payload authority comes
# from the offline same-map audit in work/.../mcc-floodlab-charge-tag-audit-v554.json.
INLINE_ORIGINAL_CACHE_SHA256 = 'b4634a44ae6be16171d4c09837c0100e0052abcdaffb7160716912b7b0c2338c'
NORMALIZED_INLINE_ORIGINAL_SHA256 = 'cec002c635600f435c65be686145849c8bcc0dab94c3ad1a01b02bc0664289e6'


def digest(data):
    return hashlib.sha256(data).hexdigest()


@dataclass(frozen=True)
class InstantChargePlan:
    decoded_sha256: str
    status: str
    reason: str
    offset: int | None = None
    before: bytes = b''
    after: bytes = b''
    datum: int | None = None
    damage: int | None = None
    payload_layout: str | None = None
    normalized_original_sha256: str = NORMALIZED_ORIGINAL_SHA256
    original_cache_sha256: str = ORIGINAL_CACHE_SHA256


def plan_instant_charge(cache):
    """Read-only plan for a real decoded MccCache; custom definitions stay put."""
    if cache.u32(4) != 13:
        raise FormatError('Instant charge projection requires MCC version13')
    decoded = digest(cache.data)
    matches = [tag for tag in cache.tags.values() if tag['group'] == 'effe' and
               tag['name'] == NAME and tag['offset'] is not None]
    if not matches:
        return InstantChargePlan(decoded, 'not_present', 'No resident named instant-charge effect')
    if len(matches) != 1:
        raise FormatError('Instant charge projection has duplicate effect identities')
    tag = matches[0]
    def unknown(reason):
        return InstantChargePlan(decoded, 'authored_tuple_unrecognized', reason, datum=tag['datum'])
    if tag['size'] not in (160, 164):
        return unknown('Complete vanilla effect must own exactly160 or164 bytes')
    inline = tag['size'] == 164
    at, address = tag['offset'], tag['address']
    cache.require(at, tag['size'])
    raw = bytes(cache.data[at:at + tag['size']])
    flags = struct.unpack_from('<I', raw)[0]
    if flags not in (2, 6):
        return unknown('Flag word is outside the exact donor/equivalent tuple')
    # Each supported layout has exact authenticated nested record placement.
    event, part = (0x34, 0x6c) if inline else (0x30, 0x68)
    if cache.u32(at + 0x18) != address + event or cache.u32(at + event + 0x1c) != address + part:
        return unknown('Event/part pointers differ from the complete vanilla owner')
    if inline and cache.u32(at + 0x10) != address + 0x30:
        return unknown('Inline root marker pointer escapes its authenticated location')
    location = cache.pointer(cache.u32(at + 0x10), cache.magic, 4)
    identifier = cache.u32(location)
    index = identifier & 0x00ffffff
    count, size = cache.u32(48), cache.u32(56)
    if not count <= 1000000 or index >= count:
        return unknown('Root marker has no complete MCC string owner')
    table, strings = cache.u32(60), cache.u32(52)
    cache.require(table, count * 4);cache.require(strings, size)
    offset = cache.u32(table + index * 4)
    if offset >= size or identifier >> 24 != 4 or cache.string(strings + offset, size - offset) != 'root':
        return unknown('Effect location is outside the authenticated root marker identity')
    datum = cache.u32(at + part + 0x10)
    damage = cache.tags.get(datum)
    if (not damage or damage['datum'] != datum or damage['group'] != 'jpt!' or
            damage['name'] != NAME or damage['offset'] is None):
        return unknown('Effect part has no actual local full-salt jpt! identity')
    normalized = bytearray(raw)
    fields = ((0, 0), (0x10, 0), (0x18, event), (event + 0x1c, part), (part + 0x10, 0))
    if inline:
        fields = ((0, 0), (0x10, 0x30), (0x18, event), (0x30, 0),
                  (event + 0x1c, part), (part + 0x10, 0))
    for field, value in fields:
        struct.pack_into('<I', normalized, field, value)
    expected = NORMALIZED_INLINE_ORIGINAL_SHA256 if inline else NORMALIZED_ORIGINAL_SHA256
    if digest(normalized) != expected:
        return unknown(f'Full normalized{tag["size"]}B payload differs from original raw authority')
    return InstantChargePlan(decoded, 'already_equivalent' if flags == 6 else 'ready',
        'Complete original instant jpt! graph and root marker match', at, raw[:4],
        struct.pack('<I', 6), tag['datum'], datum,
        'inline-root-164' if inline else 'external-root-160', expected,
        INLINE_ORIGINAL_CACHE_SHA256 if inline else ORIGINAL_CACHE_SHA256)


def apply_instant_charge_plan(cache, plan):
    """Apply the preflighted word in memory; never open/write an asset file."""
    if digest(cache.data) != plan.decoded_sha256:
        raise FormatError('Instant charge plan has stale decoded input')
    changed = 0
    if plan.status in ('ready', 'already_equivalent'):
        if plan.offset is None or bytes(cache.data[plan.offset:plan.offset + 4]) != plan.before:
            raise FormatError('Instant charge flag owner changed before publication')
        if plan.status == 'ready':
            cache.data[plan.offset:plan.offset + 4] = plan.after
            changed = 1
    return dict(profile=PROFILE, status=plan.status, reason=plan.reason,
        changed_store_blocks=changed, changed_bytes=4 if changed else 0,
        effect=plan.datum, damage=plan.damage, source_decoded_sha256=plan.decoded_sha256,
        resulting_decoded_sha256=digest(cache.data), payload_layout=plan.payload_layout,
        normalized_original_sha256=plan.normalized_original_sha256,
        original_cache_sha256=plan.original_cache_sha256, original_xbe_sha256=ORIGINAL_XBE_SHA256,
        original_assets_runtime_input=False, full_effect_or_weapon_parity=False)
