"""Full original/MCC authored-payload admission; no game services or assets."""
import hashlib
import json
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from audit_rendering import FormatError
from mcc_content import MccCache
import mcc_instant_charge as profile

FIXTURE=json.loads((Path(__file__).parent/'fixtures/mcc_instant_charge_reference.json').read_text())
# Complete actual same-map payload words retained by offline v554. These are
# independent of the planner's layout selection and normalized digest.
INLINE_MCC_RAW=struct.pack('<41I',
    0x2,0xffff,0,1,0x00de89f4,1,0x00de89f8,0x6c736e64,0xffffffff,0xffff,0,0,
    0x0400055b,0,0,0,0,0x3d088889,0x3d088889,1,0x00de8a30,0,0,0,0,0,0,
    0,0,0x6a707421,0x6a707421,0xeea20c6e,0,0,0,0,0,0,0,0,0)
INLINE_ORIGINAL_RAW=struct.pack('<41I',
    0x6,0xffff,0,1,0x80639c28,1,0x80639c2c,0x6c736e64,0xffffffff,0xffff,0,0,
    0x0400054f,0,0,0,0,0x3d088889,0x3d088889,1,0x80639c64,0,0,0,0,0,0,
    0,0,0x6a707421,0x6a707421,0xe36201ed,0,0,0,0,0,0,0,0,0)


def fixture():
    cache=object.__new__(MccCache)
    cache.data=bytearray(0x10000);cache.magic=0
    def put(at,value):struct.pack_into('<I',cache.data,at,value)
    for at,value in ((4,13),(48,0x55c),(52,0x3000),(56,5),(60,0x4000)):
        put(at,value)
    cache.data[0x3000:0x3005]=b'root\0';put(0x2000,0x0400055b)
    cache.data[0x1000:0x10a0]=bytes.fromhex(FIXTURE['mcc_raw_hex'])
    for field,value in ((0x10,0x2000),(0x18,0x1030),(0x4c,0x1068)):
        put(0x1000+field,value)
    effect=0xee214f14;damage=0xee224f15
    cache.tags={effect:dict(group='effe',name=FIXTURE['damage_name'],datum=effect,
        address=0x1000,offset=0x1000,size=160),damage:dict(group='jpt!',
        name=FIXTURE['damage_name'],datum=damage,address=0x6000,offset=0x6000,size=320)}
    return cache


def inline_fixture():
    cache=fixture()
    cache.data[0x1000:0x10a4]=INLINE_MCC_RAW
    cache.tags[0xee214f14]['size']=164
    for field,value in ((0x10,0x1030),(0x18,0x1034),(0x50,0x106c),(0x7c,0xee224f15)):
        struct.pack_into('<I',cache.data,0x1000+field,value)
    return cache


class InstantChargeProjection(unittest.TestCase):
    def test_complete_offline_original_fixture_matches_compiled_profile(self):
        raw=bytearray.fromhex(FIXTURE['original_raw_hex'])
        self.assertEqual(struct.unpack_from('<I',raw)[0],6)
        self.assertEqual(FIXTURE['original_cache_sha256'],profile.ORIGINAL_CACHE_SHA256)
        self.assertEqual(FIXTURE['original_xbe_sha256'],profile.ORIGINAL_XBE_SHA256)
        for offset,value in ((0,0),(0x10,0),(0x18,0x30),(0x4c,0x68),(0x78,0)):
            struct.pack_into('<I',raw,offset,value)
        self.assertEqual(hashlib.sha256(raw).hexdigest(),profile.NORMALIZED_ORIGINAL_SHA256)

    def test_exact_donor_changes_only_original_whole_flag_word_and_is_idempotent(self):
        cache=fixture();before=bytes(cache.data)
        plan=profile.plan_instant_charge(cache)
        self.assertEqual(plan.status,'ready');self.assertEqual(bytes(cache.data),before)
        receipt=profile.apply_instant_charge_plan(cache,plan)
        expected=bytearray(before);struct.pack_into('<I',expected,0x1000,6)
        self.assertEqual(cache.data,expected);self.assertEqual(receipt['changed_store_blocks'],1)
        again=profile.apply_instant_charge_plan(cache,profile.plan_instant_charge(cache))
        self.assertEqual(again['status'],'already_equivalent')
        self.assertEqual(again['changed_store_blocks'],0);self.assertEqual(cache.data,expected)

    def test_whole_inline_original_and_mcc_payloads_match_separate_authority(self):
        self.assertEqual(hashlib.sha256(INLINE_ORIGINAL_RAW).hexdigest(),
            '5567e761f59b8a1cdf44ed8b4679efe5b38ecae2bf13a2430bd7a03b0d7b8315')
        self.assertEqual(hashlib.sha256(INLINE_MCC_RAW).hexdigest(),
            '9481fe137fcbffd4f795c6f253843151d6f02ca72e1c3801f51fd5d0051dae61')
        self.assertEqual(profile.INLINE_ORIGINAL_CACHE_SHA256,
            'b4634a44ae6be16171d4c09837c0100e0052abcdaffb7160716912b7b0c2338c')
        for raw in (INLINE_ORIGINAL_RAW,INLINE_MCC_RAW):
            normalized=bytearray(raw)
            for field,value in ((0,0),(0x10,0x30),(0x18,0x34),(0x30,0),(0x50,0x6c),(0x7c,0)):
                struct.pack_into('<I',normalized,field,value)
            self.assertEqual(hashlib.sha256(normalized).hexdigest(),profile.NORMALIZED_INLINE_ORIGINAL_SHA256)

    def test_inline_tuple_changes_only_flags_preserving_allocation_and_is_idempotent(self):
        cache=inline_fixture();before=bytes(cache.data)
        plan=profile.plan_instant_charge(cache)
        self.assertEqual(plan.status,'ready');self.assertEqual(bytes(cache.data),before)
        receipt=profile.apply_instant_charge_plan(cache,plan)
        expected=bytearray(before);struct.pack_into('<I',expected,0x1000,6)
        self.assertEqual(cache.data,expected)
        self.assertEqual((receipt['changed_store_blocks'],receipt['changed_bytes']),(1,4))
        self.assertEqual(receipt['payload_layout'],'inline-root-164')
        self.assertEqual(receipt['original_cache_sha256'],profile.INLINE_ORIGINAL_CACHE_SHA256)
        self.assertEqual(receipt['normalized_original_sha256'],profile.NORMALIZED_INLINE_ORIGINAL_SHA256)
        again=profile.apply_instant_charge_plan(cache,profile.plan_instant_charge(cache))
        self.assertEqual(again['status'],'already_equivalent')
        self.assertEqual(again['changed_store_blocks'],0);self.assertEqual(cache.data,expected)

    def test_every_other_inline_payload_byte_is_guarded_before_any_write(self):
        separately_guarded={*range(0,4),*range(0x10,0x14),*range(0x18,0x1c),
                           *range(0x30,0x34),*range(0x50,0x54),*range(0x7c,0x80)}
        for offset in range(164):
            if offset in separately_guarded:continue
            cache=inline_fixture();cache.data[0x1000+offset]^=0x40;before=bytes(cache.data)
            with self.subTest(offset=offset):
                plan=profile.plan_instant_charge(cache)
                self.assertEqual(plan.status,'authored_tuple_unrecognized')
                self.assertEqual(profile.apply_instant_charge_plan(cache,plan)['changed_store_blocks'],0)
                self.assertEqual(cache.data,before)

    def test_inline_marker_and_nested_pointers_require_exact_local_ownership(self):
        for at,value in ((0x1010,0x2000),(0x1018,0x1030),(0x1050,0x1068),
                         (0x1030,0x0400055c),(0x1030,0x0300055b),(0x107c,0xffff4f15)):
            cache=inline_fixture();struct.pack_into('<I',cache.data,at,value);before=bytes(cache.data)
            with self.subTest(at=at,value=value):
                plan=profile.plan_instant_charge(cache)
                self.assertEqual(plan.status,'authored_tuple_unrecognized')
                profile.apply_instant_charge_plan(cache,plan);self.assertEqual(cache.data,before)
        cache=inline_fixture();cache.data[0x3000:0x3004]=b'fake'
        self.assertEqual(profile.plan_instant_charge(cache).status,'authored_tuple_unrecognized')

    def test_inline_custom_flags_damage_identity_and_allocation_sizes_are_preserved(self):
        for flags in (0,4,7,0x80000002):
            cache=inline_fixture();struct.pack_into('<I',cache.data,0x1000,flags);before=bytes(cache.data)
            with self.subTest(flags=flags):
                plan=profile.plan_instant_charge(cache)
                self.assertEqual(plan.status,'authored_tuple_unrecognized')
                profile.apply_instant_charge_plan(cache,plan);self.assertEqual(cache.data,before)
        for field,value in (('datum',0x12344f15),('group','proj'),('name','custom\\damage'),('offset',None)):
            cache=inline_fixture();cache.tags[0xee224f15][field]=value;before=bytes(cache.data)
            with self.subTest(field=field):
                plan=profile.plan_instant_charge(cache)
                self.assertEqual(plan.status,'authored_tuple_unrecognized')
                profile.apply_instant_charge_plan(cache,plan);self.assertEqual(cache.data,before)
        for size in (159,161,163,165,168):
            cache=inline_fixture();cache.tags[0xee214f14]['size']=size;before=bytes(cache.data)
            with self.subTest(size=size):
                plan=profile.plan_instant_charge(cache)
                self.assertEqual(plan.status,'authored_tuple_unrecognized')
                profile.apply_instant_charge_plan(cache,plan);self.assertEqual(cache.data,before)

    def test_inline_marker_string_id_is_resolved_without_rewriting_it(self):
        cache=inline_fixture()
        # Same actual root string, independently relocated into another local
        # table index. A source StringID must never be copied across caches.
        struct.pack_into('<I',cache.data,0x1030,0x0400055a)
        before=bytes(cache.data)
        receipt=profile.apply_instant_charge_plan(cache,profile.plan_instant_charge(cache))
        self.assertEqual(receipt['status'],'ready')
        self.assertEqual(cache.u32(0x1030),0x0400055a)
        self.assertEqual(cache.data[0x1004:],before[0x1004:])

    def test_every_other_payload_byte_is_guarded_before_any_write(self):
        # Pointer words and the reference salt have separate identity guards.
        normalized_fields={*range(0,4),*range(0x10,0x14),*range(0x18,0x1c),
                           *range(0x4c,0x50),*range(0x78,0x7c)}
        for offset in range(160):
            if offset in normalized_fields:continue
            cache=fixture();cache.data[0x1000+offset]^=0x40;before=bytes(cache.data)
            with self.subTest(offset=offset):
                plan=profile.plan_instant_charge(cache)
                self.assertEqual(plan.status,'authored_tuple_unrecognized')
                self.assertEqual(profile.apply_instant_charge_plan(cache,plan)['changed_store_blocks'],0)
                self.assertEqual(cache.data,before)

    def test_no_blanket_bit_or_and_unknown_flag_words_are_preserved(self):
        for flags in (0,4,7,0x80000002):
            cache=fixture();struct.pack_into('<I',cache.data,0x1000,flags);before=bytes(cache.data)
            with self.subTest(flags=flags):
                plan=profile.plan_instant_charge(cache)
                self.assertEqual(plan.status,'authored_tuple_unrecognized')
                profile.apply_instant_charge_plan(cache,plan);self.assertEqual(cache.data,before)

    def test_full_salt_group_name_and_resident_damage_owner_are_required(self):
        for field,value in (('datum',0x12344f15),('group','proj'),('name','custom\\damage'),('offset',None)):
            cache=fixture();cache.tags[0xee224f15][field]=value;before=bytes(cache.data)
            with self.subTest(field=field):
                self.assertEqual(profile.plan_instant_charge(cache).status,'authored_tuple_unrecognized')
                self.assertEqual(cache.data,before)
        cache=fixture();struct.pack_into('<I',cache.data,0x1078,0xffff4f15)
        self.assertEqual(profile.plan_instant_charge(cache).status,'authored_tuple_unrecognized')

    def test_marker_and_nested_pointer_ownership_are_not_ignored(self):
        for at,value in ((0x1018,0x2030),(0x104c,0x2068),(0x2000,0x0400055c)):
            cache=fixture();struct.pack_into('<I',cache.data,at,value);before=bytes(cache.data)
            with self.subTest(at=at):
                self.assertEqual(profile.plan_instant_charge(cache).status,'authored_tuple_unrecognized')
                self.assertEqual(cache.data,before)
        cache=fixture();cache.data[0x3000:0x3004]=b'fake'
        self.assertEqual(profile.plan_instant_charge(cache).status,'authored_tuple_unrecognized')

    def test_stale_plan_does_not_publish_even_when_flag_owner_is_unchanged(self):
        cache=fixture();plan=profile.plan_instant_charge(cache);cache.data[0x9000]=1;before=bytes(cache.data)
        with self.assertRaises(FormatError):profile.apply_instant_charge_plan(cache,plan)
        self.assertEqual(cache.data,before)

    def test_missing_effect_is_noop_and_non_mcc_or_duplicate_identity_rejects(self):
        cache=fixture();del cache.tags[0xee214f14]
        self.assertEqual(profile.plan_instant_charge(cache).status,'not_present')
        cache=fixture();struct.pack_into('<I',cache.data,4,8)
        with self.assertRaises(FormatError):profile.plan_instant_charge(cache)
        cache=fixture();cache.tags[0xef114f16]=dict(cache.tags[0xee214f14],datum=0xef114f16)
        with self.assertRaises(FormatError):profile.plan_instant_charge(cache)


if __name__=='__main__':unittest.main()
