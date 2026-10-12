#!/usr/bin/env python3
"""Complete untouched165A20 over explicit queues, current owner and tag banks.

The actual32875 root row is independently compared with later32893. Cluster/
instance streams were not exported there; their controls remain explicit.
No source patches, return hooks, guest IO or gameplay activation.
"""
from pathlib import Path
import argparse,collections,hashlib,json,random,struct,subprocess,sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from audit_rendering import Cache
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d';NONE=0xffffffff
def pack(v):return struct.pack('<'+'I'*len(v),*v)
def words(v):return list(struct.unpack('<'+'I'*(len(v)//4),v))
def uint(v,at=0):return struct.unpack_from('<I',v,at)[0]
def ushort(v,at=0):return struct.unpack_from('<H',v,at)[0]
def sha(v):return hashlib.sha256(v).hexdigest()
def row(root,keep,mask,owners,definitions,models,storage,counts):
    return [1,root,keep,mask,len(owners),len(definitions),len(models),*map(len,storage),*[v&65535 for v in counts],
        *[v for item in owners for v in item],*[v for item in definitions for v in item],*[v for item in models for v in item],*[v for group in storage for item in group for v in item]]
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--native',type=Path);ap.add_argument('--count',type=int,default=2048);args=ap.parse_args()
    x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)==PIN
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
    BASE=0x2100000;u.mem_map(BASE,0x500000);STOP=0x3000000;u.mem_map(STOP,4096);STACK=BASE+0x4fff00
    HEADER=BASE;HANDLES=BASE+0x1000;TAGS=BASE+0x100000;OBJECTS=BASE+0x210000;DEFS=BASE+0x230000;MODELS=BASE+0x250000;CONTEXT=BASE+0x270000;QUEUES=BASE+0x280000
    u.mem_write(0x4e0300,pack([HEADER]));u.mem_write(HEADER+0x44,pack([HANDLES]));u.mem_write(0x4e3b44,pack([TAGS]))
    executed={};branches=collections.Counter();calls=0
    def observe(g,pc,size,_):
        nonlocal calls
        if pc==STOP:g.emu_stop();return
        assert bytes(g.mem_read(pc,size))==x.read(pc,size);executed[pc]=size
        if pc==0x165a20:calls+=1
        if pc in [0x165a6b,0x165a91,0x165ab5,0x165acd,0x165b0c,0x165b16,0x165b50,0x165b80,0x165ba4,0x165bad]:branches[f'{pc:08X}']+=1
    u.hook_add(UC_HOOK_CODE,observe)
    def original(input):
        version,root,keep,mask,no,nd,nm,*_=input;assert version==1
        sizes=input[7:10];counts=input[10:13];at=13
        for i in range(no):
            identity,definition,flags,type=input[at:at+4];at+=4;body=OBJECTS+i*0x100
            u.mem_write(body,bytes(0x100));u.mem_write(HANDLES+(identity&65535)*12+8,pack([body]));u.mem_write(body,pack([definition,flags]));u.mem_write(body+0xaa,bytes([type]))
        for i in range(nd):
            identity,model=input[at:at+2];at+=2;ptr=DEFS+i*0x100
            u.mem_write(ptr,bytes(0x100));u.mem_write(TAGS+(identity&65535)*16+8,pack([ptr]));u.mem_write(ptr+0x38,pack([model]))
        for i in range(nm):
            identity,mode=input[at:at+2];at+=2;ptr=MODELS+i*0x100
            u.mem_write(ptr,bytes(0x100));u.mem_write(TAGS+(identity&65535)*16+8,pack([ptr]));u.mem_write(ptr+4,pack([mode]))
        u.mem_write(CONTEXT,bytes(0x40));qheaders=[]
        for j in range(3):
            q=QUEUES+j*0x10000;qheaders.append(q);flag=q+0x100;ids=q+0x2100;other=q+0x6100;association=q+0xa100
            u.mem_write(CONTEXT+[0xc,0x14,0x18][j],pack([q]));u.mem_write(q,pack([sizes[j],counts[j],flag,ids,other,association]))
            for i in range(sizes[j]):
                identity,flags,token,group=input[at:at+4];at+=4
                u.mem_write(ids+i*4,pack([identity]));u.mem_write(flag+i*2,struct.pack('<H',flags));u.mem_write(other+i*4,pack([token]));u.mem_write(association+i*2,struct.pack('<H',group))
        assert at==len(input)
        for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
        u.reg_write(UC_X86_REG_EDI,CONTEXT);u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.mem_write(STACK,pack([STOP,root,keep,mask]));u.emu_start(0x165a20,STOP+1,count=100000)
        assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==STACK+16
        out=[ushort(u.mem_read(q+4,2))for q in qheaders]
        for j,q in enumerate(qheaders):
            flag,ids,other,association=words(u.mem_read(q+8,16))
            for i in range(sizes[j]):out.extend([uint(u.mem_read(ids+i*4,4)),ushort(u.mem_read(flag+i*2,2)),uint(u.mem_read(other+i*4,4)),ushort(u.mem_read(association+i*2,2))])
        return out
    directory=ROOT/'analysis/multiplayer-shading/shadow-live-phase-20261008/capture-opengl';capture=json.loads((directory/'capture.json').read_text());phases={p['name']:p for p in capture['phases']};livefiles={}
    def raw(phase,suffix):
        p=phases[phase];name=phase+'-'+suffix+'.bin';f=next(f for f in p['files']if f['file']==name);v=(directory/name).read_bytes();assert len(v)==f['size']and sha(v)==f['sha256'];livefiles[name]={'sha256':sha(v),'size':len(v),'address':f['address']};return v
    before='32875-secondary-admission';after='32893-after-part-admission';assert phases[before]['clock_tick']==phases[after]['clock_tick']==12475 and phases[before]['view_index']==phases[after]['view_index']==0
    identity=phases[before]['fields']['EBP'];body=raw(before,f'object-{identity:08X}');definition=uint(body);assert phases[after]['fields']['EBP']==identity
    metadata=directory.parent/'fade-metadata-3290A';manifest=json.loads((metadata/'capture.json').read_text());assert manifest['registers']['EIP']==0x3290a and not manifest['initial']['running']and not manifest['final']['running']
    assert manifest['clock_tick']==phases[before]['clock_tick'];metafiles={f['file']:f for f in manifest['files']}
    def fresh(name):
        v=(metadata/name).read_bytes();f=metafiles[name];assert len(v)==f['size']and sha(v)==f['sha256'];livefiles['later3290A/'+name]={'sha256':sha(v),'size':len(v),'address':f['address']};return v
    fresh_owner=fresh(f'object-{identity:08X}.bin');assert uint(fresh_owner)==definition
    d=fresh(f'definition-{definition:08X}.bin');model=uint(d,0x38);m=fresh(f'model-{model:08X}.bin');mode=uint(m,4)
    assert model!=NONE and mode!=NONE
    corroboration=metadata/'comparison.json';correlated=json.loads(corroboration.read_text());assert correlated['source_tick']==12475 and correlated['source_view']==0
    for id,v in [(definition,d),(model,m)]:
        record=next(item for item in correlated['metadata']if item['identity']==f'{id:08X}')
        assert record['sha256']==sha(v)and record['authored_cache_prefix_match']and len(record['baseline_matches'])==2
        assert all(b['matching_prefix_sha256']==sha(v)for b in record['baseline_matches'])
    cache=Cache(ROOT/'assets/disc/maps/zanzibar.map')
    for id,v in [(definition,d),(model,m)]:
        tag=cache.tags[id];at=cache.pointer(tag['address'],cache.magic,tag['size']);assert v==cache.data[at:at+len(v)]
    cache.close()
    queue=raw(before,'secondary-object-queue');n=ushort(queue,4);assert n==1
    before_row=[uint(raw(before,'secondary-queue-handles')),ushort(raw(before,'secondary-queue-flags')),uint(raw(before,'secondary-queue-other')),ushort(raw(before,'secondary-queue-associations'))]
    after_row=[uint(raw(after,'secondary-queue-handles')),ushort(raw(after,'secondary-queue-flags')),uint(raw(after,'secondary-queue-other')),ushort(raw(after,'secondary-queue-associations'))]
    assert before_row[0]==after_row[0]==identity and before_row[1]==0xf000 and after_row[1]==0xe000
    assert ushort(raw(after,'secondary-object-queue'),4)==1
    live=row(identity,0,0,[[identity,definition,uint(body,4),body[0xaa]]],[[definition,model]],[[model,mode]],[[],[before_row],[]],[0,1,0])
    rows=[live];labels=['actual32875 root row / later3290A corroborated definition/hlmt'];source_live=original(live);assert source_live==[0,1,0,*after_row]
    rng=random.Random(0x165a20)
    root=0xe0000040;d_id=0xe0100041;m_id=0xe0100042;mode_id=0xe0100043
    def controlled(type=0,flags=0,keep=0,mask=0,root_model=m_id,root_mode=mode_id,counts=(3,5,3),rootid=root):
        owners=[[root,d_id,0,0],[0xe1000050,d_id,flags,type],[0xe1000051,d_id,flags^0x80000000,(type+32)&255]]
        records=lambda ids:[[id,rng.getrandbits(16),rng.getrandbits(32),rng.getrandbits(16)]for id in ids]
        storage=[records([0,1,2,3]),records([root,0xe1000050,root,0xe1000051,0xe1000050,root]),records([10,11,12,13])]
        return row(rootid,keep,mask,owners,[[d_id,root_model]],[[m_id,root_mode]],storage,counts)
    for type in range(256):
        for flags in [0,0x80000000]:
            for mask in [0,1<<(type&31),0xffffffff]:
                for keep in [0,1]:rows.append(controlled(type,flags,keep,mask));labels.append('byte-type shift,flag31,root-repeat,receiver arg')
    for root_model,root_mode in [(NONE,mode_id),(m_id,NONE),(m_id,mode_id)]:
        for counts in [(-1,-1,-1),(0,0,0),(1,1,1),(3,5,3),(0,5,0)]:
            for keep in [0,1,128,255]:
                rows.append(controlled(32,0x80000000,keep,0,root_model,root_mode,counts));labels.append('signed queuecounts,current modelNONE/modeNONE,nonzero receiverbytes')
    for i in range(args.count):
        counts=(rng.randrange(5),rng.randrange(7),rng.randrange(5));rootid=NONE if i%11==0 else root;rootmodel=NONE if i%13==0 else m_id;rootmode=NONE if i%17==0 else mode_id
        rows.append(controlled(rng.randrange(256),rng.getrandbits(32),rng.randrange(256),rng.getrandbits(32),rootmodel,rootmode,counts,rootid));labels.append('literal source flags/counts/current metadata; inactive prefixes')
    expected=[original(r)for r in rows];failures=[]
    if args.native:
        result=subprocess.run([str(args.native.resolve()),'--probe'],input=''.join(' '.join(f'{v:x}'for v in r)+'\n'for r in rows),text=True,capture_output=True,check=True)
        actual=[[int(v,16)for v in line.split()]for line in result.stdout.splitlines()];assert len(actual)==len(expected)
        for i,(a,e)in enumerate(zip(actual,expected)):
            if a!=e:failures.append({'case':i,'label':labels[i],'differences':[{'word':j,'native':f'{v:08X}','source':f'{w:08X}'}for j,(v,w)in enumerate(zip(a,e))if v!=w]})
    fixture=ROOT/'tests/fixtures/retail_source_shadow_secondary.tsv';fixture.write_text('# Complete unchanged165A20 over explicit metadata/queue banks. Actual root row; other actual streams absent, controls explicitly separate.\n'+''.join(' '.join(f'{v:x}'for v in r)+' | '+' '.join(f'{v:x}'for v in e)+'\n'for r,e in zip(rows,expected)))
    report={'executable_sha256':PIN,'function':'00165A20','function_size':0x194,'function_sha256':sha(x.read(0x165a20,0x194)),'unchanged_source':True,'executed_instructions':len(executed),'calls':calls,'branches':dict(branches),'cases':len(rows),'checked_words':sum(map(len,expected)),
        'native_comparison':bool(args.native),'native_binary_sha256':sha(args.native.read_bytes())if args.native else None,'native_failed_cases':len(failures),'native_failures':failures[:10],'fixture_sha256':sha(fixture.read_bytes()),
        'retained_control':{'before':before,'after':after,'source_tick':12475,'source_view':0,'root':f'{identity:08X}','before_root_row':[f'{v:08X}'for v in before_row],'after_root_row':[f'{v:08X}'for v in after_row],
            'computed_object_count':1,'independently_checked_root_words':5,'later_model_metadata_phase':'3290A','earlier_bank_comparison_sha256':sha(corroboration.read_bytes()),'definition':f'{definition:08X}','model':f'{model:08X}','render_model':f'{mode:08X}','files':livefiles},
        'boundaries':['No guest IO/advance, source patches or service/callee/return stubs. Original165A20 performs all literal body/tag lookups and queue mutations over explicitly relocated valid banks.','Actual32875 root row/count is observed before165A20; independent32893 row/count follows165A20,2CBC0,44720. The computed whole row matches that later observed row. No immediately simultaneous165A20 exit dump is claimed.','Actual definition+38/hlmt+4 comes from later3290A frozen metadata, equal literal cache and independently corroborated earlier pages; actual32875 body definition equals later body. Temporal reconstruction remains explicit.','Actual cluster/instance queue streams were not captured. The retained root control supplies explicitly empty controlled side queues; only its5 object count/row words are an independent phase comparison. Every other case is controlled raw source input, not live admission.','Negative/zero signedcounts retain source skip behavior; current modelNONE clears clustercount alone and preserves underlying storage. Native missing metadata/out-of-span counts reject unresolved services; no original crash/rollback behavior or gameplay activation claim.']}
    output=ROOT/'analysis/multiplayer-shading/shadow-secondary-original.json';output.write_text(json.dumps(report,indent=2)+'\n');fixture.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k not in ['retained_control','boundaries','native_failures','branches']},indent=2));return int(bool(failures))
if __name__=='__main__':raise SystemExit(main())
