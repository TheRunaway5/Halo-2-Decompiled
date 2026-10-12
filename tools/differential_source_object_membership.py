#!/usr/bin/env python3
"""Re-run untouched sphere membership with the retained Xbox PC53 FP state.

The prior independent oracle stays unchanged. Its executable and report paths
are replaced in memory and reset additionally restores FPCW027F/MXCSR1F80.
"""
import argparse
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary',type=Path,required=True)
parser.add_argument('--append-fixture',type=Path)
args=parser.parse_args()
source=ROOT/'analysis/multiplayer-shading/object-spatial-membership-original.py'
text=source.read_text()
text=text.replace("exe=ROOT/'build-mp-lightmap/object_light_spatial_tests'",
    'exe=Path('+repr(str(args.binary.resolve()))+')')
text=text.replace('object-spatial-membership-original.json',
    'object-membership-pc53-original.json')
marker=' u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_ESP,STACK)'
if text.count(marker)!=1:
    raise RuntimeError('original membership reset seam changed')
text=text.replace(marker,marker+';u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)')
if args.append_fixture:
    text='frozen=[]\n'+text
    marker=" actual=list(map(int,subprocess.check_output([str(exe),'--portal',str(path)],input='\\n'.join(rows)+'\\n',text=True).split()))"
    if text.count(marker)!=1:raise RuntimeError('portal fixture export seam changed')
    text=text.replace(marker," for i,row in enumerate(rows):\n  if i%16==0:frozen.append('asset '+str(path.relative_to(ROOT))+' '+ ' '.join(format(v&0xffffffff,'x') for v in [1,0,*map(int,row.split())])+' | '+format(wanted[i],'x'))\n"+marker)
    marker=" actual=[list(map(int,row.split())) for row in subprocess.check_output([str(exe),'--membership',str(path)],input='\\n'.join(rows)+'\\n',text=True).splitlines()];assert len(actual)==len(wanted)"
    if text.count(marker)!=1:raise RuntimeError('membership fixture export seam changed')
    text=text.replace(marker," for i,row in enumerate(rows):\n  if i%8==0 or i==len(rows)-1:frozen.append('asset '+str(path.relative_to(ROOT))+' '+ ' '.join(format(v&0xffffffff,'x') for v in [1,1,*map(int,row.split())])+' | '+' '.join(format(v&0xffffffff,'x')for v in wanted[i]))\n"+marker)
namespace={'__name__':'__main__','__file__':str(source)}
exec(compile(text,str(source),'exec'),namespace)
report_path=ROOT/'analysis/multiplayer-shading/object-membership-pc53-original.json'
report=json.loads(report_path.read_text());report['fpcw']='027F';report['mxcsr']='1F80'
if args.append_fixture:
    with args.append_fixture.open('a')as output:output.write('\n'.join(namespace['frozen'])+'\n')
    report['frozen_authored_cases']=len(namespace['frozen'])
report_path.write_text(json.dumps(report,indent=2)+'\n')
