#!/usr/bin/env python3
"""Private complete2cec60 scratch-backed codec scale row vs unchanged TU1.5."""
import argparse,copy,hashlib,json,os,random,re,shlex,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_smg_firing_1_1 import Xbe
from native_mopp_ray_dense_original import physical_original_dense
from native_sphere_frustum_mmx_original import physical_original_mmx
UNITS=('src/rendering/models/source_model_render_tu15_generated.cpp',)
PINS=(('e271e4c17e2618f7c0155e787bfda13982fff799c6a1c6fd1626f637d92bcbcf','57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db','f928af6956a3965aef767189095b9732744dcbdb209a345a3aec57baa618e0d1','457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'),)
REGIONS=((0x510000,8192),(0x61000000,8192),(0x62000000,8192))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def contexts(require_binding=False):
 import native_codec_copy_replacements as binding
 patterns=(r'static void run_2cec60\(WorldState &s\) \{.*?\n\}',r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;')
 result={}
 for name in UNITS:
  source=(ROOT/name).read_text()
  if require_binding and binding.installed_entries(source)!=[f'{entry:x}'for entry in binding.ENTRY_GROUPS[1]]:
   raise ValueError('Installed codec copy scale group is incomplete')
  canonical=binding.remove_codec_copy(source);parts=[re.search(pattern,canonical,re.S)[0]for pattern in patterns]
  result[name]=tuple(hashlib.sha256(part.encode()).hexdigest()for part in parts)
 return result
def corpus(smoke):
 from validate_native_packed_rotation_tu15 import corpus as profiles
 rng=random.Random(0x2cec60);rows=[];mask=0xffffffff
 for i,seed in enumerate(profiles(smoke)):
  row=copy.deepcopy(seed);row.update(name=f'codec-scale-row-{i}',entry=0x2cec60,max_steps=40)
  banks={b['address']:bytearray.fromhex(b['before'])for b in row['banks']};sp=row['gp'][4]
  factory=0x61001000;base=(0x61000040,0x510f60,sp-0x80,0x61000500)[i%4]
  stride=(0x30,0x80000010,0xfffffff4)[i%3];index=(0,2,0xffffffff)[i%3]
  frame=(0,1,0x40000000,0x40000001,0xffffffff,2)[i%6];source=(base+frame*4)&mask
  destination=(source,0x61000800,0x510fa0,sp-0x70)[i%4]
  def put(address,value):
   address&=mask
   for region,bank in banks.items():
    if region<=address<=region+len(bank)-4:bank[address-region:address-region+4]=(value&mask).to_bytes(4,'little');return
   raise ValueError('Codec scale row fixture escaped backing '+hex(address))
  put(source,rng.getrandbits(32));put(0x510f60,factory);put(factory+28,stride);put(factory+16,base-factory-stride*index)
  put(0x510f9c,index);put(0x510f44,frame);put(0x510fa0,destination-28)
  put(sp,0x63000000);put(sp+4,0);put(sp+8,0)
  row['banks']=[dict(address=address,before=bank.hex())for address,bank in sorted(banks.items())];rows.append(row)
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compiler',action='append');p.add_argument('--smoke',action='store_true');p.add_argument('--prefixes',action='store_true');p.add_argument('--mmx',action='store_true');p.add_argument('--sanitizers',action='store_true');p.add_argument('--semantic-mutants',action='store_true');p.add_argument('--require-binding',action='store_true');p.add_argument('--xbe',type=Path,default=NEW_DEFAULT);a=p.parse_args()
 if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use run_test.py')
 import platform
 if platform.system()!='Linux'or platform.machine().lower()not in('x86_64','amd64'):return 77
 if not a.xbe.is_file():print('Original TU1.5 XBE unavailable');return 77
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
 names=('include/halo2/native_tu15_codec_scale_kernel.hpp','src/physics/runtime/native_tu15_codec_scale_row_frame.hpp','tests/native_tu15_codec_scale_row_tests.cpp','tools/validate_native_codec_scale_row_tu15.py','tools/native_mopp_ray_dense_original.py','tools/native_sphere_frustum_mmx_original.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp','tools/oracle_native_campaign_screen_lifecycle_tu15.py','tools/validate_native_tu15_prt_helpers.py')
 tools_before={str(q.relative_to(ROOT)):sha(q)for q in(ROOT/'tools').rglob('*.py')}
 pins={n:sha(ROOT/n)for n in names};headers={q.resolve():sha(q)for tree in('src','include','tests')for q in(ROOT/tree).rglob('*')if q.suffix in('.h','.hpp','.inc','.cpp')};r=dict(passed=False,private_unbound=not a.require_binding,readonly_gp_xmm_before_provider_domain=True,source_before=pins,source_contexts_before=contexts(a.require_binding),owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert tuple(r['source_contexts_before'].values())==PINS,'Complete source context drift'
  assert sha(a.xbe)==NEW_PIN;image=Xbe(a.xbe)
  import oracle_native_campaign_screen_lifecycle_tu15 as common
  old=common.metadata;common.ENTRIES=(0x2cec60,)
  def metadata(image):
   data=old(image);code=common._Code(image,'.text')
   for site in data['sites']:
    if code.at(site['instruction']).mnemonic=='ret':site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
   return data
  common.metadata=metadata;sites=metadata(image)['sites'];assert len(sites)==10;io_sites=[v['instruction']for v in sites if v['operands']];rows=corpus(a.smoke);normal=len(rows);proofs=[]
  if a.mmx:
   rng=random.Random(0x2cec60a11)
   for mask in range(16 if a.smoke else 256):
    row=copy.deepcopy(rows[mask%normal]);row.update(id=len(rows),name=f'codec-scale-row-opaque-{mask}',live_x87=[]);row['before_fp'][1]&=~0x3800;row['active_mmx']=dict(known_mask=mask,physical=[(rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(mask+n)%4].to_bytes(2,'little')).hex()for n in range(8)]);rows.append(row)
  def measure(values,label):
   target=scratch/label;target.mkdir();opaque='active_mmx'in values[0];runner=physical_original_mmx if opaque else physical_original_dense
   result,proof=runner(image,values,target,entries=(0x2cec60,),regions=REGIONS+(((0x64000000,4096),)if opaque else()),trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io_sites],trace_extended=True);proofs.append(proof);return result['cases']
  actual=measure(rows[:normal],'normal')
  if len(rows)>normal:actual+=measure(rows[normal:],'opaque')
  if a.prefixes:
   pending=[]
   for index in list(range(min(4,normal)))+([normal+n for n in(0,1)if normal+n<len(rows)]if a.mmx else[]):
    for phase,key in(('before_read','reads'),('before_write','writes'),('after_write','writes')):
     for ordinal in range(len(actual[index][key])):
      row=copy.deepcopy(rows[index]);row.update(id=len(rows)+len(pending),name=row['name']+'-'+phase+str(ordinal),phase=phase,fault=dict(phase=phase,ordinal=ordinal));pending.append(row)
   for opaque in(False,True):
    selected=[v for v in pending if('active_mmx'in v)==opaque]
    if selected:rows+=selected;actual+=measure(selected,'prefix-'+str(opaque))
  coverage=set()
  for row,observed in zip(rows,actual,strict=True):
   assert observed['completed']==('phase'not in row);coverage.update(observed['executed_sites']);pc=observed['boundary']['instruction']if'phase'in row else 0x2cec86
   row['expected']=dict(entry=pc,gp=observed['boundary_gp'],eflags=observed['boundary_eflags'],xmm=observed['boundary_xmm'],fp=observed['after_fp'],live_x87=observed.get('boundary_live_x87',[]))
   if'active_mmx'in observed:row['expected']['active_mmx']=observed['active_mmx']
   row.update(reads=observed['reads'],writes=observed['writes'],calls=observed['calls'])
   for call in row['calls']:assert call.pop('arguments')==[]
   for bank,after in zip(row['banks'],observed['banks'],strict=True):assert bank['address']==after['address'];bank['after']=after['after']
  fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')));results=[]
  for compiler in a.compiler or('g++','clang++'):
   binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d');command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_codec_scale_row_tests.cpp'),'-o',str(binary),'-MD','-MF',str(dep)]
   if a.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
   done=subprocess.run(command,capture_output=True,text=True);assert done.returncode==0,done.stderr[-7000:]
   for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
    path=Path(value).resolve()
    if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
   done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True);assert done.returncode==0,(done.stdout+done.stderr)[-8000:];defects=[]
   if a.semantic_mutants:
    relative='src/physics/runtime/native_tu15_codec_scale_row_frame.hpp';text=(ROOT/relative).read_text()
    for label,old,new in(('wrong-output-offset','s_.gp[0]+28u','s_.gp[0]+24u'),('wrong-first-row-address','return{g[1]+g[0],0u,1u,0u};','return{g[1]+g[0]+4u,0u,1u,0u};'),('invented-lea-flags','g[1]+=g[2]*4u;','g[1]=s_.flags.plus(g[1],g[2]*4u,4);'),('wrong-escaped-value-register','return s_.gp[2]=word(address);','return s_.gp[1]=word(address);')):
     assert old in text;overlay=scratch/(Path(compiler).name+'-'+label);target=overlay/relative;target.parent.mkdir(parents=True);target.write_text(text.replace(old,new,1));mutant=overlay/'test';changed=list(command);changed.insert(1,'-I'+str(overlay/'src'));changed[changed.index('-o')+1]=str(mutant);built=subprocess.run(changed,capture_output=True,text=True);assert built.returncode==0,built.stderr[-6000:];checked=subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True);diagnostic=checked.stdout+checked.stderr;assert checked.returncode and'codec scale row'in diagnostic,label+diagnostic[-3000:];defects.append(dict(name=label,rejected=True,exit_code=checked.returncode,diagnostic=diagnostic[-4000:]))
   results.append(dict(compiler=compiler,command=command,result=json.loads(done.stdout),compiled_defects=defects))
  r.update(passed=True,physical_paths=len(rows),original_normal_paths=normal,source_sites=sorted(hex(v)for v in coverage),compilers=results,original=proofs)
 except BaseException as error:r['error']=repr(error);raise
 finally:
  for module in tuple(sys.modules.values()):
   filename=getattr(module,'__file__',None)
   if filename and Path(filename).resolve().is_relative_to(ROOT/'tools'):
    name=str(Path(filename).resolve().relative_to(ROOT));pins[name]=tools_before[name]
  r['source_after']={n:sha(ROOT/n)for n in pins};r['source_contexts_after']=contexts(a.require_binding)
  if r['source_after']!=pins or r['source_contexts_before']!=r['source_contexts_after']:r.update(passed=False,source_drift=True)
  (a.output/'report.json').write_text(json.dumps(r,indent=2)+'\n')
 return 0 if r['passed']else 1
if __name__=='__main__':raise SystemExit(main())
