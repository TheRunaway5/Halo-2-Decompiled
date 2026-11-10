#!/usr/bin/env python3
"""Private actual1E11B0/disc1DC8F0 native marker lookup versus unchanged i386."""
import argparse,copy,hashlib,json,os,random,struct,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from validate_native_tu15_prt_helpers import physical_original
from oracle_smg_firing_1_1 import Xbe
OLD_PIN='aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
REGIONS=((0x4eb000,4096),(0x4f0000,4096),(0x61000000,16384),(0x62000000,8192))
INPUTS=('include/halo2/native_tu15_model_marker_lookup_kernel.hpp','include/halo2/native_tu15_id_search_kernel.hpp','src/gameplay/weapons/native_tu15_model_marker_lookup.hpp','src/gameplay/weapons/native_tu15_model_marker_lookup_admission.hpp','include/halo2/native_weapon_effect_birth_color_observer.hpp','tests/native_tu15_model_marker_lookup_tests.cpp','tools/validate_native_model_marker_lookup_tu15.py','src/physics/runtime/native_tu15_raw_x87_scope.hpp','src/physics/runtime/native_tu15_sse_mmx_scope.hpp','src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp','src/physics/runtime/native_tu15_mopp_ray_integer_frame.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp','src/physics/runtime/physics_world_runtime_internal.hpp','tools/validate_native_tu15_prt_helpers.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp')
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def corpus(image,smoke):
 rng=random.Random(0x1e11b0);rows=[]
 live=('0000000000000080ff3f','35c26821a2da0fc90040','45230100000000c0ff7f','01000000000000000000','00000000000000800040','00000000000000000000','0000000000000080ffbf','00000000000000000080')
 families=('datum-none','name-none','name-zero','count-zero','count-negative','count-min','match-first','match-last','exhaust','duplicate','signed-limit','slot-65535','saved-si-count','saved-si-rows','saved-si-key','return-key')
 for ordinal,kind in enumerate(families[:8]if smoke else families):
  for variant in range(2 if smoke else 8):
   banks=[bytearray(rng.randbytes(size))for _,size in REGIONS];sp=0x62000e00+variant;tag=0x61000100+variant;markers=0x61001000+variant
   datum=(rng.getrandbits(16)<<16)|5;name=0x53420000+ordinal;count=5;si=rng.getrandbits(32)
   if kind=='slot-65535':datum=(datum&0xffff0000)|65535
   if kind=='saved-si-count':tag=sp-4-0x58;si=0
   if kind=='saved-si-rows':tag=sp-4-0x5c;si=markers+12
   if kind=='saved-si-key':markers=sp-4;si=name
   if kind=='return-key':markers=sp;name=0x63000000
   def put(address,value,width=4):
    for(base,size),bank in zip(REGIONS,banks):
     if base<=address<=base+size-width:bank[address-base:address-base+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little');return
    raise ValueError('Marker fixture escaped backing '+hex(address))
   def get(address):
    for(base,size),bank in zip(REGIONS,banks):
     if base<=address<=base+size-4:return int.from_bytes(bank[address-base:address-base+4],'little')
    raise ValueError('Marker fixture read escaped backing')
   for index in range(8):put(markers+index*12,name+index+1)
   if kind not in('exhaust','count-zero','count-negative','count-min'):put(markers+(0 if kind=='match-first'else 4)*12,name)
   if kind=='duplicate':put(markers+12,name);put(markers+36,name)
   if kind=='count-zero':count=0
   if kind=='count-negative':count=0xffffffff
   if kind=='count-min':count=0x80000000
   if kind=='signed-limit':count=0x7fffffff;put(markers,name)
   put(tag+0x58,count);put(tag+0x5c,markers)
   # The table base may wrap; only the actual selected slot is dereferenced.
   slot=0x61000080+variant;put(slot,tag)
   put(0x4f0414 if image=='TitleUpdate15'else 0x4eb114,(slot-8-(datum&65535)*16)&0xffffffff)
   if kind=='datum-none':datum=0xffffffff
   if kind=='name-none':name=0xffffffff
   if kind=='name-zero':name=0
   put(sp,0x63000000)
   gp=[rng.getrandbits(32)for _ in range(8)];gp[1]=datum;gp[4]=sp;gp[6]=si;gp[7]=name
   depth=(0,1,4,8)[variant%4];status=((variant%8)<<11)|0x4500|(variant*7&0x3f)
   rows.append(dict(id=len(rows),name=f'{image}-{kind}-{variant}',entry=0x1e11b0 if image=='TitleUpdate15'else 0x1dc8f0,image=image,observation='actual'if image=='TitleUpdate15'else'canonical',gp=gp,eflags=0x202|rng.choice((0,1,0x10,0x40,0x80,0x800,0x8d5))|(0x400 if variant&1 else 0),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[(0x27f,0x37f,0x77f,0xf7f)[variant%4],status,0x1f80|((ordinal%4)<<13)],live_x87=list(live[:depth]),matrix=get(sp+4),source=get(sp+8),max_steps=10000,banks=[dict(address=base,before=bank.hex())for(base,_),bank in zip(REGIONS,banks)]))
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true');p.add_argument('--smoke',action='store_true');p.add_argument('--prefixes',action='store_true');p.add_argument('--mmx',action='store_true');args=p.parse_args()
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
 pins={name:digest(ROOT/name)for name in INPUTS};report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),source_before=pins,retained_large_artifacts=[])
 try:
  import oracle_native_campaign_screen_lifecycle_tu15 as common
  old_metadata=common.metadata
  def metadata(image):
   result=old_metadata(image);code=common._Code(image,'.text')
   for site in result['sites']:
    if code.at(site['instruction']).mnemonic in('ret','pop'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
   return result
  common.metadata=metadata;all_rows=[];authorities=[];all_sites=set()
  for image_name,path,pin,entry in(('TitleUpdate15',NEW_DEFAULT,NEW_PIN,0x1e11b0),('MapPack11',ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',OLD_PIN,0x1dc8f0)):
   assert digest(path)==pin;image=Xbe(path);common.ENTRIES=(entry,);sites=metadata(image)['sites'];code=common._Code(image,'.text')
   io=[site['instruction']for site in sites if site['operands']or code.at(site['instruction']).mnemonic=='push'];writes=[site['instruction']for site in sites if any(o['write']for o in site['operands'])or code.at(site['instruction']).mnemonic=='push']
   rows=corpus(image_name,args.smoke);directory=scratch/image_name;directory.mkdir();normal=directory/'normal';normal.mkdir()
   observed,authority=physical_original(image,rows,normal,entries=(entry,),regions=REGIONS,trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,preserve_x87_prefix=True)
   if args.prefixes:
    prefix=[]
    for index in(family*8+variant for family in(6,7,8,9,12,13,14,15)for variant in range(4)):
     for phase,key in(('before_read','reads'),('before_write','writes'),('after_write','writes')):
      for ordinal,_ in enumerate(observed['cases'][index][key]):
       row=copy.deepcopy(rows[index]);row.update(id=len(rows)+len(prefix),name=row['name']+f'-{phase}-{ordinal}',fault=dict(phase=phase,ordinal=ordinal));prefix.append(row)
    folder=directory/'prefixes';folder.mkdir();stopped,prefix_authority=physical_original(image,prefix,folder,entries=(entry,),regions=REGIONS,trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,preserve_x87_prefix=True)
    rows+=prefix;observed['cases']+=stopped['cases'];authority['prefix_authority']=prefix_authority
   if args.mmx:
    from native_sphere_frustum_mmx_original import physical_original_mmx,PROBE
    mmx=[];rng=random.Random(0x80c1)
    for index in range(max(256,len(rows))):
     source=rows[index%len(rows)]
     row=copy.deepcopy(source);row.update(id=len(rows)+index,name='mmx-'+row['name'],live_x87=[])
     row['before_fp'][1]&=~0x3800;row['active_mmx']=dict(known_mask=index%256,physical=[(rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(index+n)%4].to_bytes(2,'little')).hex()for n in range(8)]);mmx.append(row)
    folder=directory/'mmx';folder.mkdir();opaque,mmx_authority=physical_original_mmx(image,mmx,folder,entries=(entry,),regions=REGIONS+((PROBE,4096),),trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,preserve_x87_prefix=True)
    rows+=mmx;observed['cases']+=opaque['cases'];authority['opaque_mmx_authority']=mmx_authority
   for row,actual in zip(rows,observed['cases'],strict=True):
    assert actual['completed']==('fault'not in row),(row['name'],actual.get('boundary'))
    all_sites.update(actual['executed_sites'])
    # Every epilogue is above its path's earlier instruction addresses.
    expected_pc=actual['boundary']['instruction']if row.get('fault')else actual['executed_sites'][-1]
    row['expected']=dict(entry=expected_pc,gp=actual['boundary_gp'],eflags=actual['boundary_eflags'],xmm=actual['boundary_xmm'],fp=actual['after_fp'],live_x87=actual.get('boundary_live_x87',row['live_x87']))
    if 'active_mmx'in row:row['expected']['active_mmx']=actual['active_mmx']
    row.update(reads=actual['reads'],writes=actual['writes'],calls=actual['calls'],write_sites=writes)
    for call in row['calls']:assert call.pop('arguments')==[]
    for bank,after in zip(row['banks'],actual['banks'],strict=True):bank['after']=after['after']
   authorities.append(dict(image=image_name,original_sha256=pin,authority=authority));all_rows+=rows
  fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=all_rows),separators=(',',':')));results=[]
  for compiler in args.compiler or('g++','clang++'):
   project=scratch/Path(compiler).name;project.mkdir();binary=project/'marker'
   command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_model_marker_lookup_tests.cpp'),'-o',str(binary)]
   if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
   done=subprocess.run(command,capture_output=True,text=True);assert done.returncode==0,done.stderr[-8000:]
   done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True);assert done.returncode==0,(done.stdout+done.stderr)[-12000:]
   checks=[]
   for option,env in(('--guards',{}),('--reference-guard',{'HALO2_REFERENCE_MODEL_MARKER_LOOKUP':'1'})):
    guarded=subprocess.run([str(binary),option],capture_output=True,text=True,env=dict(os.environ,**env));assert guarded.returncode==0,(guarded.stdout+guarded.stderr)[-6000:]
    checks.append(dict(option=option,result=json.loads(guarded.stdout)))
   results.append(dict(compiler=compiler,native=json.loads(done.stdout),admission_checks=checks,command=command))
  report.update(passed=True,physical_paths=len(all_rows),source_sites=sorted(hex(pc)for pc in all_sites),compilers=results,originals=authorities)
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:
  report['source_after']={name:digest(ROOT/name)for name in INPUTS}
  if report['source_after']!=pins:report.update(passed=False,source_drift=True)
  (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
