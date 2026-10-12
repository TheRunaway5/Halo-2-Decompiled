#!/usr/bin/env python3
"""Measure actual contact allocation/getters/refresh/removal, without new input.

Extends the immutable source-created crate/world schedule read-only. Every
source field and instruction still comes from the genuine owning preparation;
the observer neither dispatches another tick nor supplies a contact/return.
"""
import argparse, hashlib, json
from pathlib import Path
import unicorn
from unicorn import x86_const as reg
import oracle_mp_power_module_schedule_1_1 as power
from oracle_mp_power_module_schedule_1_1 import PowerTrace, ROOT, OWNER
from oracle_mp_sandbox_owner_1_1 import digest
from oracle_mp_item_current_producers_1_1 import frozen_write, encoded
from oracle_smg_firing_1_1 import STACK, STOP
from oracle_mp_spawn_biped_physics import RAW_HAVOK, RAW_COMPONENTS
from functions import _trace

OUT=ROOT/'analysis/multiplayer/sandbox-source/contact-lifecycle'
PRIOR=ROOT/'analysis/multiplayer/sandbox-source/power-module-schedule/frozen'
PRIOR_PIN='948723b3077688265b4392ef130c679deb4744fdb41c6558f260520150d9185a'
RECORD=ROOT/'analysis/multiplayer/sandbox-source/contact-record/manifest.json'
RECORD_PIN='53a3ed036f9a412d879cc34ce6fba72c5c6d1aafb7bf56037913c32f4184b626'
ENTRIES={0x1d3df0,0x2df710,0x2df360,0x22e950,
         0x31f4b0,0x31f4d0,0x9d900,0x1d2f10,0x1d32a0,
         0x1d3fc0,0x1d40b0,0x2298b0,0x229a90,0x22b530,0x22b5a0}
ROOTS={0x1d3df0,0x2df710,0x22e950,0x1d2f10,0x1d3fc0,0x1d40b0,0x2298b0}
MATH_SITES={0x1d2fc1,0x1d2ff8,0x1d300e,0x1d3029,0x1d3037,
            0x1d30a8,0x1d30b2,0x1d30b6}

def verify():
    counts={}
    for path,pin in ((PRIOR/'manifest.json',PRIOR_PIN),(RECORD,RECORD_PIN)):
        assert digest(path)==pin,path
        manifest=json.loads(path.read_text())
        for p,h in manifest['files'].items():assert digest(ROOT/p)==h,p
        counts[str(path.relative_to(ROOT))]=len(manifest['files'])
    return counts

class LifecycleTrace(PowerTrace):
    def __init__(self,data):
        self.life_enabled=False;self.life_calls=[];self.life_pending=[]
        self.life_reads=[];self.life_stores=[];self.life_math=[];self.life_blocks=[];self.life_packets=[]
        self.life_states=[];self.life_state_ids={};self.life_order=0
        super().__init__(data)
        self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.life_read)

    def life_state(self):
        # Exact raw OS request extents inherited from the original owning
        # constructors, not inferred physical-object sizes or enlarged arenas.
        r=dict(source=PowerTrace.snapshot(self),
            raw_havok=dict(pointer=f'{RAW_HAVOK:08x}',size=0x106470,
                raw_hex=bytes(self.g.mem_read(RAW_HAVOK,0x106470)).hex()),
            raw_components=dict(pointer=f'{RAW_COMPONENTS:08x}',size=0x1409b,
                raw_hex=bytes(self.g.mem_read(RAW_COMPONENTS,0x1409b)).hex()),
            retained_globals=[dict(address=f'{a:08x}',raw_hex=bytes(self.g.mem_read(a,n)).hex())
                for a,n in ((0x487638,4),(0x480db0,0x18),(0x480d98,12),
                    (0x526248,4),(0x4e78d0,4),(0x52648c,8),(0x518444,4))])
        h=hashlib.sha256(encoded(r)).hexdigest()
        if h not in self.life_state_ids:
            self.life_state_ids[h]=len(self.life_states)
            self.life_states.append(dict(id=len(self.life_states),sha256=h,**r))
        return self.life_state_ids[h]

    def life_return(self,address):
        matches=[x for x in self.life_pending if x[0]==address]
        if not matches:return
        sp=matches[-1][1]
        for ret,entry_sp,c in reversed(matches):
            if sp!=entry_sp:continue
            c.update(return_order=self.life_order,returned_eax=f'{self.g.reg_read(reg.UC_X86_REG_EAX):08x}',
                return_esp_delta=self.g.reg_read(reg.UC_X86_REG_ESP)-sp)
            self.life_order+=1
            if int(c['entry'],16)in ROOTS:c['after_state']=self.life_state()
            if c['entry']in ('0031f4b0','0031f4d0'):
                p=self.g.reg_read(reg.UC_X86_REG_EAX)
                n=32 if c['entry']=='0031f4b0' else 28
                c['emitted_pointer']=f'{p:08x}';c['emitted_full_record_hex']=bytes(self.g.mem_read(p,n)).hex()
            self.life_pending.remove((ret,entry_sp,c))

    def hook(self,g,a,size,user):
        if self.life_enabled:
            self.life_return(a)
            if a in ENTRIES:
                sp=g.reg_read(reg.UC_X86_REG_ESP)
                c=dict(id=len(self.life_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',
                    order=self.life_order,phase=self.active,registers=self.registers(),
                    stack16=[f'{self.u(sp+4+i*4):08x}'for i in range(16)])
                self.life_order+=1
                if a in ROOTS:c['before_state']=self.life_state()
                if a==0x22e950:
                    c['actual_allocator']=f'{g.reg_read(reg.UC_X86_REG_ECX):08x}'
                if a==0x1d3fc0:
                    p=self.u(sp+8)
                    c['callback_packet_pointer']=f'{p:08x}'
                    c['callback_consumed_index_hex']=bytes(g.mem_read(p,2)).hex()
                if a in (0x31f4b0,0x31f4d0):
                    m=g.reg_read(reg.UC_X86_REG_ECX)
                    c.update(manager_pointer=f'{m:08x}',manager120_hex=bytes(g.mem_read(m,0x120)).hex(),banks=[])
                    for off,stride in ((0x30,1),(0x44,32),(0xa0,28)):
                        p,n,capacity=(self.u(m+off+i*4)for i in range(3));cap=capacity&0x7fffffff
                        assert n<=cap<100
                        c['banks'].append(dict(descriptor_offset=off,pointer=f'{p:08x}',count=n,
                            capacity_word=f'{capacity:08x}',stride=stride,
                            raw_capacity_hex=bytes(g.mem_read(p,cap*stride)).hex()if p else None))
                self.life_calls.append(c);self.life_pending.append((self.u(sp),sp,c))
            if a in MATH_SITES:
                sp=g.reg_read(reg.UC_X86_REG_ESP)
                row=dict(instruction=f'{a:08x}',order=self.life_order,phase=self.active,
                    registers=self.registers(),x87_control_word=f'{g.reg_read(reg.UC_X86_REG_FPCW):04x}',
                    x87_status_word=f'{g.reg_read(reg.UC_X86_REG_FPSW):04x}',
                    mxcsr=f'{g.reg_read(reg.UC_X86_REG_MXCSR):08x}',
                    x87_fp0=list(g.reg_read(reg.UC_X86_REG_FP0)),
                    xmm={str(n):f'{g.reg_read(getattr(reg,"UC_X86_REG_XMM"+str(n))):032x}'for n in range(4)},
                    local18_hex=bytes(g.mem_read(sp+0x18,4)).hex(),local1c_hex=bytes(g.mem_read(sp+0x1c,4)).hex())
                self.life_math.append(row);self.life_order+=1
            if a==0x22e980:
                # Internal block reached by22E950's JMP, not a callable entry.
                self.life_blocks.append(dict(instruction=f'{a:08x}',order=self.life_order,
                    phase=self.active,registers=self.registers()))
                self.life_order+=1
            if a==0x1d404d:
                p=g.reg_read(reg.UC_X86_REG_ESI)
                self.life_packets.append(dict(instruction=f'{a:08x}',order=self.life_order,
                    phase=self.active,pointer=f'{p:08x}',consumed_hex=bytes(g.mem_read(p,2)).hex()))
                self.life_order+=1
        super().hook(g,a,size,user)

    def life_read(self,g,access,address,size,value,user):
        if not self.life_enabled or not self.life_pending:return
        if STACK-0x100000<=address<STACK+0x100000:return
        self.life_reads.append(dict(call=self.life_pending[-1][2]['id'],order=self.life_order,
            instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,
            raw_hex=bytes(g.mem_read(address,size)).hex()))
        self.life_order+=1

    def mem_write(self,g,access,address,size,value,user):
        if self.life_enabled and self.life_pending and not STACK-0x100000<=address<STACK+0x100000:
            assert size in (1,2,4,8)
            self.life_stores.append(dict(call=self.life_pending[-1][2]['id'],order=self.life_order,
                instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,
                before_hex=bytes(g.mem_read(address,size)).hex(),
                written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()))
            self.life_order+=1
        super().mem_write(g,access,address,size,value,user)

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'final/measured/report.json');args=ap.parse_args()
    pins=verify();expected=json.loads((PRIOR/'measured/report.json').read_text())
    data=json.loads((OWNER/'raw-host-context.json').read_text());t=power.player.prepare(data,trace_type=LifecycleTrace)
    t.selected_physics_fixup(0xe6f80583);factory=t.factory(0xe6f80583)
    prior=json.loads((OWNER/'report.json').read_text())['case']
    keys=set(factory)-{'calls','ordered_stores'}
    assert {k:factory[k]for k in keys}=={k:prior[k]for k in keys}
    t.sandbox_enabled=False;assert power.player.spawn_and_activate(t,0,activate=False)
    t.power_observe=True;t.life_enabled=True
    for stage in expected['stages']:
        assert t.phase(stage['name'],int(stage['entry'],16),tuple(int(a,16)for a in stage['arguments']))
        t.life_return(STOP)
    assert not t.life_pending,[(f'{a:08x}',sp,c['entry'])for a,sp,c in t.life_pending]
    # Observer-only extension must retain every old stored input, result,
    # write/order and object/player/world phase output exactly.
    for name,value in [('stages',t.power_stages),('calls',t.power_calls),('stores',t.power_stores),
            ('callbacks',t.power_callbacks),('states',t.power_states),('actual_broadphase_calls',t.power_broadphase)]:
        assert value==expected[name],name
    args.output.parent.mkdir(parents=True,exist_ok=True);listings=[]
    for a in sorted(ENTRIES):
        f=_trace(t.code,a,t.starts|ENTRIES|power.ENTRIES);ins=[t.code.at(p)for p in sorted(f.code)]
        for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
        raw=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode()
        path=args.output.parent/f'{a:08x}.asm';frozen_write(path,raw)
        listings.append(dict(entry=f'{a:08x}',file=path.name,sha256=digest(path),rows=len(ins)))
    result=dict(schema=1,oracle_sha256=digest(__file__),prior_manifest_pins={str(PRIOR.relative_to(ROOT))+'/manifest.json':PRIOR_PIN,
        str(RECORD.relative_to(ROOT)):RECORD_PIN},prior_artifacts_verified=pins,
        all_prior_phase_calls_stores_states_byte_identical=True,stages=t.power_stages,
        calls=t.life_calls,reads=t.life_reads,stores=t.life_stores,math=t.life_math,blocks=t.life_blocks,
        removal_packet_consumption=t.life_packets,states=t.life_states,
        original_listings=listings,original_constants=[dict(address=f'{a:08x}',raw_hex=t.image.read(a,4).hex())
            for a in (0x465030,0x46503c,0x4651a0,0x465198)],
        new_supplied_game_or_Havok_fields=[],supplied_source_results=[],instruction_patches=[],
        boundaries=expected['boundaries']+[
          'Complete declared106470/1409B raw source-requested backing extents and whole current source buffers are retained; source allocator and current manager construct their actual packets.',
          'The same unchanged single selected world/object schedule is executed. No new tick, contact, manager vector, damage packet, body/key identity or solver success is supplied.',
          'Native bank allocation/getters/refresh/removal and matching later-update bodies remain a subsequent integration task; this observer never invokes native kernels.'])
    verify();frozen_write(args.output,encoded(result))
    print(json.dumps(dict(report=digest(args.output),calls=len(t.life_calls),reads=len(t.life_reads),
        stores=len(t.life_stores),states=len(t.life_states),math=len(t.life_math))))

if __name__=='__main__':main()
