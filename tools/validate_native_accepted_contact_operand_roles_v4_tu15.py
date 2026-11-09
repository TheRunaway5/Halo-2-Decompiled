#!/usr/bin/env python3
"""Exact V4 semantic role metadata guards and alias discriminators."""
import argparse,hashlib,json,os,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-accepted-body-contact-semantic-stage-v4-20261103/staged'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);r=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],interface_only=True,compilers=[])
 try:
  source=ROOT/'tools/probe_native_accepted_contact_operand_roles_v4_tu15.cpp';probe=scratch/'probe.cpp';shutil.copyfile(source,probe);include=scratch/'include';shutil.copytree(STAGE/'include',include);pin={str(p.relative_to(include)):sha(p)for p in include.rglob('*')if p.is_file()};r['header_pins']=pin;r['probe_sha256']=sha(probe);r['validator_sha256']=sha(__file__)
  variants={'normal':include};header='halo2/native_tu15_accepted_contact_role_memory.hpp'
  controls={'raw_read_bypass':('require_accepted_retained(p,out.size());memory_.read(p,out);','memory_.read(p,out);'),'retained_writer_bypass':('require_accepted_retained(p,n);return domain_.observes_retained_store(pc,p,n);','return domain_.observes_retained_store(pc,p,n);'),'request_role_bypass':('if(!contains(r.first,r.bytes,p,n))refuse("Native accepted request read has wrong DATA role");','(void)r;')}
  for name,(old,new)in controls.items():
   variant=scratch/name;shutil.copytree(include,variant);file=variant/header;s=file.read_text();assert s.count(old)==1;file.write_text(s.replace(old,new));variants[name]=variant
  r['mutant_header_pins']={name:sha(path/header)for name,path in variants.items()if name!='normal'}
  for c in ('g++','clang++'):
   cases=[]
   for name,headers in variants.items():
    binary=scratch/(c+'-'+name);command=[c,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(headers),str(probe),'-o',str(binary)];b=subprocess.run(command,capture_output=True,text=True);(a.output/(c+'-'+name+'-build.log')).write_text(b.stdout+b.stderr);assert b.returncode==0,b.stderr
    q=subprocess.run([str(binary)],capture_output=True,text=True);(a.output/(c+'-'+name+'.log')).write_text(q.stdout+q.stderr)
    if name=='normal':assert q.returncode==0,q.stderr;v=json.loads(q.stdout);assert v['passed'] and v['controls']==63 and v['role_refusals']==41;cases.append(v)
    else:
     reasons={'raw_read_bypass':'Defined private alias accepted as a retained read','retained_writer_bypass':'Retained row writer aliased private DATA before preimage check','request_role_bypass':'Request role accepted forged scalar address'};assert q.returncode==1 and q.stderr.strip()==reasons[name],q.stderr;cases.append(dict(mutant=name,rejected=True,expected_failure=q.stderr.strip()))
   r['compilers'].append(dict(compiler=c,results=cases))
  assert sha(source)==r['probe_sha256'] and sha(__file__)==r['validator_sha256'] and all(sha(STAGE/'include'/n)==v for n,v in pin.items());r['passed']=True
 except BaseException as e:r['error']=str(e)or repr(e);raise
 finally:r['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(r,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
