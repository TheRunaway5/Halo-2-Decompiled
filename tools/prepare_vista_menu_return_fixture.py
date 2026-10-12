#!/usr/bin/env python3
"""Project frozen original observer traces into a portable consumer TSV.

Expected actions come only from the unchanged-original report. This does not
execute or consult the native policy implementation.
"""
from pathlib import Path
import hashlib,json,struct
ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'analysis/frontend/vista-session-flow-audit/pcr-return-original/report.json'
PIN='0ea73aa561775fdcd0e20c232f7598e1991c497ef2b0084fef4f619bff3c2e9b'
FIELDS=['screen','world_ready','world_kind','network_enabled','lifecycle_enabled','kind','protocol','ui_disabled','ui_active','controllers','task_a','task_b','task_state','task_retained','task_blocks','closing','world_gate','world_gate_b','controller']
QUERIES={'screen':0,'protocol':3,'ui_active':7,'task_gate_a':9,'task_gate_b':10,'task_state':11,'task_blocks':12,'local_controller_count':13,'closing':14,'world_gate':15,'world_gate_b':16,'controller':17,'loading':18,'loading_secondary':19}
ACTIONS={'set_ui_active':0,'ui_channel':2,'ui_channel_close':3,'session_close':4,'screen_factory':5,'pcr16_request':6,'mainmenu_request':7,'other_mainmenu_request':8,'observer_suffix':9}
b=SOURCE.read_bytes();assert hashlib.sha256(b).hexdigest()==PIN
r=json.loads(b);rows=[]
for x in r['records']:
 if x['kind']!='U':continue
 q=x['input'];out=x['output'];actions=[];queries=[]
 for e in out['events']:
  name=e['service']
  if name=='screen_factory':args=e['descriptor']
  elif name=='mainmenu_request':args=[e['reason']]
  elif name=='observer_suffix':args=[q['screen'],e['ecx'],e['args'][0]]
  else:args=e.get('args',[])
  if name in ACTIONS:actions.append((ACTIONS[name],args))
  elif name in QUERIES:queries.append((QUERIES[name],e['value']))
 # Source writes863B3C, independently retained after execution. Even a second
 # task-query value equal to retained storage can still be a write: identify
 # the actual branch by its TWO task queries, not value inequality.
 if sum(e['service']=='task_state' for e in out['events'])==2:
  actions.insert(0,(1,[out['retained_task_after']]))
 words=[*[q[f]&0xffffffff for f in FIELDS],out['retained_task_after'],len(actions)]
 for kind,args in actions:words += [kind,len(args),*args]
 words += [len(queries)]
 for kind,value in queries:words += [kind,value]
 rows.append('U\t'+x['label']+'\t'+str(len(words))+'\t'+'\t'.join(f'{v&0xffffffff:08x}' for v in words))
p=ROOT/'tests/fixtures/vista_menu_return_observer_original.tsv'
p.write_text('# H2V_MENU_RETURN_OBSERVER_ORIGINAL_V1\n# U label words inputs19 retained_after actions_count {action argc args...} query_count {query value}\n'+'\n'.join(rows)+'\n')
print(json.dumps({'path':str(p),'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'cases':len(rows),'original_report_sha256':PIN}))
