#!/usr/bin/env python3
"""Private native184690 classification against unchanged physical TU1.5."""
import argparse,copy,hashlib,json,os,platform,random,re,struct,subprocess,sys
from pathlib import Path
from validate_native_tu15_prt_helpers import physical_original,PIN
ROOT=Path(__file__).resolve().parents[1]
REGIONS=((0x61000000,16384),(0x62000000,4096))
INPUTS=('include/halo2/native_tu15_sphere_frustum_kernel.hpp','src/rendering/native_tu15_sphere_frustum_abi.hpp','tests/native_tu15_sphere_frustum_tests.cpp','include/halo2/native_tu15_sphere_frustum_value.hpp','tests/native_tu15_sphere_frustum_value_reference.inc','src/rendering/world/source_visibility.cpp','include/halo2/source_visibility.hpp','tools/validate_native_sphere_frustum_tu15.py','src/physics/runtime/native_tu15_raw_x87_scope.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp','src/physics/runtime/physics_world_runtime_internal.hpp','src/physics/runtime/physics_step_internal.hpp','src/physics/runtime/physics_solver_internal.hpp','tools/native_sphere_frustum_replacements.py','src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp')
INPUTS+=('cmake/NativeTu15SphereFrustum.cmake','tools/specialize_native_operands.py','tools/generate_physics_live_support_tu15.py','tools/native_selected_operand_binding.py','tools/validate_native_tu15_prt_helpers.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp')
INPUTS+=('src/physics/runtime/native_tu15_sse_mmx_scope.hpp','tools/native_sphere_frustum_mmx_original.py')
def prepare(scratch,unit,require_binding):
 import native_sphere_frustum_replacements as binding
 from generated_routine_ownership import function_bodies
 from specialize_native_operands import specialize
 body=lambda source:next(b for signature,b in function_bodies(source)if int(signature[1],16)==0x184690)
 current=body(unit);original=binding.canonical_body(current);bound=binding.replace_sphere_frustum(unit)
 if require_binding and bound!=unit:raise ValueError('Production sphere/frustum binding missing')
 if binding.replace_sphere_frustum(bound)!=bound:raise ValueError('Nonidempotent sphere binding')
 prepared,_=specialize(bound);specialized=body(prepared)
 if binding.PREFIX not in specialized or 's.instruction_literal('not in specialized:raise ValueError('Specialized sphere admission/retained route drift')
 (scratch/'sphere_frustum_bodies.inc').write_text('namespace bound {\n'+body(bound)+'\n}\nnamespace specialized {\n'+specialized+'\n}\nnamespace retained {\n'+original+'\n}\n')
 resolver=next(line for line in unit.splitlines()if line.startswith('std::uint32_t selected('));dispatch=next(line for line in unit.splitlines()if line.startswith('static void dispatch(WorldState &s,std::uint32_t target'))
 mutations=[
  unit.replace(current,current.replace('0x64u','0x65u',1),1),
  unit.replace(current,current.replace('_mm_sub_ss','_mm_add_ss',1),1),
  unit.replace(current,current.replace('s.gp[4]+=0u','s.gp[4]+=4u',1),1),
  unit.replace(resolver,resolver.replace('return p;','return p+1u;',1),1),
  unit.replace(dispatch,dispatch.replace('run_184690(s);','run_1770c0(s);',1),1),
  unit.replace(dispatch,dispatch.replace('s.required(site,target,tail);','s.required(site,target+1u,tail);',1),1),
  bound.replace(binding.PREFIX,binding.PREFIX.replace('::bridge','::changed_bridge'),1)]
 for mutation in mutations:
  if mutation in(unit,bound):raise ValueError('Missing sphere source mutation witness')
  try:binding.replace_sphere_frustum(mutation)
  except ValueError:pass
  else:raise ValueError('Sphere binding admitted changed body/resolver/routing')
 public=(ROOT/'src/rendering/world/source_visibility.cpp').read_text();shared=binding.replace_world_visibility(public)
 if binding.replace_world_visibility(shared)!=shared:raise ValueError('Nonidempotent public sphere value binding')
 if require_binding and shared!=public:raise ValueError('Public sphere shared kernel binding missing')
 from audit_native_execution_ownership import unique_function
 target=unique_function(public,'source_sphere_frustum_visibility')
 changed=public.replace(target,target.replace('radius','changed_radius',1),1)
 try:binding.replace_world_visibility(changed)
 except ValueError:pass
 else:raise ValueError('Public sphere binder admitted signature drift')
 return len(mutations)+1

def semantic_mutants(scratch,command,fixture,env,mmx=False):
 kernel='include/halo2/native_tu15_sphere_frustum_kernel.hpp';adapter='src/rendering/native_tu15_sphere_frustum_abi.hpp'
 definitions=(
  ('plane4-containment',kernel,'{0u,1u,2u,3u,5u}','{0u,1u,2u,3u,4u,5u}'),
  ('plane5-multiply-order',kernel,'frame.multiply(d.scratch,normal)','frame.multiply(normal,d.scratch)'),
  ('pre-read-publication',adapter,'complete(d);const auto axis=d.axis','const auto axis=d.axis'),
  ('unprotected-provider',adapter,'scope_.io([&]{s_.memory.read(address,bytes);});','s_.memory.read(address,bytes);'),
  ('rejection-flags',adapter,'s_.gp[0]=s_.flags.logic(0,4)','s_.gp[0]=0'),
  ('cached-clear-guard-removed',adapter,'if(!s.services.instruction&&(s.instruction_cursor||s.literal_service_sites))return false;','(void)0;'))
 if mmx:
  scope='src/physics/runtime/native_tu15_sse_mmx_scope.hpp'
  definitions+=(
   ('mmx-host-tags-unparked',scope,'source.word(8,0xffffu)','source.word(8,host_.word(8))'),
   ('mmx-provider-fault-unrestored',scope,'catch(...) {native_raw_x87_detail::restore(source);throw;}','catch(...) {throw;}'),
   ('mmx-source-mxcsr-lost',scope,'returned.mxcsr=source.mxcsr','returned.mxcsr=host_.mxcsr'),
   ('mmx-mask-corrupted',scope,'auto source=host_;source.word(4,state.fp.word());','const_cast<State&>(state).active_mmx->known_mask^=1u;auto source=host_;source.word(4,state.fp.word());'))
 results=[]
 for name,path,old,new in definitions:
  source=(ROOT/path).read_text()
  if source.count(old)!=1:raise ValueError('Missing unique sphere semantic witness '+name)
  overlay=scratch/('mutant-'+name);target=overlay/Path(path).relative_to(Path(path).parts[0]);target.parent.mkdir(parents=True);target.write_text(source.replace(old,new,1))
  binary=overlay/'tests';cmd=list(command);cmd[cmd.index('-o')+1]=str(binary);cmd.insert(1,'-I'+str(overlay));compiled=subprocess.run(cmd,capture_output=True,text=True)
  if compiled.returncode:raise ValueError('Sphere mutant failed to compile '+name+': '+compiled.stderr[-3000:])
  tested=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=env)
  if tested.returncode==0:raise ValueError('Sphere semantic mutant survived '+name)
  results.append(dict(name=name,rejected=True,exit=tested.returncode,diagnostic=(tested.stdout+tested.stderr)[-1800:]))
 return results
def corpus():
 rng=random.Random(0x184690);rows=[];live=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f','01000000000000000000','00000000000000800040','00000000000000000000','0000000000000080ffbf')
 for family in range(25):
  for variant in range(12):
   n=len(rows);banks=[bytearray(rng.randbytes(size))for _,size in REGIONS];frustum=0x61000100+n%4;center=0x61001000+n%4;sp=0x62000800+n%4
   def put(at,v):
    for(base,size),bank in zip(REGIONS,banks):
     if base<=at<=base+size-4:struct.pack_into('<I',bank,at-base,v&0xffffffff);return
    raise ValueError('Sphere fixture escaped backing')
   def f32(value):return struct.unpack('<I',struct.pack('<f',value))[0]
   point=[0.,0.,0.];bounds=[-10.,10.,-10.,10.,-10.,10.];planes=[[1.,0.,0.,9.],[-1.,0.,0.,9.],[0.,1.,0.,9.],[0.,-1.,0.,9.],[0.,0.,1.,9.],[0.,0.,-1.,9.]];radius=f32(1.)
   if family<3:bounds[family*2+1]=-2.
   elif family<6:bounds[(family-3)*2]=2.
   elif family<12:planes[family-6][3]=-2.
   elif family<18:planes[family-12][3]=0.
   for axis,v in enumerate(point):put(center+axis*4,f32(v))
   for i,v in enumerate(bounds):put(frustum+0x60+i*4,f32(v))
   for i,p in enumerate(planes):
    for k,v in enumerate(p):put(frustum+0x78+i*16+k*4,f32(v))
   special=(0,0x80000000,1,0x80000001,0x7f800000,0xff800000,0x7fc12345,0x7f812345,0x3f800001,0xbf800001,0x00800000,0x7f7fffff)[variant]
   if family==19:radius=special
   if family==20:put(center+(variant%3)*4,special)
   if family==21:put(frustum+0x78+(variant%6)*16+(variant%4)*4,special)
   if family==22:
    put(center,0x7fc12345);put(frustum+0xc8,0x7fc56789)
   if family==23:
    for axis in range(3):put(center+axis*4,f32((variant-6)*0.125))
   if family==24:
    for i in range(6):
     for k in range(4):put(frustum+0x78+i*16+k*4,rng.getrandbits(32))
   put(sp,0x63000000);put(sp+4,0x11111111);put(sp+8,0x22222222)
   gp=[rng.getrandbits(32)for _ in range(8)];gp[0]=frustum;gp[1]=center;gp[4]=sp;xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)];xmm[1][0]=radius
   rows.append(dict(id=n,name=f'family-{family}-variant-{variant}',entry=0x184690,gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),matrix=0x11111111,source=0x22222222,before_fp=[(0x7f,0x27f,0x37f)[variant%3]|((variant%4)<<10),((n%8)<<11)|((n%8)<<8)|((n//7)%64),0x1f80|((variant%4)<<13)|(0x8040 if variant&4 else 0)],xmm=xmm,live_x87=list(live[:(0,1,4,8)[variant%4]]),max_steps=500,banks=[dict(address=base,before=data.hex())for(base,_),data in zip(REGIONS,banks)]))
 # Read-only inputs may occupy one another or the RET word. No captured
 # value copy is permitted to replace the original operand acquisition.
 for n,offset in enumerate((0x60,0x64,0x78,0x7c,0xc8,0xcc)):
  for stack_alias in(False,True):
   row=copy.deepcopy(rows[216+n]);row.update(id=len(rows),name=f'center-frustum-{offset:x}-ret-{stack_alias}')
   row['gp'][1]=row['gp'][0]+offset
   if stack_alias:
    row['gp'][4]=row['gp'][1]+4
    for bank in row['banks']:
     data=bytearray.fromhex(bank['before']);offset_in_bank=row['gp'][4]-bank['address']
     if 0<=offset_in_bank<=len(data)-12:
      struct.pack_into('<III',data,offset_in_bank,0x63000000,row['matrix'],row['source']);bank['before']=data.hex();break
    else:raise ValueError('Aliased classifier RET escaped backing')
   rows.append(row)
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--require-binding',action='store_true');p.add_argument('--semantic-mutants',action='store_true');p.add_argument('--mmx',action='store_true');p.add_argument('--mmx-candidate',action='store_true');args=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
 if sys.platform!='linux'or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not args.xbe.is_file():return 77
 if hashlib.sha256(args.xbe.read_bytes()).hexdigest()!=PIN:raise ValueError('Wrong original image')
 pins={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in INPUTS}
 from audit_native_execution_ownership import unique_function
 public=unique_function((ROOT/'src/rendering/world/source_visibility.cpp').read_text(),'source_sphere_frustum_visibility')
 import native_sphere_frustum_replacements as binding
 if public!=binding.PUBLIC_BODY and hashlib.sha256(public.encode()).hexdigest()!=binding.PUBLIC_PIN:raise ValueError('Public world visibility body drift')
 reference=(ROOT/'tests/native_tu15_sphere_frustum_value_reference.inc').read_text().split('\n',1)[1].rstrip('\n').replace('source_sphere_frustum_visibility_reference','source_sphere_frustum_visibility',1)
 if hashlib.sha256(reference.encode()).hexdigest()!=binding.PUBLIC_PIN:raise ValueError('World native-native reference body drift')
 from generated_routine_ownership import function_bodies
 unit=(ROOT/'src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp').read_text()
 canonical=next(body for signature,body in function_bodies(unit)if int(signature[1],16)==0x184690)
 if hashlib.sha256(binding.canonical_body(canonical).encode()).hexdigest()!=binding.BODY_PIN:raise ValueError('Complete original classifier source body drift')
 rejected=prepare(scratch,unit,args.require_binding)
 sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
 import oracle_native_campaign_screen_lifecycle_tu15 as common
 original_metadata=common.metadata
 def metadata(image):
  result=original_metadata(image);code=common._Code(image,'.text')
  for site in result['sites']:
   if code.at(site['instruction']).mnemonic=='ret':site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
  return result
 common.metadata=metadata;image=Xbe(args.xbe);common.ENTRIES=(0x184690,);sites=metadata(image)['sites'];assert not any(operand['write']for site in sites for operand in site['operands']), 'Classifier is not store-free';assert all(common._Code(image,'.text').at(site['instruction']).mnemonic not in('call','push')for site in sites), 'Classifier has a call or stack store'
 source_code=common._Code(image,'.text')
 for site in sites:
  instruction=source_code.at(site['instruction'])
  assert not instruction.mnemonic.startswith('f')and not any(instruction.reg_name(register).startswith('mm')for register in instruction.regs_access()[0]+instruction.regs_access()[1]), 'Classifier is not x87/MMX-neutral'
 io=[site['instruction']for site in sites if site['operands']];rows=corpus();first=scratch/'normal';first.mkdir();physical,authority=physical_original(image,rows,first,entries=(0x184690,),regions=REGIONS,trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,preserve_x87_prefix=True)
 prefixes=[]
 for index in (0,36,72,144,216,228):
  for ordinal,event in enumerate(physical['cases'][index]['reads']):
   row=copy.deepcopy(rows[index]);row.update(id=len(rows)+len(prefixes),name=row['name']+f'-read-{ordinal}',fault=dict(phase='before_read',ordinal=ordinal));prefixes.append(row)
 second=scratch/'prefix';second.mkdir();stopped,stop_authority=physical_original(image,prefixes,second,entries=(0x184690,),regions=REGIONS,trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,preserve_x87_prefix=True)
 rows+=prefixes;physical['cases']+=stopped['cases']
 mmx_authorities=[]
 if args.mmx or args.mmx_candidate:
  from native_sphere_frustum_mmx_original import physical_original_mmx,PROBE
  rng=random.Random(0x18469080);mmx_rows=[]
  for index,source in enumerate(rows):
   row=copy.deepcopy(source);row['id']=len(rows)+index;row['name']='mmx-'+row['name'];row['live_x87']=[];row['before_fp'][1]&=~0x3800
   patterns=('00000000000000000000','00000000000000000080','0000000000000080ff3f','01000000000000000000','45230100000000c0ff7f','efcdab8967452301ff3f')
   row['active_mmx']=dict(known_mask=index%256,physical=[patterns[(index+n)%11]if(index+n)%11<len(patterns)else(rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(index+n)%4].to_bytes(2,'little')).hex()for n in range(8)])
   mmx_rows.append(row)
  mmx_dir=scratch/'mmx';mmx_dir.mkdir()
  mmx,mmx_authority=physical_original_mmx(image,mmx_rows,mmx_dir,entries=(0x184690,),regions=REGIONS+((PROBE,4096),),trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,preserve_x87_prefix=True)
  mmx_authorities.append(mmx_authority);rows+=mmx_rows;physical['cases']+=mmx['cases']
 for row,observed in zip(rows,physical['cases'],strict=True):
  row.update({key:observed[key]for key in('boundary_gp','boundary_eflags','boundary_xmm','after_fp','reads','calls')});row['expected_pc']=observed['boundary']['instruction']if row.get('fault')else(0x184874 if observed['boundary_gp'][0]==0 else 0x18489f)
  for call in row['calls']:
   assert call.pop('arguments')==[]
  assert observed['live_x87_unchanged']and not observed['writes']
  if 'active_mmx'in row:row['boundary_active_mmx']=observed['active_mmx']
  for before,after in zip(row['banks'],observed['banks'],strict=True):before['after']=after['after']
 fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')));binary=scratch/'sphere-tests';cmd=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(scratch),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_sphere_frustum_tests.cpp'),'-o',str(binary)]
 if args.sanitizers:cmd+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
 result=subprocess.run(cmd,capture_output=True,text=True)
 if result.returncode:raise ValueError('Sphere compile failed: '+result.stderr[:7000])
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');env.pop('HALO2_REFERENCE_SPHERE_FRUSTUM',None)
 if args.mmx_candidate:env['HALO2_SPHERE_FRUSTUM_MMX_CANDIDATE']='1'
 cached_clear=subprocess.run([str(binary),'--cached-clear',str(fixture)],capture_output=True,text=True,env=env)
 if cached_clear.returncode:
  args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(dict(passed=False,stage='authentic_cached_clear_source_regression',owned_scratch=str(scratch),retained_large_artifacts=[],source_sha256=pins,exit=cached_clear.returncode,diagnostic=(cached_clear.stdout+cached_clear.stderr)[-12000:]),indent=2)+'\n');raise ValueError('Sphere cached-clear regression failed: '+cached_clear.stderr[-3000:])
 guards=[]
 for option,actual_env in(('--refusals',env),('--reference-guard',dict(env,HALO2_REFERENCE_SPHERE_FRUSTUM='1'))):
  check=subprocess.run([str(binary),option],capture_output=True,text=True,env=actual_env)
  if check.returncode:raise ValueError('Sphere admission/control failed: '+check.stderr[-3000:])
  guards.append(json.loads(check.stdout))
 reference_run=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=dict(env,HALO2_REFERENCE_SPHERE_FRUSTUM='1',HALO2_SPHERE_FRUSTUM_REFERENCE_TEST='1'))
 if reference_run.returncode:raise ValueError('Sphere actual retained-reference route failed: '+reference_run.stderr[-4000:])
 result=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=env)
 if result.returncode:
  args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(dict(passed=False,source_sha256=pins,exit=result.returncode,diagnostic=(result.stdout+result.stderr)[-12000:]),indent=2)+'\n');raise ValueError('Sphere native failed: '+(result.stdout+result.stderr)[-12000:])
 mutants=semantic_mutants(scratch,cmd,fixture,env,args.mmx or args.mmx_candidate)if args.semantic_mutants else []
 if pins!={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in INPUTS}:raise ValueError('Sphere proof source drift')
 report=dict(passed=True,owned_scratch=str(scratch),retained_large_artifacts=[],cached_clear_scope="Exact canonical retained vs raw and prepared bound fallbacks, with current common State live-cursor contract",private_unbound=not args.require_binding,mmx_candidate=args.mmx_candidate,authenticated_store_free_and_no_children=True,cached_clear_source=json.loads(cached_clear.stdout),runtime_guards=guards,source_drift_rejections=rejected,semantic_mutants=mutants,positive_retained_reference=json.loads(reference_run.stdout),world_value_scope='Native-native old public value algorithm regression; raw ABI separately compared to unchanged physical Original',executed_sites=sorted({hex(pc)for row in physical['cases']for pc in row['executed_sites']}),physical_cases=len(rows),normal_cases=sum('fault'not in row for row in rows),read_prefixes=sum('fault'in row for row in rows),physical_original=authority,physical_prefix_original=stop_authority,physical_mmx=mmx_authorities,native=json.loads(result.stdout),source_sha256=pins,compiler=args.compiler,sanitizers=args.sanitizers);args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(result.stdout.strip());return 0
if __name__=='__main__':raise SystemExit(main())
