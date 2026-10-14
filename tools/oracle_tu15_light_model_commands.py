#!/usr/bin/env python3
"""Unchanged TU1.5 secondary44AB0 model/BSP part-command projection.

Original C35D0/31C70 supply current units, cache palettes and resources.
The declared command-bank reset matches40E80's secondary reset; whole44AB0
then runs without replaced instructions/callees. Object queues, collector
production and unused45CD0 outputs are not accepted as native live owners.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot,u
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
def words(raw):return list(struct.unpack('<'+'I'*(len(raw)//4),raw))
def pack(values):return struct.pack('<'+'I'*len(values),*values)
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',type=Path,required=True)
    p.add_argument('--xbe',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();s=Snapshot(a.snapshot);x=Xbe(a.xbe)
    assert hashlib.sha256(x.data).hexdigest()==PIN
    state=json.loads((a.snapshot/'source-state.json').read_text());assert not state['guest_code_patches']
    assert hashlib.sha256(s.ram).hexdigest()==state['ram_sha256']
    t=x.section('.text');assert s.read(t.va,t.rsize)==x.read(t.va,t.rsize)
    g=s.guest();g.mem_map(0x70000000,0x100000);sp,stop=0x700fe000,0x700fff00
    points={0x44ab0:0x44ac0,0x449d0:0x449e0,0x45cd0:0x45ce0,0x186e70:0x184000,
        0x460c0:0x460d0,0x41030:0x41040,0x1bf30:0x1bf50,0x40e20:0x40e30,0x3b9c0:0x3b9d0,0x1d690:0x1d6a0}
    trace=[];executed={};tracing=False
    def hook(vm,pc,n,_):
        if not tracing:return
        if pc in points:trace.append(points[pc])
        if pc not in executed:
            raw=bytes(vm.mem_read(pc,n));assert raw==x.read(pc,n);executed[pc]=raw
    # Install before any translation. Adding a code hook after replaying the
    # collectors can leave their cached translated blocks uninstrumented.
    g.hook_add(UC_HOOK_CODE,hook)
    def read(at,n):
        for page in range(at&~4095,(at+n+4095)&~4095,4096):
            if not any(b<=page<=e for b,e,_ in g.mem_regions()):g.mem_map(page,4096);g.mem_write(page,s.read(page,4096))
        return bytes(g.mem_read(at,n))
    def val(at):return u(read(at,4))
    def write(at,raw):read(at,len(raw));g.mem_write(at,raw)
    def call(entry,end=stop,args=()):
        g.mem_write(sp,pack([stop,*args]));g.reg_write(R.UC_X86_REG_ESP,sp)
        g.reg_write(R.UC_X86_REG_FPCW,0x23f);g.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
        g.emu_start(entry,end,count=30000000);assert g.reg_read(R.UC_X86_REG_EIP)==end
    call(0xc35d0);call(0x31c70,0x31177)
    context=val(0x4cd3a0);prefix=val(0x4cd39c);count=val(0x4cd398)
    headers=[0x4ce268+i*24 for i in range(3)]
    for at in headers[1:]:write(at+4,pack([0,0,0]))
    special=0x4d2b18+val(0x515f20)*0x408
    def banks():return [dict(header=words(read(at,24)),storage=[words(read(val(at+20)+i*24,24))for i in range(val(at))])for at in headers]
    before=banks();units=[words(read(0x4c6958+i*32,32))for i in range(count)];models=[]
    for index,unit in enumerate(units[prefix:],prefix):
        if unit[1]==0xffffffff:continue
        if unit[0]&0x400 or not unit[0]&1 or not(unit[6]>>16&255) or unit[3]>>4&31!=31:continue
        table=val(0x4f0414);mode=val(table+(unit[1]&65535)*16+8)
        node_count=val(mode+0x48);assert 0<node_count<=256,(index,hex(unit[1]),node_count)
        g.reg_write(R.UC_X86_REG_ECX,unit[2]);call(0x1d690);palette=g.reg_read(R.UC_X86_REG_EAX);assert palette
        section=unit[3]>>9&511;metadata=val(mode+0x28)+section*92
        flags=struct.unpack('<H',read(mode+4,2))[0];rigid=struct.unpack('<h',read(metadata+0x2c,2))[0]
        regions=[list(struct.unpack('<HH',read(palette+4+i*4,4)))for i in range(val(mode+0x1c))]
        node=0 if rigid==-1 else regions[unit[3]&15][0] if flags&4 else rigid
        assert node>=0
        models.append(dict(unit=index,model=unit[1],section=section,model_flags=flags,rigid_node=rigid,
            mode_node_count=node_count,regions=regions,consumed_center_node=node,
            palette_boundary='Only the prefix through45CD0 consumed center is retained; not full palette production',
            matrices=[words(read(palette+0x44+i*48,48))for i in range(node+1)]))
    source=dict(schema='tu15-light-model-commands-v1',scope=__doc__,original_xbe_sha256=PIN,
        snapshot_sha256=state['ram_sha256'],mode=val(context+0x2a78),flag_mask=0xffffffff,group=31,
        prefix=prefix,count=count,units=units,coverage=list(read(0x4cd3a4,count)),
        override_slots=list(read(0x4cd6f6,count)),overrides=words(read(0x4cda48,192*4)),
        arena_address=context+0x25c,arena=words(read(context+0x25c,2048)),
        bsp_selector=struct.unpack('<h',read(0x474c24,2))[0],visibility=read(val(0x4f9b50),0x4204).hex(),
        player_multiplier=val(0x473644),view_modifier=read(0x4c6839,1)[0],lod_view=words(read(0x492348,28)),
        before=before,models=models,special_before=read(special,0x406).hex())
    tracing=True;call(0x44ab0,args=(source['mode'],0xffffffff,31))
    assert trace and executed,'Original command trace was not instrumented'
    source.update(after=banks(),special_after=read(special,0x406).hex(),calls=trace,
        instruction_count=len(executed),intercepted_services=0,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(source,indent=2)+'\n')
    print('PASS original combined command projection',source['after'][1]['header'][1],'commands,',len(models),'model owners')
if __name__=='__main__':main()
