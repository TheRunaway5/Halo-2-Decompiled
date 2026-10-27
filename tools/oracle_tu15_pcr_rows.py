#!/usr/bin/env python3
"""Check unchanged TU1.5 PCR row arithmetic and medal-buffer operations.

The hit-percentage instruction corridor receives explicit retained row words;
the complete original UTF16 append receives explicit strings and capacity256.
No widget, localization, identity or network service is replaced or executed.
Whole row-body correspondence is static and separately labelled below.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX,
                              UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_EDX,
                              UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_FPCW,
                              UC_X86_REG_MXCSR)
from oracle_tu15_team_balance import (Xbe, NEW_DEFAULT, NEW_PIN, ROOT, VERSIONS,
                                      STOP, _Code, _trace, full_tokens)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source, old = Xbe(NEW_DEFAULT), Xbe(ROOT / VERSIONS['retail']['path'])
    assert hashlib.sha256(source.data).hexdigest() == NEW_PIN
    assert hashlib.sha256(old.data).hexdigest() == VERSIONS['retail']['pin']
    mapping = []
    for image, filename in ((old, 'analysis/retail/functions.tsv'),
                            (source, 'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv')):
        starts = {int(r['address'], 16) for r in csv.DictReader((ROOT / filename).open(), delimiter='\t')}
        image.code, image.starts = _Code(image, '.text'), starts
    for retail, tu15 in ((0x232d77, 0x23975c), (0x23418a, 0x23ab6f),
                         (0x2342e9, 0x23acce), (0x234527, 0x23aeee),
                         (0x2346d9, 0x23b0be), (0x23482c, 0x23b211),
                         (0x234a97, 0x23b47c)):
        bodies = [[x.code.at(p) for p in sorted(_trace(x.code, at, x.starts | {at}).code)]
                  for x, at in ((old, retail), (source, tu15))]
        assert full_tokens(bodies[0], old) == full_tokens(bodies[1], source)
        mapping.append(dict(retail=hex(retail), tu15=hex(tu15), instructions=len(bodies[1]),
                            normalized_instructions_and_internal_branches_equal=True))
    # These literal bytes, rather than a readable decompilation annotation,
    # establish that the original medal delimiter is ONE SPACE, not comma-space.
    assert source.read(0x437544, 4) == b' \0\0\0'
    assert struct.unpack('<f', source.read(0x4500b8, 4))[0] == 100.0
    machine = Uc(UC_ARCH_X86, UC_MODE_32)
    machine.mem_map(0x10000, 0x800000)
    for section in source.sections:
        machine.mem_write(section.va, source.section_bytes(section))
    machine.mem_map(0x1100000, 0x10000)
    machine.mem_map(STOP, 0x1000)
    stack, destination, text = 0x110ff00, 0x1100000, 0x1101000
    executed = set()
    machine.hook_add(UC_HOOK_CODE, lambda _, address, size, user: executed.add(address))

    def put(address, value):
        machine.mem_write(address, struct.pack('<I', value & 0xffffffff))

    def append(value):
        machine.mem_write(text, value.encode('utf-16le') + b'\0\0')
        put(stack, STOP)
        for register, value in ((UC_X86_REG_ESP, stack), (UC_X86_REG_EAX, destination),
                                (UC_X86_REG_EDX, text), (UC_X86_REG_ECX, 256)):
            machine.reg_write(register, value)
        machine.emu_start(0x142550, STOP, count=10000)
        assert machine.reg_read(UC_X86_REG_EIP) == STOP

    cases, native_input, expected = [], [], []
    counts = [(0, 0), (1, 2), (2, 3), (424, 175), (388, 219), (1, 0x7fffffff),
              (0x7fffffff, 1), (0x7fffffff, 0x7fffffff), (987654321, 123456789),
              (1, -1), (1, -0x80000000), (-1, 1), (1, 1)]
    for mxcsr in (0x1f80, 0x3f80, 0x5f80, 0x7f80):
        for shots, hits in counts:
            put(0x56ba14, shots)
            put(0x56ba18, hits)
            for register, value in ((UC_X86_REG_EBP, stack - 0x1000), (UC_X86_REG_EBX, 0),
                                    (UC_X86_REG_EDI, 0), (UC_X86_REG_ESP, stack),
                                    (UC_X86_REG_FPCW, 0x27f), (UC_X86_REG_MXCSR, mxcsr)):
                machine.reg_write(register, value)
            machine.emu_start(0x23b58c, 0x23b5c7, count=1000)
            assert machine.reg_read(UC_X86_REG_EIP) == 0x23b5c7
            result = struct.unpack('<i', machine.mem_read(stack - 0x1000 + 8, 4))[0]
            cases.append(dict(shots=shots, hits=hits, mxcsr=mxcsr, percentage=result))
            native_input.append(f'0 {hits} {shots} {mxcsr}')
            expected.append(str(result))
    medals = []
    for length in (1, 31, 32, 63, 64, 255, 511):
        machine.mem_write(destination, bytes(1024))
        for index in range(8):
            if index:
                append(' ')
            append(chr(65 + index) * length)
        value = bytes(machine.mem_read(destination, 512)).decode('utf-16le').split('\0', 1)[0]
        assert len(value) <= 255
        medals.append(dict(name_length=length, text=value))
        native_input.append(f'1 {length} 0 0')
        expected.append(value)
    cpp = scratch / 'pcr-rows.cpp'
    cpp.write_text('''#include "halo2/vista_pcr_screen.hpp"
#include <iostream>
using namespace halo2;
void put(auto& b,unsigned at,unsigned value){for(unsigned i=0;i<4;++i)b[at+i]=std::byte(value>>(i*8));}
int main(){int kind,a,b;unsigned mx;while(std::cin>>kind>>a>>b>>mx){
VistaPcrResults bank;bank.player_count=1;put(bank.players[0],0x80,0xffffff);
put(bank.players[0],0x88,a);put(bank.players[0],0x84,b);
VistaPcrRowServices s;s.grid_child=s.text_child=[](unsigned){return true;};
s.appearance=[](unsigned){};s.visible=[](unsigned,bool){};
std::u16string result;unsigned index=0;
s.text=[&](unsigned i,std::u16string_view value){if(i==unsigned(kind?2:3))result=value;};
s.localized=[&](unsigned){return kind?std::u16string(a,char16_t(u'A'+index++)):u"%d";};
s.format=[](auto,std::span<const std::int32_t> values){auto text=std::to_string(values[0]);return std::u16string(text.begin(),text.end());};
vista_pcr_bind_row(kind?VistaPcrPane::medals:VistaPcrPane::weapons,0,bank,s,{0x27f,mx?mx:0x1f80},VistaPcrRowStyle::original_xbox);
for(auto c:result)std::cout<<char(c);std::cout<<'\\n';}}
''')
    binary = scratch / 'pcr-rows'
    subprocess.run(['c++', '-std=c++20', '-O1', '-fno-fast-math', '-ffp-contract=off',
                    '-I', str(ROOT / 'include'), str(cpp),
                    str(ROOT / 'src/ui/vista/vista_pcr_screen.cpp'), '-o', str(binary)], check=True)
    actual = subprocess.check_output([str(binary)], input=('\n'.join(native_input) + '\n').encode()).decode().splitlines()
    assert actual == expected, [(i, a, b) for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    for at in executed:
        instruction = next(decoder.disasm(source.read(at, 15), at))
        assert bytes(machine.mem_read(at, instruction.size)) == instruction.bytes
    report = dict(passed=True, source_sha256=NEW_PIN, scope=__doc__, static_body_mapping=mapping,
                  percentage_cases=cases, medal_cases=medals, original_instructions=len(executed),
                  delimiter_literal=dict(address='0x437544', bytes='20000000', utf16=' '),
                  no_instruction_patches=True, no_replaced_callees=True,
                  native_source_sha256=hashlib.sha256((ROOT / 'src/ui/vista/vista_pcr_screen.cpp').read_bytes()).hexdigest(),
                  oracle_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  scratch=str(scratch), scratch_retained=False)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS unchanged TU1.5 PCR: {len(cases)} percentage controls, '
          f'{len(medals)} eight-medal append controls, {len(executed)} original instructions; space delimiter/cap256')


if __name__ == '__main__':
    main()
