"""Compare complete scenario light creation with unchanged TU1.5 execution.

Input is a cold native world's actual retained memory and caller ABI. The
original C29F0/C2550 and their callees execute without supplied light results,
replaced services, instruction patches or altered authored placement bytes.
This proves this CPU constructor; rendered appearance is a separate check.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path
from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_INVALID
from unicorn import x86_const as R

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from differential_physics_native_frame_tu15_20261012 import regions,segment

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--case',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
abi=json.loads((args.case/'before-abi.json').read_text())
expected_abi=json.loads((args.case/'after-abi.json').read_text())
assert abi['x87_depth']==0
guest=Uc(UC_ARCH_X86,UC_MODE_32);mapped=set()
def mount(address,data):
    pending=[p for p in range(address&~4095,(address+len(data)+4095)&~4095,4096)if p not in mapped]
    begin=0
    while begin<len(pending):
        end=begin+1
        while end<len(pending)and pending[end]==pending[end-1]+4096:end+=1
        guest.mem_map(pending[begin],(end-begin)*4096);begin=end
    mapped.update(pending);guest.mem_write(address,bytes(data))
for address,data in regions(args.case/'before.bin'):mount(address,data)
gdt=0x7fff0000;assert gdt not in mapped;mount(gdt,bytes(4096))
guest.mem_write(gdt+8,segment(0,0x9b)+segment(0,0x93)+segment(abi['fs_base'],0x93))
guest.reg_write(R.UC_X86_REG_GDTR,(0,gdt,0xfff,0))
for reg,value in ((R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)):
    guest.reg_write(reg,value)
registers=[getattr(R,'UC_X86_REG_'+s)for s in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
for reg,value in zip(registers,abi['gp']):guest.reg_write(reg,value)
for index,lanes in enumerate(abi['xmm']):guest.reg_write(R.UC_X86_REG_XMM0+index,sum(value<<(32*lane)for lane,value in enumerate(lanes)))
for reg,value in ((R.UC_X86_REG_EFLAGS,abi['eflags']),(R.UC_X86_REG_FPCW,abi['x87_control']),
                  (R.UC_X86_REG_FPSW,abi['x87_status']),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,abi['mxcsr'])):
    guest.reg_write(reg,value)
image=Xbe(NEW_DEFAULT);trace=[];faults=[];writes=[];pending=[]
def stack(address):return 0x60000000<=address<0x60200000
def flush():
    for pc,address,size in pending:writes.append((pc,address,bytes(guest.mem_read(address,size)).hex()))
    pending.clear()
def code(cpu,pc,size,_):
    flush()
    assert bytes(cpu.mem_read(pc,size))==image.read(pc,size),hex(pc)
    trace.append(pc)
def write(cpu,access,address,size,value,_):
    flush()
    if not stack(address):pending.append((cpu.reg_read(R.UC_X86_REG_EIP),address,size))
def fault(cpu,access,address,size,value,_):
    faults.append(dict(pc=cpu.reg_read(R.UC_X86_REG_EIP),access=access,address=address,size=size));return False
guest.hook_add(UC_HOOK_CODE,code);guest.hook_add(UC_HOOK_MEM_WRITE,write);guest.hook_add(UC_HOOK_MEM_INVALID,fault)
stop=int.from_bytes(guest.mem_read(abi['gp'][4],4),'little');error=None
try:
    guest.emu_start(0xc29f0,stop,count=1000000);flush()
    assert guest.reg_read(R.UC_X86_REG_EIP)==stop,'Original execution did not return'
except (UcError,AssertionError,ValueError)as failure:error=str(failure)
expected=[]
for line in (args.case/'stores.tsv').read_text().splitlines():
    pc,address,data=line.split('\t');address=int(address)
    if not stack(address):expected.append((int(pc),address,data))
first_store_difference=next((dict(index=i,native=n,original=o)for i,(n,o)in enumerate(zip(expected,writes))if n!=o),None)
differences=[];changed_bytes=0
if error is None:
    for address,data in regions(args.case/'after.bin'):
        if stack(address):continue
        actual=bytes(guest.mem_read(address,len(data)))
        if actual!=data:
            count=sum(x!=y for x,y in zip(actual,data));changed_bytes+=count
            differences.append(dict(address=address,bytes=len(data),different_bytes=count))
exit_gp=[guest.reg_read(reg)for reg in registers]
report=dict(scope=__doc__,source_sha256=NEW_PIN,instructions=len(trace),unique_instructions=len(set(trace)),
    native_stores=len(expected),original_stores=len(writes),first_store_difference=first_store_difference,
    final_different_bytes=changed_bytes,final_different_regions=differences,gp_exact=exit_gp==expected_abi['gp'],
    native_gp=expected_abi['gp'],original_gp=exit_gp,eflags_exact=guest.reg_read(R.UC_X86_REG_EFLAGS)==expected_abi['eflags'],
    faults=faults,error=error,instruction_patches=[],replaced_services=[])
report['status']=int(bool(error or faults or first_store_difference or len(expected)!=len(writes)or changed_bytes or not report['gp_exact']))
args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items()if k not in ('scope','native_gp','original_gp')}))
raise SystemExit(report['status'])
