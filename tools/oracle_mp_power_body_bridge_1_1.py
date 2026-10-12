#!/usr/bin/env python3
"""Observe source-created crate body/entity packets and quantized bounds.

Unchanged original factory/PVS/world/object roots, no contact/pose/body result
or extra clock. The complete frozen power schedule must correlate unchanged.
"""
import argparse,hashlib,json
from pathlib import Path
import unicorn
from unicorn import x86_const as reg
import oracle_mp_power_module_schedule_1_1 as power
from oracle_mp_power_module_schedule_1_1 import PowerTrace,ROOT,OWNER
from oracle_mp_physics_contact_lifecycle_1_1 import verify as previous_verify
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_mp_spawn_biped_physics import RAW_HAVOK,RAW_COMPONENTS
from oracle_smg_firing_1_1 import STACK,STOP
from functions import _trace

OUT=ROOT/'analysis/multiplayer/sandbox-source/power-body-bridge'
LIFE=ROOT/'analysis/multiplayer/sandbox-source/contact-lifecycle/final/manifest.json'
LIFE_PIN='6b5497098b0c31d382f1e5e1e345a921b3dd882318dc1ee1d4f74491d2802189'
ENTRIES={0x1daac0,0x318100,0x312410,0x318af0,0x318a20,0x317ea0,
    0x321320,0x317bf0,0x317d10,0x317890,0x31c1a0,0x31c110,
    0x2e7610,0x2e65a0,0x1d4160,0x318a90,0x2df780,0x2df710,
    0x148a10,0x148a70,0x22e950,0x22eb20,0x2df360}
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def verify():
    previous_verify();assert sha(LIFE)==LIFE_PIN
    m=json.loads(LIFE.read_text())
    for p,h in m['files'].items():assert sha(ROOT/p)==h,p
    return len(m['files'])
class BodyTrace(PowerTrace):
    def __init__(self,data):
        self.bridge_enabled=False;self.bridge_calls=[];self.bridge_pending=[]
        self.bridge_reads=[];self.bridge_stores=[];self.bridge_states=[];self.bridge_ids={};self.bridge_order=0
        super().__init__(data)
        self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.bridge_read)
    def bridge_state(self):
        pool=self.u(0x4e78d0);data=self.u(pool+0x44);capacity=self.u(pool+0x20)
        r=dict(raw_backing=[dict(pointer=f'{p:08x}',size=n,raw_hex=bytes(self.g.mem_read(p,n)).hex())
            for p,n in ((RAW_HAVOK,0x106470),(RAW_COMPONENTS,0x1409b),(pool,0x4c),(data,capacity*12))],
            globals=[dict(address=f'{p:08x}',raw_hex=bytes(self.g.mem_read(p,n)).hex())
                for p,n in ((0x487638,4),(0x480db0,0x18),(0x480d98,12),(0x4e78d0,4),
                    (0x526248,4),(0x526234,4),(0x4eb114,4),(0x518444,4))])
        h=hashlib.sha256(encoded(r)).hexdigest()
        if h not in self.bridge_ids:
            self.bridge_ids[h]=len(self.bridge_states);self.bridge_states.append(dict(id=len(self.bridge_states),sha256=h,**r))
        return self.bridge_ids[h]
    def bridge_return(self,a):
        matches=[x for x in self.bridge_pending if x[0]==a]
        if not matches:return
        sp=matches[-1][1]
        for ret,entry_sp,c in reversed(matches):
            if entry_sp!=sp:continue
            c.update(return_order=self.bridge_order,returned_eax=f'{self.g.reg_read(reg.UC_X86_REG_EAX):08x}',
                return_esp_delta=self.g.reg_read(reg.UC_X86_REG_ESP)-sp,after_state=self.bridge_state())
            self.bridge_order+=1
            if c['entry']=='00318100':
                p=int(c['registers']['ecx'],16);c['body_a0_after_hex']=bytes(self.g.mem_read(p,0xa0)).hex()
            if 'output32_pointer'in c:
                c['output32_after_hex']=bytes(self.g.mem_read(int(c['output32_pointer'],16),32)).hex()
            self.bridge_pending.remove((ret,entry_sp,c))
    def hook(self,g,a,size,user):
        if self.bridge_enabled:
            self.bridge_return(a)
            if a in ENTRIES:
                sp=g.reg_read(reg.UC_X86_REG_ESP)
                c=dict(id=len(self.bridge_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',phase=self.active,
                    order=self.bridge_order,registers=self.registers(),stack16=[f'{self.u(sp+4+n*4):08x}'for n in range(16)],
                    before_state=self.bridge_state(),x87_control_word=f'{g.reg_read(reg.UC_X86_REG_FPCW):04x}',
                    mxcsr=f'{g.reg_read(reg.UC_X86_REG_MXCSR):08x}')
                self.bridge_order+=1
                if a in (0x318100,0x312410,0x318af0,0x318a20):
                    p=self.u(sp+4);c.update(info_pointer=f'{p:08x}',info_b8_hex=bytes(g.mem_read(p,0xb8)).hex())
                if a==0x318100:
                    c['body_a0_before_hex']=bytes(g.mem_read(g.reg_read(reg.UC_X86_REG_ECX),0xa0)).hex()
                if a in (0x2e7610,0x2e65a0):
                    p=self.u(sp+4);c.update(input_pointer=f'{p:08x}',input10_hex=bytes(g.mem_read(p,16)).hex(),
                        output32_pointer=f'{self.u(sp+8):08x}',output32_before_hex=bytes(g.mem_read(self.u(sp+8),32)).hex(),
                        backend_pointer=f'{g.reg_read(reg.UC_X86_REG_ECX):08x}',backend80_hex=bytes(g.mem_read(g.reg_read(reg.UC_X86_REG_ECX),0x80)).hex())
                if a==0x1d4160:
                    p=g.reg_read(reg.UC_X86_REG_ESI);desc=self.u(self.u(p+0xc));n=self.u(desc+4)
                    assert n<100
                    c.update(context_pointer=f'{p:08x}',context14_hex=bytes(g.mem_read(p,0x14)).hex(),
                        descriptor_pointer=f'{desc:08x}',descriptor0c_hex=bytes(g.mem_read(desc,12)).hex(),
                        manager_rows_pointer=f'{self.u(desc):08x}',manager_rows_hex=bytes(g.mem_read(self.u(desc),n*0x30)).hex()if n else '')
                self.bridge_calls.append(c);self.bridge_pending.append((self.u(sp),sp,c))
        super().hook(g,a,size,user)
    def bridge_read(self,g,access,address,size,value,user):
        if not self.bridge_enabled or not self.bridge_pending:return
        # Stack reads are retained too: constructor info/current callback rows
        # are actual source-owned stack packets, not inferred fixture inputs.
        self.bridge_reads.append(dict(call=self.bridge_pending[-1][2]['id'],order=self.bridge_order,
            instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,
            raw_hex=bytes(g.mem_read(address,size)).hex()))
        self.bridge_order+=1
    def mem_write(self,g,access,address,size,value,user):
        if self.bridge_enabled and self.bridge_pending:
            assert size in (1,2,4,8)
            self.bridge_stores.append(dict(call=self.bridge_pending[-1][2]['id'],order=self.bridge_order,
                instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,
                before_hex=bytes(g.mem_read(address,size)).hex(),written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex(),
                source_stack=STACK-0x100000<=address<STACK+0x100000))
            self.bridge_order+=1
        super().mem_write(g,access,address,size,value,user)
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'measured/report.json');args=ap.parse_args()
    count=verify();data=json.loads((OWNER/'raw-host-context.json').read_text());t=power.player.prepare(data,trace_type=BodyTrace)
    t.selected_physics_fixup(0xe6f80583);t.bridge_enabled=True
    factory=t.factory(0xe6f80583);old=json.loads((OWNER/'report.json').read_text())['case'];keys=set(factory)-{'calls','ordered_stores'}
    assert {k:factory[k]for k in keys}=={k:old[k]for k in keys}
    t.sandbox_enabled=False;assert power.player.spawn_and_activate(t,0,activate=False)
    t.power_observe=True;expected=json.loads((power.OUT/'frozen/measured/report.json').read_text())
    for s in expected['stages']:
        assert t.phase(s['name'],int(s['entry'],16),tuple(int(a,16)for a in s['arguments']));t.bridge_return(STOP)
    assert not t.bridge_pending,[(hex(a),c['entry'])for a,_,c in t.bridge_pending]
    for name,value in [('stages',t.power_stages),('calls',t.power_calls),('stores',t.power_stores),('callbacks',t.power_callbacks),
        ('states',t.power_states),('actual_broadphase_calls',t.power_broadphase)]:assert value==expected[name],name
    args.output.parent.mkdir(parents=True,exist_ok=True);listings=[]
    for a in sorted(ENTRIES):
        f=_trace(t.code,a,t.starts|ENTRIES|power.ENTRIES);ins=[t.code.at(p)for p in sorted(f.code)]
        for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
        p=args.output.parent/f'{a:08x}.asm';frozen_write(p,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode())
        listings.append(dict(entry=f'{a:08x}',file=p.name,sha256=sha(p),rows=len(ins)))
    r=dict(schema=1,oracle_sha256=sha(__file__),prior_lifecycle_manifest_sha256=LIFE_PIN,prior_lifecycle_verified_files=count,
        source_factory_outputs_and_full_prior_schedule_identical=True,calls=t.bridge_calls,reads=t.bridge_reads,stores=t.bridge_stores,
        states=t.bridge_states,instruction_listings=listings,stages=t.power_stages,
        new_source_fields_or_results_supplied=[],instruction_patches=[],boundaries=expected['boundaries']+[
            'Whole genuine constructor info/callback contexts, actual caller stack reads/writes and current quantized backend output are observed. No native body, solver, pose, clock, contact or allocator success is supplied.',
            'Current-bounds diagnostic retains the same separately declared original BEDE0 root; no natural repeated-frame scheduling claim is added.'])
    verify();frozen_write(args.output,encoded(r));print(json.dumps(dict(report=sha(args.output),calls=len(t.bridge_calls),states=len(t.bridge_states),reads=len(t.bridge_reads),stores=len(t.bridge_stores))))
if __name__=='__main__':main()
