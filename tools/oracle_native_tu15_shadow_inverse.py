#!/usr/bin/env python3
"""Capture genuine unchanged C3DC0→C4879 inverse frames and caller controls.

Only declared parent inputs precede execution. Baseline frames execute the
whole original parent/math closure. Separate liveness controls never qualify
an original outcome and are explicitly marked as post-return perturbations.
"""
import argparse,copy,gzip,hashlib,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
sha=common.sha
BASE,CAMERA,VIEW,RECORDS,COUNT=0x1000000,0x1000100,0x1000200,0x1001000,0x1000800
ENTRIES=(0xc3dc0,0x121fb0,0x144090,0x166c90,0x1454d0,0x122130,0x30bd0)
def bits(f):return struct.unpack('<I',struct.pack('<f',f))[0]
def prepare(image,meta,scratch):
 p=scratch/'physical-original';p.mkdir();assembly='.code32\n';layout=[]
 for n,section in enumerate(image.sections):
  data=image.read(section.va,section.vsize);raw=p/f'image-{n}.bin';raw.write_bytes(data);name=f'image{n}';flags='a'+('w'if section.flags&1 else'')+('x'if section.flags&4 else'')
  assembly+=f'.section .{name},"{flags}"\n.incbin "{raw}"\n';layout.append(f'.{name} 0x{section.va:x}:{{*(.{name})}}')
 for name,address,size in(('inputs',BASE,0x10000),('stack',geometry.STACK,65536)):
  assembly+=f'.section .{name},"aw"\n.space {size}\n';layout.append(f'.{name} 0x{address:x}:{{*(.{name})}}')
 assembly+='''.section .stop,"ax"
int3
.section .bridge,"ax"
.globl _start
_start:
fninit
int3
jmp _start
''';layout +=[f'.stop 0x{geometry.STOP:x}:{{*(.stop)}}','.bridge 0x70000000:{*(.bridge)}']
 (p/'reference.s').write_text(assembly);(p/'link.ld').write_text('ENTRY(_start)\nSECTIONS{'+''.join(layout)+'}\n');(p/'metadata.json').write_text(json.dumps(meta))
 for command in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_tu15_shadow_inverse.cpp'),'-o',str(p/'observer')]):
  r=subprocess.run(command,text=True,capture_output=True);assert r.returncode==0,r.stderr[-6000:]
 return p
def prepare_fault(image,meta,scratch,name,input_region,stack_region):
 p=scratch/name;p.mkdir();assembly='.code32\n';layout=[];phdrs=[]
 def section(label,address,data,flags):
  raw=p/(label+'.bin');raw.write_bytes(data);mode='a'+('w'if flags&2 else'')+('x'if flags&1 else'');assembly_line=f'.section .{label},"{mode}"\n.incbin "{raw}"\n'
  phdrs.append(f'{label} PT_LOAD FLAGS({flags});');layout.append(f'.{label} 0x{address:x}:{{*(.{label})}} :{label}');return assembly_line
 for n,s in enumerate(image.sections):
  if s.vsize:assembly+=section('image'+str(n),s.va,image.read(s.va,s.vsize),4+(2 if s.flags&1 else 0)+(1 if s.flags&4 else 0))
 if input_region:assembly+=section('inputs',input_region[0],bytes(input_region[1]),6)
 assembly+=section('stack',stack_region[0],bytes(stack_region[1]),6)
 assembly+=f'.section .bridge,"ax"\n.globl _start\n_start:\nfninit\nint3\njmp _start\n';phdrs.append('bridge PT_LOAD FLAGS(5);');layout.append('.bridge 0x70000000:{*(.bridge)} :bridge')
 (p/'reference.s').write_text(assembly);(p/'link.ld').write_text('ENTRY(_start)\nPHDRS{\n'+'\n'.join(phdrs)+'\n}\nSECTIONS{\n'+'\n'.join(layout)+'\n}\n');(p/'metadata.json').write_text(json.dumps(meta))
 for command in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_tu15_shadow_inverse_faults.cpp'),'-o',str(p/'observer')]):
  r=subprocess.run(command,text=True,capture_output=True);assert r.returncode==0,r.stderr[-6000:]
 return p
def inputs():
 rows=[]
 def make(name,kind=1,width=2.,height=2.,near=1.,far=10.,position=(0.,0.,0.),basis=(1.,0.,0.),up=(0.,1.,0.),backward=(0.,0.,-1.),depth=0,rounding=0,denorm=0,changes=(),records=RECORDS,stack_pointer=geometry.SP):
  n=len(rows);data=bytearray((k*13+n*7+3)&255 for k in range(0x10000))
  def put(at,value,size=4):data[at-BASE:at-BASE+size]=int(value&((1<<(size*8))-1)).to_bytes(size,'little')
  for k in range(0x100//4):put(CAMERA+k*4,0);put(VIEW+k*4,0)
  for p,v in(zip((CAMERA+0x28,CAMERA+0x2c,CAMERA+0x30),backward)):put(p,bits(v))
  for p,v in(zip((CAMERA+0x34,CAMERA+0x38,CAMERA+0x3c),up)):put(p,bits(v))
  put(CAMERA,bits(1.));put(CAMERA+4,bits(1.));put(CAMERA+0x14,bits(1.));put(CAMERA+0x24,bits(1.))
  put(VIEW,kind,2)
  for p,v in(zip((VIEW+0x1c,VIEW+0x20,VIEW+0x24),basis)):put(p,bits(v))
  for p,v in(zip((VIEW+0x2c,VIEW+0x30,VIEW+0x34),position)):put(p,bits(v))
  for p,v in(zip((VIEW+0x3c,VIEW+0x40,VIEW+0x44),(0.,0.,-1.))):put(p,bits(v))
  for p,v in((VIEW+0x28,near),(VIEW+0x58,width),(VIEW+0x5c,height),(VIEW+0x64,far)):put(p,bits(v))
  for p,v in changes:put(p,v)
  sp=stack_pointer;words=(geometry.STOP,CAMERA,VIEW,records,COUNT);stack=struct.pack('<5I',*words);gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=sp
  live=[struct.pack('<QH',0x8000000000000000+(k<<57),0x3fff+(k%3)).hex()for k in range(depth)]
  rows.append(dict(id=n,name=name,gp=gp,banks=[dict(address=BASE,before=data.hex()),dict(address=sp,before=stack.hex())],before_fp=[0x27f|(rounding<<10),(n%8)<<11,0x1f80|(rounding<<13)|denorm],live_x87=live,control='none'))
 make('normal')
 make('early_width',width=0.)
 make('early_height',height=0.)
 make('next_loop',kind=0)
 make('new_inverse_precision',basis=(1.5,0.,0.),position=(.1,0.,0.))
 make('new_inverse_overflow',basis=(struct.unpack('<f',struct.pack('<I',0x7f7fffff))[0],0.,0.),position=(2.,0.,0.))
 for depth in(0,1,3,4):
  for rounding in range(4):make(f'context_{depth}_{rounding}',depth=depth,rounding=rounding,position=(.1,-.2,.3),basis=(1.5,.25,0.))
 for width,height in((-2.,2.),(2.,-2.),(-2.,-2.),(1e-30,1e30),(1e30,1e-30)):
  make(f'extent_{width}_{height}',width=width,height=height)
 for near in(0.,-.0,-1.,1e-30):make(f'near_{near}',near=near)
 for at in(VIEW+0x1c,VIEW+0x24,VIEW+0x2c,CAMERA+0x28):
  for v in(0x7f800000,0x7fc12345,0x7f812345):make(f'special_{at:x}_{v:x}',changes=((at,v),))
 make('page_split_record',records=BASE+0x1fc8)
 make('page_split_return',stack_pointer=geometry.SP+0x14c,basis=(1.5,0.,0.),position=(.1,0.,0.))
 return rows
def physical(p,rows,name):
 source=p/f'{name}-inputs.json';target=p/f'{name}-outputs.json';source.write_text(json.dumps(rows,separators=(',',':')));r=subprocess.run([str(p/'observer'),str(p/'reference'),str(p/'metadata.json'),str(source),str(target)],text=True,capture_output=True,timeout=300);assert r.returncode==0,r.stdout+r.stderr
 data=json.loads(target.read_text());assert not data['instruction_patches']and data['supplied_original_outcomes']is False;return data['cases']
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);p.add_argument('--prepare-only',action='store_true');a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;meta=common.metadata(image);owned=prepare(image,meta,scratch);declared=inputs();runs=physical(owned,declared,'parents');print([(r['name'],len(r['frames']),r['normal'],r['returns166c90'],r['next_loop'])for r in runs],flush=True)
 if a.prepare_only:
  a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(dict(passed=False,status='boundedinputprobeonly',runs=[{k:v for k,v in r.items()if k not in('banks','frames','trace','parent_writes')}for r in runs],owners=meta['owners'],owned_scratch=str(scratch)),indent=2)+'\n');return
 byname={r['name']:r for r in runs};assert all(r['completed']for r in runs)
 assert byname['normal']['normal']and not byname['early_width']['normal']and not byname['early_height']['normal']and byname['next_loop']['next_loop']==5
 # Controls execute every original instruction/callee. Only after the inverse
 # actually returns do they perturb the expressly dead transport fields.
 controls=[];control_names=('normal','early_width','early_height','next_loop','new_inverse_precision','new_inverse_overflow')
 for row in declared:
  if row['name']in control_names:
   for mode in('opaque','taint','owed_data'):control=copy.deepcopy(row);control['control']=mode;controls.append(control)
 observed_controls=physical(owned,controls,'controls');control_receipts=[]
 for actual in observed_controls:
  baseline=byname[actual['name']];keys=('banks','trace','parent_writes','fp','normal','returns166c90','next_loop','global48c7a6')
  # Unoccupied x87 register bytes are private hardware residue, not owed data.
  def semantics(row):
   result={k:row[k]for k in keys if k!='fp'};result['fp']=row['fp']['fp'];x=row['fp']['x87'];result['x87']=dict(occupied=x['occupied'],values=[(n,v)for n,v in enumerate(x['values'])if x['occupied']&(1<<n)])
   result['gp']=row['gp'];result['defined_flags']=row['eflags']&0xcd5;return result
  same=semantics(actual)==semantics(baseline);assert same==(actual['control']!='owed_data'),(actual['name'],actual['control'],'liveness control differs')
  control_receipts.append(dict(parent=actual['name'],control=actual['control'],semantic_match=same,expected=same if actual['control']!='owed_data'else not same,instructions=actual['instructions']))
 prefix=[]
 for name in('normal','early_width','next_loop','new_inverse_overflow'):
  original=next(r for r in declared if r['name']==name);base=byname[name]
  # A real frame from the first and last cube iteration authenticates both
  # initial and repeated ownership; every selected leaf access is cut.
  for index in([0,5]if name=='next_loop'else[0]):
   frame=base['frames'][index]
   for phase,events in(('before_read',frame['reads']),('before_write',frame['writes']),('after_write',frame['writes'])):
    for n in range(len(events)):
     row=copy.deepcopy(original);row.update(name=f'{name}_leaf{index}_{phase}_{n}',wanted_leaf=index,fault=dict(phase=phase,ordinal=n));prefix.append(row)
 prefix_runs=physical(owned,prefix,'prefix');frames=[]
 for run in runs:
  for frame in run['frames']:frames.append(dict(frame,name=run['name']+'_leaf'+str(frame['frame_index']),parent=run['name'],physical_verified=True))
 for run in prefix_runs:
  assert not run['completed'];frame=run['frames'][-1];assert not frame['completed'];frames.append(dict(frame,name=run['name'],parent=run['name'].split('_leaf',1)[0],physical_verified=True))
 assert any(f['before_fp'][2]!=f['after_fp'][2]for f in frames),'No genuine fresh SIMD status witness'
 parents=[dict(name=r['name'],frames=len(r['frames']),normal=r['normal'],early=r['returns166c90']-r['normal'],next_loop=r['next_loop'],instructions=r['instructions'],trace_sha256=hashlib.sha256(json.dumps(r['trace'],separators=(',',':')).encode()).hexdigest())for r in runs]
 # True unmapped faults replay exact actual CALL inputs. Separate PT_LOADs
 # prevent an ELF segment gap from accidentally backing the absent page.
 true_faults=[]
 for kind,parent,input_region,stack_region in(
  ('read','normal',None,(geometry.STACK,65536)),
  ('write','page_split_record',(BASE+0x2000,4096),(geometry.STACK,65536)),
  ('return','page_split_return',(BASE,0x10000),(geometry.STACK,geometry.SP-geometry.STACK))):
  source=byname[parent]['frames'][0];row=copy.deepcopy(source);row.update(id=0,name='actual_unmapped_'+kind,parent=parent,entry=0x144090,live_x87=source['before_x87']['values'],x87_occupied=source['before_x87']['occupied'],matrix=0,source=0,physical_verified=True,actual_read_fault=kind!='write',actual_write_fault=kind=='write',reference_phase='exact-captured-C4879-input-frame-leaf-replay')
  if kind=='read':row['banks']=copy.deepcopy(source['banks'][1:])
  elif kind=='write':row['banks']=[dict(address=source['gp'][1],before=source['banks'][0]['before'][0x38*2:0x6c*2]),copy.deepcopy(source['banks'][1])]
  else:row['banks']=copy.deepcopy(source['banks'])
  for bank in row['banks']:bank.pop('after',None)
  if kind!='return':row['banks'].append(dict(address=source['gp'][4],before=source['real_return']))
  program=prepare_fault(image,meta,scratch,'fault-'+kind,input_region,stack_region);actual=physical(program,[row],kind)[0];actual.pop('id');banks=actual.pop('banks')
  for before,after in zip(row['banks'],banks):assert before['address']==after['address'];before['after']=after['after']
  row.update(actual);assert not row['completed'];assert row['boundary']['address']==(source['gp'][1]if kind=='read'else source['gp'][0]if kind=='write'else source['gp'][4]),row['boundary']
  true_faults.append(row)
 frames+=true_faults
 fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-complete-C3DC0-caller',instruction_patches=[],supplied_original_outcomes=False,owners=meta['owners'],parent_runs=parents,liveness_controls=control_receipts,cases=frames)
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0));repeat=None
 if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Independent actualcaller repeat differs'
 report=dict(passed=True,fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),parent_runs=len(runs),frames=len(frames),fault_prefixes=len(prefix),actual_unmapped_faults=[dict(name=r['name'],parent=r['parent'],boundary=r['boundary'],provenance=r['reference_phase'])for r in true_faults],liveness_controls=control_receipts,full_parent_GP_definedflags_compared=True,whole_original_instructions=sum(r['instructions']for r in runs+observed_controls+prefix_runs),owners=meta['owners'],actual_caller_return='C4879→144090→C487E',repeat_identical=repeat,owned_scratch=str(scratch),source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_tu15_shadow_inverse.cpp':sha(ROOT/'tools/observe_native_tu15_shadow_inverse.cpp'),'tools/observe_native_tu15_shadow_inverse_faults.cpp':sha(ROOT/'tools/observe_native_tu15_shadow_inverse_faults.cpp')},retained_large_artifacts=[],scope_limits=['FinitegenuineC3DC0callerframes, notwholeparentnativeimplementation','Baselineinstructionsandcalleeoutcomesunchanged; controlsseparatelypost-returnperturbations','TrueunmappedfaultsreplayexactgenuineCALLinputframesonunchangedoriginalleaf, notwholeparentfaultingexecution','OnlyTU15C4879;MapPack142D00andotherinversecallersunqualified'])
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(frames)} genuine original inverse frames/prefixes; {len(control_receipts)} liveness controls',flush=True)
if __name__=='__main__':main()
