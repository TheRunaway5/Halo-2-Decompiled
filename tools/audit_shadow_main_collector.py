#!/usr/bin/env python3
"""Fresh original admission over actual33400 owner/list/frustum inputs.

All guest input is immutable exported data. Missing phase globals and tag/light
service pages are explicitly supplied by the earlier restored snapshot, and are
reported as temporal corroboration rather than simultaneous live capture.
No guest IO, re-registration, target lookup or original instruction hooks.
"""
import argparse, collections, hashlib, json, re, struct, subprocess, sys
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_UNMAPPED
from unicorn.x86_const import *
from differential_source_object_admission_snapshot import Snapshot, serialize, packed, words, uint, ROOT, PIN
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from audit_rendering import Cache

def h(raw,at=0):return struct.unpack_from('<H',raw,at)[0]
def sha(raw):return hashlib.sha256(raw).hexdigest()

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--native',type=Path,default=ROOT/'build-mp-lightmap/source_object_admission_snapshot_tests')
    ap.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/shadow-main-collector-replay.json')
    ap.add_argument('--fixture-prefix',type=Path,default=ROOT/'tests/fixtures/retail_source_object_admission_shadow')
    args=ap.parse_args()
    directory=ROOT/'analysis/multiplayer-shading/shadow-live-phase-20261008/capture-opengl'
    capture=json.loads((directory/'capture.json').read_text())
    phase=next(p for p in capture['phases']if p['name']=='33400-entry')
    assert not phase['status']['running'] and phase['clock_tick']==12475 and phase['view_index']==0
    earlier=Snapshot(ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008')
    image=Xbe(ROOT/'assets/disc/default.xbe');assert sha(image.data)==PIN
    rawfiles={}
    for f in phase['files']:
        raw=(directory/f['file']).read_bytes();assert len(raw)==f['size'] and sha(raw)==f['sha256']
        rawfiles[f['file']]=(int(f['address'],16),raw)
    # Duplicate exports and snapshot after-export observations must agree.
    ranges=[]
    for name,(address,raw)in rawfiles.items():
        if name.endswith('-after-export.bin'):continue
        for previous,old in ranges:
            first=max(address,previous);last=min(address+len(raw),previous+len(old))
            if first<last:assert raw[first-address:last-address]==old[first-previous:last-previous],name
        ranges.append((address,raw))
    def source_read(address,size):
        out=bytearray(earlier.read(address,size))
        for base,raw in ranges:
            first=max(address,base);last=min(address+size,base+len(raw))
            if first<last:out[first-address:last-address]=raw[first-base:last-base]
        return bytes(out)
    def data(name):return rawfiles[f'33400-entry-{name}.bin'][1]
    context=data('main-context');visible=data('main-visibility');view=data('view-context')
    assert uint(context,0x2a60)==uint(context,0x2a78)==uint(context,0x2a7c)==uint(context,0x2ad0)==0
    assert h(context,8)==0 and h(visible)==1 and h(visible,0xa6c)==h(visible,0x1970)==15
    u=Uc(UC_ARCH_X86,UC_MODE_32);mapped={};executed=set();calls=collections.Counter();events=[];prefix={}
    STOP=0x200ff000;STACK=0x200fe000;u.mem_map(0x20000000,0x100000)
    source_entries=[0x132220,0x133050,0x132e60,0x165010,0x181800,0x132fd0,0x132b10,
        0x134300,0x163c80,0xbec70,0x16e210,0x1348e0,0x134390,0x132a30,0xc3950,0x1cac60,0x16b840,
        0x134240,0x163d50,0x4baf0,0xbaf80,0x322b0c]
    def page(address):
        base=address&~4095
        if base not in mapped:
            raw=source_read(base,4096);u.mem_map(base,4096);u.mem_write(base,raw);mapped[base]=raw
    def missing(g,access,address,size,value,unused):
        for base in range(address&~4095,((address+size-1)&~4095)+4096,4096):page(base)
        return True
    def observe(g,pc,size,unused):
        if prefix and pc==prefix.get('return'):
            prefix['fade']=[bytes(g.mem_read(address,1))[0]for address in prefix['out']];g.emu_stop();return
        if prefix and pc==0x4baf0:
            esp=g.reg_read(UC_X86_REG_ESP);prefix['return']=uint(bytes(g.mem_read(esp,4)))
            prefix['out']=[uint(bytes(g.mem_read(esp+offset,4)))for offset in [4,8]];prefix['metric']=g.reg_read(UC_X86_REG_XMM1)&0xffffffff
        if pc in source_entries:calls[pc]+=1
        if pc==0x133050:events.append(dict(owner=f'{uint(bytes(g.mem_read(g.reg_read(UC_X86_REG_ESP)+4,4))):08X}',cluster_ordinal=uint(bytes(g.mem_read(g.reg_read(UC_X86_REG_ESP)+8,4)))))
        if pc in executed or 0x20000000<=pc<0x20100000:return
        assert bytes(g.mem_read(pc,size))==image.read(pc,size),f'original instruction drift {pc:08X}'
        executed.add(pc)
    u.hook_add(UC_HOOK_MEM_UNMAPPED,missing);u.hook_add(UC_HOOK_CODE,observe)
    def read(address,size):
        for base in range(address&~4095,((address+size-1)&~4095)+4096,4096):
            if not 0x20000000<=base<0x20100000:page(base)
        return bytes(u.mem_read(address,size))
    def write(address,raw):read(address,len(raw));u.mem_write(address,raw)
    def put(address,*values):write(address,packed(values))
    def run(address,argv=(),registers={}):
        for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,registers.get(reg,0))
        put(STACK,STOP,*argv);u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2)
        u.reg_write(UC_X86_REG_FPCW,0x023f);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_MXCSR,0x1fb0)
        u.emu_start(address,STOP,count=20000000)
        assert u.reg_read(UC_X86_REG_EIP)==STOP,f'original{address:X} did not return'
    context_address=rawfiles['33400-entry-main-context.bin'][0]
    # Actual full owner/pool states determine links. Only the independent epoch
    # and destination counts are reset, exactly as the original caller does.
    run(0x1348e0,registers={UC_X86_REG_EDX:context_address})
    put(0x4e7c1c,uint(source_read(0x4e7c1c,4))+1);put(0x4e7c18,1)
    run(0x134390,[context_address])
    prior_count=h(read(context_address+0xa5e,2));prior=[words(read(context_address+0xa60+i*16,16))for i in range(prior_count)]
    put(0x4de2fc,uint(source_read(0x4de2fc,4))+1);put(0x4e0308,uint(source_read(0x4e0308,4))+1);put(0x4e7c18,0)
    # Actual source flags0 admits light-volume services. Their complete pages
    # come from the earlier immutable snapshot, not invented empty lists.
    run(0x132220,[context_address])
    queue=uint(context,0x14);n=h(read(queue+4,2));flags,ids,other,associations=words(read(queue+8,16));result=[n]
    for i in range(n):result.extend([uint(read(ids+i*4,4)),h(read(flags+i*2,2)),uint(read(other+i*4,4)),h(read(associations+i*2,2))])
    na=h(read(context_address+0xa5e,2));result.extend([na,*words(read(context_address+0xa60,na*16))])
    #133720 invokes this literal current-model/PRT classification before its
    # per-owner133790 loop. It swaps complete records; this is not a stable
    # partition of the ordinary owners that were displaced from the prefix.
    run(0x134240,[context_address])
    reordered=[]
    for i in range(n):reordered.append([uint(read(ids+i*4,4)),h(read(flags+i*2,2)),uint(read(other+i*4,4)),h(read(associations+i*2,2))])
    fades=[]
    for ordinal,row in enumerate(reordered):
        prefix['active']=True
        for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
        u.reg_write(UC_X86_REG_ESI,context_address);put(STACK,STOP,ordinal,0,0,0xffffffff,0)
        u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2)
        u.reg_write(UC_X86_REG_FPCW,0x23f);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_MXCSR,0x1fb0)
        u.emu_start(0x133790,STOP,count=100000)
        assert 'fade'in prefix,(ordinal,row)
        fades.append(dict(owner=f'{row[0]:08X}',metric_bits=f'{prefix["metric"]:08X}',output=prefix['fade']))
        if prefix['fade'][0]==0:row[1]|=1
        prefix.clear()
    post=[n]+[value for row in reordered for value in row]+result[1+n*4:]
    protected=[]
    for name,(address,raw)in rawfiles.items():
        if re.fullmatch(r'33400-entry-object-[0-9A-F]{8}\.bin',name)or any(side in name for side in ['collidable','noncollidable']):
            assert read(address,len(raw))==raw,name;protected.append(name)
    assert calls[0x1cac60]==calls[0x16b840]==0
    bsp=uint(source_read(0x4e0348,4));cluster_count=uint(source_read(bsp+0x9c,4));assert cluster_count==55
    fog=[]
    for cluster in range(cluster_count):
        run(0x16e210,registers={UC_X86_REG_ECX:cluster,UC_X86_REG_EDI:uint(source_read(0x4b9f8c,4))});fog.append(u.reg_read(UC_X86_REG_EAX)&255)
    fog.extend([0]*(512-cluster_count))
    snapshot_fixture=args.fixture_prefix.with_suffix('.bin');fixture=bytearray(b'H2OASNP1'+packed([cluster_count,17]))
    names=['object-header','object-handles','object-occupied']
    for side in ['collidable','noncollidable']:
        names.append(side+'-heads')
        for kind in ['entries','refs']:names.extend(f'{side}-{kind}-{suffix}'for suffix in ['header','records','occupied'])
    for name in names:raw=data(name);fixture.extend(packed([len(raw)]));fixture.extend(raw)
    fixture.extend(packed([len(phase['objects'])]))
    for owner in phase['objects']:
        raw=data('object-'+owner['handle']);fixture.extend(packed([int(owner['handle'],16),int(owner['address'],16),len(raw)]));fixture.extend(raw)
    snapshot_fixture.write_bytes(fixture)
    case=dict(capacity=uint(data('main-object-queue')),flags=uint(context,0x2a60),plane=words(context[0x2a88:0x2a98]),groups=h(visible),
        entries=[list(struct.unpack_from('<13H',visible,0xa6e+i*0x1a))for i in range(h(visible,0xa6c))],
        frustums=[words(visible[0x1974+i*0x108+0x60:0x1974+i*0x108+0xd8])for i in range(h(visible,0x1970))],fog=fog,prior=prior)
    native=subprocess.check_output([str(args.native.resolve()),'--probe',str(snapshot_fixture.resolve())],input=' '.join(f'{v:x}'for v in serialize(case))+'\n',text=True)
    native=[int(v,16)for v in native.split()]
    captured=[h(data('main-object-queue'),4)]
    for i in range(captured[0]):captured.extend([uint(data('main-queue-handles'),i*4),h(data('main-queue-flags'),i*2),uint(data('main-queue-other'),i*4),h(data('main-queue-associations'),i*2)])
    captured.extend([h(context,0xa5e),*words(context[0xa60:0xa60+h(context,0xa5e)*16])])
    differences=[dict(word=i,fresh=f'{a:08X}',captured=f'{b:08X}')for i,(a,b)in enumerate(zip(result,captured))if a!=b]
    metadata=[];cache=Cache(ROOT/'assets/disc/maps/zanzibar.map');tag_table=uint(source_read(0x4e3b44,4))
    def literal_tag(identity,fields):
        address=uint(source_read(tag_table+(identity&65535)*16+8,4));assert address
        tag=cache.tags[identity];at=cache.pointer(tag['address'],cache.magic,tag['size'])
        for off,size in fields:assert source_read(address+off,size)==cache.data[at+off:at+off+size],(f'{identity:08X}',off)
        return address
    for owner in phase['objects']:
        body=data('object-'+owner['handle']);definition=uint(body);dp=literal_tag(definition,[(0x38,4)])
        assert earlier.read(dp,32)==data('definition-'+owner['handle'])
        hlmt=uint(source_read(dp+0x38,4));item=dict(owner=owner['handle'],definition=f'{definition:08X}',model=f'{hlmt:08X}')
        if hlmt!=0xffffffff:
            hp=literal_tag(hlmt,[(4,4),(0x28,8),(0x34,20),(0x4c,2)])
            mode=uint(source_read(hp+4,4));item['render_model']=f'{mode:08X}'
            if mode!=0xffffffff:
                mp=literal_tag(mode,[(0x74,4)]);item['prt_count']=uint(source_read(mp+0x74,4))
        metadata.append(item)
    cache.close()
    report=dict(source_executable_sha256=PIN,main_phase='33400-entry',source_tick=phase['clock_tick'],source_view=phase['view_index'],
        capture_manifest_sha256=sha((directory/'capture.json').read_bytes()),earlier_ram_sha256=sha(earlier.ram),prior_bsp_associations=prior_count,
        actual_visibility_clusters=[e[0]for e in case['entries']],fresh_owners=n,fresh_associations=na,captured_owners=captured[0],
        fresh_original_words=result,native_words=native,captured_words=captured,original_native_match=result==native,captured_differences=differences,
        reordered_and_fade_marked_words=post,postprocess_matches_capture=post==captured,source_fades=fades,literal_cache_correlated_metadata=metadata,
        original_calls={f'{a:08X}':calls[a]for a in source_entries},verified_original_instructions=len(executed),protected_live_banks=len(protected),
        raw_snapshot_sha256=sha(fixture),actual_context_sha256=sha(context),actual_visibility_sha256=sha(visible),fog=fog[:55],
        native_binary_sha256=sha(args.native.read_bytes()),raw_snapshot_file=str(snapshot_fixture.relative_to(ROOT)),
        earlier_runtime_inputs={f'{a:08X}':source_read(a,size).hex()for a,size in [(0x4b9f8c,4),(0x4686c4,2),(0x4e0348,4),(0x4e0350,4),(0x4e64c8,4),(0x4e0310,4),(0x4e0314,4),(0x4e030c,4)]},
        append_events=events,boundaries=['Actual33400 main visibility/context/202fullowner/rawspatial pools are supplied without reconstructed registration or sorted lists.',
        'Uncaptured activefog/sample/global service pointers and complete light-volume/tag pages use earlier immutable12474 snapshot; later current phase equality is not assumed.',
        'Fresh source1348E0/134390/132220 executes unchanged. Epoch increments reset duplicate state. Saved33400 queue includes later133790 flag modifications; it is compared separately.',
        'No target lookup/registration/allocation/arithmetic hooks, guest IO or advancement. Native gameplay shadows remain inactive.'])
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    cases_fixture=args.fixture_prefix.with_suffix('.tsv')
    cases_fixture.write_text('# Actual33400 lists/owners/frusta, earlier uncaptured metadata services explicitly documented\n'+' '.join(f'{v:x}'for v in serialize(case))+' | '+' '.join(f'{v:x}'for v in result)+'\n')
    args.fixture_prefix.with_suffix('.provenance.json').write_text(json.dumps({**report,'cases_fixture_sha256':sha(cases_fixture.read_bytes())},indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items()if k not in ['fresh_original_words','native_words','captured_words','reordered_and_fade_marked_words','append_events','fog','boundaries','literal_cache_correlated_metadata','source_fades']},indent=2))
    assert result==native

if __name__=='__main__':main()
