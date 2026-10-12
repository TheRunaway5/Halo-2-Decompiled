exec(open('tools/audit_instance_contact_ties_original.py').read().split('for line in Path')[0])
from unicorn import UC_HOOK_CODE
path=Path('assets/disc/maps/headlong.map');c=Cache(path);entry=c.block(c.scenario+0x210,0x44)[0];start,size,memory=c.u(entry),c.u(entry+4),c.u(entry+8);alloc=(size+4095)//4096*4096;g.mem_map(memory,alloc);g.mem_write(memory,bytes(c.data[start:start+size]));g.mem_write(0x4e0348,struct.pack('<I',memory+16))
original=json.loads(Path('analysis/multiplayer-shading/instance-tie-original.json').read_text());real=original['false_admissions'][0];origin=real['origin'];ray=real['ray']
pointer=lambda at:c.u(start+16+at)
original_instances=pointer(0x144);count=c.u(start+16+0x140);g.mem_map(0x2020000,0x20000);g.mem_write(0x2020000,bytes(g.mem_read(original_instances,count*88)));g.mem_write(memory+16+0x144,struct.pack('<I',0x2020000));g.mem_write(memory+16+0x140,struct.pack('<I',count+1))
def run(i,p,d,maxval=1):
 raw=bytearray(96);struct.pack_into('<f',raw,4,maxval);g.mem_write(result,bytes(raw));g.mem_write(point,struct.pack('<3f',*p));g.mem_write(rayptr,struct.pack('<3f',*d));g.reg_write(UC_X86_REG_EAX,i);g.reg_write(UC_X86_REG_EBX,result);stack=0x200f000;g.mem_write(stack,struct.pack('<5I',0x3000000,0x04800c0d,9,point,rayptr));g.reg_write(UC_X86_REG_ESP,stack);g.emu_start(0x1691a0,0x3000000,count=1000000);assert g.reg_read(UC_X86_REG_EIP)==0x3000000
 raw=bytes(g.mem_read(result,96));return [g.reg_read(UC_X86_REG_EAX)&255,struct.unpack_from('<I',raw,4)[0],struct.unpack_from('<I',raw,0x4c)[0],struct.unpack_from('<I',raw,0x50)[0],struct.unpack_from('<I',raw,0x3c)[0]]
def order(ids,p,d,initial=1,static=False):
 g.mem_write(result,bytes(96));g.mem_write(result+4,struct.pack('<f',initial));g.mem_write(result,struct.pack('<I',1 if static else 0));g.mem_write(result+0x3c,struct.pack('<I',0xffffffff if static else 0));g.mem_write(0x4e7c1c,struct.pack('<I',1));g.mem_write(0x4e7c20,bytes(512*4));g.mem_write(point,struct.pack('<3f',*p));g.mem_write(rayptr,struct.pack('<3f',*d));g.mem_write(callback,struct.pack('<8I',0x459e48,floatbits(initial),0x04800c0d,9,point,rayptr,0,result));trace=[]
 for i in ids:
  g.mem_write(shape,struct.pack('<2I',0,i));g.reg_write(UC_X86_REG_ECX,callback);stack=0x200f000;g.mem_write(stack,struct.pack('<3I',0x3000000,shape,0));g.reg_write(UC_X86_REG_ESP,stack);g.emu_start(0x244c10,0x3000000,count=1000000);assert g.reg_read(UC_X86_REG_EIP)==0x3000000
  raw=bytes(g.mem_read(result,96));trace.append([i,*struct.unpack_from('<II',raw),struct.unpack_from('<I',raw,0x3c)[0],struct.unpack_from('<I',raw,0x50)[0]])
 return trace
# Read-only observation of the exact original local query inputs.
observed=[]
def observe(g,a,size,_):
 if a==0x1de630:
  sp=g.reg_read(UC_X86_REG_ESP);p,d=struct.unpack('<2I',g.mem_read(sp+12,8));observed.append([*struct.unpack('<3f',g.mem_read(p,12)),*struct.unpack('<3f',g.mem_read(d,12))])
hook=g.hook_add(UC_HOOK_CODE,observe);run(5,origin,ray);g.hook_del(hook);local=observed[0]
controls=[]
copy=bytes(g.mem_read(0x2020000+27*88,88))
for fraction,label in [(-.1,'tie_then_closer'),(.1,'tie_then_farther')]:
 raw=bytearray(copy)
 for offset in [0x28,0x3c]:
  p=struct.unpack_from('<3f',raw,offset);q=[fbits(floatbits(p[j]+fbits(floatbits(ray[j]*fraction)))) for j in range(3)];struct.pack_into('<3f',raw,offset,*q)
 g.mem_write(0x2020000+count*88,bytes(raw));ids=[5,27,count];records=[run(i,origin,ray) for i in ids];trace=order(ids,origin,ray)
 words=list(struct.unpack('<13I',raw[:52]))+list(struct.unpack_from('<4I',raw,60))
 controls.append({'label':label,'origin':origin,'ray':ray,'new_copy':27,'override_instance':count,'override_words':words,'queries':records,'bounded_queries':[run(i,origin,ray,fbits(real['expected_fraction_bits'])) for i in ids],'order':trace,'main_definition':None})
# Same authored collision bank is an explicit static/identity-instance control.
raw=bytearray(g.mem_read(0x2020000+5*88,88));struct.pack_into('<13f',raw,0,1,1,0,0,0,1,0,0,0,1,0,0,0);struct.pack_into('<4f',raw,60,0,0,0,1000);g.mem_write(0x2020000+5*88,bytes(raw));p,d=local[:3],local[3:];record=run(5,p,d);assert record[0]
definition=struct.unpack_from('<H',raw,52)[0];collision_root=pointer(0x13c)+definition*200+0x70
qresult=0x2003000;g.mem_write(qresult,bytes(1052));g.mem_write(point,struct.pack('<3f',*p));g.mem_write(rayptr,struct.pack('<3f',*d));g.reg_write(UC_X86_REG_EAX,qresult);g.reg_write(UC_X86_REG_ECX,9);g.reg_write(UC_X86_REG_EDX,collision_root);g.reg_write(UC_X86_REG_XMM0,floatbits(1));stack=0x200f000;g.mem_write(stack,struct.pack('<5I',0x3000000,256,0x2010001,point,rayptr));g.reg_write(UC_X86_REG_ESP,stack);g.emu_start(0x1de630,0x3000000,count=1000000);assert g.reg_read(UC_X86_REG_EIP)==0x3000000 and g.reg_read(UC_X86_REG_EAX)&255
static=list(struct.unpack('<5I',g.mem_read(qresult,20)));assert static[0]==record[1]
trace=order([5],p,d,fbits(static[0]),True);assert trace[-1][1]==1 and trace[-1][3]==0xffffffff
controls.append({'label':'static_equal_retains_static','origin':p,'ray':d,'new_copy':None,'override_instance':5,'override_words':list(struct.unpack('<13I',raw[:52]))+list(struct.unpack_from('<4I',raw,60)),'queries':[record],'order':trace,'main_definition':definition,'static_contact':{'fraction_bits':static[0],'leaf':static[2],'surface':static[3]},'equal_bound':run(5,p,d,fbits(static[0]))})
report={'xbe_sha256':sha,'source':'unchanged1691A0/183FC0/1DE630/244C10; observational1DE630 input read only','scope':'Controlled placement copies and identity-instance/static-bank inputs based on actual Headlong authored collision bank. Explicit callback order; no actual3009E0 order claim.','cases':controls};Path('analysis/multiplayer-shading/instance-tie-controls-original.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
