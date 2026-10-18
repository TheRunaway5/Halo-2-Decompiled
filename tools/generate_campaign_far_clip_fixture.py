"""Unchanged TU1.5 far-clip evaluator/parser/completion and consumer region.

Only declared HS caller storage/stack and real authored cache nodes are inputs.
No callee result, source instruction patch, hook service or output is supplied.
The render consumer is bounded12B0B..12B4E, not whole12A30 frame execution.
"""
from pathlib import Path
import argparse,hashlib,json,struct,sys,traceback
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--xbe',type=Path,required=True)
parser.add_argument('--map',type=Path,required=True)
parser.add_argument('--repository-root',type=Path,default=Path(__file__).resolve().parents[1])
parser.add_argument('--output',type=Path)
parser.add_argument('--report',type=Path)
args=parser.parse_args();R=args.repository_root.resolve()
fixture_output=args.output or R/'tests/fixtures/retail_cinematic_far_clip_tu15.inc'
sys.path[:0]=[str(R/'tools/reference/halo2-decompiled/tools'),str(R/'tools')]
from xbe import Xbe
from inspect_multiplayer import Cache
XBE=args.xbe
x=Xbe(XBE);pin=hashlib.sha256(x.data).hexdigest();assert pin=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
md=Cs(CS_ARCH_X86,CS_MODE_32)
def ins(p,n):return [{'pc':hex(i.address),'bytes':i.bytes.hex(),'mnemonic':i.mnemonic,'operands':i.op_str} for i in md.disasm(x.read(p,n),p)]
def memory():
 u=Uc(UC_ARCH_X86,UC_MODE_32);pages=set()
 for s in x.sections:
  for p in range(s.va&~4095,(s.va+s.vsize+4095)&~4095,4096):
   if p not in pages:u.mem_map(p,4096);pages.add(p)
  u.mem_write(s.va,x.section_bytes(s))
 for p,n in [(0x90000000,0x20000),(0x60000000,0x10000),(0x70000000,0x1000)]:u.mem_map(p,n)
 return u
def put(u,a,v,n=4):u.mem_write(a,int(v).to_bytes(n,'little'))
def get(u,a,n=4):return int.from_bytes(u.mem_read(a,n),'little')
c=Cache(args.map);nodes=c.u(c.scenario+0x23c)-c.magic
actual={}
for datum in (0xe9fe068b,0xe9ff068c,0xea00068d):
 p=nodes+(datum&65535)*20;raw=bytes(c.data[p:p+20]);assert struct.unpack_from('<H',raw)[0]==datum>>16;actual[datum]=raw
assert struct.unpack_from('<H',actual[0xe9fe068b],2)[0]==711
assert struct.unpack_from('<I',actual[0xea00068d],16)[0]==0x45000000
authored={'map':'06b_floodzone','script':'c06_intra2_02_setup','root':'e9fe068b','nodes':{hex(k):v.hex() for k,v in actual.items()},'metadata_regions':[{'offset':nodes+(d&65535)*20,'bytes':20,'sha256':hashlib.sha256(v).hexdigest()} for d,v in actual.items()]}
c.data.close();c.stream.close()
report={'status':'RUNNING','scope':__doc__,'xbe_sha256':pin,'authored':authored,'wrapper_cases':[],'consumer_cases':[],'reset_cases':[]}
try:
 assert struct.unpack('<HHIIHH',x.read(0x45a208,16))==(4,0,0x2b5150,0,1,6)
 for owner in (0,0x90010000):
  for bits in (0,0x80000000,0x45000000,0xc0800000,0x7f800000,0xff800000,0x7fc12345,0x3c000000):
   u=memory();pool=0x90002000;syntax_pool=0x90002100;thread=0x90003000;frame=thread+0x40;parent=thread+0x18;node_bank=0x90004000;output=0x90003800;stack=0x60008000;stop=0x70000000
   for a,v in [(0x505e5c,pool),(0x505e60,1),(0x505e6c,syntax_pool),(pool+0x44,thread),(syntax_pool+0x20,0x68e),(syntax_pool+0x24,20),(syntax_pool+0x44,node_bank),(thread+0x10,frame),(frame,parent),(frame+4,0xe9fe068b),(parent+8,output),(0x515ec0,owner)]:put(u,a,v)
   put(u,thread+2,1,1);put(u,thread+3,1,1);put(u,thread+0xc,0x100,2)
   for d,raw in actual.items():u.mem_write(node_bank+(d&65535)*20,raw)
   put(u,node_bank+0x68d*20+16,bits)
   if owner:u.mem_write(owner,b'\xa5'*0x3fc)
   seen=[];stores=[]
   u.hook_add(UC_HOOK_CODE,lambda uc,p,n,_:seen.append(p))
   def write(uc,access,a,n,v,_):
    if a==owner+0x194 and owner:stores.append({'pc':hex(uc.reg_read(UC_X86_REG_EIP)),'address':hex(a),'before':bytes(uc.mem_read(a,n)).hex(),'after':int(v).to_bytes(n,'little').hex()})
   u.hook_add(UC_HOOK_MEM_WRITE,write)
   phases=[]
   # Whole211B10 owns both normal evaluator visits and resets frame+C
   # before each one. Execute it rather than supplying a continuation cursor.
   put(u,stack,stop);u.reg_write(UC_X86_REG_ESP,stack)
   u.emu_start(0x212470,stop,count=1000)
   put(u,stack,stop);u.reg_write(UC_X86_REG_ESP,stack)
   u.reg_write(UC_X86_REG_EDI,0xa0010000);u.reg_write(UC_X86_REG_EFLAGS,0x202)
   u.emu_start(0x211b10,stop,count=10000)
   assert u.reg_read(UC_X86_REG_EIP)==stop and u.reg_read(UC_X86_REG_ESP)==stack+4
   phases.append({'entry':'0x211b10','thread_frame':hex(get(u,thread+0x10)),'eax':hex(u.reg_read(UC_X86_REG_EAX))})
   assert 0x212010 in seen and 0x211cb0 in seen and 0x211da0 in seen and get(u,thread+0x10)==parent and get(u,output)==0
   assert stores==([{'pc':'0x2b5185','address':hex(owner+0x194),'before':'a5a5a5a5','after':bits.to_bytes(4,'little').hex()}] if owner else []),stores
   if owner:assert bytes(u.mem_read(owner,0x194))==b'\xa5'*0x194 and bytes(u.mem_read(owner+0x198,0x3fc-0x198))==b'\xa5'*(0x3fc-0x198)
   report['wrapper_cases'].append({'owner':hex(owner),'bits':hex(bits),'phases':phases,'instructions':len(seen),'stores':stores,'actual_parser_and_hs_completion':True})
 for owner in (0,0x90010000):
  for near_bits in (0,0x80000000,0x3d000000,0xbc800000,0x7fc12345,0x7f800000):
   for far_bits in (0,0x80000000,0x45000000,0xc4000000,0x7fc12345,0x7f800000):
    u=memory();put(u,0x515ec0,owner)
    if owner:put(u,owner+0x190,near_bits);put(u,owner+0x194,far_bits)
    u.reg_write(UC_X86_REG_ESP,0x60008000)
    for reg,value in ((UC_X86_REG_XMM0,0x3c800000),(UC_X86_REG_XMM1,0x44800000),(UC_X86_REG_XMM3,0)):u.reg_write(reg,value)
    u.emu_start(0x12b0b,0x12b4e,count=100)
    near,far=get(u,0x4922f4),get(u,0x4922f8)
    def selected(bits,default):return bits if struct.unpack('<f',struct.pack('<I',bits))[0]>0 else default
    assert near==(selected(near_bits,0x3c800000) if owner else 0x3c800000)
    assert far==(selected(far_bits,0x44800000) if owner else 0x44800000)
    report['consumer_cases'].append({'owner':hex(owner),'near_bits':hex(near_bits),'far_bits':hex(far_bits),'result':[hex(near),hex(far)]})
 for owner in (0,0x90010000):
  u=memory();put(u,0x515ec0,owner);put(u,0x4c1eb0,0)
  if owner:u.mem_write(owner,b'\xa5'*0x3fc)
  put(u,0x60008000,0x70000000);u.reg_write(UC_X86_REG_ESP,0x60008000)
  u.emu_start(0x1fcb0,0x70000000,count=1000)
  assert u.reg_read(UC_X86_REG_ESP)==0x60008004
  if owner:assert bytes(u.mem_read(owner,0x3fc))==bytes(0x3fc)
  report['reset_cases'].append({'owner':hex(owner),'complete_original_1fcb0':True,'clear_bytes':0x3fc if owner else 0})
 report['instructions']={hex(p):ins(p,n) for p,n in [(0x2b5150,72),(0x12ad1,125),(0x1fcb0,47),(0x2f920,32),(0x248cf,122)]}
 report['status']='PASS_UNCHANGED_ORIGINAL_FAR_CLIP_PARSER_STORE_CONSUMER_AND_RESET'
except Exception as e:
 report['status']='FAIL_RETAINED';report['error']=repr(e);traceback.print_exc()
def preserve(path,text):
 path.parent.mkdir(parents=True,exist_ok=True)
 if path.exists():
  if path.read_text()!=text:raise ValueError(f'Preserve differing existing output: {path}')
 else:path.write_text(text)
if args.report:preserve(args.report,json.dumps(report,indent=2)+'\n')
if report['status']!='PASS_UNCHANGED_ORIGINAL_FAR_CLIP_PARSER_STORE_CONSUMER_AND_RESET':
 raise SystemExit('Original far-clip oracle execution failed; fixture not generated')
s='// Generated by tools/generate_campaign_far_clip_fixture.py from OG Xbox TU1.5.\n'
s+='struct OriginalClipCase {bool owner;std::uint32_t near_word,far_word,expected_near,expected_far;};\n'
s+='constexpr OriginalClipCase original_clip_cases[]{\n'
for row in report['consumer_cases']:
 s+='    {'+','.join(['true' if int(row['owner'],16) else 'false',row['near_bits']+'u',row['far_bits']+'u',row['result'][0]+'u',row['result'][1]+'u'])+'},\n'
s+='};\nstruct OriginalFarCase {bool owner;std::uint32_t word;};\nconstexpr OriginalFarCase original_far_cases[]{\n'
for row in report['wrapper_cases']:s+='    {'+('true' if int(row['owner'],16) else 'false')+','+row['bits']+'u},\n'
s+='};\n';preserve(fixture_output,s)
print(report['status'],len(report['wrapper_cases']),'whole-dispatch cases',len(report['consumer_cases']),'selection cases',len(report['reset_cases']),'reset cases')
print('Fixture:',fixture_output)
