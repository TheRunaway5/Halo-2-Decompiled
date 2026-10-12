#!/usr/bin/env python3
"""Unchanged134240/163D50 and133790 mark slice, plus actual main41 replay.

The live row computes133790 distance and complete4BAF0 from explicit actual
owners/view and cache-correlated metadata. It never supplies captured queue
flags or observed fade bytes as native fade inputs. No guest operation.
"""
import argparse, collections, hashlib, json, random, struct, subprocess, sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from differential_source_object_admission_snapshot import Snapshot
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
NONE=0xffffffff
def pack(v):return struct.pack('<'+'I'*len(v),*(x&NONE for x in v))
def word(raw,at=0):return struct.unpack_from('<I',raw,at)[0]
def half(raw,at=0):return struct.unpack_from('<H',raw,at)[0]
def words(raw):return list(struct.unpack('<'+'I'*(len(raw)//4),raw))
def sha(raw):return hashlib.sha256(raw).hexdigest()
def encode(queue,owners,definitions,models,geometry):return [1,len(queue),len(owners),len(definitions),len(models),len(geometry),*[v for row in queue for v in row],*[v for row in owners for v in row],*[v for row in definitions for v in row],*[v for row in models for v in row],*[v for row in geometry for v in row]]

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--native',type=Path);args=ap.parse_args()
    image=Xbe(ROOT/'assets/disc/default.xbe');assert sha(image.data)==PIN
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for s in image.sections:u.mem_write(s.va,image.section_bytes(s))
    BASE=0x2100000;u.mem_map(BASE,0x500000)
    HEADER,HANDLES,TAGS,OBJECTS,DEFS,MODELS,GEOMETRY,CONTEXT,QUEUE,ARRAYS,STACK,STOP=[BASE+i for i in [0,0x1000,0x100000,0x200000,0x210000,0x220000,0x230000,0x240000,0x250000,0x260000,0x4fe000,0x4ff000]]
    u.mem_write(0x4e0300,pack([HEADER]));u.mem_write(HEADER+0x44,pack([HANDLES]));u.mem_write(0x4e3b44,pack([TAGS]));u.mem_write(CONTEXT+0x14,pack([QUEUE]))
    executed={};calls=collections.Counter();mark=False
    def observe(g,pc,size,unused):
        if mark and pc in [0x133a5a,0x133aa5]:g.emu_stop();return
        if pc not in executed:assert bytes(g.mem_read(pc,size))==image.read(pc,size),(hex(pc),'changed instruction');executed[pc]=size
        if pc in [0x134240,0x163d50]:calls[f'{pc:08X}']+=1
    u.hook_add(UC_HOOK_CODE,observe)
    def put(at,*values):u.mem_write(at,pack(values))
    def original(queue,owners,definitions,models,geometry):
        nonlocal mark
        u.mem_write(HANDLES,bytes(24576));u.mem_write(TAGS,bytes(0x100000))
        for i,(identity,present,definition)in enumerate(owners):
            pointer=OBJECTS+i*0x100;put(HANDLES+(identity&65535)*12+8,pointer if present else 0);put(pointer,definition)
        for i,(identity,present,model)in enumerate(definitions):
            pointer=DEFS+i*0x100;put(TAGS+(identity&65535)*16+8,pointer if present else 0);put(pointer+0x38,model)
        for i,(identity,present,render)in enumerate(models):
            pointer=MODELS+i*0x100;put(TAGS+(identity&65535)*16+8,pointer if present else 0);put(pointer+4,render)
        for i,(identity,count)in enumerate(geometry):
            pointer=GEOMETRY+i*0x100;put(TAGS+(identity&65535)*16+8,pointer);put(pointer+0x74,count)
        flags,ids,other,association=ARRAYS,ARRAYS+0x1000,ARRAYS+0x2000,ARRAYS+0x3000
        put(QUEUE,256,len(queue),flags,ids,other,association)
        fades={}
        for i,(identity,f,aux,a,first,second)in enumerate(queue):
            put(ids+i*4,identity);u.mem_write(flags+i*2,struct.pack('<H',f));put(other+i*4,aux);u.mem_write(association+i*2,struct.pack('<H',a));fades[identity]=(first,second)
        put(STACK,STOP,CONTEXT);u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.emu_start(0x134240,STOP,count=100000)
        assert u.reg_read(UC_X86_REG_EIP)==STOP
        # Actual source branch and OR byte execute without the model-render
        # service at133AB0. Nonzero branch stops before that independent service.
        mark=True
        for i in range(len(queue)):
            identity=word(u.mem_read(ids+i*4,4));first,second=fades[identity]
            u.reg_write(UC_X86_REG_ESI,CONTEXT);u.reg_write(UC_X86_REG_EBP,i);u.reg_write(UC_X86_REG_EBX,0);u.reg_write(UC_X86_REG_ESP,STACK)
            u.mem_write(STACK+0x1cc,bytes([first]));u.emu_start(0x133a51,STOP,count=100)
            assert u.reg_read(UC_X86_REG_EIP)==(0x133a5a if first else 0x133aa5)
        mark=False
        return [len(queue)]+[v for i in range(len(queue))for v in [word(u.mem_read(ids+i*4,4)),half(u.mem_read(flags+i*2,2)),word(u.mem_read(other+i*4,4)),half(u.mem_read(association+i*2,2))]]
    rng=random.Random(0x134240);cases=[];labels=[];expected=[]
    for index in range(1024):
        count=index%129;owners=[];definitions=[];models=[];geometry=[];queue=[]
        for i in range(count):
            identity=0xe0000100+i;did=0xd0000200+i;mid=0xc0000300+i;gid=0xb0000400+i
            path=rng.randrange(9);owners.append([identity,int(path!=1),NONE if path==2 else did]);definitions.append([did,int(path!=3),NONE if path==4 else mid]);models.append([mid,int(path!=5),NONE if path==6 else gid]);geometry.append([gid,rng.choice([-2,-1,0,1,3,16])&NONE])
            first=rng.choice([0,0,1,17,254,255]);second=rng.randrange(256)
            queue.append([NONE if path==0 else identity,rng.getrandbits(16),rng.getrandbits(32),rng.randrange(65536),first,second])
        # Explicit NONE queue rows share a fade result in the source fixture.
        for q in queue:
            if q[0]==NONE:q[4:]=[1,255]
        row=encode(queue,owners,definitions,models,geometry);out=original(queue,owners,definitions,models,geometry)
        cases.append(row);expected.append(out);labels.append('controlled literal-null/NONE/current-model PRT and full-row swap '+str(index))
    # Actual admitted rows are from the independent unchanged admission audit,
    # not copied from the saved render-processed queue.
    replayfile=ROOT/'analysis/multiplayer-shading/shadow-main-collector-replay.json';replay=json.loads(replayfile.read_text())
    assert replay['original_native_match'] and replay['postprocess_matches_capture'] and replay['fresh_owners']==41
    directory=ROOT/'analysis/multiplayer-shading/shadow-live-phase-20261008/capture-opengl'
    capture=json.loads((directory/'capture.json').read_text());phase=next(p for p in capture['phases']if p['name']=='33400-entry');files={f['file']:f for f in phase['files']}
    def data(name):
        record=files['33400-entry-'+name+'.bin'];raw=(directory/record['file']).read_bytes();assert len(raw)==record['size'] and sha(raw)==record['sha256'];return raw
    earlier=Snapshot(ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008');table=word(earlier.read(0x4e3b44,4))
    owners=[];fade_owners=[];definitions={};models={};geometry={};fade_defs={};fade_models={}
    for owner in phase['objects']:
        identity=int(owner['handle'],16);body=data('object-'+owner['handle']);definition=word(body);dp=word(earlier.read(table+(definition&65535)*16+8,4));assert earlier.read(dp,32)==data('definition-'+owner['handle'])
        mid=word(earlier.read(dp+0x38,4));owners.append([identity,1,definition]);definitions[definition]=[definition,1,mid]
        fade_owners.append([identity,word(body,4),word(body,0x14),definition,word(body,0x3c),*words(body[0x30:0x3c])]);fade_defs[definition]=[definition,mid]
        if mid!=NONE:
            hp=word(earlier.read(table+(mid&65535)*16+8,4));gid=word(earlier.read(hp+4,4));models[mid]=[mid,1,gid]
            fade_models[mid]=[mid,*words(earlier.read(hp+0x28,8)),*words(earlier.read(hp+0x34,20)),half(earlier.read(hp+0x4c,2))]
            if gid!=NONE:
                gp=word(earlier.read(table+(gid&65535)*16+8,4));geometry[gid]=[gid,word(earlier.read(gp+0x74,4))]
    fresh=replay['fresh_original_words'];queue=[fresh[1+i*4:5+i*4]+[0,0]for i in range(41)]
    row=encode(queue,owners,list(definitions.values()),list(models.values()),list(geometry.values()));row[0]=2
    view=data('view-context');row += [len(fade_owners),len(fade_defs),len(fade_models),0,*words(view[0x20:0x2c]),word(view,0x4c),*[v for a in fade_owners for v in a],*[v for a in fade_defs.values()for v in a],*[v for a in fade_models.values()for v in a]]
    out=replay['captured_words'][:165]+[41]
    for fade in replay['source_fades']:out += [int(fade['owner'],16),int(fade['metric_bits'],16),*fade['output']]
    cases.append(row);expected.append(out);labels.append('actual33400 main41, native133790 distance/full4BAF0 plus literal134240')
    fixture=ROOT/'tests/fixtures/retail_source_object_render_queue.tsv'
    fixture.write_text('# Unchanged134240/163D50 +133790 conditional flag slice; final live row computes metric/full4BAF0 from explicit source inputs\n'+''.join(' '.join(f'{v:x}'for v in row)+' | '+' '.join(f'{v:x}'for v in out)+'\n'for row,out in zip(cases,expected)))
    failures=[]
    if args.native:
        run=subprocess.run([str(args.native.resolve()),'--probe'],input=''.join(' '.join(f'{v:x}'for v in row)+'\n'for row in cases),text=True,capture_output=True,check=True)
        actual=[[int(v,16)for v in row.split()]for row in run.stdout.splitlines()];assert len(actual)==len(expected)
        failures=[dict(case=i,label=labels[i],source=s,native=n)for i,(s,n)in enumerate(zip(expected,actual))if s!=n]
    report=dict(source_executable_sha256=PIN,native_binary_sha256=sha(args.native.read_bytes())if args.native else None,cases=len(cases),output_words=sum(map(len,expected)),verified_original_instruction_addresses=len(executed),original_calls=dict(calls),
        unchanged_source_ranges={f'{at:08X}':dict(size=size,sha256=sha(image.read(at,size)))for at,size in [(0x134240,0xb9),(0x163d50,0x5d),(0x133a51,9),(0x133a98,13)]},
        fixture_sha256=sha(fixture.read_bytes()),live_replay_sha256=sha(replayfile.read_bytes()),live_main_collector_rows=41,computed_live_fade_pairs=41,live_fade_zero_rows=17,
        raw_main_admission_words=190,live_native_output_words=len(out),failures=failures[:2],boundaries=['Controlled rows execute unchanged original134240/163D50 and133790+133A51/+133A98 flag instructions. Supplied first/second fade bytes are explicit controlled inputs, not inferred from flags.',
        'Live row supplies actual202 owners/main view/fresh admitted41 rows. Native derives each metric and complete4BAF0 first/second bytes; captured33400 flags and unchanged133790-prefix metric/fade outputs remain independent expectations.',
        'Uncaptured main-phase metadata/services come from earlier12474 immutable RAM, byte-correlated against authored cache by the replay audit. Temporal distinction remains; no simultaneous missing-global export claim.',
        'No native successful registration/history/visibility producer or shadow gameplay activation. No guest IO/advance; original instruction hooks only verify, count and stop at explicit slice boundaries.'])
    fixture.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');(ROOT/'analysis/multiplayer-shading/object-render-queue-original.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2));assert not failures

if __name__=='__main__':main()
