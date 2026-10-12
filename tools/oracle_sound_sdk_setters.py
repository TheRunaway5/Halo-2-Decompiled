#!/usr/bin/env python3
"""Measure original SDK setter CPU fragments against genuine whole calls.

The public interface wrapper executes to its synchronized caller. That caller
resumes at its post-access CPU boundary with the genuinely captured acquisition
flag; original kernel access is not executed or given a chosen success result.
Declared no-acquisition controls separately execute379E9E's original nonzero
IRQL-byte branch after its FS load. Kernel release, FIFO and pending MMIO are
explicit fragment boundaries. Immediate spatial commits are caller contracts;
filter CPU code executes unchanged. These are not live native/device results.
"""
import argparse
import atexit
import ctypes
import hashlib
import json
import os
from pathlib import Path
import random
import shutil
import struct
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from functions import _Code
from oracle_sound_sdk_commit import FragmentCpu

sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
u32=lambda raw,at=0:struct.unpack_from('<I',raw,at)[0]
put=lambda raw,at,value:struct.pack_into('<I',raw,at,value&0xffffffff)
ALIASES={name:0x1001000+i*0x1000 for i,name in enumerate(('object','voice','data','source','parameters','payload','replacement_source','replacement_data'))}
ALIASES['busy']=0x386b0c
ALIASES['adjacent_payload']=ALIASES['source']+0xa4
SPEC={'position':(0x37c668,0x37c3bf,0x37c3c8,5),
      'environment':(0x37c6e5,0x37c4d2,0x37c4d8,3),
      'filter':(0x37b68b,0x37b1e6,0x37b1ec,2)}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=Path,action='append',required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    image_path=ROOT/'assets/disc/default.xbe'
    assert sha(image_path)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
    image=Xbe(image_path);code=_Code(image,'DSOUND')
    captures={}
    for p in args.capture:
        r=json.loads((p/'report.json').read_text());assert r['original_whole_setter_returned_normally']
        assert r['original_lock_acquired']==1 and r['busy_under_lock']==0
        assert not any(r[k] for k in ('instruction_patches','guest_memory_writes','guest_register_writes','supplied_callee_returns'))
        captures[r['kind']]=(p,r)
    assert set(captures)==set(SPEC)
    layout=args.output/'user-layout.c'
    layout.write_text('#include <stddef.h>\n#include <sys/user.h>\n_Static_assert(offsetof(struct user,u_debugreg)==848,"Unsupported ptrace ABI");\nint main(void){return 0;}\n')
    subprocess.run(['cc',str(layout),'-o',str(args.output/'user-layout')],check=True)
    subprocess.run([str(args.output/'user-layout')],check=True)
    context=captures['position'][1]['registers'];fp=(context['fctrl'],context['mxcsr'])
    host=FragmentCpu(image,args.output/'original32.elf',*fp);atexit.register(host.close)
    host.trace(6,host.debug_offset+7*8,0)
    stack,stop=0x100f000,0x3000000;visited=set();rows=[]
    kernel={0x381722,0x38184c,0x37c3dd,0x37c418,0x37c4ed,0x37c517,0x37b201,0x37b227}
    device={0x38180d:0xfe8202f8,0x381813:0xfe8202fc,0x38181c:0xfe820318,
            0x381825:0xfe820374,0x38182e:0xfe820378,0x381834:0xfe8202fc}
    boundaries=(stop,0x382032,0x381710,0x3817f4,*kernel,*device)

    def normalise(report,field):
        state={r['name']:bytearray.fromhex(r[field]) for r in report['regions']}
        for name,at,target in [('object',0xc,'voice'),('object',0x10,'data'),('voice',0x70,'data'),
                               ('data',0xb4,'source'),('data',0xb8,'parameters')]:
            if u32(state[name],at):put(state[name],at,ALIASES[target])
        return state

    def measure(label,kind,state,arguments,acquired=1,actions=(),actual=None,context=fp):
        host.control_word,host.mxcsr=context
        for name,raw in state.items():
            address=ALIASES[name]
            if name=='busy':assert os.pwrite(host.memory_fd,raw,address)==len(raw)
            else:host.write(address,raw)
        entry,wrapper,post,count=SPEC[kind]
        host.write(stack-0x1000,bytes([0x5b])*0x1100)
        host.write(stack,struct.pack('<'+'I'*(len(arguments)+1),stop,*arguments))
        host.continuing=False
        result,_,executed=host.run_traced(entry,stack,code,ALIASES['object'],0x8000,stop_addresses=(wrapper,))
        visited.update(executed)
        inner=list(struct.unpack('<'+'I'*count,host.read(result.rsp+4,count*4)))
        expected_owner=arguments[0]-0x1c if arguments[0] else 0
        assert inner[0]==expected_owner
        # Explicit CPU checkpoint after379E9E. Acquisition1 is observed in
        # each genuine call. For0, separately execute the actual original
        # nonzero-IRQL branch after its platform FS-byte input boundary.
        if not acquired:
            host.continuing=False;host.write(stack,struct.pack('<I',stop))
            irq,_,executed=host.run_traced(0x379ea6,stack,code,0,0,stop_addresses=(stop,),rax=1)
            visited.update(executed);assert irq.rax&0xffffffff==0
        host.continuing=False
        if kind=='position':
            # EBP frame, saved ESI; original position wrapper has a prologue.
            host.write(stack-4,struct.pack('<'+'I'*(count+3),0x5b5b5b5b,0x5b5b5b5b,stop,*inner))
            inputs={'rax':acquired,'rbp':stack};start_stack=stack-4;return_stack=stack+count*4+8
        else:
            host.write(stack-4,struct.pack('<'+'I'*(count+2),0x5b5b5b5b,stop,*inner))
            inputs={'rax':acquired};start_stack=stack-4;return_stack=stack+count*4+4
        pc=post;stores=[];calls=[];writes=[];release_count=0;execute=True
        action_sites={0x37c046 if kind=='position' else 0x37c103} if actions else set()
        stops=(*boundaries,*action_sites)
        for fragment in range(128):
            if host.continuing and pc in stops and not execute:
                host.prepare(pc,start_stack,{});result=host.registers();changed=[];executed=set()
            else:
                try:
                    result,changed,executed=host.run_traced(pc,start_stack,code,ALIASES['object'],0x8000,
                        stop_addresses=tuple(b for b in stops if not execute or b!=pc),**inputs)
                except BaseException:
                    registers=host.registers()
                    print(label,'fragment',hex(pc),'CPU', {n:hex(getattr(registers,n))for n in ('rip','rsp','rbp','rax','rsi','rdi')},flush=True)
                    raise
            execute=False;inputs={};visited.update(executed)
            for store in changed:
                stores.append(store)
                for action in actions:
                    if action['after_store']==len(stores):host.write(action['address'],bytes.fromhex(action['hex']))
            pc=result.rip
            if pc==stop:break
            host.continuing=True
            if pc in action_sites:execute=True;continue
            if pc==0x382032:
                calls.append(dict(owner='commit',voice=f'{result.rcx&0xffffffff:08x}',arguments=[]))
                # Caller-only immediate commit boundary. Original37A669
                # subsequently sets EAX0; no callee result is supplied here.
                result.rip=u32(host.read(result.rsp,4));result.rsp+=4
                host.trace(13,0,ctypes.addressof(result));pc=result.rip;continue
            if pc==0x381710:
                calls.append(dict(owner='filter',voice=f'{result.rcx&0xffffffff:08x}',
                    arguments=[f'{u32(host.read(result.rsp+4,4)):08x}']))
                execute=True;continue
            if pc in kernel:
                if pc not in (0x381722,0x38184c):
                    release_count+=1
                    # The original imported release is stdcall with the
                    # pushed lock identity. Advance its actual argument ABI;
                    # no return register or success value is supplied.
                    result.rsp+=4;host.trace(13,0,ctypes.addressof(result))
                pc+=code.at(pc).size;continue
            if pc==0x3817f4:pc=0x381801;continue
            assert pc in device
            operand=code.at(pc).operands[1]
            registers={'eax':'rax','ecx':'rcx','edx':'rdx','ebx':'rbx','esi':'rsi','edi':'rdi'}
            value=operand.imm&0xffffffff if operand.type==2 else getattr(result,registers[code.at(pc).reg_name(operand.reg)])&0xffffffff
            writes.append(dict(address=f'{device[pc]:08x}',value=f'{value:08x}'))
            pc+=code.at(pc).size
        else:raise RuntimeError('Setter CPU fragment limit reached')
        assert result.rsp==return_stack,(label,hex(result.rsp),hex(return_stack))
        assert release_count==acquired
        after={name:host.read(ALIASES[name],len(raw)).hex() for name,raw in state.items()}
        if actual:
            assert after=={name:raw.hex() for name,raw in normalise(actual,'after_hex').items()},(label,'whole retained state')
            assert writes==[{k:w[k] for k in ('address','value')}for w in actual['observed_device_writes']],(label,'whole device commands')
            assert result.rax&0xffffffff==int(actual['actual_return_eax'],16)
        row=dict(label=label,kind=kind,genuine=bool(actual),arguments=[f'{a:08x}'for a in arguments],
            acquired=acquired,context=list(context),actions=list(actions),result=f'{result.rax&0xffffffff:08x}',
            releases=release_count,calls=calls,device_writes=writes,
            regions=[dict(name=name,address=f'{ALIASES[name]:08x}',before_hex=raw.hex(),after_hex=after[name])for name,raw in state.items()],
            ordered_stores=[dict(pc=f'{pc:08x}',address=f'{address:08x}',size=size,before=f'{before:0{size*2}x}',after=f'{after:0{size*2}x}')
                for pc,address,size,before,after in stores])
        rows.append(row)

    prototypes={}
    for kind,(path,report) in captures.items():
        state=normalise(report,'before_hex');arguments=[int(a,16)for a in report['arguments']]
        arguments[0]=ALIASES['object']+0x1c
        if kind!='position':arguments[1]=ALIASES['payload']
        context=(report['registers']['fctrl'],report['registers']['mxcsr'])
        measure(path.name,kind,state,arguments,actual=report,context=context)
        prototypes[kind]=(state,arguments,context)
    def declared(label,kind,edit=None,arguments=None,acquired=1,actions=(),context=None):
        raw,initial,fp=prototypes[kind];state={n:bytearray(b)for n,b in raw.items()};a=list(initial)
        if edit:edit(state,a)
        measure(label,kind,state,arguments or a,acquired,actions,context=context or fp)
    for kind in SPEC:
        for busy in (1,0x100,0x80000000,0xffffffff):
            for acquired in (0,1):
                declared(f'{kind}_busy_{busy:08x}_access{acquired}',kind,
                    lambda s,a,busy=busy:put(s['busy'],0,busy),acquired=acquired)
        declared(kind+'_access0',kind,acquired=0)
    for kind in ('position','environment'):
        for apply in (0,1,2,3,0x100,0x101,0x80000000,0xffffffff):
            declared(f'{kind}_apply_{apply:08x}',kind,lambda s,a,apply=apply:a.__setitem__(-1,apply))
        declared(kind+'_dirty_width',kind,lambda s,a:put(s['source'],0 if kind=='position'else 0x7c,0xdead0000))
    words=(0,0x80000000,1,0x007fffff,0x00800000,0x3f800000,0xbf800000,0x7f7fffff,0x7f800000,0xff800000,0x7fc01234,0x7f801234)
    for axis in range(3):
        for value in words:
            declared(f'position_axis{axis}_{value:08x}','position',lambda s,a,axis=axis,value=value:a.__setitem__(axis+1,value))
    rng=random.Random(0x37c0e9)
    for i in range(48):
        payload=bytes(rng.randrange(256)for _ in range(36))
        declared(f'environment_words_{i}','environment',lambda s,a,payload=payload:s.__setitem__('payload',bytearray(payload)))
    for offset in (-8,-4,0,4,8):
        def alias(s,a,offset=offset):
            a[1]=ALIASES['source']+0x80+offset
            if offset>0:s['adjacent_payload']=bytearray([0x5b]*offset)
        declared(f'environment_alias_{offset:+d}','environment',alias)
    for kind in ('position','environment'):
        # Declared external retained mutation after the first source store;
        # the original observer stops at that exact write before resuming.
        raw=prototypes[kind][0]['source']
        def replacement(s,a,raw=raw):s['replacement_source']=bytearray(raw)
        actions=[dict(after_store=1,address=ALIASES['data']+0xb4,hex=struct.pack('<I',ALIASES['replacement_source']).hex())]
        declared(kind+'_source_reload',kind,replacement,actions=actions)
    declared('environment_payload_alias','environment',lambda s,a:a.__setitem__(1,ALIASES['source']+0x7c))
    for flags in (0,1,2,3,4,0xff):
        declared(f'filter_active_{flags:02x}','filter',lambda s,a,flags=flags:s['voice'].__setitem__(0x12,flags))
    for control in (0,1,2,3,0x10,0xffffffff):
        declared(f'filter_control_{control:08x}','filter',lambda s,a,control=control:put(s['payload'],0,control))
    for context in ((0x027f,0x1f80),(0x067f,0x3f80),(0x0a7f,0x5f80),(0x0e7f,0x7f80),(0x007f,0x9fc0)):
        declared(f'position_fp_{context[0]:04x}_{context[1]:04x}','position',context=context)
    fixture=dict(schema='ORIGINAL_SOUND_SDK_SETTER_CPU_V1',boundary=__doc__,aliases=ALIASES,cases=rows)
    (args.output/'fixtures.json').write_text(json.dumps(fixture,indent=2)+'\n')
    listing=''.join(f'{pc:08x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'for pc in sorted(visited))
    for pc in visited:assert host.read(pc,code.at(pc).size)==image.read(pc,code.at(pc).size)==code.at(pc).bytes
    (args.output/'executed-original.asm').write_text(listing)
    report=dict(boundary=__doc__,tool_sha256=sha(__file__),host_runner_sha256=sha(ROOT/'tools/original_audio_cpu.py'),
        fragment_runner_sha256=sha(ROOT/'tools/oracle_sound_sdk_commit.py'),original_image_sha256=sha(image_path),
        captures={str(path/'report.json'):sha(path/'report.json')for path,_ in captures.values()},
        fixture_sha256=sha(args.output/'fixtures.json'),genuine_whole_calls=3,declared_cases=len(rows)-3,
        command_words=sum(len(r['device_writes'])for r in rows),ordered_stores=sum(len(r['ordered_stores'])for r in rows),
        unchanged_executed_instructions=len(visited),executed_listing_sha256=sha(args.output/'executed-original.asm'),
        acquisition1='Actual captured379E9E return CPU boundary',acquisition0='Declared nonzero IRQL-byte input to original379EA6 branch',
        original_host_kernel_execution=False,immediate_commit_callee='Caller-only contract, not numerical commit execution',
        instruction_patches=[],chosen_kernel_results=[],native_or_physical_DSP_claimed=False)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items()if isinstance(v,(int,bool))},indent=2))

if __name__=='__main__':main()
