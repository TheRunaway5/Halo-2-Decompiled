#!/usr/bin/env python3
"""Execute original spatial-commit CPU fragments and record caller contracts.

Unchanged original CPU instructions execute between explicit kernel, FIFO and
pending device-write boundaries. The original initializer and every numerical
callee execute unchanged. Continuing at the next CPU fragment preserves all
CPU/FPU context; no kernel/FIFO/callee result is supplied. This does not execute
platform locks or device MMIO on the host. Genuine whole emulator calls must
agree first. Declared controls are ABI states, not extra gameplay events.
"""
import argparse
import atexit
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
from original_audio_cpu import OriginalAudioCpu

sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
u32=lambda raw,offset=0:struct.unpack_from('<I',raw,offset)[0]
put=lambda raw,offset,value:struct.pack_into('<I',raw,offset,value&0xffffffff)
bits=lambda value:struct.unpack('<I',struct.pack('<f',value))[0]
ALIASES={name:0x1001000+i*0x1000 for i,name in enumerate((
    'voice','data','device','device_data','source','parameters','parent_link',
    'parent_owner','distance_curve','parent_data','parent_parameters'))}
ALIASES['speaker_config']=0x386b04


class FragmentCpu(OriginalAudioCpu):
    continuing=False
    def prepare(self,entry,stack,inputs):
        if not self.continuing:return super().prepare(entry,stack,inputs)
        assert not inputs
        registers=self.registers();registers.rip=entry
        self.trace(13,0,__import__('ctypes').addressof(registers))
    def wait_stop(self):
        try:return super().wait_stop()
        except RuntimeError:
            registers=self.registers()
            print('Original CPU stopped:',{name:hex(getattr(registers,name)) for name in ('rip','rsp','rbp','rax','rcx','rdx','rsi','rdi')},flush=True)
            raise


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=Path,action='append',required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    image_path=ROOT/'assets/disc/default.xbe'
    assert sha(image_path)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
    image=Xbe(image_path)
    codes=[_Code(image,name) for name in ('DSOUND','.text')]
    class Code:
        def at(self,address):return next((c.at(address) for c in codes if c.inside(address)),None)
    code=Code()
    captures=[(p,json.loads((p/'report.json').read_text())) for p in args.capture]
    for _,r in captures:
        assert r['image_sha256']==sha(image_path) and r['original_whole_commit_returned_normally']
        assert r['device_writes_executed_normally']
        assert not any(r[k] for k in ('instruction_patches','guest_memory_writes','guest_register_writes','supplied_callee_returns'))
    layout=args.output/'user-layout.c'
    layout.write_text('#include <stddef.h>\n#include <sys/user.h>\n_Static_assert(offsetof(struct user,u_debugreg)==848,"Unsupported ptrace ABI");\nint main(void){return 0;}\n')
    subprocess.run(['cc',str(layout),'-o',str(args.output/'user-layout')],check=True)
    subprocess.run([str(args.output/'user-layout')],check=True)
    context=captures[0][1]['registers'];fp=(context['fctrl'],context['mxcsr'])
    host=FragmentCpu(image,args.output/'original32.elf',*fp);atexit.register(host.close)
    stack,stop=0x100f000,0x3000000
    kernel_targets={0x379cc9,0x379cee,0x379c90,0x379cb2}
    kernel_sites={};device_sites={};entries={0x37ca0a:'calculate',0x37ae57:'voice_data',
        0x381e2a:'dispatch',0x381ce4:'gain',0x381d7c:'pitch',0x3812e0:'hrtf',0x381710:'filter'}
    argument_counts={'calculate':2,'voice_data':5,'dispatch':0,'gain':0,'pitch':0,'hrtf':2,'filter':1}
    instruction_pins={}
    for start in (0x382032,*entries):
        path=ROOT/f'analysis/audio-spatial-20261011/sdk-image-pairing-v9/instructions/{start:08x}-base.asm'
        for line in path.read_text().splitlines():
            fields=line.split();pc=int(fields[0],16);raw=bytes.fromhex(fields[1]);instruction=code.at(pc)
            assert image.read(pc,len(raw))==host.read(pc,len(raw))==raw==instruction.bytes
            instruction_pins[pc]=raw
            if instruction.mnemonic=='call' and instruction.operands[0].type==2 and instruction.operands[0].imm in kernel_targets:
                kernel_sites[pc]=instruction.operands[0].imm
            if instruction.mnemonic=='mov' and instruction.operands[0].type==3:
                op=instruction.operands[0]
                if op.mem.disp&0xff000000==0xfe000000:
                    device_sites[pc]=(op.mem.disp&0xffffffff,start)
    # One coefficient loop uses EBX as the device address rather than an
    # absolute encoded address. It is established by the complete original.
    device_sites[0x3813ec]=(None,0x3812e0)
    fifo_exits={0x381d13:0x381d20,0x381dcd:0x381dda,0x3817f4:0x381801,0x38138f:0x38139d}
    boundaries=tuple({stop,*kernel_sites,*device_sites,*fifo_exits,*entries})
    host.trace(6,host.debug_offset+7*8,0)
    host.write(stack,struct.pack('<I',stop))
    host.continuing=False
    _,_,initializer=host.run_traced(0x37e126,stack,code,ALIASES['voice'],0xc000,stop_addresses=(stop,))
    assert all(host.read(0x3871c8,44).hex()==r['selected_sdk_table_hex'] for _,r in captures)
    rows=[];visited=set(initializer);boundary_counts={}

    def normalise(report,field):
        state={r['name']:bytearray.fromhex(r[field]) for r in report['regions']}
        links=[('voice',8,'device'),('voice',0x70,'data'),('device',0xc,'device_data'),
               ('data',0xb4,'source'),('data',0xb8,'parameters'),('data',0xb0,'parent_link'),
               ('source',0x50,'distance_curve'),('parent_link',0x20,'parent_owner'),
               ('parent_owner',0x80,'parent_data'),('parent_data',0xb8,'parent_parameters')]
        for name,offset,target in links:
            if name in state and u32(state[name],offset):
                assert target in state,(name,target)
                put(state[name],offset,ALIASES[target])
        return state

    def measure(label,state,actual=None,mode='pipeline',actions=(),context=fp):
        host.control_word,host.mxcsr=context
        for name,raw in state.items():
            address=ALIASES[name]
            if name=='speaker_config':assert os.pwrite(host.memory_fd,raw,address)==len(raw)
            else:host.write(address,raw)
        host.write(stack-0x2000,bytes([0x5b])*0x2004)
        host.write(stack,struct.pack('<I',stop))
        host.continuing=False;entry=0x382032 if mode in ('pipeline','update') else 0x381e2a
        inputs={'rcx':ALIASES['voice']};calls=[];stores=[];writes=[];pending_entries={}
        call_index=0;execute_entry=True
        for fragment in range(2000):
            stops=boundaries if mode=='pipeline' else tuple({stop,*kernel_sites,*entries})
            if host.continuing and entry in stops and not execute_entry:
                host.prepare(entry,stack,{})
                result=host.registers();changed=[];executed=set()
            else:
                result,changed,executed=host.run_traced(entry,stack,code,ALIASES['voice'],0xc000,
                    stop_addresses=tuple(pc for pc in stops if not execute_entry or pc!=entry),**inputs)
            execute_entry=False
            stores.extend(changed);visited.update(executed);pc=result.rip
            if pc==stop:break
            host.continuing=True;inputs={};boundary_counts[pc]=boundary_counts.get(pc,0)+1
            if pc in entries:
                owner=entries[pc];count=argument_counts[owner]
                arguments=list(struct.unpack('<'+'I'*count,host.read(result.rsp+4,count*4))) if count else []
                calls.append(dict(owner=owner,this=f'{result.rcx&0xffffffff:08x}',arguments=[f'{a:08x}' for a in arguments]))
                if mode!='pipeline':
                    # Explicit caller-only contract. Declared service actions
                    # are retained mutations, never fabricated return values.
                    call_index+=1
                    for action in actions:
                        if action['after_call']==call_index:
                            raw=bytes.fromhex(action['hex']);address=action['address']
                            assert ALIASES['voice']<=address<0x100d000
                            host.write(address,raw)
                    returned=host.registers();returned.rip=u32(host.read(returned.rsp,4));returned.rsp+=4+count*4
                    host.trace(13,0,__import__('ctypes').addressof(returned));entry=returned.rip
                    continue
                # Execute the unchanged real callee prologue next; exclude
                # only this entry from the next fragment's stop set.
                entry=pc;execute_entry=True;continue
            if pc in fifo_exits:entry=fifo_exits[pc];continue
            if pc in kernel_sites:entry=pc+code.at(pc).size;continue
            assert pc in device_sites
            address,_=device_sites[pc];instruction=code.at(pc);source=instruction.operands[1]
            if address is None:address=result.rbx&0xffffffff
            if source.type==2:value=source.imm&0xffffffff
            else:
                assert source.type==1
                name=instruction.reg_name(source.reg)
                registers={'eax':'rax','ebx':'rbx','ecx':'rcx','edx':'rdx','esi':'rsi','edi':'rdi'}
                value=getattr(result,registers[name])&0xffffffff
            writes.append(dict(address=f'{address:08x}',value=f'{value:08x}'))
            entry=pc+instruction.size
        else:raise RuntimeError('Original spatial commit CPU fragment limit reached')
        assert result.rsp==stack+4
        after={name:host.read(ALIASES[name],len(raw)).hex() for name,raw in state.items()}
        if actual is not None:
            expected=normalise(actual,'after_hex')
            assert after=={name:raw.hex() for name,raw in expected.items()},(label,'retained outputs differ')
            assert writes==[{k:w[k] for k in ('address','value')} for w in actual['observed_device_writes']],(label,'commands differ')
            old_to_new={int(r['address'],16):ALIASES[r['name']] for r in actual['regions']}
            def alias(address):
                for region in actual['regions']:
                    base=int(region['address'],16)
                    if base<=address<base+region['size']:return ALIASES[region['name']]+address-base
                return address
            expected_calls=[]
            for call in actual['calls']:
                owner=call['owner'];this=alias(int(call['this'],16)) if owner not in ('calculate','voice_data') else None
                expected_calls.append((owner,this,[alias(int(a,16)) if a else 0 for a in call['arguments']]))
            assert [(c['owner'],int(c['this'],16) if c['owner'] not in ('calculate','voice_data') else None,
                     [int(a,16) for a in c['arguments']]) for c in calls]==expected_calls,label
        rows.append(dict(label=label,mode=mode,actual_whole_capture=actual is not None,
            x87_control_word=context[0],mxcsr=context[1],calls=calls,actions=list(actions),writes=writes,
            regions=[dict(name=name,address=f'{ALIASES[name]:08x}',before_hex=raw.hex(),after_hex=after[name]) for name,raw in state.items()],
            stores=[dict(instruction=f'{pc:08x}',address=f'{address:08x}',bytes=size,before=f'{old:08x}',after=f'{new:08x}')
                    for pc,address,size,old,new in stores],return_eax=f'{result.rax&0xffffffff:08x}'))

    for path,report in captures:
        measure(path.name,normalise(report,'before_hex'),actual=report,
                context=(report['registers']['fctrl'],report['registers']['mxcsr']))
    selected=next(report for _,report in captures if report['observed_device_writes'])
    base=normalise(selected,'before_hex')
    def seed():return {name:bytearray(raw) for name,raw in base.items()}
    for flags in range(256):
        s=seed();put(s['parameters'],0,flags|0xdead0000)
        measure(f'dispatch_flags_{flags:02x}',s,mode='dispatch')
    for flags in range(256):
        s=seed();s['voice'][0x12]=flags
        measure(f'update_flags_{flags:02x}',s,mode='update')
    for mask in (1,0x20,0x40,0x80,0xff,0x123400ff):
        for replacement in (0,0x20,0x40,0x80,0xff,0xabcd0000):
            s=seed();put(s['parameters'],0,mask)
            action=[dict(after_call=1,address=ALIASES['parameters'],hex=struct.pack('<I',replacement).hex())]
            measure(f'dispatch_reload_{mask:08x}_{replacement:08x}',s,mode='dispatch',actions=action)
    # A declared reached service can replace retained source/parameter
    # identities. Keep all replacement backing explicit in the fixture.
    for which,after_call,offset,target in [('source',1,0xb4,0x100a000),('parameters',1,0xb8,0x100b000),('source_after_voice',2,0xb4,0x100a000)]:
        s=seed();name='replacement_source' if offset==0xb4 else 'replacement_parameters'
        ALIASES[name]=target;s[name]=bytearray(s['source'] if offset==0xb4 else s['parameters'])
        measure('update_reload_'+which,s,mode='update',actions=[dict(after_call=after_call,address=ALIASES['data']+offset,hex=struct.pack('<I',target).hex())])
    s=seed();ALIASES['replacement_parameters']=0x100b000
    s['replacement_parameters']=bytearray(s['parameters']);put(s['replacement_parameters'],0,0xabcd00ff);put(s['parameters'],0,1)
    measure('dispatch_reload_identity',s,mode='dispatch',actions=[dict(after_call=1,address=ALIASES['data']+0xb8,hex=struct.pack('<I',0x100b000).hex())])
    s=seed();ALIASES['replacement_device']=0x1007000;ALIASES['replacement_device_data']=0x1008000
    s['replacement_device']=bytearray(s['device']);s['replacement_device_data']=bytearray(s['device_data'])
    put(s['replacement_device'],0xc,ALIASES['replacement_device_data'])
    measure('update_reload_device',s,mode='update',actions=[dict(after_call=1,address=ALIASES['voice']+8,hex=struct.pack('<I',0x1007000).hex())])
    for mode in (0,1,2):
        for flags in (0,0x10000,0x78010000,0x07ff0000,0xffffffff):
            for count in (1,2,3):
                s=seed();put(s['source'],0x40,mode);put(s['source'],0,flags)
                s['voice'][0x64]=count;put(s['source'],4,0)
                measure(f'pipeline_mode_flags_{mode}_{flags:08x}_{count}',s)
    for offset in (8,0xc,0x10,0x14,0x18,0x1c,0x28,0x2c,0x30,0x38,0x3c,0x44,0x48,0x4c):
        for value in (0.,.001,.5,1.,2.,-1.,20.,-20.):
            s=seed();put(s['source'],offset,bits(value));put(s['source'],0,0x07ff0000);put(s['source'],4,0)
            measure(f'pipeline_source_{offset:02x}_{bits(value):08x}',s)
    rng=random.Random(0x434f4d4d)
    for i in range(96):
        s=seed();put(s['source'],0,0x07ff0000);put(s['source'],4,0)
        for offset in (8,0xc,0x10,0x14,0x18,0x1c,0x28,0x2c,0x30):put(s['source'],offset,bits(rng.randrange(-500,501)/37))
        put(s['device_data'],0x30,0x7f);put(s['device_data'],0x34,0)
        put(s['source'],0x40,i%3);s['voice'][0x64]=i%3+1
        measure(f'pipeline_varied_{i}',s)
    for flags in (0,1,2,3,4,7):
        s=seed();put(s['source'],0,0xffffffff);put(s['source'],4,0);put(s['source'],0x58,flags)
        measure(f'pipeline_tuning_flags_{flags}',s)
    for flags in (0,1,2,4):
        s=seed();s['voice'][0x12]=flags;measure(f'pipeline_inactive_{flags}',s)
    s=seed();put(s['source'],0,0xffffffff);put(s['source'],4,0);put(s['source'],0x58,3);put(s['device_data'],0x30,0x7f)
    measure('pipeline_all_dirty',s)
    for pc in visited:
        instruction=code.at(pc);assert image.read(pc,instruction.size)==host.read(pc,instruction.size)==instruction.bytes
    listing=args.output/'executed-original.asm'
    listing.write_text(''.join(f'{pc:08x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n' for pc in sorted(visited)))
    host.close()
    fixture=args.output/'fixtures.json'
    fixture.write_text(json.dumps(dict(schema='ORIGINAL_SOUND_SDK_COMMIT_CPU_V1',aliases=ALIASES,cases=rows),separators=(',',':'))+'\n')
    report=dict(boundary=__doc__,tool_sha256=sha(__file__),host_runner_sha256=sha(ROOT/'tools/original_audio_cpu.py'),
        original_image_sha256=sha(image_path),capture_sha256={str(p/'report.json'):sha(p/'report.json') for p,_ in captures},
        fixture_sha256=sha(fixture),genuine_whole_captures=len(captures),declared_cases=len(rows)-len(captures),
        mode_counts={mode:sum(r['mode']==mode for r in rows) for mode in ('pipeline','dispatch','update')},
        command_words=sum(len(r['writes']) for r in rows),ordered_stores=sum(len(r['stores']) for r in rows),
        unchanged_executed_instructions=len(visited),executed_listing_sha256=sha(listing),
        kernel_boundaries={f'{pc:08x}':f'{target:08x}' for pc,target in kernel_sites.items()},
        FIFO_boundaries={f'{pc:08x}':f'{target:08x}' for pc,target in fifo_exits.items()},
        declared_mutable_image_words=['00386b04'],supplied_callee_returns=[],instruction_patches=[],
        native_or_physical_DSP_claimed=False)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))


if __name__=='__main__':main()
