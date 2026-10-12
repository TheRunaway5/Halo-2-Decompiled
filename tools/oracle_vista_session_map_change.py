#!/usr/bin/env python3
"""Execute original PC5D90FA/5D94A1 and448DD2 multiplayer validation.

Only CRT memory/string helpers, explicit configuration overrides/RNG/clock owners, optional
validator-result cases, protected40911B native fulfillment and later UI setter
effects are controlled services. No protected loader or live world readiness
body parity is claimed. Original admission getters/options constructor execute.
"""
from pathlib import Path
import hashlib,itertools,json,struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_vista_pcr_screen import ROOT,SHA,BASE,STOP,STACK,OBJ,machine,u32,w32

SESSION,HANDLER,LOADED,OPTIONS=OBJ,OBJ+0x8000,OBJ+0x9000,OBJ+0xb000
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/map-change-original'
MACHINE=bytes.fromhex('123456789abc')
IDENTITY=bytes.fromhex('1032547698badcfe')
DEFAULT=bytes.fromhex(next(l.split()[1] for l in (ROOT/'tests/fixtures/vista_pcr_publication_original.tsv').read_text().splitlines() if l.startswith('D ')))

def make_bank(q):
    b=bytearray(0x78c0)
    def put(at,v,n=4):b[at:at+n]=(v&((1<<(8*n))-1)).to_bytes(n,'little')
    put(0x73a0,q.get('state',5));put(0x4dd4,q.get('connection',0));put(0x4dc8,0x76543210);put(0x4dcc,0xfedcba98)
    put(0x4dd0,0x1badb002);put(0x4cb8,2);put(0x4dd8,q.get('map_id',123))
    path=q.get('path','levels\\test\\actual');raw=path.encode('utf-16-le')
    b[0x4cbc:0x4cbc+min(len(raw),520)]=raw[:520]
    put(0x5101,q.get('prepared',1),1);put(0x5104,q.get('machines',1));b[0x5108:0x510e]=MACHINE
    put(0x5eb8,q.get('campaign',0xffff),2);put(0x70+0x11e4,q.get('peer_count',1));put(0x70+0x14,q.get('role',0))
    put(0x64,q.get('local',0));put(0x71c0,q.get('host',0));put(0x1c,q.get('type',0));put(0x4c74,q.get('local_flag',0),1)
    put(0x4c78,q.get('ui_mode',0));b[0x4e3c:0x4f6c]=DEFAULT
    for i in range(17):b[0x70+i*0x10c+0x22:0x70+i*0x10c+0x28]=bytes((x+i)&255 for x in MACHINE)
    b[0x4ddc:0x4e3c]=bytes((i*13+7)&255 for i in range(96))
    put(0x5178,q.get('occupied',1),1);put(0x5179,q.get('guest',0),1);put(0x517a,q.get('controller',0),2)
    b[0x5180:0x5186]=MACHINE;b[0x5186:0x518e]=IDENTITY
    for i in range(0x84):b[0x5190+i]=(i*17+29)&255
    return b

class Original:
    def __init__(self,image):
        self.image=image;self.u=machine(image);self.visits=0;self.executed=set()
        self.u.hook_add(UC_HOOK_CODE,self.hook)
    def ret(self,value=None,pop=0):
        s=self.u.reg_read(UC_X86_REG_ESP)
        if value is not None:self.u.reg_write(UC_X86_REG_EAX,value&0xffffffff)
        self.u.reg_write(UC_X86_REG_EIP,u32(self.u,s));self.u.reg_write(UC_X86_REG_ESP,s+4+pop)
    def hook(self,u,pc,size,_):
        self.visits+=1;self.executed.add(pc);s=u.reg_read(UC_X86_REG_ESP)
        if pc==STOP:u.emu_stop();return
        if pc==0x687b7e:
            target=u32(u,s);u.reg_write(UC_X86_REG_ESP,s+4-u.reg_read(UC_X86_REG_EAX));u.reg_write(UC_X86_REG_EIP,target);return
        if pc==0x6875d3:self.ret();return
        if pc==0x687ba9:
            dst,value,count=(u32(u,s+4+i*4) for i in range(3));u.mem_write(dst,bytes([value&255])*count);self.ret(dst);return
        if pc==0x687e18:
            dst,src,count=(u32(u,s+4+i*4) for i in range(3));u.mem_write(dst,bytes(u.mem_read(src,count)));self.ret(dst);return
        if pc==0x687ab5:
            dst,capacity,src=(u32(u,s+4+i*4) for i in range(3));text=bytearray()
            for i in range(capacity-1):
                c=bytes(u.mem_read(src+2*i,2))
                if c==b'\0\0':break
                text.extend(c)
            u.mem_write(dst,bytes(text)+b'\0\0');self.ret(0);return
        if pc==0x448d87:self.events.append(['constructor_byte',self.q.get('ctor',0)])
        if pc==0x664abb:
            self.events.append(['tick_rate'])
            if 'tick' in self.q:self.ret(self.q['tick'])
            return
        if pc==0x5b5a84:
            assert u32(u,s+4)==8;self.events.append(['random']);u.mem_write(u32(u,s+8),b'\x5a'*8);self.ret();return
        if pc==0x437e51:self.ret(0x12345678);return
        if pc==0x5b3c3a:self.events.append(['pause',u32(u,s+4)])
        if pc==0x448dd2 and self.mode=='builder':
            self.options=bytes(u.mem_read(u32(u,s+4),0x1188));self.events.append(['validate'])
            if 'validator' in self.q:self.ret(self.q['validator']);return
        if pc==0x40911b:
            self.events.append(['request']);self.accepted=bool(self.q.get('load',1))
            loaded=bytearray(self.options)
            loaded[0x10:0x18]=bytes.fromhex('8899aabbccddeeff')
            u.mem_write(LOADED,bytes(loaded));self.ret(self.q.get('load',1));return
        if pc==0x449801 and self.mode=='builder':self.events.append(['loaded_options']);self.ret(LOADED);return
        if pc in (0x5c596e,0x5cc0a8):
            self.events.append(['phase_write' if pc==0x5c596e else 'remote_close',u32(u,s+4)]);self.ret(0,4);return
        if pc==0x6a85e9:
            self.events.append(['context',u32(u,s+4),u32(u,s+8),u32(u,s+12)]);self.ret(0,12);return
        if pc in (0x5b06fc,0x5b0718,0x5b0655,0x5b07a0) and self.mode=='builder':
            names={0x5b06fc:'select_map',0x5b0718:'select_scenario',0x5b0655:'select_variant',0x5b07a0:'ui_reason'}
            self.events.append([names[pc],u32(u,s+4)] if pc in (0x5b06fc,0x5b07a0) else [names[pc]])
            self.ret();return
    def call(self,pc,args=(),this=0):
        s=STACK+0x18000;self.u.mem_write(s,struct.pack('<'+'I'*(len(args)+1),STOP,*args))
        self.u.reg_write(UC_X86_REG_ESP,s);self.u.reg_write(UC_X86_REG_ECX,this)
        self.u.emu_start(pc,STOP+1,count=500000)
        assert self.u.reg_read(UC_X86_REG_EIP)==STOP,hex(self.u.reg_read(UC_X86_REG_EIP))
        return self.u.reg_read(UC_X86_REG_EAX)&255
    def builder(self,q):
        self.q=q;self.mode='builder';self.events=[];self.options=None;self.accepted=False
        b=make_bank(q);self.u.mem_write(SESSION,bytes(b));self.u.mem_write(HANDLER,bytes(0x18));w32(self.u,HANDLER+0x10,0xdeadbeef);w32(self.u,HANDLER+0x14,0xfeed1234)
        self.u.mem_write(0x882d40,bytes([q.get('ctor',0)]));self.call(0x5d94a1,(SESSION,),HANDLER)
        assert bytes(self.u.mem_read(SESSION,len(b)))==b
        identity=bytes(self.u.mem_read(HANDLER+0x10,8))
        return dict(kind='builder',input=q,accepted=int(self.accepted),options=self.options.hex() if self.options else None,
                    identity=identity.hex(),events=self.events)
    def validator(self,options):
        self.mode='validator';self.q={};self.events=[];self.u.mem_write(OPTIONS,bytes(options))
        return self.call(0x448dd2,(OPTIONS,))

    def publish(self,options):
        self.mode='publication';self.q={};self.events=[]
        self.u.mem_write(OPTIONS,bytes(options));self.u.mem_write(HANDLER,bytes(0x1270))
        w32(self.u,0x882d3c,HANDLER);w32(self.u,0x8a8280,HANDLER+0x1400)
        self.u.reg_write(UC_X86_REG_EAX,OPTIONS);self.call(0x449e4c)
        self.call(0x449801);address=self.u.reg_read(UC_X86_REG_EAX)
        return dict(pointer=address,options=bytes(self.u.mem_read(address,0x1188)).hex(),seed=u32(self.u,HANDLER+0x1400))

    def presence(self,operation,value):
        self.mode='presence';self.q={};self.events=[]
        if operation=='select_map':entry=0x5b06fc;arg=value
        elif operation=='select_variant':
            entry=0x5b0655;self.u.mem_write(OPTIONS,DEFAULT);w32(self.u,OPTIONS+0x44,value);arg=OPTIONS
        elif operation=='select_scenario':entry=0x5b0718;arg=OPTIONS
        else:entry=0x5b07a0;arg=value
        self.call(entry,(arg&0xffffffff,))
        return dict(kind='presence',operation=operation,value=value,events=self.events)

def main():
    image=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes();assert hashlib.sha256(image).hexdigest()==SHA
    references=[];at=0;needle=struct.pack('<I',0x882d40)
    while (at:=image.find(needle,at))>=0:references.append(hex(BASE+at));at+=1
    assert references==['0x448d88'] and image[0x882d40-BASE]==0
    pe=struct.unpack_from('<I',image,0x3c)[0];sections=struct.unpack_from('<H',image,pe+6)[0];optional=struct.unpack_from('<H',image,pe+20)[0]
    constructor_section=None
    for index in range(sections):
        at=pe+24+optional+index*40;virtual_size,rva,raw_size,raw_offset=struct.unpack_from('<IIII',image,at+8)
        if rva<=0x882d40-BASE<rva+max(virtual_size,raw_size):
            constructor_section=dict(name=image[at:at+8].rstrip(b'\0').decode(),rva=hex(rva),virtual_size=hex(virtual_size),raw_size=hex(raw_size),raw_offset=hex(raw_offset))
            assert image[raw_offset+0x882d40-BASE-rva]==0
    assert constructor_section
    o=Original(image);rows=[]
    cases=[{}]
    for state in range(12):cases.append(dict(state=state))
    for connection in (-1,0,1,2,3):
        for local,host in ((0,0),(1,0),(0,1),(1,1)):cases.append(dict(connection=connection,local=local,host=host))
    for key,values in {'prepared':(0,128),'machines':(0,0x20000),'map_id':(-1,0),'path':('', 'x'*260),
                       'tick':(-1,0,1,30,300,301,32768),'occupied':(0,128),'controller':(3,4,65535),
                       'local_flag':(0,128),'guest':(0,128),'ui_mode':(0,1),'type':(0,1,2,3),
                       'ctor':(0,128),'load':(0,128),'validator':(0,128)}.items():
        for value in values:cases.append({key:value})
    for campaign,count,role in itertools.product((0,0xffff),(1,2,3,0xffffffff),(0,1,2,0xffffffff)):
        cases.append(dict(campaign=campaign,peer_count=count,role=role,validator=1))
    for q in cases:rows.append(o.builder(q))
    baseline=bytes.fromhex(rows[0]['options']);assert o.validator(baseline)==1
    mutations=[(4,1,v) for v in (0,5,6,128,255)]+[(8,2,v) for v in (0,1,300,301,32768,65535)]
    mutations += [(0x290,2,v) for v in (0,15,16,65535)]+[(0x3d4,4,v) for v in (0,1,3,0x10000,0x20000,0xffffffff)]
    mutations += [(0x448,1,v) for v in (0,1,128)]+[(0x44a,2,v) for v in (0,3,4,65535)]
    mutations += [(0x43e,1,v) for v in (0,1,128)]+[(6,1,v) for v in (0,1,128)]
    for at,width,value in mutations:
        options=bytearray(baseline);options[at:at+width]=value.to_bytes(width,'little')
        rows.append(dict(kind='validator',offset=at,width=width,value=value,options=options.hex(),accepted=o.validator(options)))
    extra=[]
    for name,edits in [
        ('zero_xuid',[(0x456,bytes(8))]),
        ('unjoined_machine',[(0x450,bytes.fromhex('abcdef123456'))]),
        ('guest_unjoined_machine',[(0x449,b'\x80'),(0x450,bytes.fromhex('abcdef123456'))]),
        ('guest_zero_machine',[(0x449,b'\x80'),(0x450,bytes(6))]),
        ('duplicate_machine',[(0x3d4,(3).to_bytes(4,'little')),(0x3de,MACHINE)]),
        ('distinct_machine',[(0x3d4,(3).to_bytes(4,'little')),(0x3de,bytes.fromhex('abcdef123456'))]),
        ('duplicate_player',[(0x51c,baseline[0x448:0x51c])]),
        ('distinct_player',[(0x51c,baseline[0x448:0x51c]),(0x52a,bytes.fromhex('abcdef1234567890'))]),
        ('local_no_players',[(6,b'\x80'),(0x448,b'\0')]),
        ('cached_scenario_no_path',[(0x1c,b'\x80'),(0x88,b'\0\0')]),
        ('selected_scenario_no_path',[(0x88,b'\0\0')]),
    ]:
        options=bytearray(baseline)
        for at,raw in edits:options[at:at+len(raw)]=raw
        extra.append(dict(kind='validator',name=name,options=options.hex(),accepted=o.validator(options)))
    rows.extend(extra)
    presence=[]
    for value in list(range(1801))+[-2147483648,-1,0x163dd,0x6c906,0x7fffffff,0xffffffff]:presence.append(o.presence('select_map',value))
    for value in list(range(12))+[-1,-2147483648,0x7fffffff]:presence.append(o.presence('select_variant',value))
    presence.append(o.presence('select_scenario',0))
    for value in (-1,0,4,5,6,7,8,0x7fffffff):presence.append(o.presence('ui_reason',value))
    rows.extend(presence)
    publications=[]
    for seed in (0,1,0x1badb002,0xffffffff):
        options=bytearray(baseline);options[0x18:0x1c]=seed.to_bytes(4,'little');options[0x294]=0x80
        result=o.publish(options);expected=bytearray(options);expected[0x294]=0
        assert result['pointer']==HANDLER+8 and result['options']==expected.hex() and result['seed']==seed
        publications.append(dict(input=options.hex(),**result))
    OUT.mkdir(parents=True,exist_ok=True);fixture=ROOT/'tests/fixtures/vista_session_map_change_original.jsonl'
    fixture.write_text(''.join(json.dumps(r,separators=(',',':'))+'\n' for r in rows))
    d=Cs(CS_ARCH_X86,CS_MODE_32);listing=[]
    for pc in sorted(o.executed):
        if not BASE<=pc<BASE+len(image):continue
        i=next(d.disasm(image[pc-BASE:pc-BASE+15],pc));listing.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}')
    (OUT/'executed-original.asm').write_text('\n'.join(listing)+'\n')
    (OUT/'current-options-publication.json').write_text(json.dumps(dict(source_sha256=SHA,cases=publications),indent=2)+'\n')
    owner_listing=[]
    for first,last in ((0x4086ad,0x4086f8),(0x428707,0x428716),(0x448d6f,0x448dd2),(0x4497ea,0x44980a),(0x449e4c,0x449ebf),(0x45912d,0x45913a),(0x5b3c3a,0x5b3c4e),(0x5b3ca0,0x5b3cd1),(0x664abb,0x664ac1)):
        for i in d.disasm(image[first-BASE:last-BASE],first):owner_listing.append(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}')
    (OUT/'publication-source.asm').write_text('\n'.join(owner_listing)+'\n')
    report=dict(source_sha256=SHA,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
                builder_cases=len(cases),validator_cases=len(mutations)+len(extra),presence_cases=len(presence),publication_cases=len(publications),instruction_visits=o.visits,
                constructor_byte=dict(address='0x882d40',initial_image_value=image[0x882d40-BASE],section=constructor_section,exact_references=references,limit='Image initialization and exact absolute references only; indirect later writes are not excluded.'),limits=__doc__.splitlines())
    (OUT/'original.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
