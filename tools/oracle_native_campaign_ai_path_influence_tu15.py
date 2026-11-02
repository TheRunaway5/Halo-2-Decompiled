#!/usr/bin/env python3
"""Unchanged physical27C520 projection/falloff and both real inverse transforms.

Inputs describe retained points, authored influence fields and FP contexts.
No instruction, private intermediate, dependency return or result is supplied.
"""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import oracle_native_campaign_ai_point_geometry_tu15 as points
import validate_native_geometry_tu15 as geometry

ENTRIES=(0x27c520,0x218960,0x145200,0x218fe0)
BASE,ORIGIN,ENDPOINT,OUTPUT,FRAMES,POOL,OBJECTS,BODY=points.BASE,points.SOURCE,points.SECOND,points.DEST,points.FRAMES,points.POOL,points.OBJECTS,points.BODY
INFLUENCE=BASE+0x800
bits=points.bits


def prepare(image,metadata,scratch):
    owned=points.prepare(image,metadata,scratch)
    assembly=(owned/'reference.s').read_text()+'.section .falloff,"aw"\n.space 4096\n'
    (owned/'reference.s').write_text(assembly)
    linker=(owned/'link.ld').read_text().replace('.stop 0x','.falloff 0x456000:{*(.falloff)} .stop 0x')
    (owned/'link.ld').write_text(linker)
    for command in (['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],
                    ['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],
                    ['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_path_influence_tu15.cpp'),'-o',str(owned/'observer')]):
        run=subprocess.run(command,text=True,capture_output=True);assert not run.returncode,run.stderr
    return owned


def inputs(image):
    rows=[]
    def make(name,origin=(0.,0.,0.),endpoint=(2.,0.,0.),center=(-1.,0.,0.),radius=3.,weight=2.,frame=0xffff,state='valid',scale=1.,output=OUTPUT,changes=(),timeline=(),rounding=None,occupied=None):
        n=len(rows);raw=bytearray(0x4000)
        def put(address,value,width=4):
            assert BASE<=address and address+width<=BASE+len(raw)
            raw[address-BASE:address-BASE+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little')
        for address,values in ((ORIGIN,origin),(ENDPOINT,endpoint),(INFLUENCE+0x2c,center)):
            for coordinate,value in enumerate(values):put(address+coordinate*4,bits(value))
        put(ORIGIN+0xc,frame,2);put(INFLUENCE+0x3c,bits(radius));put(INFLUENCE+0x40,bits(weight))
        put(POOL+0x44,OBJECTS);put(OBJECTS+8,BODY)
        for index in (-2,0,1):
            address=FRAMES+index*16;put(address,0x198b0000);put(address+6,0,2);put(address+0xa,2,1);put(address+0xb,1,1)
        put(BODY+0x114,52,2);put(BODY+0x116,0x200,2)
        for offset,value in ((0,scale),(4,1.),(0x14,1.),(0x24,1.)):put(BODY+0x200+offset,bits(value))
        if frame!=0xffff:
            address=FRAMES+struct.unpack('<h',struct.pack('<H',frame))[0]*16
            if state=='missing_object':put(address,0xffffffff)
            elif state=='negative_node':put(address+6,0xffff,2)
            elif state=='node_bound':put(address+6,1,2)
        for address,value,width in changes:put(address,value,width)
        gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[1]=ENDPOINT;gp[4]=geometry.SP+n%4*4;gp[6]=ORIGIN;gp[7]=INFLUENCE
        r=n%4 if rounding is None else rounding;depth=(0,1,3,7)[n%4]if occupied is None else occupied
        assert depth<8
        values=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range(depth)]
        banks=[dict(address=BASE,before=raw.hex()),dict(address=gp[4]+4,before=struct.pack('<I',output).hex())]
        for address,size in ((0x44ac8c,36),(0x45695c,4),(0x46a0d0,4),(0x46a0e8,4)):
            banks.append(dict(address=address,before=image.read(address,size).hex()))
        for address,value in ((0x4ecbd0,POOL),(0x505e78,FRAMES)):banks.append(dict(address=address,before=struct.pack('<I',value).hex()))
        rows.append(dict(id=n,name=name,entry=0x27c520,matrix=output,source=0,gp=gp,banks=sorted(banks,key=lambda b:b['address']),origin=ORIGIN,endpoint=ENDPOINT,influence=INFLUENCE,destination=output,before_fp=[0x7f|((0,2,3)[n%3]<<8)|(r<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|(r<<13)|(0,0x40,0x8000,0x8040)[n%4]|(n*7%64)],live_x87=values,input_timeline=list(timeline)))
    for center in ((-1.,0.,0.),(1.,0.,0.),(-3.,0.,0.),(0.,1.,0.),(-2.,0.,0.),(-1.,2.,3.)):
        for rounding in range(4):make('projection_'+str(center)+'_'+str(rounding),center=center,radius=10.,rounding=rounding)
    for radius in (0.,-0.,1.,2.,3.,-3.,float('inf'),float('-inf')):
        for weight in (0.,1.,-2.):make('radius_'+str(radius)+'_'+str(weight),radius=radius,weight=weight)
    for radius in (0.,1.,2.):make('zero_segment_'+str(radius),endpoint=(0.,0.,0.),center=(1.,0.,0.),radius=radius)
    for frame in (0,1,0xfffe,0xffff):
        for state in ('valid','missing_object','negative_node','node_bound'):
            for scale in (0.,1.,.75):make('frame_'+str(frame)+'_'+state+'_'+str(scale),frame=frame,state=state,scale=scale)
    for address in (ORIGIN,ORIGIN+4,ORIGIN+8,ENDPOINT,ENDPOINT+4,ENDPOINT+8,INFLUENCE+0x2c,INFLUENCE+0x3c,INFLUENCE+0x40):
        for value in (1,0x80000001,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
            make(f'exception_{address:x}_{value:x}',changes=((address,value,4),))
    for output in (ORIGIN,ORIGIN+4,ENDPOINT,INFLUENCE+0x2c,INFLUENCE+0x3c,INFLUENCE+0x40):make('output_alias_'+hex(output),output=output)
    def publication(address,occurrence,target,value):return dict(on_read=dict(address=address,occurrence=occurrence),writes=[dict(address=target,bytes=struct.pack('<I',value).hex())])
    make('fresh_radius',timeline=(publication(INFLUENCE+0x3c,1,INFLUENCE+0x3c,bits(6.)),))
    for value in (0,0x80000000,0x7fc12345,0x7f812345):
        make(f'fresh_exceptional_radius_{value:x}',timeline=(publication(INFLUENCE+0x3c,1,INFLUENCE+0x3c,value),))
    make('fresh_weight',timeline=(publication(INFLUENCE+0x3c,2,INFLUENCE+0x40,bits(4.)),))
    make('fresh_frame',timeline=(publication(ORIGIN+0xc,1,ORIGIN+0xc,0),))
    for n,options in enumerate(({},dict(endpoint=(0.,0.,0.)),dict(frame=0),dict(center=(1.,0.,0.)))):make('fault_seed_'+str(n),**options)
    make('fault_seed_deferred_sum',endpoint=(0.,0.,0.),center=(1.,4096.,1.),radius=5000.)
    rows[-1]['before_fp'][2]&=~0x20
    return rows


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--verify',type=Path);args=parser.parse_args()
    assert not args.output.exists();assert common.sha(NEW_DEFAULT)==NEW_PIN
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;metadata=common.metadata(image);owned=prepare(image,metadata,scratch)
    complete=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in (row for row in complete if row['name'].startswith('fault_seed')):
        for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for ordinal in range(len(events)):
                # The ordinary caller's immutable pushed output pointer is
                # transport, outside the typed data kernel's retained inputs.
                if phase=='before_read'and events[ordinal]['address']==seed['gp'][4]+4:continue
                row=copy.deepcopy(seed);row.update(id=len(complete)+len(prefix),name=seed['name']+'_'+phase+'_'+str(ordinal),fault=dict(phase=phase,ordinal=ordinal));prefix.append(row)
    cases=complete+geometry.physical(owned,prefix,'prefix')
    faults=[]
    for key,register in (('origin',6),('endpoint',1),('influence',7),('destination',None)):
        row=copy.deepcopy(complete[0]);row.update(id=len(cases)+len(faults),name='actual_missing_'+key,expected_fault=True)
        row[key]=0xdead0000
        if register is not None:row['gp'][register]=row[key]
        else:
            row['matrix']=row[key]
            for bank in row['banks']:
                if bank['address']==row['gp'][4]+4:bank['before']=struct.pack('<I',row[key]).hex()
        faults.append(row)
    cases+=geometry.physical(owned,faults,'actual-faults')
    root_sites={s['instruction']for s in metadata['sites']if 0x27c520<=s['instruction']<0x27c6b2};reached=set().union(*(set(row['executed_sites'])for row in complete));assert root_sites<=reached,sorted(root_sites-reached)
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=metadata['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=cases)
    data=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
    if args.verify:assert data==args.verify.read_bytes(),'Original influence repeat differs'
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(data)
    report=dict(passed=True,scope=__doc__,original_sha256=NEW_PIN,cases=len(cases),complete_calls=len(complete),fault_prefixes=len(prefix),actual_faults=len(faults),root_sites=len(root_sites),reached_closure_sites=len(reached),pending_quantity_prefixes=sum(bool(row['pending_extended'])for row in cases),fixture=str(args.output),fixture_bytes=len(data),fixture_sha256=common.sha(args.output),independent_repeat_identical=True if args.verify else None,source_sha256={str(p.relative_to(ROOT)):common.sha(p)for p in (Path(__file__),ROOT/'tools/observe_native_campaign_ai_path_influence_tu15.cpp')},owned_scratch=str(scratch),retained_large_artifacts=[],native_implementation_qualified=False,scope_limits=['One free original x87 temporary required when the falloff branch executes; full-stack and unmasked trap owners remain outside this profile','Typed data kernel, not numeric World/parent ABI or full navigation-search qualification'])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(cases)} original influence cases, {len(root_sites)} root sites')


if __name__=='__main__':main()
