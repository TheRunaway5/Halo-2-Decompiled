#!/usr/bin/env python3
"""Preserve complete public fog policy through the shared DATA selector."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
import native_fog_membership_replacements as binding
ROOT=Path(__file__).resolve().parents[1]
CPP='src/rendering/world/bsp_atmosphere.cpp';CALLER='src/rendering/world/source_bsp_admission.cpp'
CALLER_PIN='1d7103cc43a4bd26a08d04923ef915bc3b43bbcb278070642758f9190d3dc89a'
CALLER_BODY=re.compile(r'bool source_bsp_cluster_contains_fog\(const BspAtmosphere &atmosphere,int cluster,\n    const SourceBspFogAdmissionInput &in\) \{.*?\n\}',re.S)
FILES=(CPP,CALLER,'include/halo2/native_tu15_fog_membership_kernel.hpp','include/halo2/native_tu15_fog_membership_value.hpp','tools/native_fog_membership_replacements.py','tools/validate_native_fog_membership_values.py','tests/native_tu15_fog_membership_value_tests.cpp','tests/native_tu15_fog_membership_value_reference.inc')
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',default='g++');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--require-binding',action='store_true');args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pins={name:binding.digest((ROOT/name).read_text())for name in FILES};reference=(ROOT/FILES[-1]).read_text();original=binding.PUBLIC_BODY.search(reference)[0];caller=CALLER_BODY.search(reference)[0]
    if binding.digest(original)!=binding.PUBLIC_PIN or binding.digest(caller)!=CALLER_PIN:raise ValueError('Complete frozen public fog predecessor drift')
    current_caller=CALLER_BODY.search((ROOT/CALLER).read_text())[0]
    if binding.digest(current_caller)!=CALLER_PIN:raise ValueError('Public BSP fog caller changed')
    planar=original.replace('bool BspAtmosphere::contains_planar(int cluster,std::uint32_t datum) const','bool reference_planar(const BspAtmosphere &self,int cluster,std::uint32_t datum)',1)
    for field in('instance_membership','clusters','fog_planes','fog_palette'):planar=re.sub(r'\b'+field+r'\b','self.'+field,planar)
    planar=planar.replace('contains_planar(int(member),datum)','reference_planar(self,int(member),datum)')
    source_reference=caller.replace('source_bsp_cluster_contains_fog(','reference_source(',1).replace('atmosphere.contains_planar(cluster,in.datum)','reference_planar(atmosphere,cluster,in.datum)')
    (scratch/'fog_membership_value_reference.inc').write_text(planar+'\n'+source_reference+'\n')
    source=(ROOT/CPP).read_text();bound=binding.replace_public(source)
    if args.require_binding and bound!=source:raise ValueError('Actual public fog DATA reuse is not installed')
    if binding.replace_public(bound)!=bound:raise ValueError('Public fog binding is not idempotent')
    copy=scratch/'bsp-atmosphere.cpp';copy.write_text(bound);binary=scratch/'values';command=[args.compiler,'-std=c++20','-O2','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I'+str(scratch),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_fog_membership_value_tests.cpp'),str(copy),str(ROOT/CALLER),'-o',str(binary)]
    if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    compiled=subprocess.run(command,capture_output=True,text=True)
    if compiled.returncode:raise ValueError('Public fog compile failed: '+compiled.stderr[-7000:])
    tested=subprocess.run([str(binary)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
    changed=[name for name,pin in pins.items()if binding.digest((ROOT/name).read_text())!=pin]
    if changed:raise ValueError('Public fog proof source drift: '+', '.join(changed))
    report=dict(passed=tested.returncode==0,owned_scratch=str(scratch),source_sha256=pins,compiler=args.compiler,sanitizers=args.sanitizers,scope='Actual public fog APIs and their complete pinned predecessors; raw ABI independently compared with physical Original',retained_large_artifacts=[],diagnostic=(tested.stdout+tested.stderr)[-7000:])
    if tested.returncode==0:report['values']=json.loads(tested.stdout)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(tested.stdout+tested.stderr);return tested.returncode
if __name__=='__main__':raise SystemExit(main())
