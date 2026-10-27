#!/usr/bin/env python3
"""Targeted hardware watchpoints on unchanged original TU1.5 world memory.

Requires run_test.py ownership. Reuses an actual retained failure input without
editing reference banks, expected outputs, code, or comparison exclusions.
Only bounded debugger diagnostics survive cleanup; ELF/banks stay in scratch.
"""
import argparse,hashlib,json,os,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True);p.add_argument('--report-output',type=Path,required=True);p.add_argument('--watch',type=lambda s:int(s,0),action='append');a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();a.capture=a.capture.resolve();a.report_output.mkdir(parents=True,exist_ok=True)
    watches=a.watch or[0x1008238c,0x100823cc,0x100823fc];assert 1<=len(watches)<=3 and all(p%4==0 for p in watches)
    assert sha(NEW_DEFAULT)==NEW_PIN;d=json.loads((a.capture/'capture.json').read_text());assert d['entry']==0x1cc8d0 and d['reference']=='English TU1.5 only'
    replay=scratch/'replay';command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(a.capture),'--output',str(replay)]
    r=subprocess.run(command,capture_output=True,text=True);assert (replay/'original-hardware').exists(),(r.returncode,r.stderr[-2000:])
    code=_Code(Xbe(NEW_DEFAULT),'.text');assert code.at(0x1ccaca).mnemonic=='ret'
    script=['set pagination off','set confirm off','set print elements 64','set disable-randomization off',f'run > "{scratch/"debuggee-output.bin"}"']
    for n,address in enumerate(watches,1):
        script += [f'watch *(unsigned int*)0x{address:x}',f'commands {n}','silent',f'printf "WATCH{n} target=0x{address:x} post_pc=%#x\\n", $pc',
          'info registers eax ecx edx ebx esp ebp esi edi eip','x/9i $pc-16','x/28wx $esp','x/32wx 0x10082380','continue','end']
    script += ['hbreak *0x1ccaca',f'commands {len(watches)+1}','silent','printf "ORIGINAL_ROOT_RET\\n"','info registers esp eip','quit','end','continue']
    commands=scratch/'watch.gdb';commands.write_text('\n'.join(script)+'\n');gdb_command=['gdb','-q','-nx','-batch','-x',str(commands),str(replay/'original-hardware')]
    debug=subprocess.run(gdb_command,capture_output=True,text=True,timeout=120);log=debug.stdout+'\nSTDERR:\n'+debug.stderr;assert len(log.encode())<1024*1024,'Unexpected unbounded watchpoint output'
    (a.report_output/'watchpoints.log').write_text(log)
    assert debug.returncode==0 and'ORIGINAL_ROOT_RET'in log,(debug.returncode,log[-2000:])
    report=dict(completed=True,capture=str(a.capture),image_sha256=NEW_PIN,root=0x1cc8d0,watches=[hex(p)for p in watches],reference_compare_exit=r.returncode,debugger_exit=debug.returncode,
      replay_command=command,debugger_command=gdb_command,source_hashes={str(p.relative_to(ROOT)):sha(p)for p in (Path(__file__),ROOT/'tools/replay_live_phantom_hardware_tu15.py')},
      capture_metadata_sha256=sha(a.capture/'capture.json'),watchpoint_events=log.count('post_pc='),retained_large_artifacts=[],scope='Diagnostic hardware data-watchpoints only; original code/inputs and comparison exclusions unchanged')
    (a.report_output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('completed','watchpoint_events','reference_compare_exit','retained_large_artifacts')}))
if __name__=='__main__':main()
