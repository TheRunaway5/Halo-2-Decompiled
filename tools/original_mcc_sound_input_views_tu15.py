"""Authenticate original sound arithmetic inputs for independent CPU execution.

The existing native timing owner presents verified original codec/encoded-byte
operands only for four reads in the complete original leaf. The CPU observer
reproduces those read views, restores canonical bytes immediately, and executes
every original instruction. It never provides duration, FPU or gameplay results.
"""
import hashlib
import json
import re
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_32


class MccSoundInputViewsTu15:
    def __init__(self, capture, regions, image, root):
        path = capture/'external-inputs.json'
        data = json.loads(path.read_text())
        assert data['supplied_gameplay_results'] is False
        views = data.get('sound_timing_input_views', [])
        assert len(views) <= 128
        self.views = []
        self.pcs = []
        self.receipt = dict(input_transcript_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                            supplied_duration_cpu_or_gameplay_results=[], views=[])
        if not views:
            return
        profile = root/'src/audio/mcc_sound_timing_reference_profiles.inc'
        encoded = bytes(int(x, 16) for x in re.findall(r'\\x([0-9a-f]{2})', profile.read_text()))
        assert len(encoded) == 24624*81
        records = {encoded[i:i+32]: encoded[i:i+81] for i in range(0, len(encoded), 81)}
        assert len(records) == 24624
        assert hashlib.sha256(b''.join(encoded[i:i+71] for i in range(0, len(encoded), 81))).hexdigest() == \
            'b0907b1f10d46f9e77f5d3b5e05e5d4e413ce619b0a317c1ed89dd5c4765cb91'
        assert hashlib.sha256(image.read(0x21f590, 211)).hexdigest() == \
            '6cab0fba83a56a36f6f934ef61eb96eb2105140cee8ac5c9039574ed918810f7'
        sites = root/'src/audio/mcc_sound_duration_pcs.inc'
        self.pcs = [int(x, 16) for x in re.findall(r'0x([0-9a-f]+)u', sites.read_text())]
        assert len(self.pcs) == 67 and self.pcs[0] == 0x21f590 and self.pcs[-1] == 0x21f660
        decoder = Cs(CS_ARCH_X86, CS_MODE_32)
        for pc in self.pcs:
            assert next(decoder.disasm(image.read(pc, 15), pc)).address == pc

        def read(address, size):
            for region in regions:
                if region['address'] <= address and address+size <= region['address']+region['size']:
                    with (capture/region['before_file']).open('rb') as stream:
                        stream.seek(address-region['address'])
                        result = stream.read(size)
                    assert len(result) == size
                    return result
            raise ValueError(f'No original sound input backing at {address:x}+{size}')

        def word(address):
            return int.from_bytes(read(address, 4), 'little')

        table = word(0x4f0414)
        for view in views:
            assert view['entry'] == 0x21f590 and view['return_address'] == 0x21f6f8
            gp = view['gp']
            assert len(gp) == 8 and gp[6] == view['root'] and gp[7] == view['range']
            assert gp[1]+gp[2] == view['permutation']
            assert view['stack'] == [view['return_address'], gp[7], gp[0]]
            for handle, group, expected_root in ((view['sound'], 0x736e6421, view['root']),
                                                  (view['gestalt'], 0x75676821, None)):
                row = table+(handle & 65535)*16
                assert word(row) == group and word(row+4) == handle
                if expected_root is not None:
                    assert word(row+8) == expected_root
                else:
                    read(word(row+8), 0x58)
            primary = word(0x52d664)
            assert gp[3] == primary and gp[2] == word(primary+0x2c) and gp[1] % 16 == 0
            pitch_bank = word(primary+0x24)
            assert gp[7] >= pitch_bank and (gp[7]-pitch_bank) % 12 == 0
            key_input = b'H2_TU15_SOUND_TIMING_KEY_V1\0'
            for name in (view['path'], view['range_name'], view['permutation_name']):
                raw = name.encode()
                assert 0 < len(raw) <= 0x100000
                key_input += struct.pack('<I', len(raw))+raw
            key = hashlib.sha256(key_input).digest()
            assert key.hex() == view['named_key'] and key in records
            record = records[key]
            assert record[32:64].hex() == view['encoded_payload']
            size, rate, encoding, compression = struct.unpack_from('<IBBB', record, 64)
            assert [rate, encoding, compression, size] == view['original_operands']
            codec = read(gp[6]+3, 3)
            assert codec.hex() == view['canonical_codec']
            reads = [dict(pc=pc, address=gp[6]+offset, width=1, before=codec[offset-3], value=value)
                     for pc, offset, value in ((0x21f5e6, 5, compression),
                                               (0x21f618, 4, encoding), (0x21f623, 3, rate))]
            reads.append(dict(pc=0x21f627, address=gp[4]+8, width=4, before=gp[0], value=size))
            self.views.append(dict(entry=view['entry'], return_address=view['return_address'],
                                   gp=gp, stack=view['stack'], reads=reads))
        self.receipt.update(mode='Authenticated original codec/encoded-byte operands at four read instructions; original leaf fully executed',
            profile_sha256=hashlib.sha256(profile.read_bytes()).hexdigest(),
            leaf_instruction_sites=self.pcs, views=views)

    def bindings(self):
        return dict(sound_timing_views=self.views, sound_timing_pcs=self.pcs)
