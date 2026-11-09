#!/usr/bin/env python3
"""Actual1E3150 node resolution with shared record DATA and genuine lifecycle calls."""
import argparse,copy,hashlib,json,os,platform,random,re,struct,subprocess,sys
from pathlib import Path
from generated_routine_ownership import function_bodies
from validate_native_tu15_prt_helpers import PIN,physical_original
import native_node_descriptor_lookup_replacements as binding
ROOT=Path(__file__).resolve().parents[1]
COPIES=('src/physics/physics_live_support_tu15_generated.cpp','src/campaign/ai/source_ai_frame_tu15_generated.cpp','src/campaign/ai/source_ai_actor_tu15_generated.cpp')
REGIONS=((0x61000000,65536),(0x62000000,65536),(0x4f0000,8192),(0x51d000,8192))
INPUTS=COPIES+('cmake/NativeTu15NodeDescriptorLookup.cmake','tools/generate_physics_live_support_tu15.py','include/halo2/native_tu15_record_lookup_kernel.hpp','include/halo2/native_tu15_node_descriptor_lookup_kernel.hpp','src/physics/runtime/native_tu15_node_descriptor_lookup.hpp','src/physics/runtime/native_tu15_record_lookup.hpp','src/physics/runtime/native_tu15_node_resource.hpp','src/physics/runtime/native_tu15_node_resource_lifecycle.hpp','tests/native_tu15_node_descriptor_lookup_tests.cpp','tools/native_node_descriptor_lookup_replacements.py','tools/validate_native_node_descriptor_lookup_tu15.py','tools/native_node_resource_lifecycle_replacements.py','tools/native_selected_operand_binding.py','tools/specialize_native_operands.py','tools/validate_native_tu15_prt_helpers.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def body(source,entry):return next(b for m,b in function_bodies(source)if int(m[1],16)==entry)
def corpus():
 rng=random.Random(0x1e3150);rows=[]
 live=('0000000000000080ff3f','35c26821a2da0fc90040','45230100000000c0ff7f','00000000000000000080','123456789abcde80fe3f','01000000000000000000','00000000000000800040','00000000000000800100')
 for node_index in (0,1,3,0x7fff,0x8000,0xfffe,0xffff):
  for descriptor_index in (0xffff,0,1,0xfffe):
   for variant in range(8):
    n=len(rows);banks=[bytearray(rng.randbytes(size))for _,size in REGIONS];sp=0x62008000+n%4
    descriptor=0x61000100;foreign=0x61000400;slots=0x61001000;objects=0x61002000
    node=0x61004000+n%4;foreign_node=0x61006000+n%4
    signed_node=struct.unpack('<h',struct.pack('<H',node_index))[0];signed_descriptor=struct.unpack('<h',struct.pack('<H',descriptor_index))[0]
    nodes=(node-signed_node*108)&0xffffffff;foreign_nodes=(foreign_node-signed_node*108)&0xffffffff
    def put(at,value,width=4):
     for (base,size),bank in zip(REGIONS,banks):
      if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little');return
     raise ValueError('Node descriptor fixture escaped backing '+hex(at))
    def get(at):
     for (base,size),bank in zip(REGIONS,banks):
      if base<=at<=base+size-4:return struct.unpack_from('<I',bank,at-base)[0]
     raise ValueError('Node descriptor lookup escaped backing')
    put(descriptor+0x30,nodes);put(foreign+0x30,foreign_nodes);put(descriptor+0x4c,4);put(descriptor+0x50,slots)
    put(slots+signed_descriptor*32+4,0xa55a0003)if descriptor_index!=0xffff else None
    put(0x4f0414,objects);put(objects+3*16+8,foreign)
    for at in (node,foreign_node):put(at+0x16,0,1);put(at+0x30,0xffff,2);put(at+0x32,0xffff,2)
    enabled=descriptor_index==0xffff and variant>=4
    put(0x51df20,1 if enabled else 0,1);put(0x51df21,1,1)
    handle=(node_index<<16)|descriptor_index;put(sp,0x63000000);put(sp+4,handle)
    gp=[rng.getrandbits(32)for _ in range(8)];gp[2]=descriptor;gp[4]=sp
    name=f'node-{node_index:x}-descriptor-{descriptor_index:x}-variant-{variant}'
    if node_index==0 and descriptor_index==0xffff and variant==4:name='control_real_lifecycle'
    rows.append(dict(id=n,name=name,entry=0x1e3150,gp=gp,matrix=handle,source=get(sp+8),eflags=0x202|(rng.getrandbits(12)&0xcd5),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10),((n%8)<<11)|(n%64)|((n*0x100)&0x4700),0x1f80|((n%4)<<13)|(0x8040 if n&4 else 0)],live_x87=list(live[:(0,1,4,8)[n%4]]),max_steps=10000,banks=[dict(address=base,before=data.hex())for (base,_),data in zip(REGIONS,banks)]))
 def extra(seed,name,edit):
  row=copy.deepcopy(seed);row.update(id=len(rows),name=name)
  def put(at,value,width=4):
   for bank in row['banks']:
    data=bytearray.fromhex(bank['before']);offset=at-bank['address']
    if 0<=offset<=len(data)-width:
     data[offset:offset+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little');bank['before']=data.hex();return
   raise ValueError('Directed descriptor alias escaped backing '+hex(at))
  edit(row,put);rows.append(row)
 direct=rows[0];foreign=rows[8]
 extra(direct,'zero-node-value',lambda row,put:put(row['gp'][2]+0x30,0))
 def direct_saved_si(row,put):
  sp=row['gp'][4];row['gp'][2]=sp-0x38;row['gp'][6]=0x61004000;put(sp-8,0xdeadbeef)
 extra(direct,'node-table-alias-saved-si',direct_saved_si)
 def count_and_slots(row,put):
  sp=row['gp'][4];row['gp'][2]=sp-0x54;row['gp'][6]=4;row['gp'][3]=0x61001000;put(sp-8,0xdeadbeef);put(sp-4,0xdeadbeef)
 extra(foreign,'count-slots-alias-saved-si-bx',count_and_slots)
 def slots_di(row,put):
  sp=row['gp'][4];row['gp'][2]=sp-0x5c;row['gp'][7]=0x61001000;put(sp-16,4);put(sp-12,0xdeadbeef)
 extra(foreign,'slots-alias-saved-di',slots_di)
 def datum_di(row,put):
  sp=row['gp'][4];row['gp'][7]=0xa55a0003;put(row['gp'][2]+0x50,sp-16);put(sp-12,0xa55a0004)
 extra(foreign,'datum-alias-saved-di',datum_di)
 def objects_di(row,put):
  sp=row['gp'][4];row['gp'][7]=0x61000400;put(0x4f0414,sp-20-3*16);put(sp-12,0)
 extra(foreign,'object-alias-saved-di',objects_di)
 def nodes_di(row,put):
  sp=row['gp'][4];row['gp'][7]=0x61006000;put(0x61002000+3*16+8,sp-0x3c);put(sp-12,0xdeadbeef)
 extra(foreign,'foreign-node-table-alias-saved-di',nodes_di)
 def global_objects(row,put):
  row['gp'][4]=0x4f0418;row['gp'][3]=0x61002000;put(0x4f0414,0xdeadbeef);put(0x4f0418,0x63000000);put(0x4f041c,row['matrix']);put(0x4f0420,row['source'])
 extra(foreign,'global-object-table-alias-saved-bx',global_objects)
 def global_stack(row,put):
  row['gp'][4]=0x51df24;row['gp'][3]=0x101;put(0x51df24,0x63000000);put(0x51df28,row['matrix']);put(0x51df2c,row['source'])
 extra(direct,'global-enable-alias-saved-bx-real-loop',global_stack)
 extra(rows[40],'foreign-zero-node-value',lambda row,put:put(0x61000400+0x30,0xffffff94))
 return rows

def prepare(scratch,require_binding):
 from native_node_resource_lifecycle_replacements import canonical_body as lifecycle_body,PREFIX as lifecycle_prefix
 from native_node_resource_replacements import canonical_body as node_body
 from native_record_lookup_replacements import canonical_body as record_body
 from specialize_native_operands import specialize
 snapshots={p:(ROOT/p).read_text()for p in COPIES};rendered='';rejected=0
 original_children={0x1e5c80:lifecycle_body(body(snapshots[COPIES[0]],0x1e5c80)),0x1e5af0:node_body(body(snapshots[COPIES[0]],0x1e5af0)),0x1e3270:record_body(body(snapshots[COPIES[0]],0x1e3270))}
 declarations='\n'.join('static void run_'+f'{entry:x}'+'(WorldState&);'for entry in (0x1e3150,0x1e5c80,0x1e5af0,0x1e3270))
 declarations+='\nstatic void run_126200(WorldState&){throw std::runtime_error("Actual resource owner was outside this normal-return cohort");}\n'
 dispatcher='static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t,bool){if(target==0x1e5c80u)run_1e5c80(s);else if(target==0x1e5af0u)run_1e5af0(s);else if(target==0x1e3270u)run_1e3270(s);else throw std::runtime_error("Undeclared genuine child boundary "+std::to_string(target));}'
 literal=lambda s:re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',s)
 for index,path in enumerate(COPIES):
  source=snapshots[path];current=body(source,0x1e3150);original=binding.canonical_body(current);bound=binding.replace_node_descriptor_lookup(source)
  if require_binding and bound!=source:raise ValueError('Node descriptor production binding missing '+path)
  if binding.replace_node_descriptor_lookup(bound)!=bound:raise ValueError('Node descriptor binding not idempotent')
  for shape in (original,literal(original)):
   raw=source.replace(current,shape,1)
   for value in (raw,binding.replace_node_descriptor_lookup(raw)):
    target=body(value,0x1e3150)
    for old,new in (('0x6cu','0x6du'),('s.reg(3,4,0)','s.reg(3,2,0)')):
     mutation=value.replace(target,target.replace(old,new,1),1)
     if mutation==value:raise ValueError('Missing descriptor mutation witness')
     try:binding.replace_node_descriptor_lookup(mutation)
     except ValueError:rejected+=1
     else:raise ValueError('Node descriptor admitted numerical body drift')
  mutated=source.replace(lifecycle_prefix,'',1)
  try:binding.replace_node_descriptor_lookup(mutated)
  except ValueError:rejected+=1
  else:raise ValueError('Node descriptor admitted missing native lifecycle')
  selected=next(line for line in source.splitlines()if line.startswith('std::uint32_t selected('))
  dispatcher_match=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
  lifecycle=body(source,0x1e5c80)
  from native_node_resource_lifecycle_replacements import INCLUDE as lifecycle_include
  mutants=[source.replace(dispatcher_match,dispatcher_match.replace('run_1e5c80(s);','run_1e3150(s);',1),1),source.replace(dispatcher_match,dispatcher_match.replace('s.required(site,target,tail);','s.required(site,target+1u,tail);',1),1),source.replace(lifecycle,lifecycle.replace('0x32u','0x30u',1),1),source.replace(lifecycle_include,'',1),bound.replace(binding.PREFIX,binding.PREFIX.replace('::bridge','::changed_bridge'),1)]
  if 'selected('in original:mutants.append(source.replace(selected,selected.replace('return p;','return p+1u;',1),1))
  for mutation in mutants:
   if mutation in(source,bound):raise ValueError('Missing descriptor routing mutation witness')
   try:binding.replace_node_descriptor_lookup(mutation)
   except ValueError:rejected+=1
   else:raise ValueError('Descriptor admitted changed owner routing/resolver/body')
  native_children='\n'.join(literal(body(snapshots[COPIES[0]],entry))for entry in (0x1e5c80,0x1e5af0,0x1e3270))
  refs='\n'.join(literal(value)for value in original_children.values())
  native_prefix=declarations+'\n'+dispatcher+'\n'+selected+'\n'+native_children+'\n'
  reference_prefix=declarations+'\n'+dispatcher+'\n'+selected+'\n'+refs+'\n'
  prepared,_=specialize(bound);prepared_body=body(prepared,0x1e3150)
  if binding.PREFIX not in prepared_body or 's.instruction_literal('not in prepared_body:raise ValueError('Prepared descriptor admission route drift')
  for name,prefix,value in ((f'bound{index}',native_prefix,body(bound,0x1e3150)),(f'specialized{index}',native_prefix,prepared_body),(f'reference{index}',reference_prefix,original),(f'specialized_reference{index}',reference_prefix,binding.canonical_body(prepared_body))):rendered+='namespace '+name+' {\n'+prefix+value+'\n}\n'
  if index:rendered+=f'namespace literal_reference{index} {{\n'+reference_prefix+literal(original)+'\n}\n'
 (scratch/'node_descriptor_lookup_bodies.inc').write_text(rendered);return rejected

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
 if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
 if sys.platform!='linux'or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not args.xbe.is_file():return 77
 if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,candidate_unbound=not args.require_binding,scratch_owner=str(scratch),retained_large_artifacts=[])
 try:
  pins={path:sha(ROOT/path)for path in INPUTS};rejected=prepare(scratch,args.require_binding)
  from oracle_smg_firing_1_1 import Xbe
  import oracle_native_campaign_screen_lifecycle_tu15 as common
  from validate_native_bsp_surface_polygon_abi_tu15 import metadata
  old_metadata=common.metadata
  def private_metadata(image):
   result=metadata(image,old_metadata);code=common._Code(image,'.text')
   for site in result['sites']:
    instruction=code.at(site['instruction'])
    if instruction.mnemonic=='pop'or instruction.mnemonic.startswith('ret'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
   return result
  common.metadata=private_metadata;image=Xbe(args.xbe);rows=corpus()
  sites={int(value,16)for value in re.findall(r'0x([0-9a-f]+)u',(ROOT/'src/physics/runtime/native_tu15_node_descriptor_lookup.hpp').read_text())if 0x1e3150<=int(value,16)<=0x1e31d3 or int(value,16)==0x1e5c80}
  observed,proof=physical_original(image,rows,scratch,entries=(0x1e3150,0x1e5c80,0x1e3270,0x1e5af0),regions=REGIONS,trace_calls=[dict(entry=pc,arguments=int(pc in(0x1e3150,0x1e5c80)),gp=True,xmm=True,x87_delta=0)for pc in sorted(sites)],trace_extended=True)
  cases=[]
  for row,out in zip(rows,observed['cases'],strict=True):
   if not out['completed']or not out['live_x87_unchanged']:raise ValueError('Original node descriptor failed normal/live80 contract '+row['name'])
   case=dict(row)
   for key in ('boundary_gp','boundary_eflags','boundary_xmm','after_fp','reads','writes'):case[key]=out[key]
   case['packets']=[{key:packet[key]for key in('entry','arguments','gp','eflags','xmm','fp','live_x87')}for packet in out['calls']];case['final_pc']=out['calls'][-1]['entry']
   case['banks']=[dict(before,after=after['after'])for before,after in zip(row['banks'],out['banks'],strict=True)];cases.append(case)
  fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=cases,explicit_read_sites=sorted({event['instruction']for row in observed['cases']for event in row['reads']})),separators=(',',':')))
  binary=scratch/'descriptor-tests';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_node_descriptor_lookup_tests.cpp'),'-o',str(binary)]
  if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
  compile=subprocess.run(command,capture_output=True,text=True)
  if compile.returncode:raise ValueError('Descriptor compile failed: '+compile.stderr[-6500:])
  run=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=180)
  if run.returncode:raise ValueError('Descriptor ABI mismatch: '+run.stderr[-7000:])
  controls=[]
  for specialized in(False,True):
   env=dict(os.environ,HALO2_REFERENCE_NODE_DESCRIPTOR_LOOKUP='1',HALO2_NODE_DESCRIPTOR_LOOKUP_REFERENCE_TEST='1',HALO2_REFERENCE_NODE_RESOURCE_LIFECYCLE='1',HALO2_REFERENCE_NODE_RESOURCE='1',HALO2_REFERENCE_RECORD_LOOKUP='1')
   if specialized:env['HALO2_NODE_DESCRIPTOR_LOOKUP_SPECIALIZED_REFERENCE_TEST']='1'
   control=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=60,env=env)
   if control.returncode:raise ValueError('Forced descriptor reference failed: '+control.stderr[-5000:])
   result=json.loads(control.stdout)
   if not result['native_refused']or not result['lifecycle_entries']or not(result['callbacks']+result['literal_evaluations']):raise ValueError('Descriptor reference did not execute genuine parent/lifecycle code')
   if specialized and not result['literal_evaluations']:raise ValueError('Prepared descriptor reference skipped literal continuation')
   controls.append(result)
  if any(sha(ROOT/path)!=pin for path,pin in pins.items()):raise ValueError('Descriptor proof input changed')
  executed={pc for row in observed['cases']for pc in row['executed_sites']}
  from generate_physics_live_support_tu15 import complete_body
  owner=set(complete_body(common._Code(image,'.text'),0x1e3150).code)
  report.update(json.loads(run.stdout),source_sha256=pins,physical_original=proof,binding_rejections=rejected,reference_controls=controls,parent_coverage=dict(total=len(owner),executed=len(owner&executed),missing=[hex(pc)for pc in sorted(owner-executed)]),qualification='Full normal parent ABI, backed banks, ordered reads/stores and complete parent pre-I/O plus genuine lifecycle call/return packets',limitations=['Variable-address read/write faults and observer exceptions remain unqualified','No broader child per-read ABI claim beyond its existing independent normal-return qualification']);print(run.stdout.strip())
 except BaseException as error:report.update(passed=False,error=str(error));raise
 finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
 return 0
if __name__=='__main__':raise SystemExit(main())
