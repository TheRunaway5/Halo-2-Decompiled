"""Execute unchanged OG Xbox TU1.5 scalar/RGB fog extern consumers.

Synthetic complete environment banks are explicit consumer input controls.
Two additional complete banks are genuine xemu 0.8.136/Complex4627 TU1.5
captures. No native implementation supplies expected values. This is CPU
consumer evidence, not GPU instruction, framebuffer, or hardware parity.
"""
from pathlib import Path
import argparse, hashlib, itertools, json, struct, sys, traceback
sys.dont_write_bytecode = True
ROOT = next(p for p in Path(__file__).resolve().parents if (p/'tools/reference/halo2-decompiled/tools/xbe.py').is_file())
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
XBE_PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
CAPTURE_PINS = {
    'fleet': 'e80fc2d489e0fcc5794376b58457d060712035af03f16c4af4bbc7d4c258c20d',
    'destruction': 'fbef1d9f6671be6ab102051b4797f3816ef42893b5e253acb4d7c11dcd2c7091',
}
SCALARS = [3, *range(24, 37)]
COLORS = list(range(25, 33))

def sha(data): return hashlib.sha256(data).hexdigest()

def generate(args):
    x = Xbe(args.xbe)
    assert sha(x.data) == XBE_PIN, 'Wrong executable authority'
    g = Uc(UC_ARCH_X86, UC_MODE_32)
    g.mem_map(0x10000, 0x900000)
    for s in x.sections: g.mem_write(s.va, x.section_bytes(s))
    stack, output, stop = 0x910800, 0x911100, 0x912000
    g.mem_map(0x910000, 0x3000)
    trace, writes = [], []
    def code(cpu, address, size, _):
        assert bytes(cpu.mem_read(address, size)) == x.read(address, size), hex(address)
        trace.append(address)
    def write(cpu, _, address, size, value, __):
        assert output <= address and address + size <= output + 12, hex(address)
        writes.append([address-output, size])
    g.hook_add(UC_HOOK_CODE, code)
    g.hook_add(UC_HOOK_MEM_WRITE, write)
    def run(entry, index):
        g.mem_write(output-4, b'\xa5'*20)
        g.mem_write(stack, struct.pack('<I', stop))
        for reg in [R.UC_X86_REG_EAX, R.UC_X86_REG_ECX, R.UC_X86_REG_EDX,
                    R.UC_X86_REG_EBX, R.UC_X86_REG_EBP, R.UC_X86_REG_EDI]:
            g.reg_write(reg, 0)
        g.reg_write(R.UC_X86_REG_EAX, index)
        g.reg_write(R.UC_X86_REG_ESI, output)
        g.reg_write(R.UC_X86_REG_ESP, stack)
        g.reg_write(R.UC_X86_REG_EFLAGS, 2)
        g.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
        g.reg_write(R.UC_X86_REG_XMM0, 0)
        trace.clear(); writes.clear()
        g.emu_start(entry, stop, count=10000)
        assert g.reg_read(R.UC_X86_REG_EIP) == stop
        assert bytes(g.mem_read(output-4, 4)) == bytes(g.mem_read(output+12, 4)) == b'\xa5'*4
        if entry == 0x336e0:
            assert not writes
            bits = [f'{g.reg_read(R.UC_X86_REG_XMM0)&0xffffffff:08x}']
        else:
            assert writes == [[0,4],[4,4],[8,4]], writes
            bits = [f'{v:08x}' for v in struct.unpack('<3I', g.mem_read(output,12))]
        return {'bits': bits, 'instructions': len(trace), 'writes': list(writes)}
    cases = []
    def add(name, bank, provenance):
        assert len(bank) == 0x120
        g.mem_write(0x491f6c, bytes(bank))
        cases.append({'name':name, 'provenance':provenance, 'bytes_hex':bytes(bank).hex(),
            'scalars':{str(i):run(0x336e0,i) for i in SCALARS},
            'colors':{str(i):run(0x33970,i) for i in COLORS}})
    for name, path in [('fleet',args.fleet_bank), ('destruction',args.destruction_bank)]:
        bank = path.read_bytes()
        assert sha(bank) == CAPTURE_PINS[name], 'Wrong genuine capture'
        add(name, bank, 'genuine_completed_original_environment')
    for n, values in enumerate(itertools.product([0.,.25,1.],[0.,.375,1.],[0.,.625,1.],[0.,.5,1.])):
        bank = bytearray(0x120)
        struct.pack_into('<I',bank,0x6c,0xffffffff);struct.pack_into('<I',bank,0x9c,0xffffffff)
        for off, rgb in [(0x1c,(.25,.5,.75)),(0x34,(.875,.375,.125)),
                         (0x4c,(.625,.125,.375)),(0xa0,(.5,.75,.25))]:
            struct.pack_into('<3f',bank,off,*rgb)
        for off,value in [(0x28,values[0]),(0x40,.75),(0x58,.5),(0xac,.125),
                          (0x11c,values[1]),(0x118,values[2]),(0x10c,values[3])]:
            struct.pack_into('<f',bank,off,value)
        add(f'finite-grid-{n:02}',bank,'explicit_consumer_input')
    for name, primary, interp, transition, eye in [
            ('signed-zero',-0.,-0.,-0.,-0.), ('unclamped-high',1.5,1.25,1.5,1.75),
            ('unclamped-negative',-.5,-.25,-.5,-.75),
            ('tiny-normal',2**-60,2**-50,2**-40,2**-30)]:
        bank = bytearray.fromhex(cases[2]['bytes_hex'])
        for off,value in [(0x28,primary),(0x11c,interp),(0x118,transition),(0x10c,eye)]:
            struct.pack_into('<f',bank,off,value)
        add(name,bank,'explicit_consumer_input')
    text = next(s for s in x.sections if s.name == '.text')
    assert bytes(g.mem_read(text.va,len(x.section_bytes(text)))) == x.section_bytes(text)
    return {'status':'PASS_UNCHANGED_ORIGINAL_FOG_EXTERNS', 'scope':__doc__,
        'xbe_sha256':XBE_PIN, 'capture_pins':CAPTURE_PINS, 'scalar_indices':SCALARS,
        'color_indices':COLORS, 'mxcsr':0x1f80, 'code_patches':[], 'service_returns':[],
        'original_segments':[{'start':a,'end_exclusive':z,'sha256':sha(x.read(a,z-a))}
            for a,z in [(0x336e0,0x33970),(0x33970,0x3403a)]], 'cases':cases}

if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for flag in ['xbe','fleet-bank','destruction-bank','output']:
        p.add_argument('--'+flag,type=Path,required=True)
    args=p.parse_args()
    if args.output.exists(): raise ValueError('Preserve existing original evidence')
    args.output.parent.mkdir(parents=True,exist_ok=True)
    try: result=generate(args)
    except Exception as e:
        args.output.write_text(json.dumps({'status':'FAIL_RETAINED','error':str(e),
            'traceback':traceback.format_exc()},indent=2)+'\n')
        raise
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(result['status'],len(result['cases']),'complete banks')
