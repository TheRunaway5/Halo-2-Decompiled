#!/usr/bin/env python3
"""Compare original retained construction CPU fragments with genuine whole calls.

The owned original constructor/reference instructions execute unchanged. Xbox
state access/release and saved-setting implementation are explicit boundaries.
The genuine capture does not observe the synchronized reference acquisition
flag; both declared0/1 inputs are measured. Saved-setting CPU input is the
actual captured value or a labeled declared service input. Whole retained
outputs must match all five genuine captures before declared controls run.
This does not execute original allocation, hardware init, kernel locks or DSP.
"""
import argparse
import atexit
import ctypes
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from functions import _Code
from oracle_sound_sdk_commit import FragmentCpu

sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
u32=lambda b,i=0:struct.unpack_from('<I',b,i)[0]
put=lambda b,i,v:struct.pack_into('<I',b,i,v&0xffffffff)
NAMES=('object','hardware_device','voice_data','device_data','source','parameters','voice_object','device_object','tail','replacement_source','payload')
ALIASES={name:0x1001000+i*0x1000 for i,name in enumerate(NAMES)}
ALIASES.update(singleton=0x387198,speaker_override=0x386b08,speaker_config=0x386b04,busy=0x386b0c)
SPEC={'device_object':(0x37a132,0),'device_data':(0x37cc6a,0),
      'buffer_object':(0x37c1d7,1),'voice_object':(0x382b22,2),
      'buffer_binding':(0x37be6c,3),'pair':(0x37a110,1),
      'reference':(0x37b16f,1),'add_reference':(0x379d0c,1),'synchronized_reference':(0x37a14f,1)}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    image_path=ROOT/'assets/disc/default.xbe';assert sha(image_path)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
    image=Xbe(image_path);code=_Code(image,'DSOUND')
    captures={p.parent.name:json.loads(p.read_text())for p in args.capture.glob('*/report.json')}
    assert set(captures)==set(SPEC)-{'pair','reference','add_reference','synchronized_reference'}
    for r in captures.values():
        assert r['original_whole_constructor_returned_normally']
        assert not any(r[k]for k in ('instruction_patches','guest_memory_writes','guest_register_writes','supplied_callee_returns'))
    layout=args.output/'user-layout.c';layout.write_text('#include <stddef.h>\n#include <sys/user.h>\n_Static_assert(offsetof(struct user,u_debugreg)==848,"Unsupported ptrace ABI");\nint main(void){return 0;}\n')
    subprocess.run(['cc',str(layout),'-o',str(args.output/'user-layout')],check=True)
    subprocess.run([str(args.output/'user-layout')],check=True)
    host=FragmentCpu(image,args.output/'original32.elf',0x23f,0x1fa0);atexit.register(host.close)
    host.trace(6,host.debug_offset+7*8,0)
    stack,stop=0x100f000,0x3000000;rows=[];visited=set()
    def normalize(r,field):
        original=[(int(region['address'],16),region['size'],ALIASES[region['name']])for region in r['regions']]
        def alias(v):
            for base,size,target in original:
                if base<=v<base+size:return target+v-base
            return v
        state={region['name']:bytearray.fromhex(region[field])for region in r['regions']}
        # Retained address relocation is declared ABI backing. The same
        # transformation applies before/after and to all pointer arguments.
        for raw in state.values():
            for at in range(0,len(raw)-3,4):put(raw,at,alias(u32(raw,at)))
        return state,alias

    def measure(label,kind,state,arguments,owner=None,acquired=0,saved=0,actual=None,actions=()):
        for name,raw in state.items():
            if name in ('singleton','speaker_override','speaker_config','busy'):
                # Mutable data inside the SDK section, not code. Only these
                # four source-owned DWORD inputs are admitted here.
                assert len(raw)==4
                assert os.pwrite(host.memory_fd,raw,ALIASES[name])==4
            else:host.write(ALIASES[name],raw)
        host.write(stack-0x1000,bytes([0x5b])*0x1100)
        entry,count=SPEC[kind];assert len(arguments)==count
        host.write(stack,struct.pack('<'+'I'*(count+1),stop,*arguments))
        host.continuing=False;pc=entry;inputs={'rcx':owner or ALIASES['object']};stores=[];releases=0;settings=0;acquisitions=0
        # Stop after an observed store only for declared external mutations.
        action_sites={pc+code.at(pc).size for pc in [int(a['pc'],16)for a in actions]}
        boundaries=(stop,0x379e9e,0x3739fa,0x37a169,0x37a18a,*action_sites)
        done_actions=set();executing=True
        for fragment in range(128):
            result,changed,executed=host.run_traced(pc,stack,code,0x300000,0xe00000,
                stop_addresses=tuple(b for b in boundaries if not executing or b!=pc),**inputs)
            inputs={};executing=False;visited.update(executed)
            for s in changed:
                if any(ALIASES[n]<=s[1] and s[1]+s[2]<=ALIASES[n]+len(raw)for n,raw in state.items()):stores.append(s)
            pc=result.rip
            if pc==stop:break
            host.continuing=True
            if pc in action_sites:
                for i,a in enumerate(actions):
                    if i not in done_actions and int(a['pc'],16)+code.at(int(a['pc'],16)).size==pc:
                        host.write(a['address'],bytes.fromhex(a['hex']));done_actions.add(i)
                executing=True;continue
            if pc in (0x379e9e,0x3739fa):
                # Resume with the explicitly labeled platform CPU input at
                # the real call's return boundary, not an emulated kernel.
                if pc==0x3739fa:settings+=1;value=saved
                else:acquisitions+=1;value=acquired
                result.rax=value;result.rip=u32(host.read(result.rsp,4));result.rsp+=4
                host.trace(13,0,ctypes.addressof(result));pc=result.rip;executing=True;continue
            assert pc in (0x37a169,0x37a18a)
            releases+=1;result.rsp+=4;result.rip+=code.at(pc).size
            host.trace(13,0,ctypes.addressof(result));pc=result.rip;executing=True
        else:raise RuntimeError('Construction fragment limit reached')
        assert result.rsp==stack+4+count*4,(label,hex(result.rsp))
        after={n:host.read(ALIASES[n],len(raw)).hex()for n,raw in state.items()}
        if actual:
            expected,_=normalize(actual,'after_hex')
            assert after=={n:raw.hex()for n,raw in expected.items()},(label,'genuine whole retained outputs')
            if kind!='buffer_binding':assert result.rax&0xffffffff==owner
        rows.append(dict(label=label,kind=kind,genuine_capture_state=bool(actual),
            owner=f'{owner or ALIASES["object"]:08x}',arguments=[f'{a:08x}'for a in arguments],
            state_access_input=acquired,saved_setting_input=f'{saved:08x}',settings=settings,releases=releases,acquisitions=acquisitions,
            result=f'{result.rax&0xffffffff:08x}',actions=list(actions),
            regions=[dict(name=n,address=f'{ALIASES[n]:08x}',before_hex=raw.hex(),after_hex=after[n])for n,raw in state.items()],
            stores=[dict(pc=f'{p:08x}',address=f'{a:08x}',size=s,before=f'{b:0{s*2}x}',after=f'{v:0{s*2}x}')for p,a,s,b,v in stores]))
    prototypes={}
    for kind,r in captures.items():
        state,alias=normalize(r,'before_hex');a=[alias(int(x,16))for x in r['arguments']]
        saved=int(r['saved_setting_calls'][0]['return_value'],16)if r['saved_setting_calls']else 0
        owner=alias(int(r['owner'],16));measure(kind,kind,state,a,owner,saved=saved,actual=r)
        prototypes[kind]=(state,a,owner)
    def declared(label,kind,edit=None,acquired=0,saved=0,actions=()):
        initial,a,owner=prototypes[kind];state={n:bytearray(raw)for n,raw in initial.items()};a=list(a)
        if edit:edit(state,a)
        measure(label,kind,state,a,owner,acquired,saved,actions=actions)
    rng=random.Random(0x382b22)
    for kind in ('device_object','device_data','buffer_object','voice_object'):
        for i in range(8):
            declared(f'{kind}_unwritten_{i}',kind,lambda s,a:s.__setitem__('object',bytearray(rng.randrange(256)for _ in s['object'])))
    for override in (0,1,2,3,0x10000,0x10001,0x10002,0x20000,0x30002,0x40000,0x80000000,0xffffffff):
        declared(f'override_{override:08x}','device_data',lambda s,a:put(s['speaker_override'],0,override),saved=0x12345678)
    for busy in (0,1,0x100,0x80000000,0xffffffff):
        for acquired in (0,1):
            declared(f'buffer_busy_{busy:08x}_access{acquired}','buffer_object',lambda s,a:put(s['busy'],0,busy),acquired)
    declared('buffer_null_device','buffer_object',lambda s,a:a.__setitem__(0,0))
    for flags in (0,0x10,0x20000,0x20010,0x80000010):
        for config in (0,1,2,0x10000,0x10001,0x20000,0x30002,0x80000002,0xffffffff):
            def edit(s,a):put(s['voice_data'],8,flags);put(s['speaker_config'],0,config)
            declared(f'bind_flags{flags:08x}_config{config:08x}','buffer_binding',edit)
    def nonempty(s,a):
        s['tail']=bytearray(8);put(s['tail'],0,ALIASES['device_object']+0x10);put(s['tail'],4,ALIASES['device_object']+0x10)
        put(s['device_object'],0x10,ALIASES['tail']);put(s['device_object'],0x14,ALIASES['tail'])
    declared('bind_nonempty_list','buffer_binding',nonempty)
    def replaced(s,a):
        put(s['speaker_config'],0,2);s['replacement_source']=bytearray(s['source'])
    declared('bind_source_reload','buffer_binding',replaced,actions=[dict(pc='0037bed2',address=ALIASES['voice_data']+0xb4,hex=struct.pack('<I',ALIASES['replacement_source']).hex())])
    for value in (0,1,0xfffffffe,0xffffffff):
        state={'object':bytearray(8)};put(state['object'],0,0x41718c);put(state['object'],4,value)
        measure(f'plain_count_{value:08x}','add_reference',state,[ALIASES['object']])
        for kind in ('synchronized_reference','reference'):
            for busy in (0,0x80000000):
                for acquired in (0,1):
                    state={'object':bytearray(8),'busy':bytearray(struct.pack('<I',busy))}
                    put(state['object'],0,0x417120);put(state['object'],4,value)
                    measure(f'{kind}_count{value:08x}_busy{busy:08x}_access{acquired}',kind,state,[ALIASES['object']],acquired=acquired)
    # Distinct overlapping words are essential: a repeated fill pattern would
    # hide the original sequential second read behind a premature snapshot.
    state={'object':bytearray(struct.pack('<8I',*[0x5b000000+i for i in range(8)])),
           'payload':bytearray(struct.pack('<II',0x12345678,0x87654321))}
    for payload in (0,ALIASES['payload'],ALIASES['object']+4,ALIASES['object']+8):
        measure(f'pair_{payload:08x}','pair',state,[payload])
    measure('retain_null','reference',{},[0])
    immutable={name:[u32(image.read(address,size),i)for i in range(0,size,4)]for name,address,size in
        [('listener_defaults',0x3857ec,60),('environment_defaults',0x3857b8,48)]}
    data=dict(singleton_address='00387198',speaker_override_address='00386b08',speaker_config_address='00386b04',busy_address='00386b0c',
        device_vtable='00417120',device_data_vtable='004170e4',buffer_base_vtable='00417140',buffer_vtable='00417150',voice_base_vtable='00417254',voice_vtable='0041727c',
        plain_reference_method='00379d0c',synchronized_reference_method='0037a14f',**immutable,
        references=[dict(vtable_identity=f'{a:08x}',method_identity=f'{u32(image.read(a+4,4)):08x}')for a in (0x417120,0x4170e4,0x417140,0x417150,0x417254,0x41727c,0x41718c,0x417200)])
    fixture=dict(schema='ORIGINAL_SOUND_SDK_CONSTRUCTION_CPU_V1',boundary=__doc__,data=data,cases=rows)
    (args.output/'original.json').write_text(json.dumps(fixture,indent=2)+'\n')
    listing=args.output/'executed-original.asm'
    listing.write_text(''.join(f'{pc:08x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'for pc in sorted(visited)))
    (args.output/'report.json').write_text(json.dumps(dict(boundary=__doc__,tool_sha256=sha(__file__),
        image_sha256=sha(image_path),capture_pins={k:sha(args.capture/k/'report.json')for k in captures},
        cases=len(rows),genuine_states=len(captures),stores=sum(len(r['stores'])for r in rows),
        visited_instructions=len(visited),executed_listing_sha256=sha(listing),
        fixture_sha256=sha(args.output/'original.json'),instruction_patches=[]),indent=2)+'\n')
    print(f'PASS {len(captures)} genuine states, {len(rows)} original CPU cases')
if __name__=='__main__':main()
