"""Unchanged original DoF parser/store/update and bounded render/depth controls.
No instruction patch, callee stub, source-result injection or GPU success.
Caller backing, raw words, clock and target metadata are explicit inputs.
"""
from pathlib import Path
import argparse,sys,json,hashlib,struct,random,traceback
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_INVALID
from unicorn.x86_const import *
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--xbe',type=Path,required=True)
parser.add_argument('--maps-directory',type=Path,required=True)
parser.add_argument('--repository-root',type=Path,default=Path(__file__).resolve().parents[1])
parser.add_argument('--output',type=Path)
cli_args=parser.parse_args();ROOT=cli_args.repository_root.resolve()
fixture_output=cli_args.output or ROOT/'tests/fixtures/retail_cinematic_dof_tu15.json'
sys.path[:0]=[str(ROOT/'tools'),str(ROOT/'tools/reference/halo2-decompiled/tools')]
from xbe import Xbe
from inspect_multiplayer import Cache
XBE=cli_args.xbe
x=Xbe(XBE);PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd';assert hashlib.sha256(x.data).hexdigest()==PIN
OWNER=0x9003f000;STACK=0x60008000;STOP=0x70000000
P=dict(status='RUNNING',scope=__doc__,xbe_sha256=PIN,authored=[],wrapper_cases=[],direct_cases=[],update_cases=[],admission_cases=[],depth_cases=[])
def memory():
 u=Uc(UC_ARCH_X86,UC_MODE_32);pages=set()
 for s in x.sections:
  for p in range(s.va&~4095,(s.va+s.vsize+4095)&~4095,4096):
   if p not in pages:u.mem_map(p,4096);pages.add(p)
  u.mem_write(s.va,x.section_bytes(s))
 for p,n in [(0x90000000,0x40000),(0x60000000,0x10000),(STOP,4096)]:u.mem_map(p,n)
 return u
def put(u,a,v,n=4):u.mem_write(a,int(v).to_bytes(n,'little',signed=False))
def get(u,a,n=4):return int.from_bytes(u.mem_read(a,n),'little')
def fword(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def setup(u,words=(),first=0):
 put(u,STACK,STOP)
 for i,w in enumerate(words):put(u,STACK+4+i*4,w)
 for reg in (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):u.reg_write(reg,0)
 u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_XMM0,first);u.reg_write(UC_X86_REG_EFLAGS,0x202)
 u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_FPTAG,0xffff)
def run(u,p,stop=STOP):
 u.emu_start(p,stop,count=100000)
 assert u.reg_read(UC_X86_REG_EIP)==stop,(hex(p),hex(u.reg_read(UC_X86_REG_EIP)))
def check_code(u,p,n,_):assert bytes(u.mem_read(p,n))==x.read(p,n)
def bank(u):return bytes(u.mem_read(OWNER,0x3fc))
def clock(u,v):u.mem_write(0x4920c0,struct.pack('<d',v))
try:
 assert struct.unpack('<HHIIH7H',x.read(0x45a19c,28))==(4,0,0x2b4f60,0,7,6,6,6,6,6,6,6)
 inventory=[{'map': '00a_introduction', 'datum': '81321dbe', 'arguments': ['3fc00000', '3f800000', '3f800000', '00000000', '00000000', '00000000', '00000000'], 'script_owners': [{'index': 147, 'name': 'c01_intro_dof_01'}]}, {'map': '01b_spacestation', 'datum': 'e812049f', 'arguments': ['3f59999a', '00000000', '00000000', '00000000', '3f000000', '3f000000', '00000000'], 'script_owners': [{'index': 52, 'name': 'x02_01_dof'}]}, {'map': '01b_spacestation', 'datum': 'e82c04b9', 'arguments': ['3f800000', '00000000', '00000000', '00000000', '3f000000', '3f000000', '00000000'], 'script_owners': [{'index': 52, 'name': 'x02_01_dof'}]}, {'map': '01b_spacestation', 'datum': 'f880150d', 'arguments': ['3fc00000', '3f800000', '3f800000', '00000000', '00000000', '00000000', '00000000'], 'script_owners': [{'index': 131, 'name': 'x02_07_dof_01'}]}, {'map': '01b_spacestation', 'datum': 'f88f151c', 'arguments': ['3fc00000', '3f800000', '00000000', '3f000000', '00000000', '3f800000', '3f000000'], 'script_owners': [{'index': 131, 'name': 'x02_07_dof_01'}]}, {'map': '01b_spacestation', 'datum': '8b6a27f6', 'arguments': ['40a00000', '00000000', '00000000', '00000000', '3f800000', '3f800000', '00000000'], 'script_owners': [{'index': 266, 'name': 'x02_08_dof'}]}, {'map': '03a_oldmombasa', 'datum': 'e8060493', 'arguments': ['41400000', '40a00000', '40a00000', '00000000', '00000000', '00000000', '00000000'], 'script_owners': [{'index': 45, 'name': 'c03_intro_01_dof_1'}]}, {'map': '03a_oldmombasa', 'datum': 'ee400acd', 'arguments': ['3f800000', '00000000', '00000000', '00000000', '3f800000', '3f800000', '00000000'], 'script_owners': [{'index': 88, 'name': 'c03_intro_03_dof_1'}]}, {'map': '03a_oldmombasa', 'datum': 'ee630af0', 'arguments': ['40400000', '00000000', '00000000', '00000000', '3f800000', '3f800000', '00000000'], 'script_owners': [{'index': 89, 'name': 'c03_intro_03_dof_2'}]}, {'map': '03a_oldmombasa', 'datum': 'ee720aff', 'arguments': ['41100000', '40200000', '40200000', '00000000', '00000000', '00000000', '00000000'], 'script_owners': [{'index': 89, 'name': 'c03_intro_03_dof_2'}]}, {'map': '03a_oldmombasa', 'datum': 'f26f0efc', 'arguments': ['40000000', '3f800000', '3f800000', '00000000', '00000000', '00000000', '00000000'], 'script_owners': [{'index': 110, 'name': 'c03_intro_04_dof_1'}]}, {'map': '03b_newmombasa', 'datum': 'ef5c0be9', 'arguments': ['3f800000', '3f800000', '3f800000', '00000000', '00000000', '00000000', '00000000'], 'script_owners': [{'index': 93, 'name': 'x03_01_dof_1'}]}, {'map': '04a_gasgiant', 'datum': 'ea880715', 'arguments': ['3f800000', '3f000000', '3f000000', '3a83126f', '00000000', '00000000', '3a83126f'], 'script_owners': [{'index': 68, 'name': 'x04_03b_dof_1'}]}, {'map': '04a_gasgiant', 'datum': 'eccf095c', 'arguments': ['40000000', '00000000', '00000000', '3a83126f', '3f000000', '3f000000', '3a83126f'], 'script_owners': [{'index': 84, 'name': 'x04_05_dof_1'}]}, {'map': '04a_gasgiant', 'datum': 'ee730b00', 'arguments': ['40000000', '00000000', '00000000', '00000000', '3f000000', '3f000000', '00000000'], 'script_owners': [{'index': 100, 'name': 'x04_06_dof_1'}]}, {'map': '04a_gasgiant', 'datum': 'ee850b12', 'arguments': ['40000000', '3f000000', '3f000000', '3f800000', '00000000', '00000000', '3f800000'], 'script_owners': [{'index': 100, 'name': 'x04_06_dof_1'}]}, {'map': '04a_gasgiant', 'datum': 'ee940b21', 'arguments': ['40000000', '00000000', '00000000', '3a83126f', '00000000', '00000000', '3a83126f'], 'script_owners': [{'index': 100, 'name': 'x04_06_dof_1'}]}, {'map': '04a_gasgiant', 'datum': 'eea30b30', 'arguments': ['40000000', '00000000', '00000000', '3a83126f', '3f000000', '3f000000', '3a83126f'], 'script_owners': [{'index': 100, 'name': 'x04_06_dof_1'}]}, {'map': '04b_floodlab', 'datum': 'eba30830', 'arguments': ['3f800000', '3fc00000', '3fc00000', '00000000', '00000000', '00000000', '00000000'], 'script_owners': [{'index': 69, 'name': 'c04_outro1_01_dof'}]}, {'map': '05a_deltaapproach', 'datum': 'e91b05a8', 'arguments': ['3f800000', '00000000', '00000000', '00000000', '3f800000', '3f800000', '00000000'], 'script_owners': [{'index': 57, 'name': 'x05_05_dof_scene_02'}]}, {'map': '05a_deltaapproach', 'datum': 'e92a05b7', 'arguments': ['3f800000', '00000000', '3f800000', '3f000000', '3f800000', '00000000', '3f000000'], 'script_owners': [{'index': 57, 'name': 'x05_05_dof_scene_02'}]}, {'map': '05a_deltaapproach', 'datum': 'ecff098c', 'arguments': ['3f000000', '00000000', '00000000', '00000000', '3f800000', '3f800000', '00000000'], 'script_owners': [{'index': 83, 'name': 'x05_05_dof_scene_04'}]}, {'map': '05a_deltaapproach', 'datum': 'ed0e099b', 'arguments': ['3f000000', '00000000', '3f800000', '3e800000', '3f800000', '00000000', '3e800000'], 'script_owners': [{'index': 83, 'name': 'x05_05_dof_scene_04'}]}, {'map': '05a_deltaapproach', 'datum': 'f3c2104f', 'arguments': ['41a00000', '00000000', '00000000', '00000000', '40a00000', '40a00000', '00000000'], 'script_owners': [{'index': 135, 'name': 'c05_intro_dof_01'}]}, {'map': '05a_deltaapproach', 'datum': 'f67c1309', 'arguments': ['3f800000', '3f800000', '3f800000', '00000000', '00000000', '00000000', '00000000'], 'script_owners': [{'index': 146, 'name': 'c05_intro_dof_02'}]}, {'map': '05b_deltatowers', 'datum': 'eb8b0818', 'arguments': ['3f800000', '3f000000', '3f000000', '00000000', '00000000', '00000000', '00000000'], 'script_owners': [{'index': 73, 'name': 'c05_intra2_dof'}]}, {'map': '05b_deltatowers', 'datum': 'ec850912', 'arguments': ['3f800000', '3f000000', '3f000000', '00000000', '00000000', '00000000', '00000000'], 'script_owners': [{'index': 81, 'name': 'c05_outro_01_dof'}]}, {'map': '06a_sentinelwalls', 'datum': 'e91f05ac', 'arguments': ['3f800000', '3f000000', '3f000000', '00000000', '00000000', '00000000', '00000000'], 'script_owners': [{'index': 48, 'name': 'x06_01b_dof_1'}]}, {'map': '06a_sentinelwalls', 'datum': 'e92e05bb', 'arguments': ['3f800000', '3f000000', '00000000', '3f800000', '00000000', '3f000000', '3f800000'], 'script_owners': [{'index': 48, 'name': 'x06_01b_dof_1'}]}, {'map': '06a_sentinelwalls', 'datum': 'ed8d0a1a', 'arguments': ['3f800000', '3f000000', '3f000000', '00000000', '00000000', '00000000', '00000000'], 'script_owners': [{'index': 83, 'name': 'x06_04_dof_1'}]}, {'map': '06a_sentinelwalls', 'datum': 'ed9c0a29', 'arguments': ['3f4ccccd', '3f000000', '00000000', '3f800000', '00000000', '3f000000', '3f800000'], 'script_owners': [{'index': 83, 'name': 'x06_04_dof_1'}]}, {'map': '06a_sentinelwalls', 'datum': 'edb10a3e', 'arguments': ['3f800000', '3f000000', '3f000000', '00000000', '00000000', '00000000', '00000000'], 'script_owners': [{'index': 84, 'name': 'x06_04_dof_2'}]}, {'map': '06a_sentinelwalls', 'datum': 'edc00a4d', 'arguments': ['3f800000', '3f000000', '00000000', '3f800000', '00000000', '3f000000', '3f800000'], 'script_owners': [{'index': 84, 'name': 'x06_04_dof_2'}]}, {'map': '06a_sentinelwalls', 'datum': 'eeec0b79', 'arguments': ['3f800000', '3f400000', '3f400000', '00000000', '00000000', '00000000', '00000000'], 'script_owners': [{'index': 96, 'name': 'x06_05_dof_1'}]}, {'map': '06a_sentinelwalls', 'datum': 'eefb0b88', 'arguments': ['3f800000', '3f400000', '00000000', '3f800000', '00000000', '3f400000', '3f800000'], 'script_owners': [{'index': 96, 'name': 'x06_05_dof_1'}]}, {'map': '06a_sentinelwalls', 'datum': 'ef100b9d', 'arguments': ['40000000', '00000000', '3f400000', '3f800000', '3f400000', '00000000', '3f800000'], 'script_owners': [{'index': 97, 'name': 'x06_05_dof_2'}]}, {'map': '06b_floodzone', 'datum': 'e72603b3', 'arguments': ['40a00000', '3f800000', '3f800000', '00000000', '00000000', '00000000', '00000000'], 'script_owners': [{'index': 41, 'name': 'c06_intra1_dof_01'}]}, {'map': '06b_floodzone', 'datum': 'e74803d5', 'arguments': ['40400000', '00000000', '00000000', '00000000', '3f000000', '3f000000', '00000000'], 'script_owners': [{'index': 42, 'name': 'c06_intra1_dof_02'}]}, {'map': '06b_floodzone', 'datum': 'e75a03e7', 'arguments': ['40400000', '00000000', '3f000000', '3f800000', '3f000000', '00000000', '3f800000'], 'script_owners': [{'index': 42, 'name': 'c06_intra1_dof_02'}]}, {'map': '07a_highcharity', 'datum': 'f2f90f86', 'arguments': ['40000000', '00000000', '00000000', '00000000', '3f800000', '3f800000', '00000000'], 'script_owners': [{'index': 129, 'name': 'c07_intro_dof_02'}]}, {'map': '07a_highcharity', 'datum': 'f3080f95', 'arguments': ['40000000', '00000000', '3f800000', '3f000000', '3f800000', '00000000', '3f000000'], 'script_owners': [{'index': 129, 'name': 'c07_intro_dof_02'}]}, {'map': '07a_highcharity', 'datum': 'fe021a8f', 'arguments': ['3f000000', '3f400000', '3f400000', '00000000', '00000000', '00000000', '00000000'], 'script_owners': [{'index': 191, 'name': 'c07_intra1_dof_04'}]}, {'map': '07a_highcharity', 'datum': 'fe111a9e', 'arguments': ['3f000000', '3f400000', '00000000', '3f000000', '00000000', '3f400000', '3f000000'], 'script_owners': [{'index': 191, 'name': 'c07_intra1_dof_04'}]}, {'map': '07b_forerunnership', 'datum': 'e6c40351', 'arguments': ['3f800000', '00000000', '00000000', '00000000', '3f400000', '3f400000', '00000000'], 'script_owners': [{'index': 35, 'name': 'c07_intra1_05_dof_01'}]}, {'map': '07b_forerunnership', 'datum': 'e6d50362', 'arguments': ['3f800000', '00000000', '3f400000', '3f000000', '3f400000', '00000000', '3f000000'], 'script_owners': [{'index': 35, 'name': 'c07_intra1_05_dof_01'}]}, {'map': '07b_forerunnership', 'datum': 'e8860513', 'arguments': ['3ecccccd', '3f000000', '3f000000', '00000000', '00000000', '00000000', '00000000'], 'script_owners': [{'index': 47, 'name': 'c07_intra1_06_dof_01'}]}, {'map': '07b_forerunnership', 'datum': 'ebf60883', 'arguments': ['41a00000', '00000000', '00000000', '00000000', '3f800000', '3f800000', '00000000'], 'script_owners': [{'index': 78, 'name': 'c07_outro_dof_01'}]}, {'map': '07b_forerunnership', 'datum': 'ec1808a5', 'arguments': ['41a00000', '40000000', '40000000', '00000000', '00000000', '00000000', '00000000'], 'script_owners': [{'index': 79, 'name': 'c07_outro_dof_02'}]}, {'map': '08a_deltacliffs', 'datum': 'e68f031c', 'arguments': ['40000000', '00000000', '00000000', '00000000', '3f000000', '3f000000', '00000000'], 'script_owners': [{'index': 30, 'name': 'intro_dof'}]}, {'map': '08a_deltacliffs', 'datum': 'e6a1032e', 'arguments': ['40000000', '00000000', '3f000000', '3f800000', '3f000000', '00000000', '3f800000'], 'script_owners': [{'index': 30, 'name': 'intro_dof'}]}, {'map': '08a_deltacliffs', 'datum': 'e7b60443', 'arguments': ['3fc00000', '00000000', '00000000', '00000000', '3f000000', '3f000000', '00000000'], 'script_owners': [{'index': 38, 'name': 'c04_intra1_dof_01'}]}, {'map': '08a_deltacliffs', 'datum': 'e7c70454', 'arguments': ['3fc00000', '00000000', '3f000000', '3f000000', '3f000000', '00000000', '3f000000'], 'script_owners': [{'index': 38, 'name': 'c04_intra1_dof_01'}]}, {'map': '08a_deltacliffs', 'datum': 'e7d80465', 'arguments': ['40000000', '00000000', '00000000', '00000000', '3f000000', '3f000000', '00000000'], 'script_owners': [{'index': 38, 'name': 'c04_intra1_dof_01'}]}, {'map': '08a_deltacliffs', 'datum': 'e93805c5', 'arguments': ['3f9c28f6', '3f000000', '3f000000', '00000000', '00000000', '00000000', '00000000'], 'script_owners': [{'index': 47, 'name': 'c04_intra1_dof_02'}]}, {'map': '08a_deltacliffs', 'datum': 'e94705d4', 'arguments': ['3f000000', '3f000000', '3f000000', '00000000', '00000000', '00000000', '00000000'], 'script_owners': [{'index': 47, 'name': 'c04_intra1_dof_02'}]}, {'map': '08a_deltacliffs', 'datum': 'e95605e3', 'arguments': ['3f000000', '3f000000', '00000000', '00000000', '00000000', '3f000000', '3f000000'], 'script_owners': [{'index': 47, 'name': 'c04_intra1_dof_02'}]}, {'map': '08b_deltacontrol', 'datum': 'f1580de5', 'arguments': ['3f95c28f', '3f000000', '3f000000', '00000000', '00000000', '00000000', '00000000'], 'script_owners': [{'index': 75, 'name': 'c04_intra1_dof_04'}]}, {'map': '08b_deltacontrol', 'datum': 'f1670df4', 'arguments': ['3f000000', '3f000000', '3f000000', '00000000', '00000000', '00000000', '00000000'], 'script_owners': [{'index': 75, 'name': 'c04_intra1_dof_04'}]}, {'map': '08b_deltacontrol', 'datum': 'f1760e03', 'arguments': ['3f000000', '3f000000', '00000000', '00000000', '00000000', '3f000000', '3f000000'], 'script_owners': [{'index': 75, 'name': 'c04_intra1_dof_04'}]}, {'map': '08b_deltacontrol', 'datum': 'f43c10c9', 'arguments': ['3f800000', '3f000000', '3f000000', '3a83126f', '00000000', '00000000', '3a83126f'], 'script_owners': [{'index': 96, 'name': 'intra2_dof'}]}, {'map': '08b_deltacontrol', 'datum': 'fc7018fd', 'arguments': ['40a00000', '00000000', '00000000', '00000000', '40000000', '40000000', '00000000'], 'script_owners': [{'index': 162, 'name': 'c08_intra3_scene_06'}]}, {'map': '08b_deltacontrol', 'datum': 'fc7c1909', 'arguments': ['40a00000', '00000000', '40000000', '3f000000', '40000000', '00000000', '3f000000'], 'script_owners': [{'index': 162, 'name': 'c08_intra3_scene_06'}]}, {'map': '08b_deltacontrol', 'datum': 'fe361ac3', 'arguments': ['3f000000', '3f800000', '3f800000', '00000000', '00000000', '00000000', '00000000'], 'script_owners': [{'index': 176, 'name': 'x09_dof_01'}]}, {'map': '08b_deltacontrol', 'datum': 'fe451ad2', 'arguments': ['3f000000', '3f800000', '00000000', '3e800000', '00000000', '3f800000', '3e800000'], 'script_owners': [{'index': 176, 'name': 'x09_dof_01'}]}]
 for mapname in dict.fromkeys(v['map'] for v in inventory):
  c=Cache(cli_args.maps_directory/f'{mapname}.map');nodes=c.u(c.scenario+0x23c)-c.magic
  for row in (v for v in inventory if v['map']==mapname):
   datum=int(row['datum'],16);root=nodes+(datum&65535)*20;raws={datum:bytes(c.data[root:root+20])};assert struct.unpack_from('<H',raws[datum])[0]==datum>>16 and struct.unpack_from('<H',raws[datum],2)[0]==706
   fn=struct.unpack_from('<I',raws[datum],16)[0];pos=nodes+(fn&65535)*20;raws[fn]=bytes(c.data[pos:pos+20]);assert struct.unpack_from('<H',raws[fn])[0]==fn>>16
   arg=struct.unpack_from('<I',raws[fn],8)[0];values=[]
   for expected in row['arguments']:
    pos=nodes+(arg&65535)*20;raw=bytes(c.data[pos:pos+20]);assert struct.unpack_from('<H',raw)[0]==arg>>16 and struct.unpack_from('<H',raw,4)[0]==6 and struct.unpack_from('<I',raw,16)[0]==int(expected,16)
    raws[arg]=raw;values.append(int(expected,16));arg=struct.unpack_from('<I',raw,8)[0]
   assert arg==0xffffffff and len(raws)==9
   audit={**row,'raw_nodes':{hex(d):v.hex() for d,v in raws.items()},'metadata_sha256':hashlib.sha256(b''.join(raws.values())).hexdigest()};P['authored'].append(audit)
   # Complete original HS dispatcher owns both parser passes and cursors.
   u=memory();pool=0x90002000;syntax=0x90002100;thread=0x90003000;frame=thread+0x40;parent=thread+0x18;nodebank=0x90004000;out=0x90003800
   for a,v in [(0x505e5c,pool),(0x505e60,1),(0x505e6c,syntax),(pool+0x44,thread),(syntax+0x20,(max(raws)&65535)+1),(syntax+0x24,20),(syntax+0x44,nodebank),(thread+0x10,frame),(frame,parent),(frame+4,datum),(parent+8,out),(0x515ec0,OWNER)]:put(u,a,v)
   put(u,thread+2,1,1);put(u,thread+3,1,1);put(u,thread+0xc,0x100,2)
   for d,raw in raws.items():u.mem_write(nodebank+(d&65535)*20,raw)
   u.mem_write(OWNER,b'\xa5'*0x3fc);clock(u,4096.0003);seen=[];stores=[]
   u.hook_add(UC_HOOK_CODE,check_code);u.hook_add(UC_HOOK_CODE,lambda uc,p,n,_:seen.append(p))
   def observe(uc,access,a,n,v,_):
    if OWNER<=a<OWNER+0x3fc:stores.append(dict(pc=hex(uc.reg_read(UC_X86_REG_EIP)),offset=a-OWNER,size=n,value=v))
   def invalid(uc,access,address,size,value,_):
    P['invalid_access']=dict(pc=hex(uc.reg_read(UC_X86_REG_EIP)),address=hex(address),size=size,access=access,sp=hex(uc.reg_read(UC_X86_REG_ESP)),map=mapname,datum=row['datum']);return False
   u.hook_add(UC_HOOK_MEM_INVALID,invalid)
   u.hook_add(UC_HOOK_MEM_WRITE,observe);setup(u);run(u,0x212470);setup(u);run(u,0x211b10)
   assert get(u,thread+0x10)==parent and get(u,out)==0 and all(p in seen for p in (0x2b4f60,0x212010,0x1fc10,0x211da0))
   P['wrapper_cases'].append(dict(map=mapname,datum=row['datum'],arguments=values,before='a5'*0x3fc,after=bank(u).hex(),clock_hex=struct.pack('<d',4096.0003).hex(),stores=stores))
  c.data.close();c.stream.close()
 # Raw special-value and independently skipped duration controls.
 cases=[]
 for first in (0,0x80000000,0xbf800000,0xff800000,0x7f800000,0x7fc12345,0x7f812345):
  for d1,d2 in ((0,0),(0x80000000,0xbf800000),(0xbf800000,0x3f800000),(0x7fc12345,0),(0x7f800000,0xff800000)):
   cases.append([first,0x80000000,0x7fc54321,d1,0x3f000000,0x7f800000,d2])
 for owner in (0,OWNER):
  for words in cases:
   u=memory();put(u,0x515ec0,owner);u.mem_write(OWNER,b'\xa5'*0x3fc);clock(u,16777216.25);u.hook_add(UC_HOOK_CODE,check_code);setup(u,words[1:],words[0]);run(u,0x1fc10)
   assert u.reg_read(UC_X86_REG_ESP)==STACK+28
   P['direct_cases'].append(dict(owner=bool(owner),arguments=words,clock_hex=struct.pack('<d',16777216.25).hex(),before='a5'*0x3fc,after=bank(u).hex()))
 # Full1FD00 interpolates/clears over explicitly initialized output/control bank.
 schedules=[(0.,[5.,1.,3.,2.,0.,2.,4.]),(4096.0003,[1.5,1.,1.,0.,0.,0.,0.]),(16777216.25,[2.,1.,0.,1.,0.,1.,1.]),(4.,[1.,0.,0.,-1.,1.,0.,-1.])]
 for epoch,values in schedules:
  u=memory();u.hook_add(UC_HOOK_CODE,check_code);put(u,0x515ec0,OWNER);u.mem_write(OWNER,bytes(0x3fc));put(u,OWNER+0xac,1,1);clock(u,epoch);words=list(map(fword,values));setup(u,words[1:],words[0]);run(u,0x1fc10)
  for delta in (0.,1/120,1/30,.5,1.,2.,3.,4.,5.):
   clock(u,epoch+delta);before=bank(u);setup(u);run(u,0x1fd00);assert u.reg_read(UC_X86_REG_EAX)==OWNER
   P['update_cases'].append(dict(epoch=epoch,delta=delta,arguments=words,clock_hex=struct.pack('<d',epoch+delta).hex(),before=before.hex(),after=bank(u).hex()))
 # Bounded actual13671..13709 admits from the completed bank. The supplied
 # pointer is the real1FD00 result, not a fabricated graphics callee return.
 rng=random.Random(0x13660)
 admissions=[(special,mode,modifier,active,focus,rect,views,ready) for special,mode,modifier,active,focus,rect,views,ready in [
 (0,0,0,1,0x3f800000,[0,0,480,640],1,1),(2,0,0,1,0x3f800000,[0,0,480,640],1,1),
 (0,5,0,1,0x3f800000,[0,0,480,640],1,1),(0,7,0,1,0x3f800000,[0,0,480,640],1,1),
 (0,8,0,1,0x3f800000,[0,0,480,640],1,1),(0,0,1,1,0x3f800000,[0,0,480,640],1,1),
 (0,0,0,0,0x3f800000,[0,0,480,640],1,1),(0,0,0,1,0,[0,0,480,640],1,1),
 (0,0,0,1,0x7fc12345,[0,0,480,640],1,1),(0,0,0,1,0x7f800000,[0,0,480,640],1,1),
 (0,0,0,1,0x3f800000,[0,1,480,640],1,1),(0,0,0,1,0x3f800000,[0,0,720,1280],1,1),
 (0,0,0,1,0x3f800000,[0,0,480,640],2,1),(0,0,0,1,0x3f800000,[0,0,480,640],0,1),
 (0,0,0,1,0x3f800000,[0,0,480,640],0xffffffff,1),(0,0,0,1,0x3f800000,[0,0,480,640],1,0)]]
 for special,mode,modifier,active,focus,rect,views,ready in admissions:
  u=memory();u.hook_add(UC_HOOK_CODE,check_code);put(u,0x491e22,special,2);put(u,0x4920b8,mode);u.mem_write(0x491e68,struct.pack('<4H',*rect));put(u,0x4c686c,views);put(u,OWNER+0x3c,active,1);put(u,OWNER+0x40,focus);put(u,0x4c1378+18*0x98+0x88,1 if ready else 0);put(u,0x4c1378+18*0x98+0x95,0,1);setup(u);u.reg_write(UC_X86_REG_EAX,OWNER);u.reg_write(UC_X86_REG_EBX,modifier)
  stop=[]
  def boundary(uc,p,n,_):
   if p in (0x1367d,0x13709,0x13756):stop.append(p);uc.emu_stop()
  u.hook_add(UC_HOOK_CODE,boundary);u.emu_start(0x13671,STOP,count=1000);assert len(stop)==1,stop
  P['admission_cases'].append(dict(special=special,mode=mode,modifier=bool(modifier),active=bool(active),focus_word=focus,rectangle=rect,views=views,ready=bool(ready),enabled=stop[0]==0x13709,stop=hex(stop[0])))
 # Pure original13430 through its mode2 source clear boundary, before graphics.
 for near,far,focus in [(.015625,1024.,1.),(.015625,2048.,5.),(-1.,128.,256.),(8.,4.,-1.),(0.,1024.,float('inf'))]:
  for mode in (1,2):
   u=memory();u.hook_add(UC_HOOK_CODE,check_code);put(u,0x4922f4,fword(near));put(u,0x4922f8,fword(far));rec=0x90001000;put(u,rec,7);put(u,rec+4,mode);put(u,rec+8,fword(focus));put(u,0x4920d8,3);setup(u);u.reg_write(UC_X86_REG_ECX,rec);stop=0x13574 if mode==1 else 0x13544;run(u,0x13430,stop)
   P['depth_cases'].append(dict(near_word=fword(near),far_word=fword(far),focus_word=fword(focus),mode=mode,current_target=3,selected_target=get(u,0x4920d8),minimum=get(u,0x4922fc),maximum=get(u,0x492300),stop=hex(stop)))
 P['status']='PASS_UNCHANGED_DOF_AUTHORED64_PARSER_RAW_SETTER_UPDATE_ADMISSION_AND_DEPTH'
except Exception as e:P['status']='FAIL_RETAINED';P['error']=repr(e);traceback.print_exc()
payload=(json.dumps(P,indent=2)+'\n').encode()
fixture_output.parent.mkdir(parents=True,exist_ok=True)
if fixture_output.exists():
 if fixture_output.read_bytes()!=payload:raise ValueError('Preserve existing differing DoF oracle evidence')
else:fixture_output.write_bytes(payload)
print(json.dumps({k:(len(v) if isinstance(v,list) else v) for k,v in P.items()},indent=2))
raise SystemExit(P['status']=='FAIL_RETAINED')
