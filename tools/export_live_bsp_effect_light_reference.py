#!/usr/bin/env python3
"""Export actual original inputs and ordered draws for the live BSP light owner.

The reference executes unchanged C15A0/31C80, or TU1.5 C35D0/31C70,
through the first31170/31150 call.
Camera/fog/main-query/light inputs are retained state. Expected range submissions
come from the independently retained original4DAA0 fixture. No original output
unit, mask, part command or material is supplied to the native implementation.
This is a draw-owner comparison, not a complete gameplay-frame comparison.
"""
import argparse, hashlib, json, struct, sys
from pathlib import Path
from unicorn import x86_const as R
from unicorn import UC_HOOK_CODE
from oracle_effect_light_snapshot import Snapshot, u

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'

def words(raw):return list(struct.unpack('<'+'I'*(len(raw)//4),raw))
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--snapshot',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--tu15-xbe',type=Path,
                   help='Authenticate and replay the unchanged Xbox TU1.5 owner addresses')
    p.add_argument('--range-fixture',type=Path,
                   help='Independent actual sorted original range fixture for the same snapshot')
    a=p.parse_args();s=Snapshot(a.snapshot)
    tu15=a.tu15_xbe is not None
    snapshot_sha=hashlib.sha256(s.ram).hexdigest()
    if tu15:
        state=json.loads((a.snapshot/'source-state.json').read_text())
        assert state['reference']=='OG Xbox TU1.5' and state['whole_live_text_matches_original']
        assert state['guest_code_patches']==[] and snapshot_sha==state['ram_sha256']
        assert a.range_fixture is not None
    else:
        assert snapshot_sha==PIN
    xbe=Xbe(a.tu15_xbe if tu15 else ROOT/'assets/disc/default.xbe');executed={}
    if tu15:
        assert hashlib.sha256(xbe.data).hexdigest()=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
    # TU1.5 addresses are retained whole-function instruction correspondences,
    # not a guessed image-wide relocation delta.
    addresses={0xc15a0:0xc35d0,0x31c80:0x31c70,0x4a3d7:0x4a3c7,
        0x547f88:0x556e18,0x4b9e14:0x4c6634,0x4b9f8c:0x4c67ac,
        0x4e3b44:0x4f0414,0x4b9ef0:0x4c6710,0x4b9ffc:0x4c681c,
        0x4ba004:0x4c6824,0x4c0b80:0x4cd3a0,0x2cb30:0x2cb10,
        0x31170:0x31150,0x48597c:0x49219c,0x4e030c:0x4ecbdc}
    def address(old):return addresses[old] if tu15 else old
    g=s.guest();g.mem_map(0x70000000,0x100000)
    stack,stop=0x700fe000,0x700fff00
    def read(at,n):
        regions=list(g.mem_regions())
        for page in range(at&~4095,(at+n+4095)&~4095,4096):
            if not any(b<=page<=e for b,e,_ in regions):
                g.mem_map(page,4096);g.mem_write(page,s.read(page,4096))
        return bytes(g.mem_read(at,n))
    def val(at):return u(read(at,4))
    def queue(header):
        n=struct.unpack('<H',read(header+4,2))[0];assert n<=512
        flags,ids,other,association=[val(header+off) for off in (8,12,16,20)]
        return [[val(ids+i*4),struct.unpack('<H',read(flags+i*2,2))[0],
                 struct.unpack('<H',read(association+i*2,2))[0] if association else 65535,
                 val(other+i*4) if other else 0] for i in range(n)]
    def admission(context):
        return dict(entries=list(read(context+0x1c,512)),
                    clusters=queue(val(context+12)),instances=queue(val(context+24)))
    def call(entry,end):
        g.mem_write(stack,struct.pack('<I',stop));g.reg_write(R.UC_X86_REG_ESP,stack)
        g.reg_write(R.UC_X86_REG_FPCW,0x23f);g.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
        g.emu_start(entry,end,count=30000000)
        assert g.reg_read(R.UC_X86_REG_EIP)==end
    main_query=admission(address(0x547f88))
    camera=words(read(address(0x4b9e14),0xc0))
    fog_datum=val(address(0x4b9f8c));table=val(address(0x4e3b44))
    fog_flags=val(val(table+(fog_datum&65535)*16+8)) if fog_datum!=0xffffffff else 0
    fog=dict(classification=val(address(0x4b9ef0)),planar_datum=fog_datum,
             eye_fraction=val(address(0x4b9ffc)),planar_flags=fog_flags,planar_only=read(address(0x4ba004),1)[0]!=0)
    captured=[];selection=[]
    def receiver_entry(_vm,_pc,_size,_data):
        context=val(address(0x4c0b80))
        captured.append((admission(context),val(context+0x2a78),val(context+0x2a7c)))
    hook=g.hook_add(UC_HOOK_CODE,receiver_entry,begin=address(0x2cb30),end=address(0x2cb30))
    def trace(vm,pc,size,_data):
        if pc not in executed:
            raw=bytes(vm.mem_read(pc,size));assert raw==xbe.read(pc,size)
            executed[pc]=raw
        if pc==address(0x31170):selection.append(val(vm.reg_read(R.UC_X86_REG_ESP)+16))
    g.hook_add(UC_HOOK_CODE,trace)
    call(address(0xc15a0),stop);call(address(0x31c80),address(0x4a3d7));g.hook_del(hook)
    assert len(captured)==1
    receiver,pass_mode,query_mode=captured[0]
    assert len(selection)==1
    selected_fade=selection[0]
    uniforms=words(read(address(0x48597c),144))
    identity=state['light_identity'] if tu15 else 0xe9810015;pool=val(address(0x4e030c))
    light=words(read(val(pool+0x44)+(identity&65535)*0x110,0x110))
    fixture=a.range_fixture or ROOT/'tests/fixtures/effect_light_draw_ranges.original.bin'
    data=words(fixture.read_bytes());assert data[:2]==[0x474e524c,1]
    at=3;expected=[]
    for _ in range(data[2] if tu15 else 13):
        command,unit,vertex_class,kind,section=data[at:at+5];at+=5
        row=data[at:at+8];at+=8
        part,flags,subparts,base=data[at:at+4];at+=4+512
        topology,filtered,returned,n=data[at:at+4];at+=4
        for _ in range(n):
            primitive,first,count=data[at:at+3];at+=3
            indices=data[at:at+count];at+=count
            expected.append(dict(kind=kind,section=section,instance=(row[3]>>18)&2047,
                                 part=part,vertex_class=vertex_class,topology=topology,
                                 first=first,count=count,indices=indices))
    report=dict(boundary=__doc__,reference='OG Xbox TU1.5' if tu15 else 'original base-disc Xbox diagnostic',
                snapshot_sha256=snapshot_sha,main=main_query,receiver=receiver,
                camera=camera,fog=fog,pass_mode=pass_mode,query_mode=query_mode,
                identity=identity,light=light,selected_fade=selected_fade,
                expected=expected,uniforms=uniforms,
                executed_instruction_addresses=len(executed),intercepted_instructions=0,
                original_xbe_sha256=hashlib.sha256(xbe.data).hexdigest(),
                range_fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest())
    if tu15:
        assert uniforms==json.loads((a.snapshot/'uniforms.json').read_text()),'Replay differs from the actual paused TU1.5 light bank'
        report.update(guest_code_patches=[],address_correspondence={hex(k):hex(v) for k,v in addresses.items()})
    a.output.parent.mkdir(parents=True,exist_ok=True)
    with a.output.open('x') as f:json.dump(report,f,indent=2);f.write('\n')
    print('Exported actual original BSP light inputs and',len(expected),'ordered draws')
if __name__=='__main__':main()
