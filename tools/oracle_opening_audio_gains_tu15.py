"""All49 authored opening gains through unchanged TU1.5 control and packet CPU.

Neutral class-fade inputs and control1 are explicit ABI inputs. This is the
centered packet gain path; listener, admission, Xbox DSP/final mix and device
delivery are not supplied or claimed. Actual map metadata is paged on demand.
"""
import sys,struct,json,hashlib,argparse
from pathlib import Path
sys.path.insert(0,str(Path.cwd()/'tools'))
from oracle_sound_control import Xbe,Cache,NEW
from oracle_sound_packet_gain import execute
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86_const import X86_OP_MEM
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_MEM_INVALID,UC_MEM_READ_UNMAPPED
from unicorn import x86_const as R
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args();p=args.output.resolve();p.mkdir(parents=True,exist_ok=False)
with Path('assets/disc/maps/00a_introduction.map').open('rb') as stream:
 map_pin=hashlib.file_digest(stream,'sha256').hexdigest()
assert map_pin=='5093e2f8ef7f0df41c11c6621bac5785a36f78a1cff3b8b2bc6c14b561c02a28'
pair=Path('analysis/multiplayer/integration/sound-platform-voice-contract/paired')
assert hashlib.sha256((pair/'report.json').read_bytes()).hexdigest()=='29bf3e2273704f6d2f3bf18ece740389febeda3b987f16e55c3fa9a29d2e9e4d'
c=Cache(Path('assets/disc/maps/00a_introduction.map'));x=Xbe(NEW)
assert hashlib.sha256(x.data).hexdigest()=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
gain=0x127cf0
md=Cs(CS_ARCH_X86,CS_MODE_32);md.detail=True
def instructions(a,n):return list(md.disasm(bytes(g.mem_read(a,n)),a))
calls=[i.operands[0].imm for i in instructions(gain,0xaa)if i.mnemonic=='call']
getter,fade=calls[0],calls[-1]
globals_=[o.mem.disp for i in instructions(getter,0x70)for o in i.operands if o.type==X86_OP_MEM and not o.mem.base and not o.mem.index]
matg_va,table_va,ugh_va=globals_[:3]
fades_va=next(o.mem.disp for i in instructions(fade,16)for o in i.operands if o.type==X86_OP_MEM and not o.mem.base and not o.mem.index)
pages=set()
def fault(machine,access,address,size,value,unused):
 if access!=UC_MEM_READ_UNMAPPED:return False
 page=address&~4095;offset=page-c.magic
 if offset<0 or offset+4096>len(c.data) or page in pages:return False
 machine.mem_map(page,4096);machine.mem_write(page,bytes(c.data[offset:offset+4096]));pages.add(page);return True
g.hook_add(UC_HOOK_MEM_INVALID,fault)
table,stack,stop,fades=0x1000000,0x10f0000,0x10fe000,0x10f3000;g.mem_map(table,0x100000)
def w(a,v):g.mem_write(a,struct.pack('<I',v))
def word(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def real(v):return struct.unpack('<f',struct.pack('<I',v))[0]
for datum,t in c.tags.items():
 if t['offset']is not None:w(table+(datum&65535)*16+8,t['offset']+c.magic)
matg=next(t['offset']for t in c.tags.values()if t['group']=='matg')
ugh=next(t['offset']for t in c.tags.values()if t['group']=='ugh!')
classes=next(t['offset']for t in c.tags.values()if t['group']=='sncl')
for a,v in ((matg_va,matg+c.magic),(table_va,table),(ugh_va,ugh+c.magic),(fades_va,fades)):w(a,v)
for n in range(64):g.mem_write(fades+n*16+12,b'\x01')
rows=[]
for line in Path('tests/fixtures/opening_dialogue_timing_tu15.tsv').read_text().splitlines():
 script,tick,datum=line.split();datum=int(datum,16);t=c.tags[datum];at=t['offset']
 pb=c.u(ugh+4)-c.magic+struct.unpack_from('<h',c.data,at+6)[0]*56
 assert c.f(pb+20)==0
 scale=c.u(ugh+12)-c.magic+c.data[at+11]*20
 cl=c.u(classes+4)-c.magic+c.data[at+2]*92
 pitch=c.u(ugh+36)-c.magic+struct.unpack_from('<h',c.data,at+8)[0]*12
 assert struct.unpack_from('<h',c.data,pitch+10)[0]==1
 permutation=c.u(ugh+44)-c.magic+struct.unpack_from('<h',c.data,pitch+8)[0]*16
 perm_gain=struct.unpack_from('<b',c.data,permutation+4)[0]
 g.mem_write(stack,struct.pack('<3I',stop,0,word(1)))
 for reg,value in ((R.UC_X86_REG_ESP,stack),(R.UC_X86_REG_ESI,at+c.magic),(R.UC_X86_REG_FPCW,0x027f),(R.UC_X86_REG_MXCSR,0x1f80)):g.reg_write(reg,value)
 g.emu_start(gain,stop,count=100000);assert g.reg_read(R.UC_X86_REG_EIP)==stop
 control=g.reg_read(R.UC_X86_REG_EAX);combined=word(c.f(pb+16)+real(control))
 rows.append(dict(script=int(script),datum=datum,name=t['name'],control=control,combined=combined,permutation_gain=perm_gain,
  sound=bytes(c.data[at:at+20]).hex(),playback=bytes(c.data[pb:pb+56]).hex(),sound_class=bytes(c.data[cl:cl+92]).hex(),scale=bytes(c.data[scale:scale+20]).hex()))
del g
answers=execute(NEW,'u5',[(0,r['datum'],word(1),r['combined'],r['permutation_gain'])for r in rows],
 pair)
for row,(packet,volume)in zip(rows,answers):row.update(packet=packet,volume=struct.unpack('<i',struct.pack('<I',volume))[0])
assert len(rows)==49
out=p/'report.json';out.write_text(json.dumps(dict(scope=__doc__,xbe_sha256=hashlib.sha256(x.data).hexdigest(),map_sha256=map_pin,paged_metadata=len(pages)*4096,cues=rows,instruction_patches=[],intercepted_callees=[]),indent=2)+'\n')
(p/'fixtures.tsv').write_text(''.join(f"{r['datum']:08x} {r['control']:08x} {r['volume']} {r['sound']} {r['playback']} {r['sound_class']} {r['scale']}\n"for r in rows))
print('PASS 49 original opening control/packet/driver gains; all actual gain variances0 and single permutations; no patched instructions or intercepted callees')
