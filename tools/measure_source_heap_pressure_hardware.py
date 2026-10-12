#!/usr/bin/env python3
"""Execute unchanged original22ECB0/148740 on host x86 through a Linux ELF32 shim.

Input banks are the original main-heap experiment's retained snapshots. The shim
only loads those banks, sets MXCSR1F80, calls the unchanged original bodies and
writes scalar outputs/status. No native implementation output is used. This
calibrates emulator SSE status; host CPU execution is not Xbox hardware proof.
"""
import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT, Xbe, PIN

def sha(raw): return hashlib.sha256(raw).hexdigest()

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--capture', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    a = ap.parse_args(); a.output.mkdir(parents=True, exist_ok=False)
    fixture = json.loads(a.capture.read_text())
    path = ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(path.read_bytes()) == fixture['image_sha256'] == PIN
    image = Xbe(path)
    source = bytearray(0x800000)
    for s in image.sections:
        raw = image.section_bytes(s)
        source[s.va-0x10000:s.va-0x10000+len(raw)] = raw
    asm = a.output/'shim.S'
    asm.write_text(''' .code32
 .section .text
 .global _start
_start:
 mov $0x200f000,%esp
next:
 mov $0x1000000,%ecx
 mov $0x107000,%edx
 call read_exact
 test %eax,%eax
 jz finish
 mov $0x480d88,%ecx
 mov $0x34,%edx
 call read_exact
 test %eax,%eax
 jz fail
 ldmxcsr control
 mov 0x480da0,%eax
 mov $0x22ecb0,%edx
 call *%edx
 movd %xmm0,output
 stmxcsr output+4
 ldmxcsr control
 mov $0x148740,%edx
 call *%edx
 mov %eax,output+8
 stmxcsr output+12
 mov $4,%eax
 mov $1,%ebx
 mov $output,%ecx
 mov $16,%edx
 int $0x80
 cmp $16,%eax
 jne fail
 jmp next
read_exact:
 mov $3,%eax
 xor %ebx,%ebx
 int $0x80
 test %eax,%eax
 jle read_end
 add %eax,%ecx
 sub %eax,%edx
 jne read_exact
 mov $1,%eax
 ret
read_end:
 ret
finish:
 xor %ebx,%ebx
 mov $1,%eax
 int $0x80
fail:
 mov $1,%ebx
 mov $1,%eax
 int $0x80
 .balign 4
control: .long 0x1f80
output: .space 16
''')
    obj, binary = a.output/'shim.o', a.output/'shim.bin'
    subprocess.run(['as','--32',str(asm),'-o',str(obj)], check=True)
    subprocess.run(['ld','-m','elf_i386','-Ttext','0x3000000','--oformat=binary',
                    str(obj),'-o',str(binary)], check=True)
    # One page-aligned source bank preserves every original section byte and
    # loader-zero virtual tail; the shim never writes source code. RWX is an
    # experiment mapping convenience, not original section protection parity.
    banks = [(0x10000, bytes(source), len(source)),
             (0x1000000, b'', 0x107000), (0x2000000, b'', 0x10000),
             (0x3000000, binary.read_bytes(), len(binary.read_bytes()))]
    elf = bytearray(4096); headers = []
    for address, data, extent in banks:
        offset = (len(elf)+4095)&~4095
        elf.extend(bytes(offset-len(elf))); elf.extend(data)
        headers.append(struct.pack('<8I',1,offset,address,address,len(data),extent,7,4096))
    ident = b'\x7fELF\x01\x01\x01'+bytes(9)
    elf[:52] = struct.pack('<16sHHIIIIIHHHHHH',ident,2,3,1,0x3000000,52,
                           0,0,52,32,len(headers),0,0,0)
    elf[52:52+32*len(headers)] = b''.join(headers)
    program = a.output/'original-pressure.elf32'
    program.write_bytes(elf); program.chmod(0o700)
    rows, differences = [], []
    proc = subprocess.Popen([str(program.resolve())], stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE)
    try:
        for si, sequence in enumerate(fixture['sequences']):
            for ri, row in enumerate(sequence['steps']):
                if row['kind'] == 'raw': continue
                raw = (a.capture.parent/row['raw_storage']).read_bytes()
                assert len(raw) == 0x107000
                proc.stdin.write(raw+bytes.fromhex(row['globals'])); proc.stdin.flush()
                result = proc.stdout.read(16)
                assert len(result) == 16, (si, ri, proc.poll(), result.hex())
                stat, stat_status, pressure, pressure_status = struct.unpack('<4I',result)
                assert stat == row['statistics_bits'] and pressure == row['pressure'], (si, ri)
                item = dict(sequence=si, step=ri, statistics_bits=stat,
                    statistics_mxcsr=stat_status, pressure=pressure,
                    pressure_mxcsr=pressure_status)
                rows.append(item)
                if (stat_status,pressure_status) != (row['statistics_mxcsr'],row['pressure_mxcsr']):
                    differences.append(dict(sequence=si,step=ri,
                        emulator=[row['statistics_mxcsr'],row['pressure_mxcsr']],
                        hardware=[stat_status,pressure_status]))
        proc.stdin.close()
        assert proc.wait(timeout=10) == 0
    finally:
        if proc.poll() is None: proc.kill(); proc.wait()
    output = a.output/'fixture.json'
    output.write_text(json.dumps(dict(rows=rows),indent=2)+'\n')
    report = dict(boundary=__doc__, image_sha256=PIN,
        input_fixture_sha256=sha(a.capture.read_bytes()),
        oracle_sha256=sha(Path(__file__).read_bytes()),
        program_sha256=sha(program.read_bytes()),shim_sha256=sha(binary.read_bytes()),
        fixture_sha256=sha(output.read_bytes()),cases=len(rows),
        scalar_outputs_match_emulator=True,
        sse_status_differences=differences, original_instruction_patches=[],
        native_expected_data_used=False, xbox_hardware_proof=False)
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(cases=len(rows),status_differences=len(differences),
                         fixture_sha256=report['fixture_sha256'])))

if __name__ == '__main__': main()
