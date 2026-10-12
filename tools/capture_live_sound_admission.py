"""GDB Python helper: replay a real stopped Xbox sound-admission call.

Use at unchanged base126C30 with H2_SOUND_CAPTURE_OUTPUT set to a new directory.
The guest has already initialized itself and produced the actual request. Every
Unicorn page comes from GDB's stopped guest, on demand. No readiness, listener,
seed, output, source handle, callee return or instruction is manufactured.
After the game returns, call compare_actual_return() in the same GDB session.
The tool records CPU admission only, not driver playback or audible parity.
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

output = Path(os.environ['H2_SOUND_CAPTURE_OUTPUT'])
output.mkdir(parents=True, exist_ok=False)
image_path = ROOT / 'assets/disc/default.xbe'
assert hashlib.sha256(image_path.read_bytes()).hexdigest() == '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
image = Xbe(image_path)
inferior = gdb.selected_inferior()
register_names = ['eax','ebx','ecx','edx','esi','edi','ebp','esp','eip','eflags','fctrl','fstat','ftag','mxcsr']
registers = {n:int(gdb.parse_and_eval('$'+n)) & 0xffffffff for n in register_names}
assert registers['eip'] == 0x126c30
stack = bytes(inferior.read_memory(registers['esp'],16))
return_pc, sound, local_output, reason_output = struct.unpack('<4I', stack)
request = registers['eax']
machine = Uc(UC_ARCH_X86, UC_MODE_32)
pages, instructions, writes = {}, {}, []
numeric_returns, pending_returns = [], []


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
    if pending_returns and address == pending_returns[-1]['return_pc']:
        row = pending_returns.pop()
        register = R.UC_X86_REG_XMM0 if row.pop('result_register') == 'xmm0' else R.UC_X86_REG_EAX
        row['result'] = cpu.reg_read(register) & 0xffffffff
        numeric_returns.append(row)
    # Observe numeric results within the whole original game call. No extra
    # function is invoked, and the game's caller inputs remain untouched.
    queries = {0x124fc0:(0x126710,'xmm0'), 0x125010:(0x126760,'xmm0'),
               0x127e20:(0x129570,'xmm0'), 0x12ac20:(0x12c370,'xmm0'),
               0x1251e0:(0x126930,'eax'), 0x127010:(0x128760,'eax')}
    if address in queries:
        canonical, result_register = queries[address]
        sp = cpu.reg_read(R.UC_X86_REG_ESP)
        ret = int.from_bytes(cpu.mem_read(sp,4),'little')
        argument = 0
        if address == 0x127e20:
            assert cpu.reg_read(R.UC_X86_REG_ECX) == request+12
            argument = cpu.reg_read(R.UC_X86_REG_EDX)
        elif address == 0x12ac20:
            assert cpu.reg_read(R.UC_X86_REG_EDI) == request+12
            argument = int.from_bytes(cpu.mem_read(sp+4,4),'little')
        elif address in (0x124fc0,0x125010):
            assert cpu.reg_read(R.UC_X86_REG_ECX) == request+12
            assert cpu.reg_read(R.UC_X86_REG_EAX) == sound
        elif address == 0x127010:
            argument = cpu.reg_read(R.UC_X86_REG_ECX)
        elif address == 0x1251e0:
            assert bytes(cpu.mem_read(sp+4,8)) == bytes(cpu.mem_read(request+20,4))+bytes(cpu.mem_read(request+16,4))
        pending_returns.append(dict(entry=canonical,original_entry=address,argument=argument,
                                    return_pc=ret,result_register=result_register))


def store(cpu, kind, address, size, value, user):
    # Unicorn can notify a write before its unmapped-page fault callback.
    fetch(cpu,kind,address,size,value,user)
    before = bytes(cpu.mem_read(address,size))
    after = (value & ((1 << (size*8))-1)).to_bytes(size,'little')
    writes.append(dict(instruction=cpu.reg_read(R.UC_X86_REG_EIP),address=address,
                       bytes=size,before=before.hex(),after=after.hex()))


machine.hook_add(UC_HOOK_MEM_INVALID,fetch)
machine.hook_add(UC_HOOK_CODE,code)
machine.hook_add(UC_HOOK_MEM_WRITE,store)
# These cells are operands in the completely paired original admission owners.
# Capture their actual game values, including currently untaken branch inputs.
root_cells = dict(system=0x4e6380,tag_table=0x4e3b44,current_matg=0x4e034c,
                  current_ugh=0x51ebd4,class_fades=0x502118,local_rng=0x4e7408,
                  first_person_globals=0x4e8c20,current_scene=0x4e0348)
state_roots = {}
for name, address in root_cells.items():
    fetch(machine,0,address,4,0,None)
    value = int.from_bytes(machine.mem_read(address,4),'little')
    state_roots[name] = value
    if value:
        fetch(machine,0,value,4,0,None)
fetch(machine,0,0x47f0e4,4,0,None)
state_roots['alternate_codec_flag'] = 0x47f0e4
for name in register_names[:10]:
    machine.reg_write(getattr(R,'UC_X86_REG_'+name.upper()),registers[name])
machine.reg_write(R.UC_X86_REG_FPCW,registers['fctrl'])
machine.reg_write(R.UC_X86_REG_FPSW,registers['fstat'])
machine.reg_write(R.UC_X86_REG_FPTAG,registers['ftag'])
machine.reg_write(R.UC_X86_REG_MXCSR,registers['mxcsr'])
floating = {}
for i in range(8):
    raw = gdb.parse_and_eval(f'$st{i}').bytes
    assert len(raw) == 10, len(raw)
    mantissa, exponent = struct.unpack('<QH',raw)
    machine.reg_write(getattr(R,f'UC_X86_REG_FP{(i+((registers["fstat"]>>11)&7))&7}'),(mantissa,exponent))
    floating[f'st{i}'] = raw.hex()
    raw = gdb.parse_and_eval(f'$xmm{i}').bytes
    assert len(raw) == 16
    machine.reg_write(getattr(R,f'UC_X86_REG_XMM{i}'),int.from_bytes(raw,'little'))
    floating[f'xmm{i}'] = raw.hex()
machine.emu_start(registers['eip'],return_pc,count=200000)
assert machine.reg_read(R.UC_X86_REG_EIP) == return_pc
assert not pending_returns
for address,raw in sorted(pages.items()):
    (output / f'page-{address:08x}.bin').write_bytes(raw)
(output / 'instructions.tsv').write_text(''.join(f'{address:08x} {raw.hex()}\n' for address,raw in sorted(instructions.items())))
# Private callee stack is scratch. Caller outputs, request bytes and RNG remain
# compared with the actual game; no output is copied from replay into the guest.
final = {}
for write in writes:
    if registers['esp']-0x10000 <= write['address'] < registers['esp']:
        continue
    final[write['address'],write['bytes']] = write['after']
report = dict(boundary=__doc__, image_sha256=hashlib.sha256(image_path.read_bytes()).hexdigest(),
              registers=registers,floating=floating,stack_hex=stack.hex(),request=request,sound=sound,
              state_roots=state_roots,root_cells=root_cells,
              numeric_returns=numeric_returns,
              local_output=local_output,reason_output=reason_output,return_pc=return_pc,
              replay_al=machine.reg_read(R.UC_X86_REG_EAX)&255,
              replay_esp=machine.reg_read(R.UC_X86_REG_ESP),
              pages=[dict(address=a,bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest()) for a,raw in sorted(pages.items())],
              writes=writes,compared_writes=[dict(address=a,bytes=n,after=raw)for (a,n),raw in sorted(final.items())],
              instruction_patches=[],intercepted_callees=[],guest_memory_writes=[],
              tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
(output / 'replay.json').write_text(json.dumps(report,indent=2)+'\n')
gdb.set_convenience_variable('audio_return_pc',return_pc)
print(f'Original replay: sound={sound:08x}, AL={report["replay_al"]}, {len(pages)} actual guest pages, {len(instructions)} unchanged instructions')


def compare_actual_return():
    actual_al = int(gdb.parse_and_eval('$eax')) & 255
    actual_esp = int(gdb.parse_and_eval('$esp')) & 0xffffffff
    comparisons = []
    for row in report['compared_writes']:
        actual = bytes(inferior.read_memory(row['address'],row['bytes'])).hex()
        comparisons.append(dict(**row,actual=actual,equal=actual==row['after']))
    result = dict(actual_al=actual_al,actual_esp=actual_esp,
                  al_equal=actual_al==report['replay_al'],esp_equal=actual_esp==report['replay_esp'],
                  writes=comparisons,actual_eip=int(gdb.parse_and_eval('$eip'))&0xffffffff)
    (output / 'actual-return.json').write_text(json.dumps(result,indent=2)+'\n')
    assert result['al_equal'] and result['esp_equal'] and all(r['equal'] for r in comparisons),result
    print(f'Actual Xbox call agrees: AL={actual_al}, {len(comparisons)} retained output/RNG writes')
