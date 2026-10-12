#!/usr/bin/env python3
"""Original construction with separately declared native void-call platform ABI.

Compare original caller behavior against the independent compiled compatibility
lock implementation. This establishes a retained caller path, not original
Xbox kernel register clobbers or all possible callers/inputs.
"""
import argparse
import hashlib
import json
import struct
import threading
import zlib
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, PIN
from oracle_physics_world_event_runtime_20261011 import captured_abi
from export_mp_physics_contact_lifecycle import segments

E = ROOT / 'analysis/multiplayer/sandbox-source/physics-world-bootstrap-runtime-20261011'
S = ROOT / 'analysis/multiplayer/integration/spawn-source'
CHECKPOINT_PIN = '77ee5d4f83e9fb4db1b8b3966b363c683db936ec7ba19b50492e3be1a31b6b52'
FIXTURE_PIN = 'ab9249f2defb3ac9aee9829a0e28c2c216167ff2939873e4af1cae8ce7f849d4'
PAIR_PIN = '7b2500b397173b0a250aed96be86b3a11848b3872fa9382d2a0e11daca6002ec'
LOCK_PIN = '4cff6712198ba763550115d184030d25f69eeb46b414b2a5e73f229e05d2a7f8'
GP = ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def load_state(g, state):
    for name, value in zip(GP, state['gp']):
        g.reg_write(getattr(R,'UC_X86_REG_'+name),value)
    for n, value in enumerate(state['xmm']):
        g.reg_write(getattr(R,f'UC_X86_REG_XMM{n}'),int(value,16))
    for name, key in (('EFLAGS','eflags'),('FPCW','x87_control_word'),('MXCSR','mxcsr'),('FPSW','x87_status'),('FPTAG','x87_tag')):
        g.reg_write(getattr(R,'UC_X86_REG_'+name),state[key])
    top = (state['x87_status'] >> 11) & 7
    for n, value in enumerate(state['x87_values']):
        raw=bytes.fromhex(value)
        g.reg_write(getattr(R,f'UC_X86_REG_FP{(top+n)&7}'),
            (int.from_bytes(raw[:8],'little'),int.from_bytes(raw[8:],'little')))


def run(entry, raw_regions, pair, prior, locks, image, mode, pattern=None):
    g=Uc(UC_ARCH_X86,UC_MODE_32)
    for record, raw in raw_regions:
        g.mem_map(record['start'],record['size'],record['protection'])
        g.mem_write(record['start'],raw)
    for p in (0x9400000,0x9500000):
        if not any(lo<=p<=hi for lo,hi,_ in g.mem_regions()):g.mem_map(p,0x1000)
    raw_lock=bytes.fromhex(locks['compiled_text_hex'])
    assert bytes(g.mem_read(0x9400000,len(raw_lock)))==raw_lock, 'Captured compatibility code differs'
    g.reg_write(R.UC_X86_REG_GDTR,tuple(entry['gdtr']))
    if any(entry['ldtr']):g.reg_write(R.UC_X86_REG_LDTR,tuple(entry['ldtr']))
    for name,value in entry['segment_selectors'].items():
        g.reg_write(getattr(R,'UC_X86_REG_'+name),value)
    load_state(g,entry['entry_abi'])
    assert captured_abi(g)==entry['entry_abi']
    section=image.section('.text')
    assert bytes(g.mem_read(section.va,section.rsize))==image.section_bytes(section)
    code=_Code(image,'.text')
    known={int(p,16) for p in pair['instruction_map']}
    trace,events,calls,packets=[],[],[],[]
    current=None
    exit_entry=None
    prefix_checked=False
    native_identity=threading.get_native_id()
    token=0x96f0000  # Declared opaque host equality token; not a thread record.

    def u(p):return int.from_bytes(g.mem_read(p,4),'little')

    def read(_g,_a,p,n,_v,_u):
        if current is None:
            events.append(dict(kind='R',instruction=g.reg_read(R.UC_X86_REG_EIP),address=p,
                raw=bytes(g.mem_read(p,n)).hex()))

    def write(_g,_a,p,n,v,_u):
        if current is not None:
            for k,b in enumerate(bytes(g.mem_read(p,n))):
                current['private_before'].setdefault(p+k,b)
            return
        events.append(dict(kind='W',instruction=g.reg_read(R.UC_X86_REG_EIP),address=p,
            before=bytes(g.mem_read(p,n)).hex(),written=(v&((1<<(n*8))-1)).to_bytes(n,'little').hex()))

    def hook(_g,p,_n,_u):
        nonlocal current,exit_entry,prefix_checked
        if current is not None:
            if p==current['return_address']:
                pointer=current['pointer']
                current['after_hex']=bytes(g.mem_read(pointer,28)).hex()
                current['reference_exit_abi']=captured_abi(g)
                if mode!='compiled_reference':
                    # Restore only reference-private stack effects. The native
                    # semantic service owns the 28-byte object, not compiler
                    # spills below the source caller's actual stack pointer.
                    private={q:b for q,b in current['private_before'].items() if not pointer<=q<pointer+28}
                    for q,b in segments(private):g.mem_write(q,b)
                    state=json.loads(json.dumps(current['entry_abi']))
                    state['gp'][4]=(state['gp'][4]+8)&0xffffffff
                    if pattern is not None:
                        state['gp'][:3]=[pattern,pattern^0xffffffff,((pattern<<13)|(pattern>>19))&0xffffffff]
                        state['eflags']=(state['eflags']&~0x8d5)|(pattern&0x8d5)
                    load_state(g,state)
                current['exit_abi']=captured_abi(g)
                current.pop('private_before')
                events.append(dict(kind='W',instruction=current['instruction'],address=pointer,
                    before=current['before_hex'],written=current['after_hex'],platform=True))
                calls.append(current)
                current=None
            elif 0x9500000<=p<0x9500100:
                assert threading.get_native_id()==native_identity
                sp=g.reg_read(R.UC_X86_REG_ESP);ret=u(sp);n=(p-0x9500000)//16
                if n==0:result,cleanup=token,0
                elif n in (1,2):
                    target=u(sp+4);before=u(target);result=(before+(1 if n==1 else -1))&0xffffffff
                    for k,b in enumerate(bytes(g.mem_read(target,4))):current['private_before'].setdefault(target+k,b)
                    g.mem_write(target,struct.pack('<I',result));cleanup=4
                else:
                    raise RuntimeError('Retained prefix reached unprovided contended wait/wake/abort')
                g.reg_write(R.UC_X86_REG_EAX,result);g.reg_write(R.UC_X86_REG_ESP,sp+4+cleanup)
                g.reg_write(R.UC_X86_REG_EIP,ret)
                return
            else:
                assert 0x9400000<=p<0x9401000,hex(p)
                return
        if not prefix_checked and len(trace)==len(prior['instructions']):
            assert p==prior['exit_entry'] and trace==prior['instructions']
            assert captured_abi(g)==prior['exit']
            assert [e for e in events if e['kind']=='W']==prior['writes']
            for b in prior['final_banks']:
                assert bytes(g.mem_read(b['address'],len(b['hex'])//2)).hex()==b['hex']
            prefix_checked=True
        if p in (0x80000115,0x80000126):
            sp=g.reg_read(R.UC_X86_REG_ESP);pointer=u(sp+4);ret=u(sp)
            site=trace[-1];expected={0x80000115:(0x2dacad,0x2dacb3),0x80000126:(0x2db25e,0x2db264)}
            assert (site,ret)==expected[p]
            before=bytes(g.mem_read(pointer,28)).hex()
            events.extend((dict(kind='R',instruction=site,address=sp,raw=bytes(g.mem_read(sp,8)).hex(),platform=True),
                dict(kind='R',instruction=site,address=pointer,raw=before,platform=True)))
            current=dict(instruction=site,target=p,return_address=ret,pointer=pointer,entry_abi=captured_abi(g),
                before_hex=before,token=token,private_before={})
            name='RtlEnterCriticalSection' if p==0x80000115 else 'RtlLeaveCriticalSection'
            g.reg_write(R.UC_X86_REG_EIP,int(locks['entries'][name],16))
            return
        if p==prior['return_address'] or p not in known:
            exit_entry=p;g.emu_stop();return
        trace.append(p)
        assert code.at(p).mnemonic not in ('rsqrtss','rdtsc'),'New CPU service owner required'
        if p==0x300100:
            sp=g.reg_read(R.UC_X86_REG_ESP);q=u(sp+4)
            packets.append(dict(input=q,destination=g.reg_read(R.UC_X86_REG_ECX),input_hex=bytes(g.mem_read(q,128)).hex()))

    g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_READ,read);g.hook_add(UC_HOOK_MEM_WRITE,write)
    g.emu_start(entry['entry'],0,count=1000000)
    assert exit_entry is not None and current is None and prefix_checked
    assert bytes(g.mem_read(section.va,section.rsize))==image.section_bytes(section)
    initial,current_bytes={},{}
    inconsistent_before=0
    for event in events:
        p=event['address'];raw=bytes.fromhex(event['raw'] if event['kind']=='R' else event['before'])
        for k,b in enumerate(raw):
            if p+k not in current_bytes:initial[p+k]=current_bytes[p+k]=b
            if current_bytes[p+k]!=b:
                # Reference-only compiler spills change dead stack bytes. Keep
                # this diagnostic; native fixtures require zero such changes.
                inconsistent_before+=1;current_bytes[p+k]=b
        if event['kind']=='W':
            for k,b in enumerate(bytes.fromhex(event['written'])):current_bytes[p+k]=b
    if mode!='compiled_reference':assert inconsistent_before==0,inconsistent_before
    return dict(id=0,phase=prior['phase'],entry=entry['entry'],entry_abi=entry['entry_abi'],
        return_address=prior['return_address'],exit=captured_abi(g),exit_entry=exit_entry,
        complete=exit_entry==prior['return_address'],instructions=trace,
        writes=[e for e in events if e['kind']=='W' and not e.get('platform')],platform_calls=calls,
        packets=packets,estimates=[],timestamps=[],
        initial_banks=[dict(address=p,hex=b.hex()) for p,b in segments(initial)],
        final_banks=[dict(address=p,hex=b.hex()) for p,b in segments(current_bytes)],
        reference_private_byte_diagnostics=inconsistent_before)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--correspondence',type=Path,default=E/'correspondence-v19/complete-report.json')
    args=parser.parse_args();tool_pin=sha(__file__)
    checkpoint_path=E/'disc-original-verified/checkpoint.json';fixture_path=E/'disc-original-verified/fixture.json'
    parent_pair_path=E/'correspondence-v19/complete-report.json';pair_path=args.correspondence
    lock_path=S/'host-lock-new-map-order/report.json'
    for path,pin in ((checkpoint_path,CHECKPOINT_PIN),(fixture_path,FIXTURE_PIN),(parent_pair_path,PAIR_PIN),(lock_path,LOCK_PIN)):
        assert sha(path)==pin
    checkpoint=json.loads(checkpoint_path.read_text());prior=json.loads(fixture_path.read_text())['cases'][0]
    pair_pin=sha(pair_path)
    pair=json.loads(pair_path.read_text());locks=json.loads(lock_path.read_text())['compiled_compatibility_locks']
    parent_pair=json.loads(parent_pair_path.read_text())
    assert pair['disc_1_1_sha256']==PIN
    for key in ('entries','instruction_map','address_map'):
        assert all(pair[key][p]==q for p,q in parent_pair[key].items()),key
    assert sha(S/'host-lock-new-map-order/locks.bin')==locks['compiled_text_sha256']
    assert sha(S/'kernel-sync-world/sources.json')==locks['source_manifest_sha256']
    for row in locks['primary_sources']:assert sha(S/'kernel-sync-world'/row['saved'])==row['sha256']
    image_path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(image_path)==PIN
    image=Xbe(image_path);entry=checkpoint['captured_entry'];regions=[]
    for r in entry['regions']:
        p=checkpoint_path.parent/'memory'/r['file'];assert sha(p)==r['compressed_sha256']
        raw=zlib.decompress(p.read_bytes());assert len(raw)==r['size'] and hashlib.sha256(raw).hexdigest()==r['sha256']
        regions.append((r,raw))
    reference=run(entry,regions,pair,prior,locks,image,'compiled_reference')
    native=run(entry,regions,pair,prior,locks,image,'preserved_native_abi')
    def behavior(c):
        return dict(instructions=c['instructions'],exit_entry=c['exit_entry'],complete=c['complete'],
            writes=[(w['instruction'],w['address'],w['written']) for w in c['writes']],
            platform=[(r['instruction'],r['target'],r['pointer'],r['before_hex'],r['after_hex']) for r in c['platform_calls']])
    assert behavior(reference)==behavior(native),'Native void ABI changes retained source caller behavior'
    variations=[]
    for pattern in (0,0xffffffff,0xaaaaaaaa,0x55555555,0x12345678,0x87654321):
        case=run(entry,regions,pair,prior,locks,image,'perturbed_volatile',pattern)
        assert behavior(case)==behavior(reference),('Retained caller depends on volatile registers',hex(pattern))
        variations.append(dict(pattern=pattern,source_behavior_equal=True,exit_abi=case['exit']))
    assert sha(__file__)==tool_pin and sha(pair_path)==pair_pin and sha(image_path)==PIN
    for path,pin in ((checkpoint_path,CHECKPOINT_PIN),(fixture_path,FIXTURE_PIN),(parent_pair_path,PAIR_PIN),(lock_path,LOCK_PIN)):
        assert sha(path)==pin
    for row in locks['primary_sources']:assert sha(S/'kernel-sync-world'/row['saved'])==row['sha256']
    result=dict(schema=1,image='1.1',tool_sha256=tool_pin,checkpoint_sha256=CHECKPOINT_PIN,
        input_fixture_sha256=FIXTURE_PIN,correspondence_sha256=sha(pair_path),compatibility_report_sha256=LOCK_PIN,
        original_sha256=PIN,cases=[native],compiled_reference_case=reference,declared_volatile_variations=variations,
        source_caller_behavior_equal=True,native_expected_data_used=False,source_instruction_patches=[],
        original_kernel_abi_proven=False,original_kernel_executed=False,production_checkpoint_replay=False,
        live_gameplay_proof=False,scope=__doc__)
    args.output.mkdir(parents=True,exist_ok=False);path=args.output/'fixture.json'
    path.write_text(json.dumps(result,sort_keys=True,separators=(',',':'))+'\n')
    print(json.dumps(dict(fixture_sha256=sha(path),instructions=len(native['instructions']),
        writes=len(native['writes']),platform_calls=len(native['platform_calls']),
        stop=hex(native['exit_entry']),source_behavior_equal=True)))


if __name__=='__main__':main()
