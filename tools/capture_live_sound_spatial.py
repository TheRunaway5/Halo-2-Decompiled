"""Replay an actual Xbox129790 CPU call up to its driver boundary.

Run from GDB stopped at unchanged base129790. H2_SOUND_SPATIAL_OUTPUT must
name a new directory. The request, retained state, registers and FPU state
come from the initialized game. The replay ends at the original220E00 call
boundary, or at129790's return when the original gates omit that call. No
callee result is supplied and no guest memory, register or instruction is
changed. compare_actual_boundary() checks the same boundary in the game.
This observes the game CPU owner; it does not emulate the SDK or the DSP.
"""
import hashlib
import json
import os
from pathlib import Path
import struct
import sys
import gdb

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / '.tools/python/lib' /
                       f'python{sys.version_info.major}.{sys.version_info.minor}/site-packages'))
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_MEM_INVALID, UC_HOOK_MEM_WRITE, UC_HOOK_CODE
from unicorn import x86_const as R

output = Path(os.environ['H2_SOUND_SPATIAL_OUTPUT'])
output.mkdir(parents=True, exist_ok=False)
image_path = ROOT / 'assets/disc/default.xbe'
assert hashlib.sha256(image_path.read_bytes()).hexdigest() == '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
image = Xbe(image_path)
inferior = gdb.selected_inferior()
register_names = ['eax','ebx','ecx','edx','esi','edi','ebp','esp','eip','eflags','fctrl','fstat','ftag','mxcsr']
registers = {n:int(gdb.parse_and_eval('$'+n)) & 0xffffffff for n in register_names}
assert registers['eip'] == 0x129790
stack = bytes(inferior.read_memory(registers['esp'],8))
return_pc, attenuation = struct.unpack('<2I',stack)
voice_index = struct.unpack('<h',struct.pack('<H',registers['eax'] & 65535))[0]
machine = Uc(UC_ARCH_X86, UC_MODE_32)
pages, instructions, writes = {}, {}, []


def fetch(cpu, kind, address, size, value, user):
    for page in range(address & ~4095, (address+size+4095) & ~4095, 4096):
        if page in pages:
            continue
        raw = bytes(inferior.read_memory(page,4096))
        assert len(raw) == 4096
        cpu.mem_map(page,4096)
        cpu.mem_write(page,raw)
        pages[page] = raw
    return True


def code(cpu, address, size, user):
    raw = bytes(cpu.mem_read(address,size))
    assert raw == image.read(address,size), ('Original instruction differs',hex(address),raw.hex())
    instructions[address] = raw
    if address == 0x220e00:
        cpu.emu_stop()


def store(cpu, kind, address, size, value, user):
    fetch(cpu,kind,address,size,value,user)
    writes.append(dict(instruction=cpu.reg_read(R.UC_X86_REG_EIP),address=address,
                       bytes=size,before=bytes(cpu.mem_read(address,size)).hex(),
                       after=(value & ((1 << (size*8))-1)).to_bytes(size,'little').hex()))


machine.hook_add(UC_HOOK_MEM_INVALID,fetch)
machine.hook_add(UC_HOOK_CODE,code)
machine.hook_add(UC_HOOK_MEM_WRITE,store)
root_cells = dict(voices=0x4e6378,sources=0x4e637c,system=0x4e6380,
                  driver=0x51ebe4,shared_voices=0x502114,tag_table=0x4e3b44,
                  current_matg=0x4e034c,current_ugh=0x51ebd4)
state_roots = {}
for name,address in root_cells.items():
    fetch(machine,0,address,4,0,None)
    value = int.from_bytes(machine.mem_read(address,4),'little')
    state_roots[name] = value
    if value:
        fetch(machine,0,value,4,0,None)
fetch(machine,0,0x4686c4,2,0,None)
state_roots['current_local_index_address'] = 0x4686c4
for name in register_names[:10]:
    machine.reg_write(getattr(R,'UC_X86_REG_'+name.upper()),registers[name])
machine.reg_write(R.UC_X86_REG_FPCW,registers['fctrl'])
machine.reg_write(R.UC_X86_REG_FPSW,registers['fstat'])
machine.reg_write(R.UC_X86_REG_FPTAG,registers['ftag'])
machine.reg_write(R.UC_X86_REG_MXCSR,registers['mxcsr'])
floating = {}
for i in range(8):
    raw = gdb.parse_and_eval(f'$st{i}').bytes
    assert len(raw) == 10
    mantissa,exponent = struct.unpack('<QH',raw)
    machine.reg_write(getattr(R,f'UC_X86_REG_FP{(i+((registers["fstat"]>>11)&7))&7}'),(mantissa,exponent))
    floating[f'st{i}'] = raw.hex()
    raw = gdb.parse_and_eval(f'$xmm{i}').bytes
    assert len(raw) == 16
    machine.reg_write(getattr(R,f'UC_X86_REG_XMM{i}'),int.from_bytes(raw,'little'))
    floating[f'xmm{i}'] = raw.hex()
machine.emu_start(registers['eip'],return_pc,count=200000)
boundary = machine.reg_read(R.UC_X86_REG_EIP)
assert boundary in (return_pc,0x220e00),hex(boundary)
driver_call = None
if boundary == 0x220e00:
    sp = machine.reg_read(R.UC_X86_REG_ESP)
    parameters = machine.reg_read(R.UC_X86_REG_EDI)
    driver_call = dict(voice_index=struct.unpack('<i',machine.mem_read(sp+4,4))[0],
                       parameters_address=parameters,
                       parameters_hex=bytes(machine.mem_read(parameters,36)).hex())
final = {}
for write in writes:
    if registers['esp']-0x10000 <= write['address'] < registers['esp']:
        continue
    final[write['address'],write['bytes']] = write['after']
for address,raw in sorted(pages.items()):
    (output/f'page-{address:08x}.bin').write_bytes(raw)
(output/'instructions.tsv').write_text(''.join(f'{address:08x} {raw.hex()}\n' for address,raw in sorted(instructions.items())))
report = dict(boundary=__doc__,image_sha256=hashlib.sha256(image_path.read_bytes()).hexdigest(),
              registers=registers,floating=floating,stack_hex=stack.hex(),
              voice_index=voice_index,attenuation_address=attenuation,
              state_roots=state_roots,root_cells=root_cells,return_pc=return_pc,
              boundary_pc=boundary,replay_esp=machine.reg_read(R.UC_X86_REG_ESP),
              driver_call=driver_call,
              pages=[dict(address=a,bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest())for a,raw in sorted(pages.items())],
              writes=writes,compared_writes=[dict(address=a,bytes=n,after=raw)for(a,n),raw in sorted(final.items())],
              instruction_patches=[],intercepted_callees=[],guest_memory_writes=[],
              tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
(output/'replay.json').write_text(json.dumps(report,indent=2)+'\n')
gdb.set_convenience_variable('audio_spatial_boundary',boundary)
gdb.set_convenience_variable('audio_spatial_return',return_pc)
print(f'Original spatial replay: voice={voice_index}, boundary={boundary:08x}, {len(pages)} actual guest pages, {len(instructions)} unchanged instructions')


def compare_actual_boundary():
    actual_pc = int(gdb.parse_and_eval('$eip')) & 0xffffffff
    actual_sp = int(gdb.parse_and_eval('$esp')) & 0xffffffff
    comparisons = []
    for row in report['compared_writes']:
        actual = bytes(inferior.read_memory(row['address'],row['bytes'])).hex()
        comparisons.append(dict(**row,actual=actual,equal=actual==row['after']))
    actual_driver = None
    if driver_call:
        address = int(gdb.parse_and_eval('$edi')) & 0xffffffff
        actual_driver = dict(voice_index=struct.unpack('<i',bytes(inferior.read_memory(actual_sp+4,4)))[0],
                             parameters_address=address,
                             parameters_hex=bytes(inferior.read_memory(address,36)).hex())
    result = dict(actual_eip=actual_pc,actual_esp=actual_sp,
                  pc_equal=actual_pc==boundary,esp_equal=actual_sp==report['replay_esp'],
                  driver_call=actual_driver,driver_equal=actual_driver==driver_call,
                  writes=comparisons)
    (output/'actual-boundary.json').write_text(json.dumps(result,indent=2)+'\n')
    assert result['pc_equal'] and result['esp_equal'] and result['driver_equal'] and all(r['equal']for r in comparisons),result
    print(f'Actual Xbox spatial boundary agrees: {len(comparisons)} retained writes, driver request={actual_driver}')
