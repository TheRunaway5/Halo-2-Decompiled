#!/usr/bin/env python3
"""Execute unchanged voice initialization CPU owners and compare genuine state.

Two whole cold-boot Xbox initializers must match first. Allocation CPU inputs
are actual captured returns/backing, or labeled declared success/failure ABI
controls. Original allocation and kernel implementations are explicit host
fragment boundaries, never claimed executed on the host. Format, routing,
frequency and cursor CPU instructions execute unchanged. No native output
supplies an expected result. Declared cases are not extra gameplay captures.
"""
import argparse,atexit,ctypes,hashlib,json,os,struct,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from functions import _Code
from oracle_sound_sdk_commit import FragmentCpu
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
u32=lambda b,i=0:struct.unpack_from('<I',b,i)[0]
put=lambda b,i,v:struct.pack_into('<I',b,i,v&0xffffffff)
NAMES=('data','descriptor','format','bins','bin_entries','parent','parent_data','replacement_source','source','parameters','buffer')
A={n:0x1001000+i*0x1000 for i,n in enumerate(NAMES)}
A.update({f'allocation_{i}':0x1008004+i*0x1000 for i in range(3)})
# Explicit separate aliases for optional inputs, disjoint from allocator returns.
A.update(replacement_source=0x100e000,parent=0x1006000,parent_data=0x1007000,source=0x100b000,parameters=0x100c000,buffer=0x100d000)
SPEC={'initialize':(0x37cef1,1),'format':(0x37c825,2),'bins':(0x37bc89,1),'buffer':(0x37c8ab,2)}
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--capture',type=Path,required=True)
    parser.add_argument('--pairing',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    image_path=ROOT/'assets/disc/default.xbe';assert sha(image_path)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
    image=Xbe(image_path);code=_Code(image,'DSOUND');pair=json.loads((args.pairing/'report.json').read_text())
    captures={p.parent.name:json.loads(p.read_text())for p in args.capture.glob('*/report.json')};assert set(captures)=={'spatial','nonspatial'}
    for r in captures.values():
        assert r['original_whole_initializer_returned_normally']
        assert not any(r[k]for k in ('instruction_patches','guest_memory_writes','guest_register_writes','supplied_callee_returns'))
    layout=args.output/'user-layout.c';layout.write_text('#include <stddef.h>\n#include <sys/user.h>\n_Static_assert(offsetof(struct user,u_debugreg)==848,"Unsupported ptrace ABI");\nint main(void){return 0;}\n')
    subprocess.run(['cc',str(layout),'-o',str(args.output/'user-layout')],check=True);subprocess.run([str(args.output/'user-layout')],check=True)
    host=FragmentCpu(image,args.output/'original32.elf',0x23f,0x1fa0);atexit.register(host.close);host.trace(6,host.debug_offset+7*8,0)
    stack,stop=0x100f000,0x3000000;rows=[];visited=set();kernel=(0x379cc9,0x379cee,0x379c90,0x379cb2)
    immutable=pair['voice_initialization_bins'][0]['regions']+[dict(address='00385748',hex=image.read(0x385748,18).hex())]
    def normalize(r,field):
        original=[(int(region['address'],16),region['size'],A[region['name']])for region in r['regions']]
        def alias(v):
            for base,size,target in original:
                if base<=v<base+size:return target+v-base
            return v
        state={region['name']:bytearray.fromhex(region[field])for region in r['regions']}
        for raw in state.values():
            for at in range(0,len(raw)-3,4):put(raw,at,alias(u32(raw,at)))
        return state,alias
    def measure(label,kind,state,arguments,allocation_returns=(),context=(0x23f,0x1fa0),actual=None,actions=()):
        host.control_word,host.mxcsr=context
        extents=sorted((A[n],A[n]+len(raw),n)for n,raw in state.items())
        assert all(a[1]<=b[0]for a,b in zip(extents,extents[1:])),(label,'overlapping declared backing')
        for n,raw in state.items():host.write(A[n],raw)
        host.write(stack-0x800,bytes([0x5b])*0x900);entry,count=SPEC[kind];assert count==len(arguments)
        host.write(stack,struct.pack('<'+'I'*(count+1),stop,*arguments));host.continuing=False
        inputs={'rcx':A['data']};pc=entry;stores=[];calls=[];frees=[];guards=[];done=set();execute=True
        action_sites={int(a['pc'],16)+code.at(int(a['pc'],16)).size for a in actions}
        boundaries=(stop,0x37d952,0x37d9ba,*kernel,*action_sites)
        for _ in range(256):
            result,changed,executed=host.run_traced(pc,stack,code,0x1001000,0xe000,
                stop_addresses=tuple(p for p in boundaries if not execute or p!=pc),**inputs)
            inputs={};execute=False;visited.update(executed)
            stores.extend(v for v in changed if any(A[n]<=v[1] and v[1]+v[2]<=A[n]+len(raw)for n,raw in state.items()))
            pc=result.rip
            if pc==stop:break
            host.continuing=True
            if pc in action_sites:
                for i,a in enumerate(actions):
                    if i not in done and int(a['pc'],16)+code.at(int(a['pc'],16)).size==pc:
                        host.write(a['address'],bytes.fromhex(a['hex']));done.add(i)
                execute=True;continue
            if pc==0x37d952:
                args32=list(struct.unpack('<III',host.read(result.rsp+4,12)));index=len(calls)
                assert index<len(allocation_returns),(label,'undeclared allocation')
                identity=allocation_returns[index];calls.append(dict(arguments=[f'{a:08x}'for a in args32],identity=f'{identity:08x}'))
                result.rax=identity;count=3
            elif pc==0x37d9ba:
                frees.append(f'{u32(host.read(result.rsp+4,4)):08x}');count=1
            else:
                guards.append(f'{pc:08x}');count=0
            result.rip=u32(host.read(result.rsp,4));result.rsp+=4+count*4
            host.trace(13,0,ctypes.addressof(result));pc=result.rip;execute=True
        else:raise RuntimeError('Voice initialization fragment limit reached')
        assert result.rsp==stack+4+SPEC[kind][1]*4,(label,hex(result.rsp))
        assert len(done)==len(actions),(label,'declared action was not reached')
        after={n:host.read(A[n],len(raw)).hex()for n,raw in state.items()}
        if actual:
            expected,_=normalize(actual,'after_hex');assert after=={n:raw.hex()for n,raw in expected.items()},(label,'genuine whole retained outputs')
            assert result.rax&0xffffffff==int(actual['actual_return_eax'],16)
            assert [a['arguments']for a in calls]==[a['arguments']for a in actual['allocations']]
        rows.append(dict(label=label,kind=kind,genuine=bool(actual),arguments=[f'{a:08x}'for a in arguments],context=list(context),
            allocations=calls,frees=frees,guards=guards,result=f'{result.rax&0xffffffff:08x}',actions=list(actions),
            regions=[dict(name=n,address=f'{A[n]:08x}',before_hex=raw.hex(),after_hex=after[n])for n,raw in state.items()],
            stores=[dict(pc=f'{p:08x}',address=f'{a:08x}',size=n,before=f'{b:0{n*2}x}',after=f'{v:0{n*2}x}')for p,a,n,b,v in stores]))
    for kind,r in captures.items():
        state,alias=normalize(r,'before_hex');returns=[alias(int(a['identity'],16))for a in r['allocations']]
        measure(kind,'initialize',state,[alias(int(r['arguments'][0],16))],returns,
            (r['registers']['fctrl'],r['registers']['mxcsr']),actual=r)
    def seed(flags=0,channels=2,rate=48000):
        state={'data':bytearray(0xe0),'descriptor':bytearray(24),'format':bytearray(40),'source':bytearray(0xa4)}
        put(state['data'],0,0x41718c);put(state['data'],4,1);put(state['data'],8,flags);put(state['data'],0xb4,A['source'])
        put(state['data'],0xc,1|(channels<<16));put(state['data'],0x10,rate);put(state['descriptor'],0,24)
        put(state['descriptor'],4,flags);put(state['descriptor'],0xc,A['format'])
        struct.pack_into('<HHIIHHH',state['format'],0,1,channels,rate,rate*4&0xffffffff,4,16,0)
        return state
    for flags in (0,0x10,0x2000,0x200010,0x400000,0x80000,0x100000,0x180000):
        for fail in (-1,0,1,2):
            state=seed(flags);put(state['descriptor'],8,32);put(state['descriptor'],0x14,0x12)
            for i in range(3):state[f'allocation_{i}']=bytearray(0x200)
            returns=[A[f'allocation_{i}']if i!=fail else 0 for i in range(3)]
            measure(f'initialize_flags{flags:08x}_failure{fail}','initialize',state,[A['descriptor']],returns)
    for rate in (0,1,7999,8000,11025,22050,32000,44100,47999,48000,48001,96000,192000,0x7fffffff,0x80000000,0xffffffff):
        for control in (0x23f,0x27f,0x63f,0xa3f,0xe3f):
            measure(f'frequency_{rate:08x}_cw{control:04x}','format',seed(rate=rate),[A['format'],0],context=(control,0x1fa0))
    for tag in (1,0x69,0xfffe,0x1234):
        for channels in (1,2,4,6):
            for mask in (0,3,0x3f,0xff,0xffffffff,0x80000000):
                state=seed(channels=channels);struct.pack_into('<H',state['format'],0,tag)
                put(state['format'],0x14,mask);state['format'][0x18:0x28]=image.read(0x3857a8,16)
                measure(f'format_tag{tag:04x}_channels{channels}_mask{mask:08x}','format',state,[A['format'],1])
    for guid in (0x385798,0x3857a8,None):
        state=seed();struct.pack_into('<H',state['format'],0,0xfffe);put(state['format'],0x14,3)
        state['format'][0x18:0x28]=image.read(guid,16)if guid else bytes([0x5b])*16
        measure(f'format_guid_{guid or 0:08x}','format',state,[A['format'],1])
    for flags in (0,0x10,0x200000,0x200010):
        for channels in (1,2,4,6):
            state=seed(flags,channels);put(state['source'],0x58,2)
            measure(f'default_bins_flags{flags:08x}_channels{channels}','bins',state,[0])
    for channels in (1,2,4,6):
        state=seed(channels=channels);state['parent']=bytearray(0x24);state['parent_data']=bytearray(0xe0)
        put(state['data'],0xb0,A['parent']);put(state['parent'],0x1c,A['parent_data']);put(state['parent_data'],8,0x82000)
        put(state['parent_data'],0xd4,0x1f)
        measure(f'parent_channels_{channels}','bins',state,[0])
    for count in (0,2,5,8):
        for fifth in (2,4):
            state=seed(0x10);state['bins']=bytearray(struct.pack('<II',count,A['bin_entries']))
            state['bin_entries']=bytearray(struct.pack('<16I',*[v for i in range(8)for v in (fifth if i==4 else i,0x12340000+i)]))
            put(state['source'],0x58,2)
            measure(f'explicit_bins_count{count}_fifth{fifth}','bins',state,[A['bins']])
    for count in (5,8):
        for existing in (0,2,6):
            state=seed(channels=6);state['bins']=bytearray(struct.pack('<II',count,A['bin_entries']))
            state['bin_entries']=bytearray(struct.pack('<16I',*[v for i in range(8)for v in (31 if i<existing else i,0)]))
            state['parent']=bytearray(0x24);state['parent_data']=bytearray(0xe0)
            put(state['data'],0xb0,A['parent']);put(state['parent'],0x1c,A['parent_data']);put(state['parent_data'],8,0x82000);put(state['parent_data'],0xd4,31)
            measure(f'parent_clip_count{count}_existing{existing}','bins',state,[A['bins']])
    state=seed(0x10);state['replacement_source']=bytearray(0xa4)
    state['bins']=bytearray(struct.pack('<II',5,A['bin_entries']))
    state['bin_entries']=bytearray(struct.pack('<10I',*[v for i in (0,1,3,4,2)for v in (i,0)]))
    measure('bins_source_reload','bins',state,[A['bins']],actions=[dict(pc='0037bd2e',address=A['data']+0xb4,hex=struct.pack('<I',A['replacement_source']).hex())])
    state=seed();state['data'][0xc:0x34]=state['format']
    measure('format_overlapping_payload','format',state,[A['data']+0xc,0])
    for flags in (0,0x80000000):
        for size in (0,1,32):
            for borrowed in (0,A['buffer']):
                state=seed(flags);put(state['data'],0xbc,A['buffer']);state['buffer']=bytearray([0x5b]*32)
                state['allocation_0']=bytearray([0xa5]*0x100)
                measure(f'buffer_flags{flags:08x}_size{size}_borrowed{borrowed:08x}','buffer',state,[borrowed,size],[A['allocation_0']])
    state=seed(0x10);state['replacement_source']=bytearray(0xa4);state['allocation_0']=bytearray(0xa4);state['allocation_1']=bytearray(0x38)
    measure('initialize_source_reload','initialize',state,[A['descriptor']],[A['allocation_0'],A['allocation_1']],
        actions=[dict(pc='0037ce4f',address=A['data']+0xb4,hex=struct.pack('<I',A['replacement_source']).hex())])
    data=dict(source_defaults=[u32(image.read(0x385854,72),i*4)for i in range(18)],environment_defaults=[u32(image.read(0x385828,36),i*4)for i in range(9)],
        pcm_guid=[u32(image.read(0x3857a8,16),i*4)for i in range(4)],adpcm_guid=[u32(image.read(0x385798,16),i*4)for i in range(4)],
        default_format_identity='00385748',spatial_bins_identity='0038589c',parent_bins_identity='0038577c',channel_bins_table_identity='0038576c',
        unsigned_correction_bits=f'{u32(image.read(0x45dbb8,4)):08x}',frequency_reciprocal_bits=f'{u32(image.read(0x45dde0,4)):08x}',immutable=immutable)
    fixture=dict(schema='ORIGINAL_SOUND_SDK_VOICE_INITIALIZATION_CPU_V1',boundary=__doc__,data=data,cases=rows)
    (args.output/'original.json').write_text(json.dumps(fixture,indent=2)+'\n')
    listing=args.output/'executed-original.asm';listing.write_text(''.join(f'{pc:08x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'for pc in sorted(visited)))
    (args.output/'report.json').write_text(json.dumps(dict(boundary=__doc__,tool_sha256=sha(__file__),image_sha256=sha(image_path),pairing_report_sha256=sha(args.pairing/'report.json'),capture_pins={k:sha(args.capture/k/'report.json')for k in captures},cases=len(rows),genuine=2,ordered_stores=sum(len(r['stores'])for r in rows),executed_instructions=len(visited),fixture_sha256=sha(args.output/'original.json'),executed_listing_sha256=sha(listing),instruction_patches=[],original_host_kernel_execution=False,original_host_allocator_execution=False),indent=2)+'\n')
    print(f'PASS two genuine states, {len(rows)} original CPU cases')
if __name__=='__main__':main()
