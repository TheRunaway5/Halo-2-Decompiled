#!/usr/bin/env python3
"""Compare exact descriptor-update frames; retain only a small JSON receipt."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--optimized',type=Path,required=True)
    parser.add_argument('--reference',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    report={'scope':'Exact controlled baked-material descriptor frame equivalence; no gameplay framebuffer or FPS claim','runs':[]}
    with tempfile.TemporaryDirectory(prefix='renderer-descriptors-',dir=os.environ.get('HALO2_TEST_SCRATCH')) as name:
        scratch=Path(name)
        for label,executable,extra in [('optimized',args.optimized,[]),('active',args.optimized,['--active-updates']),('reference',args.reference,[])]:
            output=scratch/label
            command=[str(executable.resolve()),str(output),*extra]
            result=subprocess.run(command,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
            report['runs'].append({'argv':command,'returncode':result.returncode,'output':result.stdout})
            print(result.stdout,end='',flush=True)
            if result.returncode:break
        report['frames']=[]
        if len(report['runs'])==3 and all(run['returncode']==0 for run in report['runs']):
            reference=sorted((scratch/'reference').glob('*.ppm'))
            for label in ('optimized','active'):
                optimized=sorted((scratch/label).glob('*.ppm'))
                if [p.name for p in reference]!=[p.name for p in optimized] or len(reference)!=9:
                    raise RuntimeError('incomplete descriptor frame comparison')
                for a,b in zip(reference,optimized):
                    old,new=a.read_bytes(),b.read_bytes()
                    report['frames'].append({'mode':label,'name':a.name,'bytes':len(new),'reference_sha256':hashlib.sha256(old).hexdigest(),
                                             'optimized_sha256':hashlib.sha256(new).hexdigest(),'equal':old==new})
        report['passed']=len(report['frames'])==18 and all(frame['equal'] for frame in report['frames'])
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print('Descriptor framebuffer comparison:',report['passed'])
    return 0 if report['passed'] else 1

if __name__=='__main__':raise SystemExit(main())
