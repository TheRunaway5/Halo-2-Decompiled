"""Named operand linking for offline TU1.5 execution on MCC cache inputs.

This is an explicit reference mode, separate from untouched-image replay.
Only authenticated compiled StringID immediates are linked. No branch, call,
authored field, clock, damage, collision or solver result is substituted.
"""
import hashlib
import re
from pathlib import Path

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from object_function_symbols_tu15 import OBJECT_FUNCTION_SYMBOLS


class MccSymbolReferenceTu15:
    def __init__(self, image, root):
        root = Path(root)
        table = root / 'src/ui/mcc_frontend_string_ids.inc'
        names = {name: ((len(name) << 24) | int(old, 16), int(new, 16))
                 for old, new, name in re.findall(
                     r'\{0x([0-9a-f]+)u,0x([0-9a-f]+)u,"([^"]*)"\}', table.read_text())}
        sites = dict(OBJECT_FUNCTION_SYMBOLS)
        # The live body owner links these two exact non-dual grip queries.
        # Their cached channel indices differ on MCC inputs without the same
        # authenticated name join in this explicit reference mode.
        for pc, name in ((0x115d68, 'grip'), (0x115d83, 'grip'),
                         (0x17b232, 'any'), (0x17b2ec, 'replace'), (0x17b35d, 'root'),
                         (0x101cad, 'forward'), (0x101cc5, 'backward'),
                         (0x100ea4, 'range_remaining'), (0x100eb0, 'tracer'),
                         (0x100eb7, 'time_remaining'), (0x100f04, 'projectile_attach'),
                         (0x100f0b, 'acceleration_range'), (0x15cbc2, 'gauss'),
                         (0x15ccb9, 'multiplayer'), (0x15ccce, 'multiplayer')):
            old, new = names[name]
            sites[pc] = (old, new, new, name)
        decoder = Cs(CS_ARCH_X86, CS_MODE_32)
        decoder.detail = True
        self.patches = []
        for pc, (old, new, target, name) in sorted(sites.items()):
            assert names[name][1] == target, f'Unverified MCC name join at {pc:x}'
            instruction = next(decoder.disasm(image.read(pc, 15), pc))
            assert instruction.mnemonic in ('cmp', 'sub', 'mov'), f'Unexpected symbol opcode at {pc:x}'
            assert instruction.imm_size == 4, f'Unexpected symbol width at {pc:x}'
            offset = instruction.imm_offset
            assert int.from_bytes(instruction.bytes[offset:offset+4], 'little') == old, f'Original symbol differs at {pc:x}'
            if old == new:
                continue
            self.patches.append(dict(instruction=pc, address=pc+offset, name=name,
                                     original=old, linked=new,
                                     original_instruction=instruction.bytes.hex()))
        self.receipt = dict(
            mode='Authenticated MCC StringID operand linking; original opcodes and control flow',
            name_table_sha256=hashlib.file_digest(table.open('rb'), 'sha256').hexdigest(),
            profile_sha256=hashlib.file_digest((root/'tools/object_function_symbols_tu15.py').open('rb'), 'sha256').hexdigest(),
            relocations=self.patches,
            supplied_gameplay_or_physics_results=[])

    def link(self, address, raw):
        relevant = [p for p in self.patches if p['address'] < address+len(raw) and p['address']+4 > address]
        if not relevant:
            return raw
        linked = bytearray(raw)
        for patch in relevant:
            first = max(address, patch['address'])
            end = min(address+len(raw), patch['address']+4)
            lo, hi = first-patch['address'], end-patch['address']
            old = patch['original'].to_bytes(4, 'little')[lo:hi]
            new = patch['linked'].to_bytes(4, 'little')[lo:hi]
            assert raw[first-address:end-address] == old, f'Changed original operand at {first:x}'
            linked[first-address:end-address] = new
        return bytes(linked)
