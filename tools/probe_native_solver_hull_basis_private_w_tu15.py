#!/usr/bin/env python3
"""Original-only sensitivity of Source313790's unprovided fourth basis lane.

Declared private BEFORE values are experiments, not actual caller context and
not native-kernel inputs. No native313790 implementation/admission is claimed.
"""
import argparse,copy,hashlib,inspect,json,os,struct
from pathlib import Path
import validate_native_solver_hull_line_tu15 as line
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,original_only=True,native_qualified=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN;image=Xbe(NEW_DEFAULT);base=line.base;physical=base.physical;rows=[];w_address=((physical.SP-4)&~15)-0x84
  seeds=line.cases(image)
  for group,seed in enumerate((seeds[0],seeds[160])):
   for w in (0,0x3f800000,0x02000000,0x60000000,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
    c=copy.deepcopy(seed);raw=bytearray.fromhex(c['banks'][0]['before']);struct.pack_into('<I',raw,0x1580,2);struct.pack_into('<4I',raw,0x1800,0x3f800001,0x40000001,0x40400001,0x3f800000);struct.pack_into('<4I',raw,0x1810,0xbf800001,0x3f000001,0x40800001,0x40000000);c['banks'][0]['before']=raw.hex();c['banks'].append(dict(address=w_address,before=struct.pack('<I',w).hex()));c['banks'].sort(key=lambda b:b['address']);c['entry']=0x313790;c['arguments']=[];c['name']=f'group{group}_privateW{w:08x}';c['declared_private_w']=w;c['group']=group;rows.append(c)
  base.OWNERS=(0x313790,0x313470,0x313390,0x313290,0x2e8d20,0x2e7d90);base.BOUNDARIES=()
  text=inspect.getsource(base.metadata).replace('counts[0]==274','counts[0]==148');namespace=dict(base.__dict__);exec(text,namespace);meta=namespace['metadata'](image,a.output)
  physical.observer_source=line.pair.pair_observer_source
  def boundary(c,observed):assert observed['actual_stop']==physical.STOP and observed['boundary_gp'][4]==physical.SP+4;return None
  physical.boundary=boundary;physical.physical(image,meta,rows,scratch);summary=[]
  for c in rows:
   public=next(b for b in c['banks']if b['address']==0x1000000);after=bytes.fromhex(public['after']);summary.append(dict(group=c['group'],private_w=c['declared_private_w'],after_fp=c['after_fp'],retained_data_sha256=hashlib.sha256(after).hexdigest(),new_count=struct.unpack_from('<I',after,0x1580)[0],append_first_hex=after[0x1520:0x1530].hex(),append_second_hex=after[0x1560:0x1570].hex(),difference2_hex=after[0x1820:0x1830].hex()))
  report.update(passed=True,declared_private_w_address=w_address,source_authority_sha256=NEW_PIN,source_sites=148,cases=len(rows),profiles=summary,source_listing_sha256=meta['listing_sha256'],scope=__doc__,native_inputs_supplied=False,source_w_writer_inside_function_proved=False,private_w_semantic_equivalence_claimed=False,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return 0
if __name__=='__main__':raise SystemExit(main())
