#!/usr/bin/env python3
"""Authenticate108 AABB asset-opcode classes and native control/refusal groups.

Only Source309960/309967 and1F76BA fetch immutable CODE dispatch tables. No physics DATA
read, captured bank or Source write is excluded. Existing physical arithmetic
qualification remains separate and is pinned, rather than reimplemented here.
"""
import argparse,ast,hashlib,json,os,re,struct
from pathlib import Path
from native_contact_process_before_input_tu15 import NativeContactProcessBeforeInput,sha
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from validate_native_contact_candidates_bound_actual_tu15 import ARCHIVE,ARCHIVE_PIN
ROOT=Path(__file__).resolve().parents[1]
TARGETS=(0x309e97,0x309cbd,0x309c7b,0x309c88,0x309c9e,0x309d59,
 0x309d7c,0x309da8,0x309b15,0x30996e,0x30998d,0x3099b6,0x3099d4,
 0x3099fc,0x309a1a,0x309a3f,0x309a65,0x309a8e,0x309ab7,0x309b57,
 0x309b8e,0x309c03,0x309c2d,0x309eef,0x309ea1,0x309ea7,0x309eb6,
 0x309ece,0x309de3,0x309df3,0x309e0c,0x309e60)
GROUPS=('return','scale','jump','key_base','projected_split','single_split',
 'offset_split','slab','wide_slab','leaf','property')
def digest(b):return hashlib.sha256(b).hexdigest()
def native_groups(text):
 body=text[text.index('    void run('):text.index('\n};',text.index('    void run('))]
 conditions=re.findall(r'^            if\((opcode[^\n]*?)\)(?:\{|return;)',body,re.M)
 assert len(conditions)==len(GROUPS),(conditions,len(conditions))
 compiled=[]
 for expression in conditions:
  expression=re.sub(r'(0x[0-9a-f]+|\d+)u\b',r'\1',expression).replace('&&',' and ').replace('||',' or ')
  tree=ast.parse(expression,mode='eval')
  assert all(isinstance(x,(ast.Expression,ast.Compare,ast.BoolOp,ast.And,ast.Or,ast.Eq,ast.GtE,ast.LtE,ast.Name,ast.Load,ast.Constant))for x in ast.walk(tree))
  assert all(x.id=='opcode'for x in ast.walk(tree)if isinstance(x,ast.Name))
  compiled.append(compile(tree,'native-opcode-condition','eval'))
 assert 'if(opcode&3u)throw std::runtime_error("Native MOPP AABB requires its additional property-slot lifetime owner")'in body
 return [next((GROUPS[i]for i,c in enumerate(compiled)if eval(c,{'__builtins__':{}},{'opcode':op})),'diagnostic')for op in range(256)],digest(body.encode())
def source_group(target):
 if target==TARGETS[0]:return'return'
 if target==TARGETS[1]:return'scale'
 for first,last,name in((2,4,'jump'),(5,7,'key_base'),(8,18,'projected_split'),(19,19,'single_split'),(20,20,'offset_split'),(21,21,'slab'),(22,22,'wide_slab'),(23,27,'leaf'),(28,30,'property'),(31,31,'diagnostic')):
  if target in TARGETS[first:last+1]:return name
 raise AssertionError('Unknown Source handler')
def verify(classes,handlers,text):
 assert tuple(handlers)==TARGETS,'Authenticated handler semantics changed'
 groups,pin=native_groups(text);rows=[]
 for op in range(256):
  target=handlers[classes[op]]if op<108 else TARGETS[31]
  source=source_group(target);assert groups[op]==source,(hex(op),source,groups[op])
  rows.append(dict(opcode=op,source_class=classes[op]if op<108 else None,source_target=target,native_group=groups[op],native_domain='explicit-property-lifetime-refusal'if source=='property'and op&3 else'explicit-invalid-program-refusal'if source=='diagnostic'else'qualified-semantic-branch'))
 return rows,pin
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  before=NativeContactProcessBeforeInput(ARCHIVE,ARCHIVE_PIN,scratch/'before',0x304350);image=Xbe(NEW_DEFAULT);assert sha(NEW_DEFAULT)==NEW_PIN
  classes=image.read(0x309fb0,108);raw=image.read(0x309f30,128);handlers=struct.unpack('<32I',raw)
  assert before.read(0x309fb0,108)==classes and before.read(0x309f30,128)==raw,'Actual captured immutable CODE table differs from XBE'
  text=(ROOT/'src/physics/native_tu15_mopp_aabb_traversal.cpp').read_text();rows,control_pin=verify(classes,handlers,text)
  code=_Code(image,'.text');assert code.at(0x309960).mnemonic=='movzx'and code.at(0x309967).mnemonic=='jmp'
  negatives=[]
  for name,c,h,t in [('captured_class',bytes([classes[0]^1])+classes[1:],handlers,text),('handler_target',classes,(handlers[0]^1,*handlers[1:]),text),('native_scale_boundary',classes,handlers,text.replace('opcode<=4u','opcode<=3u',1))]:
   try:verify(c,h,t)
   except AssertionError:negatives.append(name)
   else:raise AssertionError('CODE mapping negative accepted '+name)
  component_source=(ROOT/'src/physics/native_tu15_collision_component.cpp').read_text();begin=component_source.index('bool native_tu15_collision_component_enabled(');finish=component_source.index('NativeTu15ContactComponent native_tu15_resolve_surface_component(',begin);component_control=component_source[begin:finish]
  key_table=image.read(0x1f77ac,20);assert before.read(0x1f77ac,20)==key_table
  key_handlers=struct.unpack('<5I',key_table);assert key_handlers==(0x1f7711,0x1f77a7,0x1f775b,0x1f775b,0x1f76c1)
  def verify_key_control(t):
   assert 'switch(kind)'in t and 'case 1:{'in t and 'case 2:return true;'in t and 'case 5:{'in t and 'case 3:case 4:{'in t
   assert 'default:throw std::runtime_error("Native TU1.5 collision key requires its unsupported encoded-kind owner")'in t
   assert t.index('case 1:{')<t.index('case 2:return true;')<t.index('case 5:{')<t.index('case 3:case 4:{')
  verify_key_control(component_control)
  try:verify_key_control(component_control.replace('case 2:return true;','case 2:return false;'))
  except AssertionError:negatives.append('native_key_kind2_return')
  else:raise AssertionError('Native key control mutation accepted')
  assert code.at(0x1f76ba).mnemonic=='jmp'and code.at(0x1f76b9).mnemonic=='dec'
  component=ROOT/'analysis/physics/native-mopp-aabb-traversal-20261102-third/report.json'
  if not component.exists()or not json.loads(component.read_text()).get('passed'):
   candidates=[]
   for f in(ROOT/'analysis/physics').glob('native-mopp-aabb-traversal-20261102-*/report.json'):
    data=json.loads(f.read_text())
    if data.get('passed'):candidates.append(f)
   assert candidates;component=sorted(candidates)[-1]
  assert json.loads(component.read_text())['passed']
  tables=[dict(address=0x309fb0,bytes=108,sha256=digest(classes),hex=classes.hex()),dict(address=0x309f30,bytes=128,sha256=digest(raw),hex=raw.hex()),dict(address=0x1f77ac,bytes=20,sha256=digest(key_table),hex=key_table.hex())]
  report.update(passed=True,scope=__doc__,original_sha256=NEW_PIN,before=before.receipt,source_hashes={str(Path(__file__).relative_to(ROOT)):sha(Path(__file__)),
   'src/physics/native_tu15_mopp_aabb_traversal.cpp':sha(ROOT/'src/physics/native_tu15_mopp_aabb_traversal.cpp'),'src/physics/native_tu15_collision_component.cpp':sha(ROOT/'src/physics/native_tu15_collision_component.cpp')},native_control_sha256=control_pin,native_component_control_sha256=digest(component_control.encode()),immutable_dispatch_CODE_tables=tables,dispatch_fetches=[dict(instruction=0x309960,table=0x309fb0,bytes=1,entries=108,instruction_hex=code.at(0x309960).bytes.hex()),dict(instruction=0x309967,table=0x309f30,bytes=4,entries=32,instruction_hex=code.at(0x309967).bytes.hex()),dict(instruction=0x1f76ba,table=0x1f77ac,bytes=4,entries=5,instruction_hex=code.at(0x1f76ba).bytes.hex())],opcode_classes=rows,exhaustive_u8_values=256,authored_opcode_classes=108,valid_key_classes=[dict(kind=k,source_handler=h,native_group=('bsp','true','object','object','instance')[k-1])for k,h in enumerate(key_handlers,1)],native_unqualified_key_kinds=[0,6,7],negative_controls=negatives,component_arithmetic_proof=dict(path=str(component.relative_to(ROOT)),sha256=sha(component)),captured_bank_exclusions=[],physics_data_read_exclusions=[],raw_original_read_ledger_retained_red='native-contact-candidates-bound-actual1479-20261102-fifth/g++/evidence/actual-report.json')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':main()
