#!/usr/bin/env python3
"""Execute unchanged voice initialization/format/dispatch and activation CPU.

Two genuine whole382F58 cold-boot calls must match all retained bytes and
actual device words first. Slot, mapping, packing, gain, pitch and HRTF CPU
owners execute unchanged. Kernel/FIFO/MMIO are explicit host boundaries;
unported stop/drain/release/attachment owners are separate declared ABI call
boundaries in synthetic cases. Genuine cases do not use those models. No
native output supplies an expected value, nor is live DSP/kernel proof claimed.
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
        assert r['original_whole_initialization_returned_normally']and r['device_writes_executed_normally']
        assert not any(r[k]for k in ('instruction_patches','guest_memory_writes','guest_register_writes','supplied_callee_returns'))
    layout=args.output/'user-layout.c';layout.write_text('#include <stddef.h>\n#include <sys/user.h>\n_Static_assert(offsetof(struct user,u_debugreg)==848,"Unsupported ptrace ABI");\nint main(void){return 0;}\n')
    subprocess.run(['cc',str(layout),'-o',str(args.output/'user-layout')],check=True);subprocess.run([str(args.output/'user-layout')],check=True)
    host=FragmentCpu(image,args.output/'original32.elf',0x23f,0x1fa0);atexit.register(host.close);host.trace(6,host.debug_offset+7*8,0)
    kernel_targets={0x379c90,0x379cb2};kernel_sites={};device_sites={}
    for entry in (0x381aa9,0x37e737,0x381a92,0x381a74,0x3808e4,0x380ae6,0x3808be,0x3812e0,0x382f58,0x380900,0x382fe3,0x38094b,0x382cd4):
        listing=args.pairing/'instructions'/f'{entry:08x}-base.asm'
        for line in listing.read_text().splitlines():
            pc,encoded,*_=line.split();pc=int(pc,16);i=code.at(pc);assert i.bytes==bytes.fromhex(encoded)==host.read(pc,i.size)
            if i.mnemonic=='call'and i.operands[0].type==2 and i.operands[0].imm in kernel_targets:kernel_sites[pc]=i.operands[0].imm
            if i.mnemonic=='mov'and i.operands[0].type==3 and i.operands[0].mem.disp&0xff000000==0xfe000000:
                device_sites[pc]=i.operands[0].mem.disp&0xffffffff
    device_sites[0x3813ec]=None
    fifo_exits={0x381b30:0x381b3e,0x38138f:0x38139d,0x380a16:0x380a23}
    services={0x382dd2:('stop',1),0x381e0e:('wait_pending',0),0x382f96:('release_channels',0),0x382be3:('attach_stream',0),0x382b7e:('attach_buffer',0)}
    entries=dict(initialize=(0x382f58,0),mark=(0x380900,1),reconfigure=(0x382fe3,0),prepare=(0x382cd4,0),format=(0x38094b,0))
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
    def measure(label,state,addresses=None,actual=None,context=(0x23f,0x1fa0),actions=(),kind="initialize",changed=0,service_results=None,service_actions=()):
        addresses=addresses or A;host.control_word,host.mxcsr=context
        extents=sorted((addresses[n],addresses[n]+len(raw),n)for n,raw in state.items())
        assert all(a[1]<=b[0]for a,b in zip(extents,extents[1:])),(label,'overlapping backing')
        for n,raw in state.items():
            address=addresses[n]
            if address<0x1000000:assert os.pwrite(host.memory_fd,raw,address)==len(raw)
            else:host.write(address,raw)
        host.write(stack-0x800,bytes([0x5b])*0x804);entry,count=entries[kind];arguments=[changed]if count else[]
        host.write(stack,struct.pack('<'+'I'*(count+1),stop,*arguments));host.continuing=False
        inputs=dict(rcx=A['voice']);stores=[];writes=[];locks=[];calls=[];execute=True;done=set();service_done=set()
        service_results=service_results or {}
        action_sites={int(x['pc'],16)+code.at(int(x['pc'],16)).size for x in actions}
        boundaries={stop,*kernel_sites,*device_sites,*fifo_exits,*action_sites,*services}
        for _ in range(2048):
            result,changed_stores,executed=host.run_traced(entry,stack,code,0x1001000,0xd000,stop_addresses=tuple(p for p in boundaries if not execute or p!=entry),**inputs)
            inputs={};execute=False;visited.update(executed)
            stores.extend(v for v in changed_stores if any(addresses[n]<=v[1]and v[1]+v[2]<=addresses[n]+len(raw)for n,raw in state.items()))
            pc=result.rip
            if pc==stop:break
            host.continuing=True
            if pc in action_sites:
                for index,x in enumerate(actions):
                    if index not in done and int(x['pc'],16)+code.at(int(x['pc'],16)).size==pc:
                        host.write(x['address'],bytes.fromhex(x['hex']));done.add(index)
                entry=pc;execute=True;continue
            if pc in services:
                assert actual is None,(label,'genuine call unexpectedly reached an unported boundary')
                name,argc=services[pc];args32=list(struct.unpack('<'+'I'*argc,host.read(result.rsp+4,argc*4)))if argc else[]
                value=service_results.get(name)
                if name in ('stop','attach_buffer'):assert value is not None,(label,'undeclared service return',name);result.rax=value
                calls.append(dict(name=name,this=f'{result.rcx&0xffffffff:08x}',arguments=[f'{v:08x}'for v in args32],result=value))
                for index,a in enumerate(service_actions):
                    if index not in service_done and a['after_call']==len(calls):host.write(a['address'],bytes.fromhex(a['hex']));service_done.add(index)
                result.rip=u32(host.read(result.rsp,4));result.rsp+=4+argc*4
                host.trace(13,0,__import__('ctypes').addressof(result));entry=result.rip;execute=True;continue
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
        assert result.rsp==stack+4+len(arguments)*4 and len(done)==len(actions) and len(service_done)==len(service_actions),label
        after={n:host.read(addresses[n],len(raw)).hex()for n,raw in state.items()}
        if actual:
            expected,_=normalize(actual,'after_hex');assert after=={n:raw.hex()for n,raw in expected.items()},(label,'whole retained output')
            assert writes==[{k:w[k]for k in ('address','value')}for w in actual['observed_device_writes']],(label,'whole device output')
            assert result.rax&0xffffffff==int(actual['actual_return_eax'],16)
        rows.append(dict(label=label,kind=kind,changed=changed,service_results=service_results,service_actions=list(service_actions),calls=calls,genuine=bool(actual),context=list(context),actions=list(actions),locks=locks,writes=writes,result=f'{result.rax&0xffffffff:08x}',
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
    for tag in (1,0x69,0xfffe,0x1234):
        for channel in (0,1,2,3,4,5,6,7,31,32,255):
            for bits in (0,8,16,24,32,64,255):
                for active in (0,1):
                    for existing in (0,1,2,3):
                        state=seed();struct.pack_into('<HBB',state['data'],0xc,tag,channel,bits)
                        state['voice'][0x12]=active;state['voice'][0x64]=existing
                        measure(f'format_tag{tag:04x}_channels{channel}_bits{bits}_active{active}_existing{existing}',state,kind='format')
    for table in (0x417254,0x41727c):
        for flags in (0,0x10,0x2000,0x2010,0x180000,0x200010,0x40000,0xffffffff):
            for changed in (0,1,2,0xffffffff):
                state=seed();put(state['voice'],0,table);put(state['data'],8,flags)
                measure(f'mark_table{table:08x}_flags{flags:08x}_changed{changed:08x}',state,kind='mark',changed=changed)
    for status in (0,1,0x7fffffff,0x80004005,0x8007000e,0xffffffff):
        for requested in (1,2,4,6):
            state=seed();state['voice'][0x12]=1;state['data'][0xe]=requested
            actions=[]
            if status<0x80000000 and requested>2:actions=[dict(after_call=3,address=A['voice']+0x12,hex='00')]
            measure(f'reconfigure_status{status:08x}_channels{requested}',state,kind='reconfigure',service_results={'stop':status,'attach_buffer':0},service_actions=actions)
    for flags in (0,0x2000,0x80000,0x100000,0x180000,0x182000):
        for buffer in (0,0x12345678):
            for size in (0,32):
                for status in (0,1,0x8007000e):
                    state=seed();state['voice'][0x12]=1;put(state['data'],8,flags)
                    put(state['data'],0xbc,buffer);put(state['data'],0xc0,size)
                    measure(f'prepare_flags{flags:08x}_buffer{buffer:08x}_size{size}_status{status:08x}',state,kind='prepare',service_results={'attach_buffer':status})
    for flags in (0,0x10,0x2000,0x2010,0x40000,0x180000,0x200010):
        for available in (0,1,3):
            state=seed(flags&0x200010!=0);put(state['data'],8,flags)
            put(state['ordinary_count'],0,available);put(state['spatial_count'],0,available)
            measure(f'initialize_flags{flags:08x}_available{available}',state,service_results={'attach_buffer':0})
    for pc,offset,value in ((0x380a31,0x14,0x12345678),(0x380a40,0x64,1),(0x380a0a,0x12,0)):
        state=seed();state['voice'][0x12]=1;state['voice'][0x64]=2;state['data'][0xe]=4
        action=dict(pc=f'{pc:08x}',address=A['voice']+offset,hex=(bytes([value]).hex()if offset in (0x64,0x12)else struct.pack('<I',value).hex()))
        if pc in (0x380a31,0x380a40):action['after_device_words']=1 if pc==0x380a31 else 2
        measure(f'format_reload_{pc:08x}',state,kind='format',actions=[action])
    # A real lifecycle provider can replace retained data while servicing stop.
    A['replacement_data']=0x100d000
    state=seed();state['voice'][0x12]=1;state['replacement_data']=bytearray(state['data']);state['replacement_data'][0xe]=4
    measure('reconfigure_data_reload',state,kind='reconfigure',service_results={'stop':0},service_actions=[
        dict(after_call=1,address=A['voice']+0x80,hex=struct.pack('<I',A['replacement_data']).hex()),
        dict(after_call=3,address=A['voice']+0x12,hex='00')])
    listing=''.join(f'{pc:08x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'for pc in sorted(visited))
    (args.output/'executed-original.asm').write_text(listing)
    fixture=dict(schema='ORIGINAL_SOUND_SDK_VOICE_DISPATCH_V1',boundary=__doc__,aliases=A,cases=rows)
    (args.output/'original.json').write_text(json.dumps(fixture,indent=2)+'\n')
    report=dict(boundary=__doc__,tool_sha256=sha(__file__),image_sha256=sha(path),pairing_report_sha256=sha(args.pairing/'report.json'),capture_pins={n:sha(args.capture/n/'report.json')for n in captures},cases=len(rows),genuine=2,ordered_stores=sum(len(r['stores'])for r in rows),device_words=sum(len(r['writes'])for r in rows),executed_instructions=len(visited),fixture_sha256=sha(args.output/'original.json'),executed_listing_sha256=sha(args.output/'executed-original.asm'),original_host_kernel_execution=False,original_host_DSP_execution=False,instruction_patches=[])
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
