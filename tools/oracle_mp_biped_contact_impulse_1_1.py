#!/usr/bin/env python3
"""Execute untouched E27E1..E2BD2 with declared recent-contact inputs.

Body, motion, component, authored node and pose bytes are retained outputs of
the frozen natural admission experiment. Contact node/normal, RNG state and FP
mode are explicit API inputs to this isolated fragment. This does not assert
that a gameplay contact produced these inputs. No instruction or dependency
return is patched; original185700/259B0/30BD0/1D4A00 execute normally.
"""
import argparse, hashlib, json, struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, Cache, PIN, CACHE_PIN
from export_mp_ragdoll_owners import spans

REPORT = ROOT/'analysis/multiplayer/integration/death-runtime-20261011/natural-admission-v2/report.json'
REPORT_PIN = '4907db719b968d8f1c15f77fd456b78f697a0a0a5cab1064f83597b82ff3eec4'
FRAME, SEED = 0x21fcdd4, 0x10fe000
def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True)
    a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    assert sha(REPORT)==REPORT_PIN
    prior=json.loads(REPORT.read_text());assert prior['source_sha256']==PIN and prior['cache_sha256']==CACHE_PIN
    root=max((c for c in prior['rebuild_calls'] if c['entry']=='000e2520'),key=lambda c:c['return_order']-c['order'])
    # Reconstruct source state at the end of this one actual admission call.
    state={}
    events=sorted([(q['order'],'R',q) for q in prior['rebuild_reads']]+[(q['order'],'W',q) for q in prior['rebuild_stores']])
    for order,kind,q in events:
        if not root['order']<order<root['return_order']:continue
        p=int(q['address'],16);raw=bytes.fromhex(q['raw_hex'] if kind=='R' else q['before_hex'])
        for i,v in enumerate(raw):
            if p+i in state:assert state[p+i]==v
            else:state[p+i]=v
        if kind=='W':state.update({p+i:v for i,v in enumerate(bytes.fromhex(q['written_hex']))})
    image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(image.data).hexdigest()==PIN
    literal={s.va+i:v for s in image.sections for i,v in enumerate(image.section_bytes(s))}
    body_datum=int(root['stack16'][0],16)
    u=lambda p:int.from_bytes(bytes(state.get(p+i,literal.get(p+i,0)) for i in range(4)),'little')
    obj=u(u(u(0x4e78d0)+0x44)+(body_datum&0xffff)*12+8)
    component=u(u(0x526248)+0x44)+(u(obj+0xb4)&0xffff)*0xa0
    assert u(component+0x74)==10
    # This immutable authored scalar was not read on the prior no-contact
    # branch. Obtain its literal bytes from the original Lockout bipd tag.
    cache_path=ROOT/'assets/disc/maps/lockout.map';assert sha(cache_path)==CACHE_PIN
    cache=Cache(cache_path);definition=u(u(0x4eb114)+(u(obj)&0xffff)*16+8)
    authored=cache.tags[u(obj)];raw=bytes(cache.data[authored['offset']+0x258:authored['offset']+0x25c])
    state.update({definition+0x258+i:v for i,v in enumerate(raw)})
    authored_input=dict(tag=u(obj),file_offset=authored['offset']+0x258,address=definition+0x258,raw=raw.hex())
    cache.data.close();cache.stream.close()
    specs=[(f'node_{node}',node,(1.,.375,-.25),0x12345678,0) for node in range(-1,34)]
    specs += [(f'normal_{i}_round_{mode}',8,normal,seed,mode)
        for mode in range(4) for i,(normal,seed) in enumerate([
            ((0.,0.,0.),0),((1.,0.,0.),1),((0.,1.,0.),0xffffffff),
            ((0.,0.,1.),0xabcdef01),((-.25,.75,-1.),0x83742615)])]
    cases=[];rows=[]
    for name,node,normal,seed,mode in specs:
        g=Uc(UC_ARCH_X86,UC_MODE_32)
        backing=dict(literal);backing.update(state)
        # The complete caller stack is an explicitly declared ABI scratch bank.
        backing.update({0x21f0000+i:0 for i in range(0x10000)})
        def put(p,raw):backing.update({p+i:v for i,v in enumerate(raw)})
        def w(p,x):put(p,struct.pack('<I',x&0xffffffff))
        w(0x4ee9d8,SEED);w(SEED,seed);w(FRAME+0x18,node)
        put(FRAME+0x38,struct.pack('<3f',*normal))
        pages=sorted({p&~0xfff for p in backing})
        for p in pages:g.mem_map(p,0x1000)
        for p,raw in spans(backing):g.mem_write(p,raw)
        fp=(0x27f+(mode<<10),0x1f80+(mode<<13))
        for register,value in [(R.UC_X86_REG_ESP,FRAME),(R.UC_X86_REG_EBX,obj),
            (R.UC_X86_REG_EBP,body_datum),(R.UC_X86_REG_EDI,component),
            (R.UC_X86_REG_FPCW,fp[0]),(R.UC_X86_REG_MXCSR,fp[1]),
            (R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_FPSW,0)]:g.reg_write(register,value)
        initial={};writes=[];calls=[];last=0;stop=None
        def touch(p,n):
            for i in range(n):
                assert p+i in backing,(name,f'unprovided read {p+i:08x}',f'{last:08x}')
                if p+i not in initial:initial[p+i]=backing[p+i]
        def on_read(_g,_access,p,n,_x,_data):touch(p,n)
        def on_write(_g,_access,p,n,x,_data):
            touch(p,n)
            raw=(x&((1<<(n*8))-1)).to_bytes(n,'little')
            if not 0x21f0000<=p<0x2200000:
                writes.append(dict(instruction=last,address=p,before=bytes(g.mem_read(p,n)).hex(),written=raw.hex()))
        def on_code(_g,p,_size,_data):
            nonlocal last,stop
            last=p
            if p in (0xe2bd2,0xe2be7,0xe2c3c):stop=p;g.emu_stop();return
            if p==0x1d4a00:
                v=g.reg_read(R.UC_X86_REG_EDX);slot=g.reg_read(R.UC_X86_REG_ECX)
                calls.append(dict(component=g.reg_read(R.UC_X86_REG_ESI),slot=slot,vector=v,
                    words=list(struct.unpack('<3I',g.mem_read(v,12))),entry_esp=g.reg_read(R.UC_X86_REG_ESP)))
        g.hook_add(UC_HOOK_MEM_READ,on_read);g.hook_add(UC_HOOK_MEM_WRITE,on_write);g.hook_add(UC_HOOK_CODE,on_code)
        g.emu_start(0xe27e1,0xe2bd2,count=1000000)
        if g.reg_read(R.UC_X86_REG_EIP)==0xe2bd2:stop=0xe2bd2
        assert stop in (0xe2bd2,0xe2be7,0xe2c3c),(name,hex(g.reg_read(R.UC_X86_REG_EIP)))
        # Native helpers own scratch storage too, but only retained source
        # state and the actual delta-owner requests form this comparison.
        output={p+i:v for s in writes for p,raw in [(s['address'],bytes.fromhex(s['written']))] for i,v in enumerate(raw)}
        rows.append(f'C {name} {body_datum:08x} {node&0xffffffff:08x} {FRAME:08x} {fp[0]:04x} {fp[1]:08x} {int(stop==0xe2bd2):x}')
        # Provide the complete real caller scratch allocation, not chosen effects.
        for p in range(FRAME,FRAME+0x5c):initial.setdefault(p,backing[p])
        for p,raw in spans(initial):rows.append(f'M {p:08x} {raw.hex()}')
        for p,raw in spans(output):rows.append(f'A {p:08x} {raw.hex()}')
        for s in writes:rows.append(f"W {s['instruction']:08x} {s['address']:08x} {s['before']} {s['written']}")
        for c in calls:rows.append('D '+' '.join(f'{x:08x}' for x in (c['component'],c['slot'],c['vector'],c['entry_esp'],*c['words'])))
        cases.append(dict(name=name,contact_node=node,contact_normal=normal,initial_rng=seed,fp=fp,
            stop=f'{stop:08x}',stores=len(writes),delta_calls=calls,input_bytes=len(initial),retained_bytes=len(output)))
        print(name,f'{stop:08x}',len(writes),len(calls),flush=True)
    fixture=a.output/'original.tsv';fixture.write_text('\n'.join(rows)+'\n')
    report=dict(tool_sha256=sha(__file__),source_sha256=PIN,cache_sha256=CACHE_PIN,
        prior_report_sha256=REPORT_PIN,original_admission_call=root['id'],authored_scalar=authored_input,fixture_sha256=sha(fixture),
        cases=cases,boundaries=__doc__,instruction_patches=[],supplied_dependency_returns=[],native_expected_data_used=False,live_gameplay_proof=False)
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(cases=len(cases),stores=sum(c['stores'] for c in cases),fixture_sha256=sha(fixture))))
if __name__=='__main__':main()
