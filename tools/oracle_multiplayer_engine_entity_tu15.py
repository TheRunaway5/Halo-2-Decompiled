#!/usr/bin/env python3
"""Compare native engine entity projections with complete unchanged Xbox calls.

Native receipts contain actual retained world reads, never a supplied callee
return. The Xbox runs constructors/differences from the authenticated image in
a separate, source-topology-checked caller frame. All world writes and reads
outside the captured actual input bytes fail. No original instruction is patched.
Run through tools/run_test.py; keep only the small comparison report.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from unicorn import (Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE,
                     UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE)
import unicorn.x86_const as R
from generate_multiplayer_engines_tu15 import Xbe, NEW_DEFAULT, NEW_PIN

REGISTERS = tuple(getattr(R, 'UC_X86_REG_' + name)
                  for name in ('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI'))


class Original:
    def __init__(self, image, sample):
        self.sample = sample
        self.cpu = Uc(UC_ARCH_X86, UC_MODE_32)
        self.pages = set()
        self.inputs = {}
        self.instructions = 0
        self.reads = 0
        self.writes = 0
        self.last = 0
        self.error = None
        self.caller = sample['oracle_caller_base']
        self.esp = self.caller + 0x8000
        self.output = self.caller + 0x8100
        self.mask = self.caller + 0x8200
        self.stop = self.caller + 0xf000
        self.text = [(s.va, s.va + len(image.section_bytes(s)))
                     for s in image.sections if s.name == '.text']
        for section in image.sections:
            data = image.section_bytes(section)
            if data:
                self.map(section.va, len(data))
                self.cpu.mem_write(section.va, data)
        for span in sample['world_inputs']:
            address, data = span['address'], bytes.fromhex(span['bytes'])
            if self.local(address, len(data)):
                raise ValueError('Original caller overlaps an actual source input')
            for n, value in enumerate(data):
                at = address + n
                if at in self.inputs and self.inputs[at] != value:
                    raise ValueError('Native query captured changing input bytes')
                self.inputs[at] = value
                if any(first <= at < last for first, last in self.text):
                    if image.read(at, 1) != bytes([value]):
                        raise ValueError('Source input differs from original instruction bytes')
            self.map(address, len(data))
            self.cpu.mem_write(address, data)
        self.map(self.caller, 0x10000)
        self.cpu.hook_add(UC_HOOK_CODE, self.code)
        self.cpu.hook_add(UC_HOOK_MEM_READ, self.read)
        self.cpu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def map(self, address, size):
        for page in range(address & ~0xfff, (address + size + 0xfff) & ~0xfff, 0x1000):
            if page not in self.pages:
                self.cpu.mem_map(page, 0x1000)
                self.pages.add(page)

    def local(self, address, size):
        return self.caller <= address and address + size <= self.caller + 0x10000

    def fail(self, message):
        self.error = message
        self.cpu.emu_stop()

    def code(self, _, pc, size, context):
        self.last = pc
        self.instructions += 1
        if not any(first <= pc and pc + size <= last for first, last in self.text):
            self.fail(f'Original query reached an undeclared code/service owner {pc:08x}')

    def read(self, _, access, address, size, value, context):
        self.reads += 1
        if not self.local(address, size) and any(address + n not in self.inputs for n in range(size)):
            self.fail(f'Original query read undeclared world bytes {address:08x}/{size} at {self.last:08x}')

    def write(self, _, access, address, size, value, context):
        self.writes += 1
        if not self.local(address, size):
            self.fail(f'Original query attempted a world store {address:08x}/{size} at {self.last:08x}')

    def call(self, entry, args):
        for register in REGISTERS:
            self.cpu.reg_write(register, 0)
        for n in range(8):
            self.cpu.reg_write(getattr(R, f'UC_X86_REG_XMM{n}'), 0)
        self.cpu.reg_write(R.UC_X86_REG_ECX, self.sample['object'])
        self.cpu.reg_write(R.UC_X86_REG_ESP, self.esp)
        self.cpu.reg_write(R.UC_X86_REG_EFLAGS, 2)
        self.cpu.reg_write(R.UC_X86_REG_FPCW, self.sample['x87_control_word'])
        self.cpu.reg_write(R.UC_X86_REG_FPSW, 0)
        self.cpu.reg_write(R.UC_X86_REG_FPTAG, 0xffff)
        self.cpu.reg_write(R.UC_X86_REG_MXCSR, self.sample['mxcsr'])
        self.cpu.mem_write(self.esp, struct.pack('<' + 'I' * (len(args) + 1), self.stop, *args))
        try:
            self.cpu.emu_start(entry, self.stop, count=20000)
        except UcError as error:
            raise RuntimeError(f'Original query failed at {self.last:08x}: {error}') from error
        if self.error:
            raise RuntimeError(self.error)
        if self.cpu.reg_read(R.UC_X86_REG_EIP) != self.stop:
            raise RuntimeError(f'Original query did not return completely at {self.last:08x}')
        if self.cpu.reg_read(R.UC_X86_REG_ESP) != self.esp + 4 * (len(args) + 1):
            raise RuntimeError('Original query returned with a different stack ABI')

    def run(self):
        self.call(self.sample['initialize'], (0, self.output))
        self.cpu.mem_write(self.mask, struct.pack('<I', 0xffffffff))
        self.call(self.sample['difference'], (self.mask, 0, self.output))
        expected = bytes.fromhex(self.sample['bytes'])
        actual = bytes(self.cpu.mem_read(self.output, len(expected)))
        mask = struct.unpack('<I', self.cpu.mem_read(self.mask, 4))[0]
        if actual != expected or mask != self.sample['changed']:
            raise RuntimeError(f'Native entity differs: mask {mask:08x}/{self.sample["changed"]:08x}, '
                               f'bytes {actual.hex()}/{expected.hex()}')
        relationships = 0
        if 'enemy_teams' in self.sample:
            for left in range(16):
                enemies = 0
                for right in range(16):
                    self.call(self.sample['relationship_entry'], (left, right))
                    if self.cpu.reg_read(R.UC_X86_REG_EAX) & 255:
                        enemies |= 1 << right
                    relationships += 1
                if enemies != self.sample['enemy_teams'][left]:
                    raise RuntimeError(f'Native enemy relationships differ for membership {left}')
        return dict(passed=True, instruction_count=self.instructions, original_reads=self.reads,
                    original_caller_stores=self.writes, changed=mask, bytes=len(actual),
                    relationship_comparisons=relationships, complete_original_returns=2 + relationships)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-report', type=Path, required=True)
    parser.add_argument('--report-output', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    assert scratch.is_dir()
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest() == NEW_PIN
    native = json.loads(args.native_report.read_text())
    report = dict(passed=False, original_sha256=NEW_PIN, native_sha256=hashlib.sha256(args.native_report.read_bytes()).hexdigest(),
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  scratch=str(scratch), instruction_patches=[], supplied_returns=[], cases=[],
                  scope='Complete unchanged original44/45 calls over actual captured canonical world inputs. '
                        'World writes and undeclared reads fail; independent caller addresses prove the output '
                        'does not retain the native query caller pointers. Transport/application remain separate.')
    failures = 0
    for case in native['cases']:
        for sample in case['samples']:
            row = dict(name=case['name'], stage=sample['stage'], type=sample['type'], passed=False)
            try:
                assert case['passed'], 'Native entity case did not pass'
                row.update(Original(image, sample).run())
            except Exception as error:
                row['error'] = str(error)
                failures += 1
            report['cases'].append(row)
    report['passed'] = failures == 0 and native['passed'] and bool(report['cases'])
    report['complete_original_returns'] = sum(r.get('complete_original_returns', 0) for r in report['cases'])
    report['relationship_comparisons'] = sum(r.get('relationship_comparisons', 0) for r in report['cases'])
    args.report_output.parent.mkdir(parents=True, exist_ok=True)
    args.report_output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=report['passed'], samples=len(report['cases']),
                         complete_original_returns=report['complete_original_returns'], failures=failures)))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
