#!/usr/bin/env python3
"""Original Xbox CPU upload wait/return with separately executed GP transfers.

Two stages avoid overlapping the original-x86 audit and GP process inside the
60/64MiB aggregate budget. Prefix saves actual original CPU state at the poll.
Resume admits only the recorded GP scratch writes, then runs unchanged CPU
instructions to the real caller return. Startup/frame/timer remain declared
reference-model inputs. No CPU result, request-clear literal or clock is given.
"""
import argparse
from bisect import bisect_right
import hashlib
import json
from pathlib import Path
import struct
from unicorn import (Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE,
                     UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_WRITE)
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

GP_NAMES = ('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI')
POLL = 0x388470
RETURN = 0x2100000
GP_RAM_OUTPUT_BYTES = (0x1000 + 0xc00 + 0x800) * 4
CONTEXT_PIN = '9557278da666fe0f7de21831140f86bfc4ccc882064e6b45ed318a45228433f4'


def sha(path):
    with path.open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def save(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n')


class Audit:
    def __init__(self, directory, manifest):
        self.directory, self.manifest = directory, manifest
        self.image = Xbe(NEW_DEFAULT)
        assert hashlib.sha256(self.image.data).hexdigest() == NEW_PIN
        assert not manifest['source_request_clear_supplied']
        self.machine = Uc(UC_ARCH_X86, UC_MODE_32)
        self.rows = sorted(manifest['regions'], key=lambda row: row['address'])
        self.starts = [row['address'] for row in self.rows]
        self.raw, self.touched = [], {}
        pages = set()
        for row in self.rows:
            pages.update(range(row['address'] & ~4095,
                               (row['address'] + row['bytes'] + 4095) & ~4095, 4096))
        for page in sorted(pages):
            self.machine.mem_map(page, 4096)
        for row in self.rows:
            raw = (directory / row['file']).read_bytes()
            assert len(raw) == row['bytes']
            if 'sha256' in row:
                assert hashlib.sha256(raw).hexdigest() == row['sha256']
            self.raw.append(raw)
            self.machine.mem_write(row['address'], raw)
        abi = manifest['abi']
        assert abi['x87_depth'] == 0
        for name, value in zip(GP_NAMES, abi['gp']):
            self.machine.reg_write(getattr(R, 'UC_X86_REG_' + name), value)
        for i, words in enumerate(abi['xmm']):
            self.machine.reg_write(getattr(R, f'UC_X86_REG_XMM{i}'),
                                   int.from_bytes(struct.pack('<4I', *words), 'little'))
        for register, value in ((R.UC_X86_REG_EFLAGS, abi['eflags']),
                                (R.UC_X86_REG_FPCW, abi['x87_control']),
                                (R.UC_X86_REG_FPSW, abi['x87_status']),
                                (R.UC_X86_REG_FPTAG, 0xffff),
                                (R.UC_X86_REG_MXCSR, abi['mxcsr'])):
            self.machine.reg_write(register, value)
        self.sites, self.stores, self.digest = set(), 0, hashlib.sha256()

    def spans(self, address, size, write=False):
        assert address < 0xfe800000, 'Unowned hardware read/write'
        while size:
            index = bisect_right(self.starts, address) - 1
            assert index >= 0, 'Unowned declared CPU address'
            row = self.rows[index]
            within = address - row['address']
            assert within < row['bytes'], 'Mapped-page padding has no declared owner'
            assert not write or not row['read_only'], 'Original CPU wrote immutable input'
            count = min(size, row['bytes'] - within)
            yield index, within, count
            address, size = address + count, size - count

    def scratch(self):
        binding = self.manifest['gp_scratch']
        assert binding['entries'] == len(binding['rows']) == 201
        result = bytearray()
        for i, row in enumerate(binding['rows']):
            assert row['offset'] == len(result) == i * 4096 and row['bytes'] == 4096
            physical, high = struct.unpack('<2I', self.machine.mem_read(binding['table_source'] + i * 8, 8))
            assert not high and physical == row['physical']
            list(self.spans(row['source'], row['bytes']))
            result.extend(self.machine.mem_read(row['source'], row['bytes']))
        return bytes(result)

    def device_write(self, offset, payload):
        """Actual recorded GP bus payload; this is not a CPU store or result."""
        rows = self.manifest['gp_scratch']['rows']
        while payload:
            page, within = divmod(offset, 4096)
            row = rows[page]
            count = min(len(payload), 4096 - within)
            address, part = row['source'] + within, payload[:count]
            consumed = 0
            for index, at, length in self.spans(address, count, write=True):
                if isinstance(self.raw[index], bytes):
                    self.raw[index] = bytearray(self.raw[index])
                self.raw[index][at:at + length] = part[consumed:consumed + length]
                consumed += length
            self.machine.mem_write(address, part)
            payload, offset = payload[count:], offset + count

    def execute(self, entry, boundary):
        def instruction(machine, pc, size, _):
            if pc == boundary:
                machine.emu_stop()
                return
            assert bytes(machine.mem_read(pc, size)) == self.image.read(pc, size), ('Changed original code', hex(pc))
            self.sites.add(pc)

        def access(machine, kind, address, size, value, _):
            for index, at, length in self.spans(address, size, kind == UC_MEM_WRITE):
                mask = self.touched.get(index)
                if mask is None:
                    mask = bytearray(self.rows[index]['bytes'])
                    self.touched[index] = mask
                mask[at:at + length] = b'\1' * length
            if kind == UC_MEM_WRITE:
                previous = bytes(machine.mem_read(address, size))
                written = (value & ((1 << (8 * size)) - 1)).to_bytes(size, 'little')
                self.digest.update(struct.pack('<3I', machine.reg_read(R.UC_X86_REG_EIP), address, size))
                self.digest.update(previous)
                self.digest.update(written)
                self.stores += 1

        self.machine.hook_add(UC_HOOK_CODE, instruction)
        self.machine.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, access)
        self.machine.emu_start(entry, boundary, count=5000000)
        assert self.machine.reg_read(R.UC_X86_REG_EIP) == boundary

    def abi(self):
        assert self.machine.reg_read(R.UC_X86_REG_FPTAG) == 0xffff
        return dict(gp=[self.machine.reg_read(getattr(R, 'UC_X86_REG_' + name)) for name in GP_NAMES],
                    xmm=[list(struct.unpack('<4I', self.machine.reg_read(getattr(R, f'UC_X86_REG_XMM{i}')).to_bytes(16, 'little'))) for i in range(8)],
                    eflags=self.machine.reg_read(R.UC_X86_REG_EFLAGS), fs_base=self.manifest['abi']['fs_base'],
                    x87_depth=0, x87_status=self.machine.reg_read(R.UC_X86_REG_FPSW),
                    x87_control=self.machine.reg_read(R.UC_X86_REG_FPCW),
                    mxcsr=self.machine.reg_read(R.UC_X86_REG_MXCSR))

    def case(self, entry, boundary):
        before, after = {}, {}
        for index, mask in sorted(self.touched.items()):
            at = 0
            while at < len(mask):
                start = mask.find(b'\1', at)
                if start < 0:
                    break
                end = mask.find(b'\0', start)
                if end < 0:
                    end = len(mask)
                address = self.rows[index]['address'] + start
                before[f'{address:08x}'] = self.raw[index][start:end].hex()
                after[f'{address:08x}'] = bytes(self.machine.mem_read(address, end - start)).hex()
                at = end
        abi = self.abi()
        return dict(scope=__doc__, xbe_sha256=NEW_PIN, entry=entry, boundary=boundary,
                    before=before, after=after, gp_after=abi['gp'], xmm_after=abi['xmm'],
                    eflags_after=abi['eflags'], abi_after=abi,
                    store_count=self.stores, store_sha256=self.digest.hexdigest(),
                    executed_instructions=sorted(self.sites), instruction_patches=[],
                    supplied_external_results=[], supplied_request_clear=False,
                    full_engine_verified=False, hardware_clock_verified=False)

    def checkpoint(self, output, pc):
        regions = []
        for i, row in enumerate(self.rows):
            raw = bytes(self.machine.mem_read(row['address'], row['bytes']))
            filename = f'region-{i}.bin'
            (output / filename).write_bytes(raw)
            regions.append(dict(address=row['address'], bytes=row['bytes'], file=filename,
                                read_only=row['read_only'], sha256=hashlib.sha256(raw).hexdigest()))
        manifest = dict(scope=__doc__, pc=pc, abi=self.abi(), root=self.manifest['root'], regions=regions,
                        gp_scratch=self.manifest['gp_scratch'], source_request_clear_supplied=False)
        save(output / 'checkpoint.json', manifest)


def prefix(args):
    manifest = json.loads((args.native_before / 'before.json').read_text())
    assert manifest['pc'] == 0x388383
    audit = Audit(args.native_before, manifest)
    audit.execute(0x388383, POLL)
    data = audit.scratch()
    assert struct.unpack_from('<I', data, 0x810)[0] == 3
    args.output.mkdir()
    audit.checkpoint(args.output, POLL)
    gp = args.output / 'gp-input'
    gp.mkdir()
    (gp / 'initial-scratch.bin').write_bytes(data)
    case = audit.case(0x388383, POLL)
    case['native_before_manifest_sha256'] = sha(args.native_before / 'before.json')
    case['gp_input_sha256'] = hashlib.sha256(data).hexdigest()
    save(args.output / 'prefix-case.json', case)
    print(f'PASS original CPU upload prefix: {len(audit.sites)} addresses, {audit.stores} stores; retained real poll/SGE input for separate GP execution')


def resume(args):
    manifest = json.loads((args.checkpoint / 'checkpoint.json').read_text())
    assert manifest['pc'] == POLL
    audit = Audit(args.checkpoint, manifest)
    before = (args.checkpoint / 'gp-input/initial-scratch.bin').read_bytes()
    assert audit.scratch() == before
    context = args.gp_run / 'reference-context.json'
    assert sha(context) == CONTEXT_PIN
    log = (args.gp_run / 'gp.log').read_text().splitlines()
    stats = json.loads(log[0])
    assert stats['format2'] == 'unverified' and stats['dma'] == 'reference'
    assert stats['declared_frame_events'] == 1 and stats['pc'] == 0x11e
    raw = (args.gp_run / 'gp-output.bin').read_bytes()
    assert len(raw) == GP_RAM_OUTPUT_BYTES + len(before)
    after = raw[GP_RAM_OUTPUT_BYTES:]
    writes = [json.loads(line) for line in (args.gp_run / 'gp-events.jsonl').read_text().splitlines()
              if json.loads(line)['event'] == 'scratch' and json.loads(line)['write']]
    assert len(writes) == 1
    for row in writes:
        assert row['instruction_active'] and row['pc'] == 0x11c
        payload = bytes.fromhex(row['first64_hex'])
        assert len(payload) == row['bytes'] == 24 and row['offset'] == 0x800
        assert hashlib.sha256(payload).hexdigest() == row['sha256']
        assert payload == after[row['offset']:row['offset'] + len(payload)]
        audit.device_write(row['offset'], payload)
    assert audit.scratch() == after
    audit.execute(POLL, RETURN)
    case = audit.case(POLL, RETURN)
    case['consumed_gp_writes'] = writes
    case['gp_result_sha256'] = sha(args.gp_run / 'gp-output.bin')
    case['gp_trace_sha256'] = sha(args.gp_run / 'gp-events.jsonl')
    case['declared_gp_context_sha256'] = sha(context)
    case['original_checkpoint_sha256'] = sha(args.checkpoint / 'checkpoint.json')
    assert case['gp_after'][0] == 0
    args.output.mkdir()
    save(args.output / 'case.json', case)
    print(f'PASS unchanged original CPU upload wait/return: {len(audit.sites)} addresses, {audit.stores} stores; actual recorded GP DMA payload, no source result/request clear supplied')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    phase = parser.add_mutually_exclusive_group(required=True)
    phase.add_argument('--native-before', type=Path)
    phase.add_argument('--checkpoint', type=Path)
    parser.add_argument('--gp-run', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise ValueError('Preserve prior original CPU/GP return evidence')
    if args.native_before is not None:
        assert args.gp_run is None
        prefix(args)
    else:
        assert args.gp_run is not None
        resume(args)


if __name__ == '__main__':
    main()
