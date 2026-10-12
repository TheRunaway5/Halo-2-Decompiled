#!/usr/bin/env python3
"""Untouched23FF50 attached-camera and orbital-degeneracy experiments.

Raw occupied pools, current WORLD node0, observer position/up, camera state,
clock and input availability are explicit API records. Node positions and
attachment flags are declared component inputs, not game-produced corpse poses.
OriginalBBCB0/170E20/30BD0/11E7B0/173FA0 execute unchanged. No dependency
return, instruction or camera result is supplied.
"""
import argparse,hashlib,json,math,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,PIN
from oracle_mp_death_camera_lifecycle_1_1 import BASE,SIZE,STACK,SP,STOP,POOL,HEADERS,UNIT,PLAYERS,ROWS,USERS,GAME,CLOCK,OUT
from oracle_mp_scope_camera_1_1 import INITIALIZERS

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True)
    a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');assert hashlib.sha256(image.data).hexdigest()==PIN
    camera=BASE+0x14000;command=BASE+0x14100
    constants=[0x465ad0,0x4650c0,0x465080,0x46507c,0x4457bc,0x465640,0x46563c,0x465044,0x465068,0x4656a4,0x465030,0x46502c,0x45ac88,0x4650d8,0x46504c,0x44aff4,0x44aff8,0x44aff8]
    scalar=lambda p:struct.unpack('<f',image.read(p,4))[0]
    threshold=round(30*scalar(0x4656a4));specs=[]
    for typ in (0,1):
        for ticks in (0,threshold-1,threshold,threshold+1):
            for flags in (0,0x8000):
                for point in ((3.,-2.,4.),(3.00001,-2.,4.),(2.34567,-4.2345,6.75321),(32760.,-.02,-1234.56)):
                    specs.append(dict(type=typ,ticks=ticks,flags=flags,point=point,pitch=.37))
    for pitch in (0.,math.pi/2,-math.pi/2,math.pi):specs.append(dict(type=0,ticks=0,flags=0,point=(2.3,-4.,6.75),pitch=pitch))
    rows=['K '+' '.join(image.read(p,4)[::-1].hex() for p in constants)];cases=[]
    for index,spec in enumerate(specs):
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
        for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
        g.mem_map(BASE,SIZE);g.mem_map(STACK,0x10000);g.mem_map(STOP,0x1000)
        w=lambda p,v:g.mem_write(p,struct.pack('<I',v&0xffffffff))
        h=lambda p,v:g.mem_write(p,struct.pack('<H',v&0xffff))
        b=lambda p,v:g.mem_write(p,bytes([v&0xff]))
        fv=lambda p,v:g.mem_write(p,struct.pack('<f',v))
        vec=lambda p,v:g.mem_write(p,struct.pack('<3f',*v))
        #173FA0 consumes startup-owned FOV/distance limits. Execute their
        #authenticated initializers rather than leaving BSS zero or supplying
        #the resulting globals as successful dependency outputs.
        for register,value in [(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_MXCSR,0x1f80),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_FPSW,0)]:g.reg_write(register,value)
        for entry in INITIALIZERS:
            w(SP,STOP);g.reg_write(R.UC_X86_REG_ESP,SP)
            g.emu_start(entry,STOP,count=100000)
            assert g.reg_read(R.UC_X86_REG_EIP)==STOP
        if index==0:
            #Literal authenticated operands for the native packet-repair owner.
            scope=[0x465ad0,0x4650c4,0x465540,0x4650c4,0x465540,0x46570c,0x465708,
                0x465030,0x47885c,0x4653b8,0x4701c8,0x46502c,0x465044,0x465060,
                0x465068,0x46506c,0x465070,0x465078,0x46507c,0x465080,0x465084,0x465218,0x44b020]
            hx=lambda p:bytes(g.mem_read(p,4))[::-1].hex()
            rows.append('G '+' '.join(hx(p)for p in scope)+' '+image.read(0x465130,8)[::-1].hex()+' '+
                image.read(0x4701d0,12).hex()+' '+image.read(0x4701dc,12).hex()+' '+image.read(0x44b010,24).hex()+' '+
                ' '.join(hx(p)for p in (0x46504c,0x4650e0,0x465090,0x54f214,0x54f218,0x465088)))
            rows.append('H '+' '.join(hx(p)for p in (0x452044,0x5563b0,0x465474))+' 3ca3d70a '+
                ' '.join(hx(p)for p in (0x46508c,0x44b004))+' '+image.read(0x465968,8)[::-1].hex()+' '+
                ' '.join(bytes(g.mem_read(int.from_bytes(g.mem_read(p,4),'little'),12)).hex()for p in (0x46fc48,0x46fc50,0x46fc44)))
        w(0x4e78d0,POOL);w(POOL+0x24,12);w(POOL+0x38,2);w(POOL+0x44,HEADERS)
        h(HEADERS+12,1);b(HEADERS+15,spec['type']);w(HEADERS+20,UNIT);b(UNIT+0xaa,spec['type'])
        h(UNIT+0x116,0x900);vec(UNIT+0x928,spec['point']);h(UNIT+0x348,spec['flags'])
        w(0x4f01f4,PLAYERS);w(PLAYERS+0x44,ROWS);w(ROWS+0x24,0xffffffff);w(ROWS+0x2c,0xffffffff);w(ROWS+0x30,0x10001)
        w(0x4f01f0,USERS);w(USERS+12,0);w(0x4edf18,GAME);w(GAME+8,2)
        w(0x518444,CLOCK);b(CLOCK,1);b(CLOCK+1,1);h(CLOCK+2,30);fv(command+8,1/30)
        h(0x46fb64,0);vec(0x4f125c,(3.,-2.,4.));vec(0x4f125c+0x2c,(0.,0.,1.))
        vec(camera,(3.,-2.,4.));vec(camera+12,(.3,-.2,.1));fv(camera+0x18,.83);fv(camera+0x1c,spec['pitch'])
        fv(camera+0x20,3.5);w(camera+0x24,int.from_bytes(image.read(0x465ad0,4),'little'));fv(camera+0x28,2.)
        w(camera+0x2c,0);w(camera+0x30,0);w(camera+0x34,0x10001);fv(camera+0x38,15.);b(camera+0x3c,spec['ticks']);b(camera+0x3d,0)
        before=bytes(g.mem_read(camera,64));node=bytes(g.mem_read(UNIT+0x928,12));pos=bytes(g.mem_read(0x4f125c,12));up=bytes(g.mem_read(0x4f125c+0x2c,12))
        w(SP,STOP);w(SP+4,camera);w(SP+8,command);w(SP+12,OUT)
        for register,value in [(R.UC_X86_REG_ESP,SP),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_MXCSR,0x1f80),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_FPSW,0)]:g.reg_write(register,value)
        g.emu_start(0x23ff50,STOP,count=100000);assert g.reg_read(R.UC_X86_REG_EIP)==STOP,hex(g.reg_read(R.UC_X86_REG_EIP))
        after=bytes(g.mem_read(camera,64));packet=bytes(g.mem_read(OUT,0xac))
        rows.append('A '+f'{index:08x} 00000000 '+bytes(g.mem_read(command+8,4))[::-1].hex()+' 30 '+pos.hex()+' '+up.hex()+' '+node.hex()+' '+before.hex()+' '+after.hex()+' '+packet.hex()+f' {spec["type"]:x} {spec["flags"]:x}')
        cases.append(dict(**spec,packet_type=int.from_bytes(packet[:4],'little'),packet_sha256=hashlib.sha256(packet).hexdigest()))
    fixture=a.output/'original.tsv';fixture.write_text('\n'.join(rows)+'\n')
    report=dict(tool_sha256=sha(__file__),source_sha256=PIN,fixture_sha256=sha(fixture),cases=cases,original_initializers=[hex(e) for e in INITIALIZERS],boundaries=__doc__,instruction_patches=[],supplied_dependency_returns=[],native_expected_data_used=False,live_gameplay_proof=False)
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(cases=len(cases),attached=sum(c['packet_type']==0x19 for c in cases),fixture_sha256=sha(fixture))))
if __name__=='__main__':main()
