#!/usr/bin/env python3
"""Replay the supplied PC23 instructions against explicit service boundaries.

Uses the frozen source-pinned recovery executor, not a Python reimplementation
of the owner. Writes only new vista_lobby_setup fixtures/evidence.
"""
from pathlib import Path
import hashlib,importlib.util,json,sys
ROOT=Path(__file__).resolve().parents[1]
RECOVERY=ROOT/'analysis/frontend/vista-session-flow-audit/pc14-details-pc23-ownership-original'
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/pc23-setup-original'
RECOVERY_SHA='f7797cd9076afcce54c0b378a5f818041a3f17f4465a68293b25c0d7bf84dd0f'
CASE_SHA='ddc2a5bf62fae65f4c400ac6113c2dff5f63d2756093d196f2da33402cc342ec'
EVIDENCE_SHA='ed066dc79a73d83c3b6471d71bc14e43bb46f3af6b6e8021685dcc7530fc78f1'
OPS={'registration','labels','details','dispatch','open_map','open_rules','open_quick','quick_request','setup_initialize'}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 for p,h in ((RECOVERY/'audit.py',RECOVERY_SHA),(RECOVERY/'cases.json',CASE_SHA),(RECOVERY/'evidence.json',EVIDENCE_SHA)):
  assert sha(p)==h,(p,sha(p))
 # The recovery module itself imports an older source audit named'audit'.
 sys.path.insert(0,str(RECOVERY))
 spec=importlib.util.spec_from_file_location('pc23_setup_recovery',RECOVERY/'audit.py')
 audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)
 from unicorn import UC_HOOK_CODE
 from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP
 audit.TAGBANK=audit.tag_bank();pe=audit.PE(audit.EXE,audit.EXE_SHA)
 original_machine=audit.machine;held={};current={}
 def machine(image):
  u=original_machine(image);held['u']=u;allocation=0
  def args(_u,pc,_size,_):
   nonlocal allocation
   if pc==0x658927 and current.get('op')=='quick_request':
    sp=u.reg_read(UC_X86_REG_ESP)
    audit.w32(u,sp+8,current.get('argument2',4));audit.w32(u,sp+12,current.get('argument3',4))
   #4667A0 is a declared allocation service. Override only its returned handle
   #at the genuine caller return instructions; original AND/index/store runs.
   if current.get('handles') and pc in (0x64fb8b,0x64fba6,0x64fbbf,0x64fbe8,0x64fc03,0x64fc1c,0x64fc4c,0x64fc7b,0x64fc96):
    u.reg_write(UC_X86_REG_EAX,current['handles'][allocation]);allocation+=1
  u.hook_add(UC_HOOK_CODE,args)
  return u
 audit.machine=machine
 inputs=[r['input'] for r in json.loads((RECOVERY/'cases.json').read_text()) if r['input']['op'] in OPS]
 inputs.extend([
  dict(op='details',row=1,engine=1,variant=0,texts=1,bitmap=1),
  dict(op='details',row=1,engine=2,row_exists=0,texts=1,bitmap=1),
  dict(op='details',row=0,ref=0,texts=1,bitmap=1),
  dict(op='details',row=0,custom=1,custom_valid=0,texts=1,bitmap=1),
  dict(op='details',row=0,custom=1,custom_valid=0x80,provider=0x80,texts=1,bitmap=1),
  dict(op='setup_initialize',enum=3,texts=1,descriptor=0),
  dict(op='quick_request',key=26,mask=0x80000001,argument2=9,argument3=12),
  dict(op='quick_request',key=31,mask=0,argument2=0xffffffff,argument3=0x80000000),
  dict(op='registration',enum=3,host=0x80,count=16,handles=[0xabcd0005,0x120002,0xaaaa0007]),
  dict(op='registration',enum=5,host=0x80,count=2,handles=[0xffff0006,0x120003,0xabcd0001,0x120005]),
 ])
 for row in (-32768,-1,0,1,2,3,4,5,6,7,8,32767):inputs.append(dict(op='dispatch',row=row,handle=0xabcd0000))
 rows=[]
 for q in inputs:
  current.clear();current.update(q);r=audit.execute(pe.data,q)
  if q['op']=='labels':r['label_result']=held['u'].reg_read(UC_X86_REG_EAX)&255
  if q['op']=='registration':
   r['row_bank_slots']=[audit.half(held['u'],audit.ARRAY+i*4+2) for i in range(8)]
   if q.get('handles'):r['rows']=[audit.half(held['u'],audit.ARRAY+(h&0xffff)*4+2) for h in q['handles']]
  rows.append(r)
 assert len(rows)==939,len(rows)
 fixture=ROOT/'tests/fixtures/vista_lobby_setup_original.json'
 fixture.write_text(json.dumps(rows,separators=(',',':'))+'\n')
 old=json.loads((RECOVERY/'evidence.json').read_text())
 evidence=dict(source_sha256=audit.EXE_SHA,map_sha256=audit.MAP_SHA,
  recovery_tool_sha256=RECOVERY_SHA,recovery_cases_sha256=CASE_SHA,recovery_evidence_sha256=EVIDENCE_SHA,
  tool_sha256=sha(Path(__file__)),fixture_sha256=sha(fixture),cases=len(rows),instructions=sum(r['instructions'] for r in rows),
  source_spans={k:v for k,v in old['source_spans'].items() if k.startswith('setup_') or k in (
   'quick_request','map_info_by_identifier','map_query','map_query_session_owner','map_query_record',
   'map_factory','rules_factory','quick_factory_generic','quick_factory_engine','bitmap_resource_owner','bitmap_frame_setter')},
  source_tables=old['source_tables'],assets=old['assets'],limits=[
   'Original owner instructions execute; service leaves use explicit controlled inputs and ordered call capture.',
   'Base widget construction, row allocation, image resolution, map identifier lookup and session queries remain explicit native binding boundaries.',
   '64FA19 callbacks3/5/6/7 are captured original target identities, not native reimplementations of their session or XLive owners.',
   '61CFED resource override remains separate from61C300 authored frame; dynamic variant text formatting is not substituted.',
   'This corpus does not establish live frontend, GPU, pixel, loading or gameplay parity.'])
 evidence['source_spans']['setup_generic_labels']=pe.span(0x6139f8,0x613a73)
 OUT.mkdir(exist_ok=True);(OUT/'evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
 print(f"PASS original Vista lobby setup cases={len(rows)} instructions={evidence['instructions']} fixture_sha256={evidence['fixture_sha256']}")
if __name__=='__main__':main()
