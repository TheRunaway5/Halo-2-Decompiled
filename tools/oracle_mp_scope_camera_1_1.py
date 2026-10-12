#!/usr/bin/env python3
"""Independent source normal-camera FOV and observer-channel research.

The inherited local view/control binding is a declared native adapter input,
not a source lobby/session producer. Source constructors own all runtime FOV
ranges; genuine camera/world and observer instructions execute unchanged.
"""
import argparse
import csv
import hashlib
import json
import struct
from pathlib import Path

from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX,
    UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_ESI, UC_X86_REG_EIP,
    UC_X86_REG_ESP, UC_X86_REG_XMM0)
from oracle_mp_weapon_attachment_1_1 import AttachmentTrace
from oracle_mp_inventory_controls_1_1 import CONTROL
from oracle_mp_inventory_1_1 import ROOT, PIN, CACHE_PIN, UNIT, UI, SCRATCH, STOP
from oracle_smg_firing_1_1 import _trace

DEPENDENCY_PIN = '9895480d4d864d67ee92307d627c04a005e4ffb8f06761be68c9de0d7274563c'
OBSERVER = 0x4f11a4
PACKET, COMMAND, PERSISTENT = SCRATCH+0x4000, SCRATCH+0x3000, SCRATCH+0x3100
INITIALIZERS = (0x37dc60, 0x37dc80, 0x37dca0, 0x37dcc0, 0x37e890, 0x37e8b0)
ENTRIES = (0x188b60, 0x23e210, 0x23de70, 0xccc60, 0xcc320,
    0x173fa0, 0x173f20, 0x173e60, 0x173ea0, 0x173ee0,
    0x170f30, 0x170e40, 0x170c10, 0x170ff0, 0x171910, 0x172630,
    0x172680, 0x171990, 0x171c70, 0x171e70, 0x1720b0, 0x1727f0,
    0x156c00, 0x170d00, 0x156ad0, 0x156eb0, 0x187610,
    0xc9be0, 0xc9cc0, 0xc9920, 0x172a50, 0x173810)

def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def h(value): return f'{value & 0xffffffff:08x}'

class ScopeCameraTrace(AttachmentTrace):
    def __init__(self):
        self.camera_ready = False
        self.camera_calls, self.camera_returns, self.camera_stores = [], [], []
        super().__init__()
        self.camera_listings = {}
        self.camera_cleanup = {}
        for entry in (*ENTRIES, *INITIALIZERS):
            f = _trace(self.code, entry, self.starts | set(ENTRIES) | set(INITIALIZERS))
            ins = [self.code.at(p) for p in sorted(f.code)]
            self.camera_listings[h(entry)] = ''.join(
                f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins)
            rets = {int(i.op_str, 0) if i.op_str else 0 for i in ins if i.mnemonic.startswith('ret')}
            if len(rets) == 1: self.camera_cleanup[entry] = next(iter(rets))
        self.camera_ready = True

    def camera_state(self):
        def raw(p, n): return bytes(self.g.mem_read(p, n)).hex()
        return dict(packet_hex=raw(PACKET, 0xac), observer_hex=raw(OBSERVER, 0x358),
            current_fov=h(self.u(OBSERVER+0x130)), target_fov=h(self.u(OBSERVER+0x30)),
            fov_velocity=h(self.u(OBSERVER+0x1a4)), fov_acceleration=h(self.u(OBSERVER+0x1d8)),
            fov_remaining=h(self.u(OBSERVER+0xac)), fov_coefficients=[h(self.u(OBSERVER+p))
                for p in (0x20c, 0x240, 0x274, 0x2a8, 0x2dc, 0x310)],
            persistent_fov=h(self.u(PERSISTENT)), local_selector=int.from_bytes(
                self.g.mem_read(CONTROL+0x42, 2), 'little', signed=True),
            packet_fov=h(self.u(PACKET+0x28)), packet_group4_flag=self.u(PACKET+0x90,1),
            packet_group4_seconds=h(self.u(PACKET+0xa4)),
            render_packet_hex=raw(OBSERVER+0xb8,0x50),
            render_horizontal_fov=h(self.u(OBSERVER+0xf0)),
            render_vertical_fov=h(self.u(OBSERVER+0x104)),
            dt=h(self.u(0x4f11a0)), runtime_fov_globals={h(p):h(self.u(p))
                for p in (0x5563a4,0x5563a8,0x5563ac,0x5563b0,0x565fe4,0x565fe8)})

    def finish_camera_returns(self, address):
        sp = self.g.reg_read(UC_X86_REG_ESP)
        for ret, end_sp, row in self.camera_returns[:]:
            if ret == address and sp == end_sp:
                row['return_eax'] = h(self.g.reg_read(UC_X86_REG_EAX))
                row['return_xmm0_low'] = h(self.g.reg_read(UC_X86_REG_XMM0))
                self.camera_returns.remove((ret,end_sp,row))

    def hook(self, g, a, size, user):
        if self.camera_ready:
            self.finish_camera_returns(a)
            if a in self.camera_cleanup:
                sp = g.reg_read(UC_X86_REG_ESP)
                row = dict(phase=self.active, entry=h(a), registers=self.registers(),
                    stack_arguments=[h(self.u(sp+4+n*4)) for n in range(3)],
                    return_site=h(self.u(sp)), original_ret_cleanup=self.camera_cleanup[a])
                self.camera_calls.append(row)
                self.camera_returns.append((self.u(sp),sp+4+self.camera_cleanup[a],row))
        super().hook(g,a,size,user)

    def mem_write(self,g,access,a,size,value,user):
        if self.camera_ready and any(p <= a and a+size <= p+n for p,n in
                ((OBSERVER,0x358),(PACKET,0xac),(PERSISTENT,4),
                 (0x5563a4,16),(0x565fe4,8),(0x4f11a0,4))):
            self.camera_stores.append(dict(phase=self.active, instruction=h(g.reg_read(UC_X86_REG_EIP)),
                address=h(a), size=size, before_hex=bytes(g.mem_read(a,size)).hex(),
                after_hex=(value & ((1<<(8*size))-1)).to_bytes(size,'little').hex()))
        super().mem_write(g,access,a,size,value,user)

    def measure(self,name,entry,argv=(),regs=(),boundary=None):
        self.active=name
        before=self.camera_state(); calls=len(self.camera_calls); stores=len(self.camera_stores); events=len(self.events)
        self.run(entry,argv,regs)
        self.finish_camera_returns(STOP)
        return dict(name=name,entry=h(entry),supplied_boundary=boundary,before=before,
            after=self.camera_state(),return_eax=h(self.g.reg_read(UC_X86_REG_EAX)),
            return_xmm0_low=h(self.g.reg_read(UC_X86_REG_XMM0)),
            calls=self.camera_calls[calls:],ordered_stores=self.camera_stores[stores:],
            inherited_service_events=self.events[events:])

def generate(out):
    assert sha(ROOT/'tools/oracle_mp_weapon_attachment_1_1.py') == DEPENDENCY_PIN
    t=ScopeCameraTrace(); initial=t.camera_state(); stages=[]
    for p in INITIALIZERS: stages.append(t.measure('source_static_initializer_'+h(p),p))
    stages.append(t.measure('source_observer_constructor',0x170f30,regs=((UC_X86_REG_ESI,OBSERVER),)))
    unit_definition=t.u(t.u(0x4eb114)+(t.u(UNIT)&65535)*16+8)
    # Complete packet memory is the caller-owned ABI buffer. The real caller
    # 156C00 clears it before invoking23E210; this isolated caller boundary is
    # retained verbatim, not claimed as an executed local-session producer.
    t.w(COMMAND,0); t.f(PERSISTENT,0)
    authored=dict(unit_definition=h(unit_definition), unit_fov=h(t.u(unit_definition+0xcc)),
        immutable_constants={h(p):h(t.u(p)) for p in (0x465030,0x46570c,0x465708,
            0x47885c,0x4653b8,0x4701c8,0x46502c,0x465044,0x465060,
            0x465068,0x46506c,0x465070,0x465078,0x46507c,0x465080,
            0x465084,0x465218)},
        x87_reciprocal_numerator=bytes(t.g.mem_read(0x465130,8)).hex(),
        field_counts=bytes(t.g.mem_read(0x4701d0,12)).hex(),
        motion_counts=bytes(t.g.mem_read(0x4701dc,12)).hex(),
        acceleration_caps=bytes(t.g.mem_read(0x44b010,24)).hex(),
        initializers=[dict(entry=h(p),pointer_table_address=h(next(s.va+sbytes.index(struct.pack('<I',p))
            for s in t.image.sections for sbytes in [t.image.section_bytes(s)]
            if s.name=='.data' and struct.pack('<I',p) in sbytes))) for p in INITIALIZERS])
    for selector in (-1,0,-1,0):
        t.sh(CONTROL+0x42,selector)
        stages.append(t.measure('source_fov_selector_'+str(selector),0x188b60,
            regs=((UC_X86_REG_EAX,0),),boundary={'native_local_view':0,'native_local_selector':selector}))
    rows=[]
    # Initial unzoomed adoption, binocular in/out, and an interrupted target.
    for frame,selector in enumerate([-1]*2+[0]*8+[-1]*8+[0]*2+[-1]*8):
        t.sh(CONTROL+0x42,selector);t.g.mem_write(PACKET,bytes(0xac))
        packet=t.measure(f'frame_{frame}_packet',0x23e210,(PERSISTENT,COMMAND,PACKET),
            boundary={'native_local_view':0,'native_local_selector':selector,
                'caller_zero_packet_bytes':0xac,'persistent_fov_carried':True})
        register=t.measure(f'frame_{frame}_register',0x170c10,
            regs=((UC_X86_REG_EAX,PACKET),(UC_X86_REG_ECX,0)))
        adopt=t.measure(f'frame_{frame}_adopt',0x170ff0,(0,))
        # The normal170D00 caller stores4701C8*dt then invokes171910 withEDI
        # view. Here the exact source writer fragment is executed, preserving
        # a declared host frame-dt input, without invoking unresolved render.
        dt=struct.unpack('<I',struct.pack('<f',1/30))[0]
        t.active=f'frame_{frame}_source_dt_writer'
        t.w(0x21fd004,dt)
        t.g.reg_write(UC_X86_REG_ESP,0x21fd000)
        t.g.emu_start(0x170d00,0x170d1c,count=20)
        assert t.g.reg_read(UC_X86_REG_EIP)==0x170d1c
        motion=t.measure(f'frame_{frame}_motion',0x171910,regs=((UC_X86_REG_EDI,0),),
            boundary={'host_frame_dt_bits':h(dt),'source_dt_writer':'00170d00..00170d1b'})
        render=t.measure(f'frame_{frame}_render_packet',0x172a50,(0,))
        rows.append(dict(frame=frame,selector=selector,packet=packet,registration=register,
            adoption=adopt,motion=motion,render_packet=render))
    out.mkdir(parents=True,exist_ok=True); (out/'instructions').mkdir(exist_ok=True)
    for p,b in t.camera_listings.items(): (out/'instructions'/f'{p}.asm').write_text(b)
    report=dict(tool_sha256=sha(__file__),attachment_dependency_sha256=DEPENDENCY_PIN,
        original_xbe_sha256=PIN,cache_sha256=CACHE_PIN,initial=initial,authored=authored,
        stages=stages,frames=rows,inherited_services=[{'entry':h(p),'contract':v}
            for p,v in t.services.items()],
        scope='Unchanged original camera packet/FOV, source static range initializers and observer construction/adoption/motion. Native local-view binding and selectors, original-clock frame dt and the zeroed caller packet are explicit adapter inputs. No source lobby/local-session owner, live jump/crouch, sensitivity-profile polling, scope overlay, rendering or hardware execution claim.',
        native_gameplay_update_applied=False)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'frames':len(rows),'report_sha256':sha(out/'report.json')}),flush=True)
    t.cache.data.close();t.cache.stream.close()

def main():
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True)
    generate(p.parse_args().output)
if __name__=='__main__':main()
