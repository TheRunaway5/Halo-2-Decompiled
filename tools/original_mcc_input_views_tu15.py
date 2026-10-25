"""Validate recorded bitmap descriptor input views for offline original code.

Only the real canonical bitmap's read-only row pointer is re-expressed. Every
consumed CPU row field and resource must exist in the captured BEFORE image. No after-state,
resource admission, allocation, sampler or gameplay result is supplied.
"""
import hashlib
import json
from pathlib import Path


class MccBitmapInputViewsTu15:
    def __init__(self, capture, regions):
        capture = Path(capture)
        path = capture/'external-inputs.json'
        recorded = json.loads(path.read_text())
        assert recorded['supplied_gameplay_results'] is False

        def read(address, size):
            for region in regions:
                if region['address'] <= address and address+size <= region['address']+region['size']:
                    with (capture/region['before_file']).open('rb') as stream:
                        stream.seek(address-region['address'])
                        result = stream.read(size)
                    assert len(result) == size
                    return result
            raise ValueError(f'Input view has no captured BEFORE backing at {address:x}+{size}')

        def word(address):
            return int.from_bytes(read(address, 4), 'little')

        table = word(0x4f0414)
        self.aliases = {}
        for alias in recorded['memory_read_aliases']:
            assert alias['entry'] == 0x13a030 and alias['caller'] == 0xd550b
            row = table+(alias['tag'] & 65535)*16
            assert word(row) == 0x6269746d and word(row+4) == alias['tag'], 'Input view lost its full-salt bitmap'
            root = word(row+8)
            assert word(row+12) >= 0x4c and alias['address'] == root+0x48
            assert word(alias['address']) == alias['canonical_value']
            count = word(root+0x44)
            assert 0 <= alias['selector'] < count <= 65536
            # Backing deliberately commits only the original consumed fields,
            # at four-byte granularity. The gaps are not resource inputs and
            # must not be populated from after-state or invented zero rows.
            cpu_row = alias['read_value']+alias['selector']*116
            fields = [(0, 24), (0x34, 4), (0x50, 8), (0x70, 4)]
            for offset, size in fields:
                read(cpu_row+offset, size)
            header, encoded = word(cpu_row+0x50), word(cpu_row+0x54)
            read(header, 20)
            read(encoded, word(cpu_row+0x34))
            previous = self.aliases.setdefault(alias['address'], alias)
            assert previous == alias or all(previous[k] == alias[k] for k in (
                'entry', 'caller', 'address', 'canonical_value', 'read_value', 'tag'))
        self.receipt = dict(mode='Actual read-only canonical bitmap descriptor input view',
                            input_transcript_sha256=hashlib.file_digest(path.open('rb'), 'sha256').hexdigest(),
                            aliases=list(self.aliases.values()), cpu_rows_present_before_execution=True,
                            supplied_after_state_or_gameplay_results=[])

    def link(self, address, raw):
        relevant = [p for p in self.aliases.values() if p['address'] < address+len(raw) and p['address']+4 > address]
        if not relevant:
            return raw
        result = bytearray(raw)
        for alias in relevant:
            first, end = max(address, alias['address']), min(address+len(raw), alias['address']+4)
            lo, hi = first-alias['address'], end-alias['address']
            assert raw[first-address:end-address] == alias['canonical_value'].to_bytes(4, 'little')[lo:hi], \
                'Canonical descriptor changed during original lookup'
            result[first-address:end-address] = alias['read_value'].to_bytes(4, 'little')[lo:hi]
        return bytes(result)
