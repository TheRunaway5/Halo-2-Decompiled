#!/usr/bin/env python3
"""Compare the live read-only query with original normalization/lookup prefixes.

Runs original1CE8C0/1CFCE0 and1DEB60/1DF670 through their authored lookup,
stopping before runtime resource sweeps. No Unit, readiness or IO is supplied.
"""
import argparse, hashlib, json, struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_smg_firing_1_1 import ROOT, PIN, CACHE_PIN, Cache, Xbe

WORK, TAGS, STACK, STOP = 0x1000000, 0x1100000, 0x21fd000, 0x3000000
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--report',type=Path,required=True);a=ap.parse_args()
    path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(path)==PIN
    x=Xbe(path);c=Cache(ROOT/'assets/disc/maps/lockout.map')
    assert hashlib.sha256(c.data).hexdigest()==CACHE_PIN
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
    for section in x.sections:g.mem_write(section.va,x.section_bytes(section))
    g.mem_map(WORK,0x200000);g.mem_map(0x2000000,0x200000);g.mem_map(STOP,0x1000)
    page=c.magic&~4095;g.mem_map(page,(len(c.data)+c.magic-page+4095)&~4095);g.mem_write(c.magic,bytes(c.data))
    def w(p,v):g.mem_write(p,struct.pack('<I',v&0xffffffff))
    def u(p):return int.from_bytes(g.mem_read(p,4),'little')
    w(0x4eb114,TAGS)
    for datum,t in c.tags.items():
        if t['offset']is not None:w(TAGS+(datum&65535)*16+8,t['offset']+c.magic)
    def run(entry,args=(),regs=(),until=STOP):
        w(STACK,STOP)
        for i,v in enumerate(args):w(STACK+4+4*i,v)
        g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_EFLAGS,2)
        g.reg_write(UC_X86_REG_FPCW,0x27f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
        for r,v in regs:g.reg_write(r,v)
        g.emu_start(entry,until,count=1000000)
        assert g.reg_read(UC_X86_REG_EIP)==until
    graphs=[t for t in c.tags.values()if t['group']=='jmad'and t['name']in
        ('objects\\characters\\masterchief\\masterchief','objects\\characters\\elite\\elite')]
    weapons=[t for t in c.tags.values()if t['group']=='weap'and c.u(t['offset']+0x288)!=0xffffffff]
    rows=[];lines=['# Original TU1.1 class mapping and authored action lookup; no runtime resource admission.']
    for graph in graphs:
        datum=int(graph['datum'],16);g.mem_write(WORK,b'\xa5'*0x90)
        run(0x1ce8c0,regs=((UC_X86_REG_EAX,WORK),));w(WORK+0x68,datum)
        for weapon in weapons:
            wc,wt=c.u(weapon['offset']+0x288),c.u(weapon['offset']+0x28c)
            for requested_class in (wc,0x0400054b):
                for action in (0x0400000c,0x06000006,0x08000002,0x08000003,0x0d000021,0x1000006c):
                    run(0x1cfce0,(WORK,action),((UC_X86_REG_EAX,0x06000086),
                        (UC_X86_REG_ECX,requested_class),(UC_X86_REG_EDX,wt),(UC_X86_REG_ESI,WORK+0x100)))
                    mode,normalized_class,name,activity=struct.unpack('<4I',g.mem_read(WORK+0x100,16))
                    for alternate in (0,1):
                        entry,stop=(0x1df670,0x1df72a)if alternate else(0x1deb60,0x1dec1a)
                        run(entry,(graph['offset']+c.magic,WORK+0x200,mode,normalized_class,name,0,0,0),
                            ((UC_X86_REG_EAX,activity),),stop)
                        packed=g.reg_read(UC_X86_REG_ECX)
                        resolved=datum
                        inheritance=struct.unpack('<h',struct.pack('<H',packed&65535))[0]
                        if packed>>16!=65535 and inheritance!=-1:
                            parents=c.block(graph['offset']+0x4c,32);assert 0<=inheritance<len(parents)
                            resolved=c.u(parents[inheritance]+4)
                        values=(datum,0x06000086,requested_class,wt,action,alternate,normalized_class,packed,resolved)
                        lines.append(' '.join(f'{v:08x}'for v in values))
                        rows.append(dict(graph=graph['name'],weapon=weapon['name'],mapped=requested_class!=normalized_class,
                                         valid=packed>>16!=65535,alternate=alternate))
    raw=('\n'.join(lines)+'\n').encode();a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(raw)
    report=dict(schema=1,original_sha256=PIN,cache_sha256=CACHE_PIN,tool_sha256=sha(Path(__file__)),
        fixture_sha256=hashlib.sha256(raw).hexdigest(),queries=len(rows),
        mapped_queries=sum(r['mapped']for r in rows),valid_queries=sum(r['valid']for r in rows),
        scope='Original read-only normalization and base/alternate lookup prefix. Explicit query graph binding on original constructed manager; stops before resource demand. No live Unit, admission, IO, clock or pose proof.')
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report))
if __name__=='__main__':main()
