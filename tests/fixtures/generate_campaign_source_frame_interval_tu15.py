from pathlib import Path
import sys,hashlib,json,struct
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
import argparse
parser=argparse.ArgumentParser(description="Reproduce the OG Xbox TU1.5 binary32 incoming camera interval oracle")
parser.add_argument("--xbe",type=Path,required=True)
parser.add_argument("--source-root",type=Path,default=Path(__file__).resolve().parents[2])
parser.add_argument("--output",type=Path,default=Path(__file__).with_name("campaign_source_frame_interval_tu15.inc"))
a=parser.parse_args();ROOT=a.source_root.resolve();D=a.output.resolve().parent;D.mkdir(parents=True,exist_ok=True)
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
X=a.xbe;x=Xbe(X);assert hashlib.sha256(x.data).hexdigest()=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000);u.mem_map(0x2000000,4096)
for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
pcs=[]
def code(g,pc,n,_):assert bytes(g.mem_read(pc,n))==x.read(pc,n);pcs.append(pc)
u.hook_add(UC_HOOK_CODE,code);stack=0x2000200;cases=[]
inputs=[0xbf800000,0x80000001,0x80000000,0,1,0x007fffff,0x00800000,0x3c088889,0x3d088889,0x3e000000,0x411fffff,0x41200000,0x41200001,0x42c80000,0x7f7fffff]
for bits in inputs:
 u.mem_write(stack,bytes(0x40));u.mem_write(stack+0x10,struct.pack('<I',bits));u.mem_write(stack+0x20,struct.pack('<I',bits));u.reg_write(R.UC_X86_REG_ESP,stack);u.reg_write(R.UC_X86_REG_FPCW,0x23f);u.reg_write(R.UC_X86_REG_FPTAG,0xffff);u.reg_write(R.UC_X86_REG_FPSW,0);u.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
 # A real unchanged FLD instruction provides the explicitly declared scalar
 # boundary operand. This does not pretend subnormal/-0 arose from counters.
 u.emu_start(0x14929d,0x1492a1,count=1);assert u.reg_read(R.UC_X86_REG_EIP)==0x1492a1
 u.emu_start(0x12dd47,0x12dd6e,count=100);assert u.reg_read(R.UC_X86_REG_EIP)==0x12dd6e
 out=u.reg_read(R.UC_X86_REG_XMM0)&0xffffffff;cases.append({'binary32_input':bits,'original_output':out})
res={'status':'UNCHANGED_ORIGINAL_SCALAR_INTERVAL_CLAMP_CASES_COMPLETE','scope':'Declared binary32 incoming scalar boundary after12DBF0counterdivision/store; not wholecounterproduction or physicalpauseUI proof. Subnormal andsignedzero cases qualify onlyscalar arithmetic boundary. NaN/Inf areoutside originalcounterproducer domain andreceive no inventedoriginal result.','xbe_sha256':hashlib.sha256(x.data).hexdigest(),'exact_upper_bound_bits':x.read(0x46a110,4).hex(),'scalar_instructions':x.read(0x12dd47,0x27).hex(),'preload_instruction':x.read(0x14929d,4).hex(),'executed_source_pcs':sorted(set(pcs)),'cases':cases,'instruction_patches':[],'game_services_supplied':[],'mxcsr':'1FA0','x87_control_word':'023F'}
a.output.with_suffix('.receipt.json').write_text(json.dumps(res,indent=2)+'\n');inc='// OriginalTU1.5 binary32 scalar-boundary inputs and outputs; see source-campaign-camera-frame incoming-scalar-bound oracle.\n';inc+='static constexpr std::array<std::array<std::uint32_t,2>,'+str(len(cases))+'> interval_cases{{\n';inc+=''.join(' {{0x%08xu,0x%08xu}},\n'%(c['binary32_input'],c['original_output'])for c in cases);inc+='}};\n';a.output.write_text(inc);print(res['status'],len(cases))
