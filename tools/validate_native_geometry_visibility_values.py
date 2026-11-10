#!/usr/bin/env python3
"""Compare actual BSP/object admission exports with pinned whole predecessors."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
import native_geometry_visibility_public_replacements as binding
ROOT=Path(__file__).resolve().parents[1]
BSP='src/rendering/world/source_bsp_admission.cpp'
OBJECT='src/gameplay/objects/source_object_admission.cpp'
LINKED=('src/rendering/world/source_visibility.cpp','src/rendering/world/source_portal_visibility.cpp','src/gameplay/objects/object_light_spatial.cpp')
REFERENCES=('tests/native_tu15_visibility_association_bsp_reference.inc','tests/native_tu15_visibility_association_object_reference.inc')
FILES=(BSP,OBJECT,*LINKED,*REFERENCES,'include/halo2/native_tu15_geometry_visibility_kernel.hpp','include/halo2/native_tu15_geometry_visibility_value.hpp','tools/native_geometry_visibility_public_replacements.py','tools/validate_native_geometry_visibility_values.py','tests/native_tu15_geometry_visibility_value_tests.cpp','tools/native_visibility_association_public_replacements.py')
def digest(value):return hashlib.sha256(value).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',default='g++');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--require-binding',action='store_true');args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pins={name:digest((ROOT/name).read_bytes())for name in FILES}
    sources={kind:(ROOT/name).read_text()for kind,name in (('bsp',BSP),('object',OBJECT))};copies=[]
    for kind,source in sources.items():
        bound=binding.replace_public(source,kind)
        if args.require_binding and bound!=source:raise ValueError('Actual public geometry policy DATA reuse missing '+kind)
        if binding.replace_public(bound,kind)!=bound:raise ValueError('Public geometry policy installer is not idempotent '+kind)
        copy=scratch/(kind+'.cpp');copy.write_text(bound);copies.append(copy)
    import native_visibility_association_public_replacements as allocation
    originals={kind:(ROOT/name).read_text().strip()for kind,name in zip(('bsp','object'),REFERENCES)}
    for kind,body in originals.items():
        if allocation.digest(body)!=allocation.OWNERS[kind][1]:raise ValueError('Frozen complete public geometry policy predecessor drift '+kind)
    bsp=sources['bsp'];helpers=bsp[bsp.index('namespace {'):bsp.index('SourceBspSphereClusters source_bsp_sphere_clusters')]
    if allocation.digest(helpers)!='ee9f4e6ad17db72bf7a3dfe06237d4b9117a40def75036966804777c8ea785f5':raise ValueError('Complete BSP admission helper namespace drift')
    reference=scratch/'predecessors.cpp'
    reference.write_text('#include "halo2/source_object_admission.hpp"\n#include <algorithm>\n#include <cmath>\nnamespace association_reference {\nusing namespace halo2;\n'+helpers+originals['bsp'].replace('source_bsp_admission(','bsp(',1)+'\n'+originals['object']+'''
SourceObjectAdmission objects(const SourcePortalVisibilitySet&visible,const ObjectLightSpatialLists&first,const ObjectLightSpatialLists&second,
 std::span<const SourceObjectAdmissionObject> owners,const SourceObjectAdmissionInput&input,std::span<const std::array<std::uint32_t,4>> prior){return admit(visible,first,second,owners,input,prior);}
}
''')
    binary=scratch/'values';command=[args.compiler,'-std=c++20','-O2','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_geometry_visibility_value_tests.cpp'),str(reference),*(str(p)for p in copies),*(str(ROOT/name)for name in LINKED),'-o',str(binary)]
    if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    report=dict(passed=False,owned_scratch=str(scratch),source_sha256=pins,compiler=args.compiler,sanitizers=args.sanitizers,scope='Actual public BSP and object admission list exports against complete frozen predecessors, sharing the full geometry policy; raw ABI independently checked against unchanged i386',retained_large_artifacts=[])
    try:
        compiled=subprocess.run(command,capture_output=True,text=True)
        if compiled.returncode:raise ValueError('Public geometry policy compile failed: '+compiled.stderr[-7000:])
        tested=subprocess.run([str(binary)],capture_output=True,text=True)
        report['diagnostic']=(tested.stdout+tested.stderr)[-7000:]
        if tested.returncode:raise ValueError('Public geometry policy comparison failed: '+report['diagnostic'])
        report['values']=json.loads(tested.stdout)
        changed=[name for name,pin in pins.items()if digest((ROOT/name).read_bytes())!=pin]
        if changed:raise ValueError('Public geometry policy proof input drift: '+', '.join(changed))
        report['passed']=True
    except Exception as error:report['error']=str(error)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({key:report[key]for key in ('passed','values','error')if key in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
