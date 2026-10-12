#!/usr/bin/env python3
"""Export literal complete source dispatcher/storage buffers and service order."""
import argparse,hashlib,json,struct
from pathlib import Path
def n(v):return int(v,16)
def words(v):return' '.join(f'{x:x}'for x in v)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('report',type=Path);p.add_argument('notification',type=Path);p.add_argument('output',type=Path);a=p.parse_args();r=json.loads(a.report.read_text())
 # Owning report is explicitly relative to the repository; find its retained
 # local composition without relying on an inferred process-working directory.
 owning=a.report.parent/'begin-observation/composition/report.json';c=json.loads(owning.read_text());local=json.loads(a.notification.read_text())
 lines=['# original-unit-action-dispatch-v1','# report-sha256 '+hashlib.sha256(a.report.read_bytes()).hexdigest(),'# notification-sha256 '+hashlib.sha256(a.notification.read_bytes()).hexdigest()]
 for d in r['descriptors']:lines.append('DESC '+words([d['type']])+' '+d['raw10_hex'])
 def objects(label,rows):
  for o in rows:lines.append(label+' '+o['datum']+' '+o['header_hex']+' '+(o['body_hex']or'-'))
 def state(label,s):
  objects(label,s['objects']);lines.append(label+'PLAYER '+s['player']['datum']+' '+s['player']['raw21c_hex'])
 def hp(call,after=False):
  return next(p['raw_hex']for p in call['after_packets'if after else'packets']if p['field']=='original_header_pool_complete')
 supported={0xe7f30,0xe7ef0,0xe7ff0,0xe7e60,0xb8250,0x1eb150}
 stores_count=leaves_count=case_count=0
 for call in c['calls']:
  entry=n(call['entry'])
  if entry not in supported:continue
  regs={k:n(v)for k,v in call['entry_cpu']['registers'].items()};stack=[n(x)for x in call['stack24_words']];req='-';reqptr=None
  if entry==0xe7f30:
   unit=regs['eax'];packet=next(p for p in call['packets']if p['field']=='edi_original_action_request20');req=packet['raw_hex'];reqptr=n(packet['pointer']);kind=int.from_bytes(bytes.fromhex(req)[:4],'little')
  elif entry==0xe7ef0:
   unit=stack[0];kind=regs['ecx'];child=next(s for s in c['calls']if s['entry']=='000e7f30'and s['caller']=='000e7f26'and call['order']<s['order']<call['return_order']);packet=next(p for p in child['packets']if p['field']=='edi_original_action_request20');req=packet['raw_hex'];reqptr=n(packet['pointer'])
  elif entry==0xe7ff0:unit=regs['ecx'];kind=regs['edi']
  elif entry==0xe7e60:unit=stack[0];kind=0
  elif entry==0xb8250:unit=regs['eax'];kind=0
  else:unit=regs['eax'];kind=stack[0]
  defined=n(call['return_cpu']['registers']['eax'])&255 if entry in(0xe7f30,0xe7ef0,0xe7e60)else 0
  lines.append('CASE '+words([entry,unit,kind,defined])+' '+req);lines.append('HPB '+hp(call));lines.append('HPA '+hp(call,True));state('B',c['states'][call['before_state']]);state('A',c['states'][call['after_state']])
  # Exact ESP identifies the immediate indirect callback, despite recursively
  # repeated E7F49 return addresses inside grenade begin.
  delta={0xe7f30:24,0xe7ef0:64,0xe7ff0:16,0xe7e60:32}.get(entry)
  leafs=[]
  if delta is not None:
   leafs=[s for s in c['calls']if s['caller']in('000e7f49','000e8023','000e7ec0')and call['order']<s['order']<call['return_order']and n(s['entry_cpu']['esp'])==n(call['entry_cpu']['esp'])-delta]
  for s in leafs:
   phase=0 if s['caller']=='000e7f49'else 1 if s['caller']=='000e7ec0'else 2
   typ=kind if phase==0 else n(s['stack24_words'][1]);leafreq=req if phase==0 else'-'
   lines.append('LEAF '+words([n(s['entry']),unit,typ,phase,n(s['return_cpu']['registers']['eax'])&255])+' '+leafreq);state('LB',c['states'][s['before_state']]);state('LA',c['states'][s['after_state']]);lines.append('END_LEAF');leaves_count+=1
  before=c['states'][call['before_state']];ownips={0xe7f30:{0xb8288},0xe7ef0:{0xb8288},0xe7ff0:{0xe8040},0xe7e60:{0xe7ed3},0xb8250:{0xb8288}}.get(entry,set())
  if entry==0xe7ef0:ownips|={0xe7ef5,0xe7ef8,0xe7efc,0xe7f00,0xe7f04,0xe7f08,0xe7f0c,0xe7f11,0xe7f1d}
  for st in c['stores']:
   pc=n(st['instruction']);at=n(st['address']);size=st['size']
   if not(call['order']<st['order']<call['return_order']and pc in ownips):continue
   if any(s['order']<st['order']<s['return_order']for s in leafs):continue
   identity=unit;tag=offset=None
   if reqptr is not None and reqptr<=at and at+size<=reqptr+32:tag=2;offset=at-reqptr
   if pc==0xb8288:
    headers=n(next(p['pointer']for p in call['packets']if p['field']=='original_header_pool_complete'));index=(at-headers)//12;raw=bytes.fromhex(hp(call));identity=(struct.unpack_from('<H',raw,index*12)[0]<<16)|index;tag=0;offset=(at-headers)%12
   for o in before['objects']:
    body=n(o['body_address'])
    if body<=at and at+size<=body+o['body_size']:identity=n(o['datum']);tag=1;offset=at-body
   assert tag is not None,(hex(entry),st)
   lines.append('STORE '+words([pc,identity,tag,offset,size,int.from_bytes(bytes.fromhex(st['before_hex']),'little'),int.from_bytes(bytes.fromhex(st['written_hex']),'little')]));stores_count+=1
  lines.append('END_CASE');case_count+=1
 for i,b in enumerate(r['body_allocation_calls']):
  clear=b['before_source_clear'];alloc=b['allocation_call'];id=n(clear['datum']);lines.append('BOOT '+words([b['requested_size'],id]));lines.append('HPB '+b['before_pool']['headers_hex']);lines.append('HPA '+b['after_pool']['headers_hex']);lines.append('HEADER_ALLOC '+alloc['header_hex']);lines.append('BODY_ALLOC '+clear['header_hex']+' '+clear['body_hex']);objects('B',b['before_objects']);objects('A',b['after_objects'])
  for st in r['body_allocation_stores']:
   if st['call']!=i:continue
   pc=n(st['instruction']);at=n(st['address']);size=st['size'];tag=offset=None
   if pc==0xbd1b6:tag=0;offset=at-n(clear['header_address'])
   elif pc in(0xbd1c6,0xbd1cd):tag=1;offset=at-n(clear['body_address'])
   else:continue
   lines.append('STORE '+words([pc,id,tag,offset,size,int.from_bytes(bytes.fromhex(st['before_hex']),'little'),int.from_bytes(bytes.fromhex(st['written_hex']),'little')]));stores_count+=1
  lines.append('END_CASE');case_count+=1
 for i,b in enumerate(r.get('mark_update_calls',[])):
  lines.append('MARK '+b['unit']);lines.append('HPB '+b['before_pool']['headers_hex']);lines.append('HPA '+b['after_pool']['headers_hex']);objects('B',b['before_objects']);objects('A',b['after_objects']);raw=bytes.fromhex(b['before_pool']['headers_hex']);headers=n(b['before_pool']['headers'])
  for st in r['mark_update_stores']:
   if st['call']!=i:continue
   at=n(st['address']);index=(at-headers)//12;id=(struct.unpack_from('<H',raw,index*12)[0]<<16)|index
   lines.append('STORE '+words([n(st['instruction']),id,0,(at-headers)%12,st['size'],int.from_bytes(bytes.fromhex(st['before_hex']),'little'),int.from_bytes(bytes.fromhex(st['written_hex']),'little')]));stores_count+=1
  lines.append('END_CASE');case_count+=1
 for row in local['cases']:
  lines.append('LOCAL '+words([n(row['player_datum']),row['type'],row['defined_result_byte']]));lines+=['VIEWB '+row['before_view_hex'],'VIEWA '+row['after_view_hex'],'CLOCK '+row['raw_clock_hex'],'BPLAYER '+row['player_datum']+' '+row['before_player_hex'],'APLAYER '+row['player_datum']+' '+row['after_player_hex']]
  for st in local['stores']:
   if st['case']!=row['index']:continue
   lines.append('STORE '+words([n(st['instruction']),0,3,st['offset'],st['size'],int.from_bytes(bytes.fromhex(st['before_hex']),'little'),int.from_bytes(bytes.fromhex(st['written_hex']),'little')]));stores_count+=1
  lines.append('END_CASE');case_count+=1
 raw=('\n'.join(lines)+'\n').encode();a.output.parent.mkdir(parents=True,exist_ok=True)
 if a.output.exists():assert a.output.read_bytes()==raw
 else:a.output.write_bytes(raw)
 print(json.dumps(dict(cases=case_count,callbacks=leaves_count,stores=stores_count,sha256=hashlib.sha256(raw).hexdigest())))
if __name__=='__main__':main()
