#!/usr/bin/env python3
"""Complete untouched4BAF0/BAF80/322B0C over explicit owners/model banks.

No root/tag lookup hooks, return hooks or source arithmetic replacements.
Authored model records come from all21 caches, labelled as authored metadata
with controlled runtime fields. Two independently observed calls additionally
use corroborated read-only3290A owner/model metadata, with temporal scope kept
separate from the directly captured incoming metric and output bytes.
"""
from pathlib import Path
import argparse,collections,hashlib,json,math,random,re,struct,subprocess,sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from audit_rendering import Cache
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
NONE=0xffffffff
MAPS=['ascension','beavercreek','burial_mounds','coagulation','colossus','cyclotron','foundation','headlong','lockout','midship','waterworks','zanzibar','containment','warlock','deltatap','turf','backwash','dune','elongation','gemini','triplicate']
def pack(v):return struct.pack('<'+'I'*len(v),*v)
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def flt(v):return struct.unpack('<f',struct.pack('<I',v))[0]
def f32(v):return flt(bits(v))
def words(raw):return list(struct.unpack('<'+'I'*(len(raw)//4),raw))
def sha(raw):return hashlib.sha256(raw).hexdigest()
def model(identity,end,start,cutoffs,count):return [identity,bits(end),bits(start),*map(bits,cutoffs),count]
def fixture(current,root,metric,radius=.44,flags=0,modifier=0,depth=0,root_none=False,current_none=False):
    handles=[0xe2ff0090,0xe3000140,0xe4000150,0xe2e60077][:depth+1]
    assert depth<=3
    definitions=[[0xdadaf000,NONE if current_none else current[0]]]
    if depth:definitions.append([0xdadaf001,NONE if root_none else root[0]])
    owners=[]
    for i,handle in enumerate(handles):owners.append([handle,flags if i==0 else 1<<19,handles[i+1]if i+1<len(handles)else NONE,
        definitions[0][0]if i==0 else definitions[-1][0],bits(radius if i==0 else 100)])
    models=[current]+([root]if root[0]!=current[0]else [])
    return [1,len(owners),len(definitions),len(models),handles[0],modifier,bits(metric),*[v for o in owners for v in o],*[v for d in definitions for v in d],*[v for m in models for v in m]]
def authored_models():
    result=[];inventory=[]
    for name in MAPS:
        file=ROOT/'assets/disc/maps'/f'{name}.map'
        if not file.exists():file=ROOT/'assets/map-pack/maps'/f'{name}.map'
        cache=Cache(file);items=[]
        try:
            for t in cache.tags.values():
                if t['group']!='hlmt' or not t['address']:continue
                at=cache.pointer(t['address'],cache.magic,t['size']);cache.require(at,0x4e)
                data=[t['datum'],*words(cache.data[at+0x28:at+0x30]),*words(cache.data[at+0x34:at+0x48]),cache.u16(at+0x4c)]
                assert len(data)==9 and all(math.isfinite(flt(v))for v in data[1:8])
                items.append({'datum':f'{t["datum"]:08X}','name':t['name'],'words':[f'{v:08X}'for v in data]});result.append((name,data))
            inventory.append({'map':name,'cache':str(file.relative_to(ROOT)),'authored_hlmt_records':len(items),'models':items})
        finally:cache.close()
    return result,inventory
def live_observations():
    directory=ROOT/'analysis/multiplayer-shading/shadow-live-phase-20261008/capture-opengl'
    capture=json.loads((directory/'capture.json').read_text());phase=next(p for p in capture['phases']if p['name']=='33400-entry')
    files={f['file']:f for f in phase['files']};observations=[];rows=[]
    metadata=directory.parent/'fade-metadata-3290A';manifest=json.loads((metadata/'capture.json').read_text())
    assert manifest['initial']['running'] is False and manifest['final']['running'] is False
    assert manifest['registers']['EIP']==0x3290a and manifest['clock_tick']==phase['clock_tick'] and manifest['view_index']==phase['view_index']
    freshfiles={f['file']:f for f in manifest['files']}
    def fresh(name):
        raw=(metadata/name).read_bytes();record=freshfiles[name]
        assert len(raw)==record['size'] and sha(raw)==record['sha256']
        return raw
    modifier=fresh('view-modifier.bin');assert len(modifier)==1 and modifier[0]==manifest['view_modifier']
    assert fresh('clock-before.bin')==fresh('clock-after.bin')
    cache=Cache(ROOT/'assets/disc/maps/zanzibar.map')
    for value in capture['fade_observations']:
        handle=int(value['owner'],16);match=re.search(r'XMM01=[0-9a-fA-F]{16}\s+[0-9a-fA-F]{8}([0-9a-fA-F]{8})',value['entry_registers']);assert match
        owner_file=f'33400-entry-object-{handle:08X}.bin';definition_file=f'33400-entry-definition-{handle:08X}.bin'
        owner=(directory/owner_file).read_bytes();definition=(directory/definition_file).read_bytes()
        for file,raw in [(owner_file,owner),(definition_file,definition)]:assert sha(raw)==files[file]['sha256']and len(raw)==files[file]['size']
        current=fresh(f'object-{handle:08X}.bin');field_comparisons={}
        for offset,name in [(0,'definition'),(0x14,'parent'),(0x3c,'current_radius')]:
            assert current[offset:offset+4]==owner[offset:offset+4]
            field_comparisons[name]=f'{words(current[offset:offset+4])[0]:08X}'
        prior_flags=words(owner[4:8])[0];current_flags=words(current[4:8])[0]
        assert (prior_flags^current_flags)&(1<<19)==0
        field_comparisons['consumed_flag19']=bool(prior_flags&(1<<19))
        definitionid=words(current[:4])[0];full_definition=fresh(f'definition-{definitionid:08X}.bin')
        assert full_definition[:len(definition)]==definition
        modelid=words(full_definition[0x38:0x3c])[0];full_model=fresh(f'model-{modelid:08X}.bin')
        for identity,raw in [(definitionid,full_definition),(modelid,full_model)]:
            tag=cache.tags[identity];at=cache.pointer(tag['address'],cache.magic,tag['size']);assert raw==cache.data[at:at+len(raw)]
        flags,parent=prior_flags,words(current[0x14:0x18])[0]
        assert parent==NONE and flags&(1<<19)==0
        def lane(registers,index):
            m=re.search(rf'XMM0{index}=[0-9a-fA-F]{{16}}\s+[0-9a-fA-F]{{8}}([0-9a-fA-F]{{8}})',registers);assert m;return int(m.group(1),16)
        width=bits(f32(flt(words(current[0x3c:0x40])[0])*7))
        assert lane(value['entry_registers'],2)==lane(value['entry_registers'],3)==0
        assert lane(value['exit_registers'],2)==width and lane(value['exit_registers'],3)==bits(1)
        # The unchanged force path4BB2F..4BB39 and modelNONE path perform no
        # SSE writes. The actual XMM2/3 transition establishes ordinary entry,
        # and therefore the prior modifier0, independently of the later byte.
        assert modifier[0]==0
        modelrow=[modelid,*words(full_model[0x28:0x30]),*words(full_model[0x34:0x48]),struct.unpack_from('<H',full_model,0x4c)[0]]
        row=[1,1,1,1,handle,modifier[0],int(match.group(1),16),handle,flags,parent,definitionid,words(current[0x3c:0x40])[0],definitionid,modelid,*modelrow]
        rows.append((row,value['fade_outputs']))
        observations.append({'owner':value['owner'],'entry_XMM1_bits':match.group(1).upper(),'observed_output_bytes':value['fade_outputs'],
            'owner_fields_from_phase':'33400-entry','definition':f'{words(owner[:4])[0]:08X}','flags':f'{words(owner[4:8])[0]:08X}',
            'parent':f'{words(owner[0x14:0x18])[0]:08X}','current_radius_bits':f'{words(owner[0x3c:0x40])[0]:08X}',
            'current_definition_model':f'{modelid:08X}',
            'owner_file':owner_file,'owner_file_sha256':sha(owner),'definition_file':definition_file,'definition_file_sha256':sha(definition),
            '3290A_matching_owner_fields':field_comparisons,'3290A_definition_sha256':sha(full_definition),'3290A_model_sha256':sha(full_model),
            'flags_phase_difference':{'33400_entry':f'{prior_flags:08X}','3290A':f'{current_flags:08X}','xor':f'{prior_flags^current_flags:08X}','consumed_mask':'00080000','fixture_flags_from':'literal33400-entry'},
            'definition_model_field_boundary':'earlier prefix ends before+38; complete fresh3290A prefix supplies literal field',
            'matching_full_hlmt_metadata':'fresh3290A read-only payload, byte-identical to literal cache, earlier definition prefix agrees',
            'view_modifier':modifier[0],
            'view_modifier_boundary':'directly exported at3290A; callentry0 independently follows from actual XMM2/3 zero-to-radius*7/1 transition, impossible on unchanged force/modelNONE paths. No simultaneous byte export claim.',
            'ordinary_branch_register_proof':{'entry_XMM2':'00000000','entry_XMM3':'00000000','exit_XMM2':f'{width:08X}','exit_XMM3':'3F800000','source_writes':['0004BBB7','0004BBBF','0004BBC4'],'force_path_no_sse_writes':['0004BB2F','0004BB39']},
            'fixture_row':[f'{v:08X}'for v in row]})
    cache.close()
    return {'capture_manifest':str((directory/'capture.json').relative_to(ROOT)),'capture_manifest_sha256':sha((directory/'capture.json').read_bytes()),
        'metadata_manifest':str((metadata/'capture.json').relative_to(ROOT)),'metadata_manifest_sha256':sha((metadata/'capture.json').read_bytes()),
        'metadata_phase':{'entry':'0003290A','tick':manifest['clock_tick'],'frame':manifest['frame'],'view':manifest['view_index'],'modifier':modifier[0]},'observations':observations,
        'boundary':'Incoming XMM1 and outputs are direct observed4BAF0 calls. Definition/parent/radius and consumed flag19 are exact across33400-entry and fresh3290A; Chief unusedflag30 clears, retained flags remain fixtureinput. Definition prefixes agree and full3290A tags equal literal cache. Both parentsNONE. Actual XMM2/3 transition proves ordinary branch/modifier0; the literal globalbyte and complete model metadata were exported later. Source replay/native equality is corroborated reconstruction, not a simultaneous fadeentrysnapshot.'},rows
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--native',type=Path);args=ap.parse_args()
    x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)==PIN
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
    BASE=0x2100000;u.mem_map(BASE,0x500000);STOP=0x3000000;u.mem_map(STOP,4096)
    HEADER=BASE;HANDLES=BASE+0x1000;TAGS=BASE+0x100000;OBJECTS=BASE+0x210000;DEFS=BASE+0x230000;MODELS=BASE+0x250000;OUT=BASE+0x270000;STACK=BASE+0x4fff00
    u.mem_write(0x4e0300,pack([HEADER]));u.mem_write(HEADER+0x44,pack([HANDLES]));u.mem_write(0x4e3b44,pack([TAGS]))
    calls=collections.Counter();executed={};branches=collections.Counter();selected=collections.Counter()
    def observe(g,pc,size,_):
        if pc==STOP:g.emu_stop();return
        if pc not in executed:assert bytes(g.mem_read(pc,size))==x.read(pc,size);executed[pc]=size
        if pc in [0x4baf0,0xbaf80,0x322b0c]:calls[f'{pc:08X}']+=1
        if pc in [0x4bb2f,0x4bb59,0x4bb78,0x4bba2,0x4bbf5,0x4bc11,0x4bc48,0x4bc6a]:branches[f'{pc:08X}']+=1
        if pc==0x4bbb3:selected[g.reg_read(UC_X86_REG_ESI)]+=1
    u.hook_add(UC_HOOK_CODE,observe)
    def original(row):
        version,no,nd,nm,handle,modifier,metric=row[:7];assert version==1;at=7;models={}
        for i in range(no):
            identity,flags,parent,definition,radius=row[at:at+5];at+=5;obj=OBJECTS+i*0x100
            u.mem_write(obj,bytes(0x100));u.mem_write(HANDLES+(identity&65535)*12+8,pack([obj]));u.mem_write(obj,pack([definition,flags]));u.mem_write(obj+0x14,pack([parent]));u.mem_write(obj+0x3c,pack([radius]))
        for i in range(nd):
            identity,modelid=row[at:at+2];at+=2;ptr=DEFS+i*0x100;u.mem_write(ptr,bytes(0x100));u.mem_write(TAGS+(identity&65535)*16+8,pack([ptr]));u.mem_write(ptr+0x38,pack([modelid]))
        for i in range(nm):
            identity,end,start,*tail=row[at:at+9];at+=9;ptr=MODELS+i*0x100;u.mem_write(ptr,bytes(0x100));models[ptr+0x28]=identity
            u.mem_write(TAGS+(identity&65535)*16+8,pack([ptr]));u.mem_write(ptr+0x28,pack([end,start]));u.mem_write(ptr+0x34,pack(tail[:5]));u.mem_write(ptr+0x4c,struct.pack('<H',tail[5]))
        assert at==len(row)
        for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
        for i in range(8):u.reg_write(UC_X86_REG_XMM0+i,0)
        u.reg_write(UC_X86_REG_EBX,handle);u.reg_write(UC_X86_REG_XMM1,metric);u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2)
        u.reg_write(UC_X86_REG_FPCW,0x023f);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_MXCSR,0x1fb0)
        u.mem_write(0x4ba019,bytes([modifier]));u.mem_write(OUT,b'\xab\xcd');u.mem_write(STACK,pack([STOP,OUT,OUT+1]));u.emu_start(0x4baf0,STOP+1,count=10000)
        assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==STACK+12
        return list(u.mem_read(OUT,2))
    rows=[];labels=[]
    def add(row,label):rows.append(row);labels.append(label)
    cutoffs=[10,5,2.8,1.8,0];base=model(0xe0000040,40,30,cutoffs,0);other=model(0xe0000041,20,10,[40,30,20,10,5],2)
    for count in [0,1,2,3,4,5,65535]:
        m=model(0xe0000040,40,30,cutoffs,count);cutoff=flt(m[3+min(4-count,4)])if count<4 else flt(0x7f7fffff)
        for radius in [0,.0001,.44,2.5]:
            width=f32(f32(radius)*7)
            metrics=[-1,0,10,30,35,40]
            if count<4:metrics += [f32(cutoff+f32(width*k))for k in [-.1,0,.5,.904,.905,.906,.999,1,1.01]]
            else:metrics += [flt(0x7f7ffffe),flt(0x7f7fffff)]
            for metric in metrics:
                for depth,root_none in [(0,False),(1,True),(1,False),(3,False)]:add(fixture(m,other,metric,radius,depth=depth,root_none=root_none),'controlled roots/cutoff/byte boundaries')
    for flags in [0,1<<19,(1<<19)|0x4000]:
        for modifier in [0,1,0x80,255]:
            for missing in [False,True]:add(fixture(base,other,10,flags=flags,modifier=modifier,depth=3,current_none=missing),'force precedence/current modelNONE')
    for end,start in [(0,0),(-1,2),(40,50),(.00001,.000005),(flt(0x7f7fffff),-flt(0x7f7fffff))]:
        m=model(0xe0000040,end,start,cutoffs,0)
        for metric in [-1,0,start,f32(end*.5),end]:add(fixture(m,other,metric),'first byte finite/end/start arithmetic')
    m=model(0xe0000040,40,30,[-flt(0x7f7fffff)]*5,0)
    add(fixture(m,other,flt(0x7f7fffff),flt(0x7f7fffff)),'finite input/intermediate NaN322B0C')
    authored,inventory=authored_models()
    for i,(mapname,m) in enumerate(authored):
        _,nextmodel=authored[(i+1)%len(authored)]
        if authored[(i+1)%len(authored)][0]!=mapname:nextmodel=m
        if nextmodel[0]==m[0]:nextmodel=m
        assert nextmodel[0]==m[0] or (nextmodel[0]&65535)!=(m[0]&65535)
        count=m[-1];cutoff=flt(m[3+min(4-count,4)])if count<4 else flt(0x7f7fffff)
        metric=f32(cutoff+f32(.44*7*.5))if count<4 else 10
        for depth,root_none,flags,modifier,selectedmetric in [(0,False,0,0,0),(0,False,0,0,metric),(1,True,0,0,metric),(3,False,0,0,metric),
            (0,False,0,0,flt(m[1])),(0,False,0,0,flt(m[2])),(3,False,1<<19,0,10),(3,False,0,0x80,10)]:
            add(fixture(m,nextmodel,selectedmetric,depth=depth,root_none=root_none,flags=flags,modifier=modifier),f'{mapname} authored hlmt {m[0]:08X}/controlled owner')
    live,live_rows=live_observations()
    for row,observed in live_rows:
        add(row,'corroborated observed4BAF0 metric/bytes with fresh3290A owner/model metadata')
    expected=[original(row)for row in rows]
    assert expected[-len(live_rows):]==[observed for row,observed in live_rows]
    function_ranges=[(0x4baf0,0x1c9),(0xbaf80,0x28),(0x322b0c,0x75)]
    function_hashes={}
    for address,size in function_ranges:
        raw=bytes(u.mem_read(address,size));assert raw==x.read(address,size)
        function_hashes[f'{address:08X}']={'size':size,'unchanged_sha256':sha(raw)}
    fixturefile=ROOT/'tests/fixtures/retail_source_model_fade.tsv';temporary=fixturefile.with_suffix('.tmp');temporary.write_text('# Complete unchanged4BAF0/BAF80/322B0C, explicit owners/global/XMM1/model banks. Authored controlled cases plus two corroborated observed calls; exact temporal scope in provenance.\n'+''.join(' '.join(f'{v:x}'for v in row)+' | '+' '.join(f'{v:x}'for v in out)+'\n'for row,out in zip(rows,expected)));temporary.replace(fixturefile)
    inventoryfile=ROOT/'analysis/multiplayer-shading/model-fade-authored-banks.json';inventoryfile.write_text(json.dumps(inventory,indent=2)+'\n')
    livefile=ROOT/'analysis/multiplayer-shading/model-fade-live-observations.json';livefile.write_text(json.dumps(live,indent=2)+'\n')
    failures=[]
    if args.native:
        result=subprocess.run([str(args.native.resolve()),'--probe'],input=''.join(' '.join(f'{v:x}'for v in row)+'\n'for row in rows),text=True,capture_output=True,check=True)
        actual=[[int(v,16)for v in line.split()]for line in result.stdout.splitlines()];assert len(actual)==len(expected)
        failures=[{'case':i,'label':labels[i],'source':b,'native':a}for i,(a,b)in enumerate(zip(actual,expected))if a!=b]
    constant_addresses=[0x45dbc0,0x45dda8,0x44aed4,0x45df60,0x45dc38]
    report={'executable_sha256':PIN,'source_entries':['0004BAF0','000BAF80','00322B0C'],'source_unchanged':True,
        'cases':len(rows),'output_bytes':len(rows)*2,'native_comparison':bool(args.native),'native_failures':failures[:20],
        'authored_maps':21,'authored_model_records':len(authored),'authored_case_count':len(authored)*8,'corroborated_observed_cases':len(live_rows),'observed_call_bytes_reproduced':[out for row,out in live_rows],
        'calls':dict(calls),'observed_branch_addresses':dict(branches),'executed_instruction_addresses':len(executed),'unchanged_function_bytes':function_hashes,
        'source_constants':{f'{at:08X}':x.read(at,4).hex()for at in constant_addresses},
        'fixture_sha256':sha(fixturefile.read_bytes()),'authored_bank_inventory_sha256':sha(inventoryfile.read_bytes()),'live_observation_sha256':sha(livefile.read_bytes()),
        'input_contract':'Full valid owner handles with literal flags/parent/definition/current radius; literal definition model pointer fields; selected current/root hlmt data; incoming XMM1; actual4BA019 byte. Original lookups execute over explicitly relocated tables.',
        'boundaries':['No live guest IO or advancement by this tool. Two observed metric/output pairs have corroborated fresh3290A tags/body fields; metadata/modifier were not exported simultaneously atfadeentry. Exact perfield/time provenance stays separate.',
            'Authored all21 model metadata uses controlled owner fields/links/metric/global. No gameplay admission or successful-registration chronology claim.',
            'No source code patches, BAF80/lookup hooks, arithmetic hooks, output/service return stubs. The hook only observes instruction addresses and stops after original ret8.',
            'Native missing/nonfinite read fields, cycles, negative sphere radius and missing full valid handles reject unresolved input; these are bounded service validation, not new original crash behavior.']}
    out=ROOT/'analysis/multiplayer-shading/model-fade-original.json';out.write_text(json.dumps(report,indent=2)+'\n');fixturefile.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items()if k not in ['boundaries','input_contract']},indent=2));assert not failures
if __name__=='__main__':main()
