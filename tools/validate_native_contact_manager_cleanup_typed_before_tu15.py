#!/usr/bin/env python3
"""Actual typed manager BEFORE -> original3232B0 and native storage lifetime."""
import argparse,inspect,os,sys
from pathlib import Path
import validate_native_contact_manager_cleanup_actual_tu15 as parent

def main():
 parser=argparse.ArgumentParser(add_help=False);parser.add_argument('--capture',type=Path,required=True);parser.add_argument('--archive-sha256',required=True);a,rest=parser.parse_known_args();sys.argv=[sys.argv[0],*rest];assert os.environ.get('HALO2_TEST_SCRATCH')
 source=inspect.getsource(parent.main);needle=" env=dict(collector.__dict__);env['main']=None;";assert source.count(needle)==1
 extra='''
 source=source.replace(" assert os.environ.get('HALO2_TEST_SCRATCH')", " ARCHIVE=Path("+repr(CAPTURE)+");PIN="+repr(ARCHIVE_PIN)+"\\n assert os.environ.get('HALO2_TEST_SCRATCH')")
 source=source.replace("capture=json.loads((ARCHIVE.parent/'manifests/capture.json').read_text());", "capture=json.loads((ARCHIVE.parent/'capture-native-before.json').read_text());assert capture['schema']=='native-tu15-contact-manager-typed-before-v1';typed=capture['input'];assert typed['owner']==0x3232b0 and typed['preserved_x87_depth']==0;")
 source=source.replace('assert word(0x10082380)==0x41f124 and word(0x10082390)==0x41f478', "assert word(typed['manager'])==0x41f124 and word(typed['manager']+0x10)==0x41f478")
 begin=source.index(' child.cases=lambda:');end=source.index('\\n source=inspect.getsource(child.main)',begin)
 rows=" child.cases=lambda:[dict(name='actual_typed_manager_'+str(n),entry=0x3232b0,receiver=typed['manager'],arguments=[flag],before_fp_input=[typed['floating_point']['x87_control_word'],typed['floating_point']['x87_status'],typed['floating_point']['mxcsr']],user_words=[])for n,flag in enumerate((typed['disposing'],0,3))]\\n"
 source=source[:begin]+rows+source[end:]
 source=source.replace("'tools/validate_native_contact_manager_cleanup_actual_tu15.py'", "'tools/validate_native_contact_manager_cleanup_actual_tu15.py','tools/validate_native_contact_manager_cleanup_typed_before_tu15.py'")
 needle=" env=dict(child.__dict__);env['main']=None;"
 fp_needle='environment=dict(base.__dict__,observer_source=observer_source);'
 fp_change="source=source.replace("+repr("case['before_fp']=fp")+","+repr("fp=case.get('before_fp_input',fp);case['before_fp']=fp")+");"+fp_needle
 append_needle="activation.write_text(activation.read_text()+(ROOT/'analysis/physics/native-activation-collector-export-20261101/append.cpp.inc').read_text())"
 append_change="activation.write_text(activation.read_text()+(('' if 'void native_tu15_destroy_activation_collector('in activation.read_text()else (ROOT/'analysis/physics/native-activation-collector-export-20261101/append.cpp.inc').read_text())))"
 transform=' source=source.replace('+repr(fp_needle)+','+repr(fp_change)+').replace('+repr(append_needle)+','+repr(append_change)+')\\n'
 assert source.count(needle)==1;source=source.replace(needle,transform+needle)
 source=source.replace('Actual initialized04 heap collector;', 'Actual typed selected manager cleanup BEFORE;').replace('Direct actual04 collector DATA;', 'Actual typed selected manager cleanup DATA;')
'''
 # CAPTURE/PIN are literal input paths/hashes in generated source, never shell.
 extra=extra.replace('repr(CAPTURE)',repr(repr(str(a.capture.resolve())))).replace('repr(ARCHIVE_PIN)',repr(repr(a.archive_sha256)))
 source=source.replace(needle,extra+needle);env=dict(parent.__dict__);env['main']=None;exec(source,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
