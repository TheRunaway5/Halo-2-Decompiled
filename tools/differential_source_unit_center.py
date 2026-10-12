#!/usr/bin/env python3
"""Compare CAFC0, DF380, DE190, DE080 and11D090 against the base disc XBE.

Root/attachment positions, B8D30 marker posing, CBA50 aim clamp and1697C0
contacts are declared services. Their exact inputs,
ordering and conditional invocation are compared, not their production. The
original observer/basis/normalize/curve/CRT instructions execute unchanged.
"""
import argparse
import bisect
import csv
import hashlib
import json
import math
from pathlib import Path
import random
import struct
import subprocess
import sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BANK,STACK,STOP=0x2100000,0x21fff00,0x3000000
HEADER,POOL,UNIT,PARENT,RELATED,META,DEF,PDEF,SEATS,POINT,FORWARD,UP,OUTPUT=[BANK+0x1000*i for i in range(13)]
UNIT_ID,PARENT_ID,RELATED_ID=0x10001,0x10002,0x10003
def packed(w):return struct.pack('<'+'I'*len(w),*[v&0xffffffff for v in w])
def fw(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def f32(v):return struct.unpack('<f',struct.pack('<f',v))[0]
def base(mode=4,**changes):
    r=dict(mode=mode,parent=0xffffffff,related=0xffffffff,player=0x10007,type=0,flags10a=0,
           state3dc=0,flag3f8=0,flags348=0,seat=-1,scale=1.,crouch=0.,
           forward=[.8,0.,-.6],up=[.6,0.,.8],node=[0.,0.,0.],initialized=1,local=[.4,.1,1.2],
           standing=1.8,crouching=1.2,minimum=.1,maximum=1.4,correction=[1.,1.,1.],minimum_forward=.2,
           root=[0.,0.,0.],attachment=[8.,9.,10.],position=[0.,0.,1.8],
           related_seat=-1,parent_type=1,seat_marker=0x6000100,camera_marker=0x7000200,
           marker_count=1,marker=[.3,.2,1.7],fallback_count=1,fallback=[.4,.3,1.6],
           channels=[[0xffffffff,-1,0,0],[0xffffffff,-1,0,0]],aim=[1.,0.,0.],contacts=[dict(hit=0,fraction=1.,point=[.1,.2,1.7]),dict(hit=0,fraction=1.,point=[.4,.5,1.6])])
    r.update(changes);return r
def encode(r):
    w=[r['mode'],r['parent'],r['related'],r['player'],r['type'],r['flags10a'],r['state3dc'],r['flag3f8'],
       r['flags348'],r['seat'],fw(r['scale']),fw(r['crouch'])]
    for key in ['forward','up','node']:w.extend(map(fw,r[key]))
    w.extend([r['initialized'],*map(fw,r['local']),fw(r['standing']),fw(r['crouching']),fw(r['minimum']),
              fw(r['maximum']),*map(fw,r['correction']),fw(r['minimum_forward'])])
    for key in ['root','attachment','position']:w.extend(map(fw,r[key]))
    w.extend([r['related_seat'],r['parent_type'],r['seat_marker'],r['camera_marker'],r['marker_count'],*map(fw,r['marker']),
              r['fallback_count'],*map(fw,r['fallback'])])
    for c in r['channels']:w.extend(c)
    w.extend(map(fw,r['aim']))
    for c in r['contacts']:w.extend([c['hit'],fw(c['fraction']),*map(fw,c['point'])])
    return [v&0xffffffff for v in w]

class Oracle:
    def __init__(self):
        self.image=Xbe(ROOT/'assets/disc/default.xbe')
        assert hashlib.sha256(self.image.data).hexdigest()==PIN
        self.rows={int(r['va'],16):r for r in csv.DictReader(open(ROOT/'tools/reference/halo2-decompiled/config/functions.csv'))}
        self.addresses=sorted(self.rows)
        self.game={0xcafc0,0xdf380,0xde190,0xde080,0x11d090,0x30bf0,0xe0dc0,0xa74c0,0xa0190,0xa0200,0x17ca10,0x10f820}
        self.g=Uc(UC_ARCH_X86,UC_MODE_32);self.g.mem_map(0x10000,0x600000)
        for section in self.image.sections:self.g.mem_write(section.va,self.image.section_bytes(section))
        self.g.mem_map(BANK,0x100000);self.g.mem_map(STOP,0x1000)
        self.visited=set();self.calls={};self.services={};self.g.hook_add(UC_HOOK_CODE,self.hook)
        self.curve=self.image.read(0x46d148,1024)
    def put(self,a,v):self.g.mem_write(a,packed([v]))
    def word(self,a):return struct.unpack('<I',self.g.mem_read(a,4))[0]
    def vector(self,a,v):self.g.mem_write(a,packed(list(map(fw,v))))
    def vector_words(self,a):return list(struct.unpack('<3I',self.g.mem_read(a,12)))
    def ret(self,pop=0):
        sp=self.g.reg_read(R.UC_X86_REG_ESP)
        self.g.reg_write(R.UC_X86_REG_EIP,self.word(sp));self.g.reg_write(R.UC_X86_REG_ESP,sp+4+pop)
    def hook(self,g,pc,size,user):
        r=self.row;sp=g.reg_read(R.UC_X86_REG_ESP)
        if pc in (0xb9dd0,0xb9ef0,0xb8d30,0xcba50,0x1697c0):
            self.services[f'{pc:08x}']=self.services.get(f'{pc:08x}',0)+1
            if pc==0xb9dd0:
                assert g.reg_read(R.UC_X86_REG_EAX)==UNIT_ID
                self.trace.extend([1,UNIT_ID]);self.vector(g.reg_read(R.UC_X86_REG_ESI),r['root']);self.ret();return
            if pc==0xb9ef0:
                assert g.reg_read(R.UC_X86_REG_EAX)==PARENT_ID
                self.trace.extend([2,PARENT_ID]);self.vector(g.reg_read(R.UC_X86_REG_ECX),r['attachment']);self.ret();return
            if pc==0xb8d30:
                owner,name,maximum=[self.word(sp+n) for n in (4,8,12)]
                assert maximum==1 and not (g.reg_read(R.UC_X86_REG_ECX)&255)
                self.trace.extend([4,owner,name])
                fallback=r['mode']==2 and name==0x4000095
                key='fallback' if fallback else 'marker'
                self.vector(g.reg_read(R.UC_X86_REG_EBX)+0x60,r[key])
                g.reg_write(R.UC_X86_REG_EAX,r[key+'_count']);self.ret(12);return
            if pc==0xcba50:
                assert g.reg_read(R.UC_X86_REG_EAX)==UNIT_ID and self.word(sp+8)==0
                aim=self.word(sp+4);self.trace.extend([6,UNIT_ID,*self.vector_words(aim)])
                self.vector(aim,r['aim']);self.ret(8);return
            if pc==0x1697c0:
                assert self.word(sp+4)==0x4808c2d and [self.word(sp+n) for n in (16,20)]==[0xffffffff]*2
                packet=self.word(sp+24)
                if self.contact==0:assert self.word(packet+0x24)&65535==65535
                self.trace.extend([7,*self.vector_words(self.word(sp+8)),*self.vector_words(self.word(sp+12))])
                c=r['contacts'][self.contact];self.contact+=1
                self.put(packet+4,fw(c['fraction']));self.vector(packet+8,c['point'])
                g.reg_write(R.UC_X86_REG_EAX,c['hit']);self.ret(24);return
        # CAFC0's inline related/parent lookups are explicit native services.
        if pc==0xcb310:self.trace.extend([3,g.reg_read(R.UC_X86_REG_EDI)])
        if pc==0xcb399:self.trace.extend([3,r['parent']])
        owner=self.addresses[bisect.bisect_right(self.addresses,pc)-1];definition=self.rows[owner]
        assert pc<owner+int(definition['size']),f'instruction outside function {pc:08x}'
        assert owner in self.game or owner==0x373b74 or definition['owner']=='xdk:libcmt',f'unexpected original owner {pc:08x}'
        assert bytes(g.mem_read(pc,size))==self.image.read(pc,size),'modified original instruction'
        self.visited.add(pc)
        if pc==owner:self.calls[f'{owner:08x}']=self.calls.get(f'{owner:08x}',0)+1
    def original(self,r):
        self.row=r;self.trace=[];self.contact=0;g=self.g
        g.mem_write(BANK,b'\0'*0x10000)
        self.put(0x4e0300,HEADER);self.put(HEADER+0x44,POOL)
        for index,address in [(1,UNIT),(2,PARENT),(3,RELATED)]:self.put(POOL+12*index+8,address)
        self.put(0x4e3b44,META);self.put(META+16+8,DEF);self.put(META+32+8,PDEF)
        self.put(UNIT,0x10001);self.put(PARENT,0x10002)
        self.put(UNIT+0x14,r['parent']);self.put(UNIT+0x24c,r['related']);self.put(UNIT+0x13c,r['player'])
        g.mem_write(UNIT+0x33e,struct.pack('<h',0x500))
        for i,c in enumerate(r['channels']):
            a=UNIT+0x500+0x9c+0x20*i;self.put(a,c[0]);g.mem_write(a+6,struct.pack('<h',c[1]));g.mem_write(a+0x11,bytes(c[2:4]))
        self.put(UNIT+0xa0,fw(r['scale']));self.put(UNIT+0x2c4,fw(r['crouch']))
        for off,key in [(0xaa,'type'),(0x10a,'flags10a'),(0x3dc,'state3dc'),(0x3f8,'flag3f8'),(0x3c8,'initialized')]:g.mem_write(UNIT+off,bytes([r[key]]))
        g.mem_write(UNIT+0x348,struct.pack('<H',r['flags348']));g.mem_write(UNIT+0x1fc,struct.pack('<h',r['seat']))
        g.mem_write(RELATED+0x1fc,struct.pack('<h',r['related_seat']));g.mem_write(PARENT+0xaa,bytes([r['parent_type']]))
        g.mem_write(UNIT+0x116,struct.pack('<h',0x440));self.vector(UNIT+0x468,r['node'])
        self.vector(UNIT+0x168,r['forward']);self.vector(UNIT+0x7c,r['up']);self.vector(UNIT+0x3cc,r['local'])
        self.put(DEF+0x1cc,SEATS);self.put(PDEF+0x1cc,SEATS)
        self.put(SEATS+8,r['seat_marker']);self.put(SEATS+0x60,r['camera_marker'])
        for off,key in [(0x218,'standing'),(0x21c,'crouching'),(0x224,'minimum'),(0x228,'maximum'),(0x238,'minimum_forward')]:self.put(DEF+off,fw(r[key]))
        self.vector(DEF+0x22c,r['correction']);self.vector(POINT,r['position']);self.vector(FORWARD,r['forward']);self.vector(UP,r['up'])
        g.mem_write(OUTPUT,b'\xcd'*32)
        # Known stack bytes make unwritten packet bytes deterministic, while
        # the declared contact service supplies all operands CAFC0 reads.
        g.mem_write(STACK-0x4000,b'\xcd'*0x4100)
        if r['mode']==0:
            entry=0xde080;registers=[(R.UC_X86_REG_EDX,FORWARD),(R.UC_X86_REG_ESI,UP),(R.UC_X86_REG_ECX,OUTPUT),(R.UC_X86_REG_EDI,OUTPUT+8)];args=[];length=4
        elif r['mode']==1:
            entry=0x11d090;registers=[(R.UC_X86_REG_ECX,FORWARD),(R.UC_X86_REG_EDX,OUTPUT)];args=[];length=3
        elif r['mode']==2:
            entry=0xde190;registers=[(R.UC_X86_REG_EAX,UNIT_ID)];args=[];length=0
        elif r['mode']==3:
            entry=0xdf380;registers=[(R.UC_X86_REG_EAX,UNIT_ID)];args=[POINT,FORWARD,UP];length=3
        elif r['mode']==4:
            entry=0xcafc0;registers=[(R.UC_X86_REG_EAX,OUTPUT),(R.UC_X86_REG_ECX,UNIT_ID)];args=[];length=3
        else:
            entry=0x10f820;registers=[(R.UC_X86_REG_EAX,UNIT_ID)];args=[];length=0
        self.g.mem_write(STACK,packed([STOP,*args]));g.reg_write(R.UC_X86_REG_ESP,STACK)
        for reg,value in registers:g.reg_write(reg,value)
        g.reg_write(R.UC_X86_REG_FPCW,0x027f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        before=bytes(g.mem_read(BANK,0x9000))
        g.emu_start(entry,STOP,count=100000)
        assert g.reg_read(R.UC_X86_REG_EIP)==STOP,'original did not return'
        assert g.reg_read(R.UC_X86_REG_ESP)==STACK+4+4*len(args),'original stack imbalance'
        after=bytearray(g.mem_read(BANK,0x9000))
        if r['mode']==2:
            for off,count in [(0x3c8,1),(0x3cc,12)]:after[UNIT-BANK+off:UNIT-BANK+off+count]=before[UNIT-BANK+off:UNIT-BANK+off+count]
        assert bytes(after)==before,'original mutated declared owner/definition bank'
        if r['mode']==2:out=[bytes(g.mem_read(UNIT+0x3c8,1))[0],*self.vector_words(UNIT+0x3cc)]
        elif r['mode']==5:out=[g.reg_read(R.UC_X86_REG_EAX)&255]
        else:out=list(struct.unpack('<'+'I'*length,g.mem_read(POINT if r['mode']==3 else OUTPUT,4*length)))
        return out+[len(self.trace),*self.trace]

def cases():
    rows=[];rng=random.Random(0xcafc0)
    epsilon=struct.unpack('<f',struct.pack('<I',0x38d1b717))[0]
    below=struct.unpack('<f',struct.pack('<I',0x38d1b716))[0]
    above=struct.unpack('<f',struct.pack('<I',0x38d1b718))[0]
    vectors=[[1.,0.,0.],[0.,0.,1.],[0.,0.,-1.],[0.,0.,0.],[-0.,0.,-0.],
             [epsilon,0.,.5],[below,0.,-.5],[above,0.,-.5],[3.,4.,-1.],[-1e-20,1e-20,1.]]
    for mode in [0,1]:
        for v in vectors:
            for up in [[0.,0.,1.],[.3,.4,.5],[-0.,0.,0.]]:rows.append(base(mode,forward=v,up=up))
    for initialized in [0,1,7]:
        for marker_count in [0,1]:
            for fallback_count in [0,1]:
                for player,suppressed in [(0xffffffff,0),(0x10007,1),(0x10007,0)]:
                    channels=[[0x10002,0,0,suppressed],[0xffffffff,-1,0,0]]
                    rows.append(base(2,initialized=initialized,marker_count=marker_count,fallback_count=fallback_count,player=player,channels=channels))
    for z in [-1.01,-1.,-.99999994,-.5,-.1,-0.,0.,.1,1.]:
        for initialized in [0,1]:
            for lo,hi in [(0.,0.),(1.,0.),(0.,1.57079637),(.1,1.4)]:
                rows.append(base(3,forward=[.8,.2,z],initialized=initialized,minimum=lo,maximum=hi))
    for crouch in [-.5,0.,.5,1.,1.5]:
        for state3dc,flag3f8,flags348 in [(0,0,0),(6,0,0x2000),(6,1,0),(6,1,0x2000)]:
            for first in [0,1]:
                for second in [0,1]:
                    rows.append(base(4,crouch=crouch,state3dc=state3dc,flag3f8=flag3f8,flags348=flags348,
                        contacts=[dict(hit=first,fraction=.3,point=[.1,.2,1.7]),dict(hit=second,fraction=.6,point=[.1,.1,1.5])]))
    for type_,flags in [(1,0),(0,4)]:
        for related,related_seat in [(0xffffffff,-1),(RELATED_ID,-1),(RELATED_ID,0)]:
            for count in [0,1]:rows.append(base(4,type=type_,flags10a=flags,related=related,related_seat=related_seat,marker_count=count))
    for seat in [-1,0]:
        for parent_type in [0,1,2]:
            for name in [0,0x7000200]:
                for count in [0,1]:rows.append(base(4,parent=PARENT_ID,seat=seat,parent_type=parent_type,camera_marker=name,marker_count=count))
    for delta in [-.02,-.01,-.009999999,0.,.009999999,.01,.02]:
        rows.append(base(2,local=[0.,0.,0.],marker=[delta,delta,delta],aim=[1.,0.,0.],initialized=7))
    for selected in [0,1]:
        for datum in [0xffffffff,0,0x80000001]:
            for index in [-1,0,7]:
                for state in [0,1,2,8,9,255]:
                    for flags in [0,1,2,3,255]:
                        channels=[[0xffffffff,-1,0,0],[0xffffffff,-1,0,0]]
                        channels[selected]=[datum,index,state,flags]
                        rows.append(base(5,channels=channels))
    # CAFC0 must discard a correction below30BF0's epsilon, and its rays use
    # the normalized offset rather than the original correction's length.
    for delta in [0.,below,epsilon,above,.01,1.,3.]:
        for first,second in [(0,0),(1,0),(0,1),(1,1)]:
            rows.append(base(4,forward=[1.,0.,-1.],minimum=0.,maximum=.1,minimum_forward=0.,
                local=[delta,0.,1.8],contacts=[dict(hit=first,fraction=.5,point=[-.25,0.,1.8]),
                    dict(hit=second,fraction=.5,point=[delta+.05,0.,1.8])]))
    for _ in range(750):
        mode=rng.randrange(5);v=lambda n=3:[rng.uniform(-3,3) for _ in range(n)]
        rows.append(base(mode,forward=v(),up=v(),node=v(),local=v(),initialized=rng.choice([0,1,7]),
            scale=rng.uniform(.2,3.),crouch=rng.uniform(-.5,1.5),root=v(),position=v(),marker=v(),fallback=v(),
            minimum=rng.uniform(-.2,1.2),maximum=rng.uniform(-.2,1.7),minimum_forward=rng.uniform(-1,1),
            correction=v(),aim=v(),marker_count=rng.randrange(2),fallback_count=rng.randrange(2),
            contacts=[dict(hit=rng.randrange(2),fraction=rng.random(),point=v()) for _ in range(2)]))
    return rows

def retained_cases():
    from differential_source_object_admission_snapshot import Snapshot,uint
    s=Snapshot(ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008')
    digest=hashlib.sha256(s.ram).hexdigest()
    assert digest=='ab7e518eda315ecd55e681d440c988d2814260c292bbbd69972754ac7d32bf55'
    rows=[];owners=[]
    for owner in s.report['objects']:
        raw=s.file('object-'+owner['handle']+'.bin')
        if raw[0xaa]!=0:continue
        assert uint(raw,0x14)==0xffffffff and uint(raw,0x24c)==0xffffffff
        vector=lambda at:list(struct.unpack_from('<3f',raw,at))
        scalar=lambda at:struct.unpack_from('<f',raw,at)[0]
        meta=uint(s.read(0x4e3b44,4));definition=uint(s.read(meta+16*(uint(raw)&65535)+8,4))
        d=list(struct.unpack('<9f',s.read(definition+0x218,36)))
        node=struct.unpack_from('<h',raw,0x116)[0]
        state=struct.unpack_from('<h',raw,0x33e)[0];channels=[]
        for i in [0,1]:
            at=state+0x9c+0x20*i;channels.append([uint(raw,at),struct.unpack_from('<h',raw,at+6)[0],raw[at+0x11],raw[at+0x12]])
        common=dict(parent=uint(raw,0x14),related=uint(raw,0x24c),player=uint(raw,0x13c),channels=channels,
            type=raw[0xaa],flags10a=raw[0x10a],state3dc=raw[0x3dc],flag3f8=raw[0x3f8],
            flags348=struct.unpack_from('<H',raw,0x348)[0],seat=struct.unpack_from('<h',raw,0x1fc)[0],
            scale=scalar(0xa0),crouch=scalar(0x2c4),forward=vector(0x168),up=vector(0x7c),node=vector(node+0x28),
            initialized=raw[0x3c8],local=vector(0x3cc),standing=d[0],crouching=d[1],minimum=d[3],maximum=d[4],
            correction=d[5:8],minimum_forward=d[8],root=vector(0x64))
        crouch=0. if common['state3dc']==6 and common['flag3f8'] and common['flags348']&0x2000 else common['crouch']
        height=f32(f32(f32(1.-crouch)*d[0])+f32(d[1]*crouch))
        position=common['root'].copy();position[2]=f32(f32(height*common['scale'])+position[2]);common['position']=position
        for mode in [0,1,3,4,5]:rows.append(base(mode,**common))
        owners.append(dict(handle=owner['handle'],definition=f'{definition:08x}',node_offset=node,modes=[0,1,3,4,5]))
    assert len(rows)==5
    return rows,dict(snapshot_sha256=digest,owners=owners,boundary='Actual retained unit fields, posed node0 and definition values in relocated banks. Diagnostic kernel calls; not captured player dispatch. No marker/contact result is consumed at this retained forward angle.')

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--binary',type=Path,required=True)
    p.add_argument('--output',type=Path,default=ROOT/'analysis/map-world-features/unit-center-differential.json')
    p.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/retail_source_unit_center.tsv');args=p.parse_args()
    oracle=Oracle();rows=cases();declared=len(rows)
    retained,provenance=retained_cases();rows.extend(retained)
    # Each next input retains the preceding original output. This exercises
    # initialization and repeated +/- .01 smoothing without claiming tick scheduling.
    chain=64;head=[0.,0.,0.];initialized=0
    for step in range(chain):
        row=base(2,initialized=initialized,local=head,marker=[math.sin(step*.2),math.cos(step*.3),1.5+step*.02])
        out=oracle.original(row);initialized=out[0];head=[struct.unpack('<f',struct.pack('<I',w))[0] for w in out[1:4]];rows.append(row)
    inputs=[encode(r) for r in rows];expected=[oracle.original(r) for r in rows]
    curve='curve '+' '.join(f'{w:x}' for w in struct.unpack('<256I',oracle.curve))+'\n'
    payload=curve+''.join(' '.join(f'{w:x}' for w in row)+'\n' for row in inputs)
    process=subprocess.run([str(args.binary.resolve()),'--probe'],input=payload,text=True,capture_output=True)
    if process.returncode:raise RuntimeError(process.stderr)
    actual=[[int(w,16) for w in line.split()] for line in process.stdout.splitlines()]
    mismatches=[i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b]
    report=dict(boundary=__doc__,original_sha256=PIN,cases=len(rows),actual_cases=len(actual),mismatches=len(mismatches),
                examples=[dict(index=i,input=rows[i],expected=expected[i],actual=actual[i]) for i in mismatches[:6]],
                distinct_original_instructions=len(oracle.visited),calls=oracle.calls,declared_services=oracle.services,
                compared_output_words=sum(map(len,expected)),curve_sha256=hashlib.sha256(oracle.curve).hexdigest(),
                declared_cases=declared,retained_snapshot_cases=len(retained),snapshot=provenance,state_chain_cases=chain)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    if not mismatches and len(actual)==len(expected):
        args.fixture.write_text('# Original base-disc unit center scalar branches; service producers are declared, not proved.\n'+curve+
            ''.join(' '.join(f'{w:x}' for w in a)+' | '+' '.join(f'{w:x}' for w in b)+'\n' for a,b in zip(inputs,expected)))
        report.update(fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
            tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),binary_sha256=hashlib.sha256(args.binary.read_bytes()).hexdigest())
    args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
    if mismatches or len(actual)!=len(expected):raise RuntimeError('unit center differential failed')
if __name__=='__main__':main()
