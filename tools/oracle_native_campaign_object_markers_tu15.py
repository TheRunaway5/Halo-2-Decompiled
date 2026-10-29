#!/usr/bin/env python3
"""Physical original model-marker lookup, quaternion and transform composition."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import oracle_native_campaign_ai_perception_group_link_tu15 as group
import validate_native_geometry_tu15 as geometry
BASE=0x1000000
TAGS,TAG,GROUPS,MARKERS,NODES,VARIANTS,INDICES,DEST=(BASE+v for v in(0x100,0x200,0x300,0x2000,0x3000,0x4000,0x4100,0x5000))
LEFT,RIGHT,Q=BASE+0x6000,BASE+0x7000,BASE+0x8000
sha=common.sha
def prepare(image,meta,scratch):
    owned=scratch/'physical-original';owned.mkdir();assembly='.code32\n';linker='ENTRY(_start)\nSECTIONS{'
    for i,section in enumerate(image.sections):
        source=owned/f'original{i}.bin';raw=image.section_bytes(section);source.write_bytes(raw);name='original'+str(i);assembly+=f'.section .{name},"'+('ax'if section.name=='.text'else'aw')+f'"\n.incbin "{source}"\n';
        if len(raw)<section.vsize:assembly+=f'.space {section.vsize-len(raw)}\n'
        linker+=f'.{name} 0x{section.va:x}:{{*(.{name})}} '
    for name,address,size in [('bank',BASE,0x10000),('stack',geometry.STACK,65536)]:assembly+=f'.section .{name},"aw"\n.space {size}\n';linker+=f'.{name} 0x{address:x}:{{*(.{name})}} '
    assembly+='.section .stop,"ax"\nint3\n.section .bridge,"ax"\n.globl _start\n_start:\nfninit\nint3\njmp _start\n';linker+=f'.stop 0x{geometry.STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n';(owned/'reference.s').write_text(assembly);(owned/'link.ld').write_text(linker);(owned/'metadata.json').write_text(json.dumps(meta))
    for command in (['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_object_markers_tu15.cpp'),'-o',str(owned/'observer')]):
        r=subprocess.run(command,text=True,capture_output=True);assert r.returncode==0,r.stderr
    return owned
def inputs(image):
    rows=[];bits=lambda v:struct.unpack('<I',struct.pack('<f',v))[0]
    def make(entry=0x1e1200,name='marker',variants=False,mapped=False,mirrored=False,capacity=3,count=3,group_index=0,model=0xe1000001,key=0x4000095,match=0,quaternion=(0,0,0,0x3f800000),q=Q,destination=DEST,left=LEFT,right=RIGHT,rounding=0,denorm=0,changes=()):
        n=len(rows);banks={BASE:bytearray((k*31+n*11+7)&255 for k in range(0x10000)),0x4f0000:bytearray([0x53]*4096),0x46a000:bytearray(image.read(0x46a000,4096))}
        def put(p,v,w=4):
            for start,data in banks.items():
                if start<=p and p+w<=start+len(data):data[p-start:p-start+w]=(v&((1<<(w*8))-1)).to_bytes(w,'little');return
            raise ValueError(hex(p))
        for p,v in[(0x4f0414,TAGS),(TAGS+16+8,TAG),(TAG+0x58,count),(TAG+0x5c,GROUPS),(GROUPS+4,count),(GROUPS+8,MARKERS)]:put(p,v)
        for i in range(4):
            put(GROUPS+i*12,key if i==match else 0x5000000+i)
            put(GROUPS+i*12+4,count if i==0 else 0);put(GROUPS+i*12+8,MARKERS)
            m=MARKERS+i*0x24;put(m,0xff if i==0 else i-1,1);put(m+1,i%2,1);put(m+2,i,1)
            for k,v in enumerate((1.5,-2.,3.25)):put(m+4+k*4,bits(v+i))
            for k,v in enumerate(quaternion):put(m+0x10+k*4,v)
            put(m+0x20,bits(0.5+i));put(INDICES+i*4,i)
        for i in range(-128,128):put(VARIANTS+i,(i+1)%2,1)
        for first,scale in((LEFT,2.),(RIGHT,0.5)):
            for k,v in enumerate((scale,1.,0.25,-0.5,0.,1.,0.75,0.5,-0.25,1.,2.,-3.,4.)):put(first+k*4,bits(v))
        for node in range(-1,5):
            first=NODES+node*0x34
            for k,v in enumerate((1.+node/4,1.,0.,0.,0.,1.,0.,0.,0.,1.,1.,2.,3.)):put(first+k*4,bits(v))
        for k,v in enumerate(quaternion):put(Q+k*4,v)
        for p,v,w in changes:put(p,v,w)
        gp=[0x5a100000+n*23+k*3 for k in range(8)];gp[4]=geometry.SP+n%4*4
        args=[]
        if entry==0x144910:gp[0]=destination;gp[1]=q
        elif entry==0x145560:gp[1]=left;gp[2]=right;args=[destination]
        elif entry==0x1e11b0:gp[1]=model;gp[7]=key
        else:gp[0]=group_index;gp[1]=model;args=[VARIANTS if variants else 0,INDICES if mapped else 0,NODES,int(mirrored),destination,capacity&0xffffffff]
        rows.append(dict(id=n,name=name+'_'+str(n),entry=entry,gp=gp,arguments=args,model=model,key=key,group=group_index,variants=VARIANTS if variants else 0,node_indices=INDICES if mapped else 0,node_transforms=NODES,mirrored=mirrored,capacity=capacity,destination=destination,quaternion=q,left=left,right=right,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|(n*3%64),0x1f80|(rounding<<13)|denorm|(n*5%64)],live_x87=[struct.pack('<QH',0x8000000000000000+k*0x10000,0x3fff+k).hex()for k in range(n%4)]))
    exceptional=(0,0x80000000,1,0x800001,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345)
    for rounding in range(4):
        for denorm in(0,0x40,0x8000,0x8040):
            make(0x144910,'q_controls',quaternion=(0x3f000000,0xbf800000,0x40000000,0x3f800000),rounding=rounding,denorm=denorm)
            make(0x145560,'product_controls',rounding=rounding,denorm=denorm)
            make(name='marker_controls',mapped=True,mirrored=bool(denorm),rounding=rounding,denorm=denorm)
    for lane in range(4):
        for value in exceptional:
            q=[0x3f000000,0xbf800000,0x40000000,0x3f800000];q[lane]=value;make(0x144910,'q_numeric',quaternion=tuple(q),rounding=len(rows)%4,denorm=(0,0x40,0x8000,0x8040)[len(rows)%4])
    for left_side in(False,True):
        for lane in range(13):
            for value in exceptional[2:]:make(0x145560,'product_numeric',changes=[((LEFT if left_side else RIGHT)+lane*4,value,4)],rounding=len(rows)%4,denorm=(0,0x40,0x8000,0x8040)[len(rows)%4])
    for destination in(Q,Q+4,Q-8,Q+8):make(0x144910,'q_alias',destination=destination,quaternion=(0x3f000000,0xbf800000,0x40000000,0x3f800000))
    for destination in(LEFT,RIGHT,LEFT+4,RIGHT+0x28,RIGHT-0x10):make(0x145560,'product_alias',destination=destination)
    for variants in(False,True):
        for mapped in(False,True):
            for mirrored in(False,True):
                for capacity in(-1,0,1,2,3,5):make(name='marker_gates',variants=variants,mapped=mapped,mirrored=mirrored,capacity=capacity)
    for count in(-1,0,1,2,3):
        for match in(-1,0,1,2):make(0x1e11b0,'lookup',count=count,match=match)
    for model,key in((0xffffffff,1),(1,0xffffffff),(1,0)):
        make(0x1e11b0,'lookup_lazy',model=model,key=key)
    make(name='no_group',group_index=0xffffffff)
    for count in(-1,0):make(name='no_markers',count=count)
    for v in(0xff,0x80,0x7f):make(name='variant_signed',variants=True,changes=[(MARKERS,v,1)])
    make(name='signed_node',mapped=True,changes=[(INDICES,0xffff,4)])
    make(name='variant_rejected',variants=True,changes=[(MARKERS,0,1),(MARKERS+1,0xee,1)])
    for destination in(MARKERS,MARKERS+8,LEFT+4,NODES+8):make(name='marker_alias',destination=destination,count=1)
    make(0x144910,'fault_seed_q',quaternion=(0x3f000000,0xbf800000,0x40000000,0x3f800000))
    make(0x145560,'fault_seed_product',destination=RIGHT+0x28)
    make(0x1e11b0,'fault_seed_lookup',match=2)
    make(name='fault_seed_marker',variants=True,mapped=True,mirrored=True,count=2)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',action='store_true');a=p.parse_args();assert a.verify or not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);common.ENTRIES=(0x1e11b0,0x1e1200,0x144910,0x145560);meta=common.metadata(image)
    for site in meta['sites']:
        # Actual0F17 MOVHPS memory destinations are eight-byte write-only
        # operands. Capstone reports them as reads; physical banks expose it.
        if site['bytes'].startswith('0f17'):
            assert len(site['operands'])==1 and site['operands'][0]['bytes']==8
            site['operands'][0].update(read=False,write=True)
    owned=prepare(image,meta,Path(os.environ['HALO2_TEST_SCRATCH']));rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in(r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for n in range(len(events)):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));missing=[hex(s['instruction'])for s in meta['sites']if s['instruction']not in visited];assert not missing,missing
    fixture=group.intern_banks(dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows));encoded=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if a.verify:assert a.output.read_bytes()==encoded,'Object-marker original repeat differs'
    else:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(encoded)
    report=dict(passed=True,owners=meta['owners'],cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),all_sites=len(visited),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),original_instructions=sum(r['original_instructions']for r in rows),fixture=str(a.output),fixture_bytes=len(encoded),fixture_sha256=sha(a.output),owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[],source_hashes={name:sha(ROOT/name)for name in[str(Path(__file__).relative_to(ROOT)),'tools/observe_native_campaign_object_markers_tu15.cpp']});a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k not in('owners','source_hashes')}))
if __name__=='__main__':main()
