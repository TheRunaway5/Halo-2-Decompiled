#!/usr/bin/env python3
"""Execute both owned multiplayer combat tag postprocessors unchanged.

Raw Lockout metadata and exact tag-table bytes are ABI inputs. Mapping/binding
these inputs does not claim the original cache loader or preceding fixups ran.
No source calls, platform return values, gameplay state or tuned fields are
supplied. All tuning outputs come from each original instruction stream.
"""
import argparse,bisect,csv,hashlib,json,struct,xml.etree.ElementTree as ET
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from capstone.x86 import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,Cache,_Code,_trace,PIN,CACHE_PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN,full_tokens,containing

TAGS,STACK,STOP=0x1080000,0x2010000,0x3000000
OLD=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
SCHEMAS=ROOT/'tools/reference/assembly/src/Assembly/Plugins/Halo2'
IMAGES=(('disc_1_1',OLD,PIN,0x164750,0x11de10,(0x4eb114,0x4e791c,0x4e7920),'analysis/map-pack-update/functions.tsv'),
        ('owned_1_5',NEW_DEFAULT,NEW_PIN,0x165b30,0x11ef70,(0x4f0414,0x4ecc1c,0x4ecc20),'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv'))

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def digest(b):return hashlib.sha256(b).hexdigest()
def h(v):return f'{v:08x}'

def schema_fields(c,tag):
    """Names are schema annotations; offsets/bytes remain original evidence."""
    p=SCHEMAS/(tag['group']+'.xml')
    if not p.exists():return []
    rows=[]
    def walk(elements,base,prefix=''):
        for e in elements:
            if 'offset'not in e.attrib:continue
            off=int(e.get('offset'),0);at=base+off;name=prefix+e.get('name','Unknown')
            if e.tag=='tagblock':
                for n,q in enumerate(c.block(at,int(e.get('elementSize'),0))):walk(e,q,name+f'[{n}]/')
            else:
                size={'float32':4,'degree':4,'int32':4,'uint32':4,'enum32':4,'flags32':4,'rangef':8,'ranged':8,
                      'vector3':12,'point3':12,'colorf':12,'tagRef':8,'int16':2,'uint16':2,'flags16':2,'enum16':2,
                      'int8':1,'uint8':1,'enum8':1,'flags8':1,'stringId':4}.get(e.tag)
                if size:rows.append({'name':name,'type':e.tag,'start':at,'size':size})
    walk(ET.parse(p).getroot(),tag['offset'])
    return rows

def run_one(c,config,out,require_changes=True):
    label,path,pin,entry,caller,globals_,inventory=config
    assert sha(path)==pin
    x=Xbe(path);code=_Code(x,'.text')
    starts={int(r['address'],16)for r in csv.DictReader((ROOT/inventory).open(),delimiter='\t')}
    fn=_trace(code,entry,starts);caller_fn=_trace(code,caller,starts)
    assert not fn.calls and not fn.tables
    ins=[code.at(p)for p in sorted(fn.code)]
    direct=[{'site':h(p),'target':h(i.operands[0].imm)}for p in sorted(caller_fn.code)
            if(i:=code.at(p)).mnemonic=='call'and i.op_str.startswith('0x')]
    assert any(r['target']==h(entry)for r in direct)
    out.mkdir(parents=True,exist_ok=True)
    for a,f in((entry,fn),(caller,caller_fn)):
        (out/(h(a)+'.asm')).write_text(''.join(f'{p:08x} {code.at(p).bytes.hex()} {code.at(p).mnemonic} {code.at(p).op_str}\n'for p in sorted(f.code)))
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
    for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
    metadata_page=c.magic&~0xfff;g.mem_map(metadata_page,((len(c.data)+(c.magic-metadata_page)+0xfff)//0x1000)*0x1000)
    g.mem_write(c.magic,bytes(c.data))
    g.mem_map(TAGS,0x100000);g.mem_map(0x2000000,0x200000);g.mem_map(STOP,0x1000)
    index=c.u(16);table=c.u(index+8)-(c.u(index)-(index+32));count=c.u(index+24)
    table_bytes=bytes(c.data[table:table+count*16]);g.mem_write(TAGS,table_bytes)
    matg=next(t for t in c.tags.values()if t['group']=='matg');scenario=next(t for t in c.tags.values()if t['offset']==c.scenario)
    bindings=[(globals_[0],TAGS),(globals_[1],matg['offset']+c.magic),(globals_[2],c.scenario+c.magic)]
    for p,v in bindings:g.mem_write(p,struct.pack('<I',v))
    g.mem_write(STACK,struct.pack('<I',STOP));g.reg_write(UC_X86_REG_ESP,STACK)
    g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
    tags=sorted((t for t in c.tags.values()if t['offset']is not None),key=lambda t:t['offset'])
    offsets=[t['offset']for t in tags];fields={};stores=[];executed=[]
    def write_hook(_g,access,p,size,v,_u):
        site=g.reg_read(UC_X86_REG_EIP);before=bytes(g.mem_read(p,size));after=(v&((1<<(8*size))-1)).to_bytes(size,'little')
        row={'instruction':h(site),'address':h(p),'size':size,'before_hex':before.hex(),'after_hex':after.hex()}
        if c.magic<=p<c.magic+len(c.data):
            off=p-c.magic;n=bisect.bisect_right(offsets,off)-1;t=tags[n]
            assert t['offset']<=off and off+size<=t['offset']+t['size'],(row,t)
            if t['datum']not in fields:fields[t['datum']]=schema_fields(c,t)
            names=[r for r in fields[t['datum']]if r['start']<=off and off+size<=r['start']+r['size']]
            row.update(region='original_cache_metadata',file_offset=h(off),tag={k:t[k]for k in('datum','group','name','offset','size')},
                       tag_relative_offset=h(off-t['offset']),schema_annotations=[r|{'field_relative_offset':off-r['start']}for r in names])
            if size==4:row.update(before_bits=before[::-1].hex(),after_bits=after[::-1].hex(),before_float=struct.unpack('<f',before)[0],after_float=struct.unpack('<f',after)[0])
        elif 0x2000000<=p<0x2200000:row['region']='declared_abi_stack'
        else:raise AssertionError(('unexpected original write',row))
        stores.append(row)
    g.hook_add(UC_HOOK_MEM_WRITE,write_hook);g.hook_add(UC_HOOK_CODE,lambda _g,p,_s,_u:executed.append(p))
    g.emu_start(entry,STOP,count=5000);assert g.reg_read(UC_X86_REG_EIP)==STOP and g.reg_read(UC_X86_REG_ESP)==STACK+4
    assert set(executed)<=set(fn.code)
    for p in executed:
        i=code.at(p);assert i.bytes==x.read(p,len(i.bytes))==bytes(g.mem_read(p,len(i.bytes)))
    authored_stores=[r for r in stores if r['region']=='original_cache_metadata'];changed=[]
    for t in tags:
        a=t['offset'];n=t['size'];before=bytes(c.data[a:a+n]);after=bytes(g.mem_read(a+c.magic,n))
        if before!=after:
            d=out/'tag-buffers';d.mkdir(exist_ok=True);stem=t['datum']+'-'+t['group'].replace('!','_')
            (d/(stem+'-raw.bin')).write_bytes(before);(d/(stem+'-processed.bin')).write_bytes(after)
            changed.append({'tag':t,'original_va':h(a+c.magic),'raw_hex':before.hex(),'processed_hex':after.hex(),
                            'raw_sha256':digest(before),'processed_sha256':digest(after),'writes':[r for r in authored_stores if r['tag']['datum']==t['datum']]})
    if require_changes:assert len(changed)>0
    return {'image_label':label,'xbe_sha256':pin,'entry':h(entry),'owning_caller':h(caller),'caller_direct_calls':direct,
            'whole_original_instruction_count':len(ins),'executed_instruction_addresses':[h(p)for p in executed],
            'declared_cache_global_bindings':[{'global_slot':h(p),'value':h(v)}for p,v in bindings],
            'source_scenario_gate_i16_at_10':c.h(c.scenario+0x10),'source_scenario_tag':scenario,
            'raw_tag_table_file_offset':h(table),'raw_tag_table_size':len(table_bytes),'raw_tag_table_sha256':digest(table_bytes),
            'ordered_stores':stores,'metadata_store_count':len(authored_stores),'metadata_actual_value_change_count':sum(r['before_hex']!=r['after_hex']for r in authored_stores),
            'changed_tags':changed,'complete_cache_output_sha256':digest(bytes(g.mem_read(c.magic,len(c.data)))),
            'original_calls_or_platform_services_supplied':[],'stack_cleanup':4}

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True);args=ap.parse_args()
    c=Cache(ROOT/'assets/disc/maps/lockout.map');assert sha(c.path)==CACHE_PIN
    try:
        rows=[run_one(c,q,args.output/q[0])for q in IMAGES]
        images=[Xbe(q[1])for q in IMAGES];codes=[_Code(x,'.text')for x in images];entries=[q[3]for q in IMAGES]
        instructions=[[code.at(p)for p in sorted(_trace(code,a,{a}).code)]for code,a in zip(codes,entries)]
        assert full_tokens(instructions[0],images[0])==full_tokens(instructions[1],images[1])
        refs=[]
        for a,b in zip(*instructions):
            for oa,ob in zip(a.operands,b.operands):
                if oa.type==ob.type==X86_OP_MEM and not oa.mem.base and not ob.mem.base and containing(images[0],oa.mem.disp)and containing(images[1],ob.mem.disp):
                    assert oa.size==ob.size;n=oa.size;left=images[0].read(oa.mem.disp,n);right=images[1].read(ob.mem.disp,n)
                    refs.append({'disc_site':h(a.address),'later_site':h(b.address),'disc_address':h(oa.mem.disp),'later_address':h(ob.mem.disp),'size':n,'disc_hex':left.hex(),'later_hex':right.hex(),'bytes_equal':left==right})
        assert all(r['bytes_equal']for r in refs)
        stores=[[{k:v for k,v in r.items()if k!='instruction'}for r in row['ordered_stores']if r['region']=='original_cache_metadata']for row in rows]
        assert stores[0]==stores[1]and rows[0]['complete_cache_output_sha256']==rows[1]['complete_cache_output_sha256']
        r={'tool_sha256':sha(__file__),'cache_path':str(c.path),'cache_sha256':CACHE_PIN,'cache_magic':h(c.magic),
           'schema_sha256':{q:sha(SCHEMAS/(q+'.xml'))for q in('weap','proj','jpt!','vehi','mulg')},
           'scope':'Each complete unchanged original combat postprocessor executes over exact raw Lockout metadata/tag table. Explicit cache-global/ABI input bindings; original cache load, preceding fixups, whole map activation and live gameplay remain outside this fixture. Schema labels are annotations; original offsets/bytes/stores are authority.',
           'normalized_instruction_and_internal_branch_topology_equal':True,'paired_image_memory_operands':refs,
           'ordered_metadata_stores_equal':True,'complete_cache_outputs_equal':True,'images':rows,'native_gameplay_update_applied':False}
        args.output.mkdir(parents=True,exist_ok=True);(args.output/'report.json').write_text(json.dumps(r,indent=2,allow_nan=False)+'\n')
        print(json.dumps({'report_sha256':sha(args.output/'report.json'),'per_image_stores':rows[0]['metadata_store_count'],'actual_changed_stores':rows[0]['metadata_actual_value_change_count'],'changed_tags':len(rows[0]['changed_tags']),'complete_original_outputs_equal':True}))
    finally:c.data.close();c.stream.close()
if __name__=='__main__':main()
