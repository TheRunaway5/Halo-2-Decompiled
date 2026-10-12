#!/usr/bin/env python3
"""Execute unchanged original hardware activation and all reached CPU helpers.

Both genuine whole cold-boot activations must match retained output and every
device word before declared CPU cases run. Kernel locks, FIFO polling and MMIO
are explicit host fragment boundaries. Numerical/packing/slot/HRTF CPU owners
execute unchanged, including the original current-algorithm initializer. No
native output supplies expected values; no host DSP/kernel execution is claimed.
"""
import argparse,atexit,hashlib,json,os,random,struct,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from functions import _Code
from oracle_sound_sdk_commit import FragmentCpu
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
u32=lambda raw,at=0:struct.unpack_from('<I',raw,at)[0]
put=lambda raw,at,value:struct.pack_into('<I',raw,at,value&0xffffffff)
A=dict(voice=0x1001000,data=0x1002000,device=0x1003000,source=0x1004000,parameters=0x1005000,
       ordinary_count=0x1006000,spatial_count=0x1007000,voice_mapping=0x1008000,
       voice_mapping_pointer=0x3872c0,speaker_config=0x386b04)
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--capture',type=Path,required=True)
    parser.add_argument('--pairing',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    path=ROOT/'assets/disc/default.xbe';assert sha(path)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
    image=Xbe(path);code=_Code(image,'DSOUND')
    captures={p.parent.name:json.loads(p.read_text())for p in args.capture.glob('*/report.json')}
    assert set(captures)=={'spatial','nonspatial'}
    for r in captures.values():
        assert r['original_whole_activation_returned_normally']and r['device_writes_executed_normally']
        assert not any(r[k]for k in ('instruction_patches','guest_memory_writes','guest_register_writes','supplied_callee_returns'))
    layout=args.output/'user-layout.c';layout.write_text('#include <stddef.h>\n#include <sys/user.h>\n_Static_assert(offsetof(struct user,u_debugreg)==848,"Unsupported ptrace ABI");\nint main(void){return 0;}\n')
    subprocess.run(['cc',str(layout),'-o',str(args.output/'user-layout')],check=True);subprocess.run([str(args.output/'user-layout')],check=True)
    host=FragmentCpu(image,args.output/'original32.elf',0x23f,0x1fa0);atexit.register(host.close);host.trace(6,host.debug_offset+7*8,0)
    kernel_targets={0x379c90,0x379cb2};kernel_sites={};device_sites={}
    for entry in (0x381aa9,0x37e737,0x381a92,0x381a74,0x3808e4,0x380ae6,0x3808be,0x3812e0):
        listing=args.pairing/'instructions'/f'{entry:08x}-base.asm'
        for line in listing.read_text().splitlines():
            pc,encoded,*_=line.split();pc=int(pc,16);i=code.at(pc);assert i.bytes==bytes.fromhex(encoded)==host.read(pc,i.size)
            if i.mnemonic=='call'and i.operands[0].type==2 and i.operands[0].imm in kernel_targets:kernel_sites[pc]=i.operands[0].imm
            if i.mnemonic=='mov'and i.operands[0].type==3 and i.operands[0].mem.disp&0xff000000==0xfe000000:
                device_sites[pc]=i.operands[0].mem.disp&0xffffffff
    device_sites[0x3813ec]=None
    fifo_exits={0x381b30:0x381b3e,0x38138f:0x38139d}
    stack,stop=0x100f000,0x3000000;host.write(stack,struct.pack('<I',stop));host.continuing=False
    _,_,visited=host.run_traced(0x37e126,stack,code,0x1001000,0xb000,stop_addresses=(stop,));visited=set(visited)
    # The first nonspatial activation precedes algorithm initialization and
    # does not read this table. Require exact initialized dispatch when reached.
    assert host.read(0x3871c8,44).hex()==captures['spatial']['selected_sdk_table_hex']
    def normalize(r,field):
        original_mapping=u32(bytes.fromhex(next(x for x in r['regions']if x['name']=='voice_mapping_pointer')[field]))
        addresses={n:A[n]for n in A}
        addresses['voice_mapping']=A['voice_mapping']+int(next(x for x in r['regions']if x['name']=='voice_mapping')['address'],16)-original_mapping
        links=[(int(x['address'],16),x['size'],addresses[x['name']])for x in r['regions']]
        def alias(value):
            if original_mapping<=value<original_mapping+0x4000:return A['voice_mapping']+value-original_mapping
            for start,size,target in links:
                if start<=value<start+size:return target+value-start
            return value
        state={x['name']:bytearray.fromhex(x[field])for x in r['regions']}
        for raw in state.values():
            for at in range(0,len(raw)-3,4):put(raw,at,alias(u32(raw,at)))
        return state,addresses
    rows=[]
    def measure(label,state,addresses=None,actual=None,context=(0x23f,0x1fa0),actions=()):
        addresses=addresses or A;host.control_word,host.mxcsr=context
        extents=sorted((addresses[n],addresses[n]+len(raw),n)for n,raw in state.items())
        assert all(a[1]<=b[0]for a,b in zip(extents,extents[1:])),(label,'overlapping backing')
        for n,raw in state.items():
            address=addresses[n]
            if address<0x1000000:assert os.pwrite(host.memory_fd,raw,address)==len(raw)
            else:host.write(address,raw)
        host.write(stack-0x800,bytes([0x5b])*0x804);host.write(stack,struct.pack('<I',stop));host.continuing=False
        entry=0x381aa9;inputs=dict(rcx=A['voice']);stores=[];writes=[];locks=[];execute=True;done=set()
        action_sites={int(x['pc'],16)+code.at(int(x['pc'],16)).size for x in actions}
        boundaries={stop,*kernel_sites,*device_sites,*fifo_exits,*action_sites}
        for _ in range(2048):
            result,changed,executed=host.run_traced(entry,stack,code,0x1001000,0xb000,stop_addresses=tuple(p for p in boundaries if not execute or p!=entry),**inputs)
            inputs={};execute=False;visited.update(executed)
            stores.extend(v for v in changed if any(addresses[n]<=v[1]and v[1]+v[2]<=addresses[n]+len(raw)for n,raw in state.items()))
            pc=result.rip
            if pc==stop:break
            host.continuing=True
            if pc in action_sites:
                for index,x in enumerate(actions):
                    if index not in done and int(x['pc'],16)+code.at(int(x['pc'],16)).size==pc:
                        host.write(x['address'],bytes.fromhex(x['hex']));done.add(index)
                entry=pc;execute=True;continue
            if pc in kernel_sites:locks.append(f'{kernel_sites[pc]:08x}');entry=pc+code.at(pc).size;continue
            if pc in fifo_exits:entry=fifo_exits[pc];continue
            assert pc in device_sites
            address=device_sites[pc];instruction=code.at(pc);op=instruction.operands[1]
            if address is None:address=result.rbx&0xffffffff
            if op.type==2:value=op.imm&0xffffffff
            else:
                assert op.type==1
                name={'eax':'rax','ebx':'rbx','ecx':'rcx','edx':'rdx','esi':'rsi','edi':'rdi'}[instruction.reg_name(op.reg)]
                value=getattr(result,name)&0xffffffff
            writes.append(dict(address=f'{address:08x}',value=f'{value:08x}'));entry=pc+instruction.size
        else:raise RuntimeError('Original hardware activation fragment limit')
        assert result.rsp==stack+4 and len(done)==len(actions),label
        after={n:host.read(addresses[n],len(raw)).hex()for n,raw in state.items()}
        if actual:
            expected,_=normalize(actual,'after_hex');assert after=={n:raw.hex()for n,raw in expected.items()},(label,'whole retained output')
            assert writes==[{k:w[k]for k in ('address','value')}for w in actual['observed_device_writes']],(label,'whole device output')
            assert result.rax&0xffffffff==int(actual['actual_return_eax'],16)
        rows.append(dict(label=label,genuine=bool(actual),context=list(context),actions=list(actions),locks=locks,writes=writes,result=f'{result.rax&0xffffffff:08x}',
            regions=[dict(name=n,address=f'{addresses[n]:08x}',before_hex=raw.hex(),after_hex=after[n])for n,raw in state.items()],
            stores=[dict(pc=f'{p:08x}',address=f'{a:08x}',size=n,before=f'{b:0{n*2}x}',after=f'{v:0{n*2}x}')for p,a,n,b,v in stores]))
    for kind,r in captures.items():
        state,addresses=normalize(r,'before_hex');measure(kind,state,addresses,actual=r,context=(r['registers']['fctrl'],r['registers']['mxcsr']))
    def seed(spatial=False,count=1):
        template=captures['spatial'if spatial else'nonspatial'];state,_=normalize(template,'before_hex')
        state['voice_mapping']=bytearray([0x71])*0x4000
        put(state['voice_mapping_pointer'],0,A['voice_mapping']);put(state['device'],0x58,A['ordinary_count']);put(state['device'],0x5c,A['spatial_count'])
        state['voice'][0x64]=count;state['voice'][0x12]&=0xfe
        for at in range(0x88,0x488,4):put(state['device'],at,0)
        put(state['ordinary_count'],0,192);put(state['spatial_count'],0,64)
        return state
    for flags in range(256):
        state=seed();state['voice'][0x12]=flags;measure(f'voice_flags{flags:02x}',state)
    for spatial in (False,True):
        for count in (1,2,3):
            for available in (0,1,2,3,0xffffffff):
                state=seed(spatial,count);put(state['spatial_count'if spatial else'ordinary_count'],0,available)
                measure(f'slots_spatial{int(spatial)}_count{count}_available{available:08x}',state)
            for occupied in (1,2,7,31):
                state=seed(spatial,count);first=0 if spatial else 64
                for index in range(occupied):put(state['device'],0x88+(first+index)*4,0x90000000+index*4)
                measure(f'slots_spatial{int(spatial)}_count{count}_occupied{occupied}',state)
    for count in range(9):
        for bins in ((0,1,2,3,4,5,6,7),(7,6,5,4,3,2,1,0),(2,2,2,2,2,2,2,2),(31,30,16,8,10,9,6,3)):
            state=seed();put(state['data'],0x24,count);state['data'][0x28:0x30]=bytes(bins)
            measure('bins_'+str(count)+'_'+bytes(bins).hex(),state)
    rng=random.Random(0x564f4943)
    for index in range(96):
        state=seed(index%2==1,index%3+1)
        for at in range(0x14,0x40,4):put(state['voice'],at,rng.getrandbits(32))
        put(state['data'],0x18,rng.getrandbits(32));put(state['data'],0x1c,rng.getrandbits(32))
        for at in range(0x30,0xb0,4):put(state['data'],at,rng.getrandbits(32))
        measure(f'varied_registers{index}',state)
    # Device callbacks can mutate reached retained words between commands.
    for pc,offset,value in ((0x381b43,0x14,0x12345678),(0x381b5c,0x1c,0x87654321),(0x37e798,0x64,1)):
        state=seed(count=2)
        action=dict(pc=f'{pc:08x}',address=A['voice']+offset,hex=(bytes([value]).hex()if offset==0x64 else struct.pack('<I',value).hex()))
        if pc in (0x381b43,0x381b5c):action['after_device_words']=1 if pc==0x381b43 else 3
        measure(f'reload_{pc:08x}',state,actions=[action])
    listing=''.join(f'{pc:08x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'for pc in sorted(visited))
    (args.output/'executed-original.asm').write_text(listing)
    fixture=dict(schema='ORIGINAL_SOUND_SDK_HARDWARE_VOICE_V1',boundary=__doc__,aliases=A,cases=rows)
    (args.output/'original.json').write_text(json.dumps(fixture,indent=2)+'\n')
    report=dict(boundary=__doc__,tool_sha256=sha(__file__),image_sha256=sha(path),pairing_report_sha256=sha(args.pairing/'report.json'),capture_pins={n:sha(args.capture/n/'report.json')for n in captures},cases=len(rows),genuine=2,ordered_stores=sum(len(r['stores'])for r in rows),device_words=sum(len(r['writes'])for r in rows),executed_instructions=len(visited),fixture_sha256=sha(args.output/'original.json'),executed_listing_sha256=sha(args.output/'executed-original.asm'),original_host_kernel_execution=False,original_host_DSP_execution=False,instruction_patches=[])
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
