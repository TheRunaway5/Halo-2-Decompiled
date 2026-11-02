#!/usr/bin/env python3
"""Original-only genuine2E full-return retained writer-PC census."""
from pathlib import Path
import json,subprocess,sys
import validate_native_moving_shape_process_full_native_actual_tu15 as full
ROOT=Path(__file__).resolve().parents[1]
SAVED_OBSERVER=full.observer_source
def observer():
 text=SAVED_OBSERVER();text=text.replace('dispatch=J::array(),zero_repeats=J::array();','dispatch=J::array(),zero_repeats=J::array(),writer_events=J::array();')
 old='pending.push_back(J{{"address",p},{"bytes",n},{"before",process_hex(original.read(p,n))}})';assert text.count(old)==1;text=text.replace(old,'pending.push_back(J{{"instruction",pc},{"address",p},{"bytes",n},{"before",process_hex(original.read(p,n))}})')
 old='e.erase("bytes");stores.push_back(e);';assert text.count(old)==1;text=text.replace(old,'e.erase("bytes");writer_events.push_back(e);e.erase("instruction");stores.push_back(e);')
 start=text.index('  J passes=J::array();for(bool read_pass:');end=text.index('  report["profiles"].push_back(',start);text=text[:start]+'  J passes=J::array();\n'+text[end:]
 text=text.replace('{"native_full_return",true}','{"original_full_return",true},{"retained_writer_events",writer_events}')
 return text
def main():
 saved=full.observer_source;original_run=subprocess.run
 def run(command,*args,**kwargs):
  if isinstance(command,list)and command and command[0]in('g++','clang++'):
   command=[p for p in command if not(str(p).startswith(str(ROOT/'src')+'/')and str(p).endswith('.cpp'))]
  return original_run(command,*args,**kwargs)
 full.observer_source=observer;subprocess.run=run
 try:rc=full.main()
 finally:full.observer_source=saved;subprocess.run=original_run
 p=Path(sys.argv[sys.argv.index('--output')+1])/'report.json';r=json.loads(p.read_text());r.update(original_only=True,native_algorithm_executed=False,private_whole_native_attempt=False,production_binding=False,selected_actual_full_return_qualified=False,scope='Original-only actual full2E RET16: retained writer instruction/address/before/written projection. No original outcomes supplied to native; no native algorithm runs.');p.write_text(json.dumps(r,indent=2)+'\n');return rc
if __name__=='__main__':raise SystemExit(main())
