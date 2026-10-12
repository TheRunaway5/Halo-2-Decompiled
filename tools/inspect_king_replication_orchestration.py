#!/usr/bin/env python3
"""Static primary-byte King packet orchestration in retail, TU1.1 and TU1.5.

No packet, game callback, allocator, transport or native implementation executes.
Locate complete unchanged owners, retain all instructions and jump tables, check
actual vtable operands/dispatch edges and report literal ABI and control sites.
"""
from pathlib import Path
import argparse, bisect, csv, hashlib, json, re, struct
from oracle_game_over import SPECS
from oracle_mp_full_scoreboard_1_5 import _Code, literal, complete_tokens, references
from research_mp_lobby_later_update import table_pair
from inspect_render_primitives_title_updates import prefix, section
from xbe import Xbe, FUNCTIONS_CSV
from inventory import read_rows

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer/game-variants/king-replication-orchestration-source-20261012'
OWNERS={
 'storage_constructor':0x81590, 'distribution_allocation':0x81780,
 'world_replication_setup':0x68c00, 'view_connection_setup':0x85650,
 'connection_callback_setup':0x892f0, 'connection_client_iterator':0x891e0,
 'network_packet_read':0x88750, 'aggregate_decode_apply':0x97d80,
 'handle_blocks_decode':0x98cf0, 'handle_block_apply':0x98fb0,
 'creation_identity_admission':0x991d0, 'entry_creation_decode':0x8a6f0,
 'entry_update_decode':0x8aac0, 'entry_creation_apply':0x8a920,
 'entry_update_apply':0x8ac40,
 'entry_delete':0x8ad20, 'handle_state_transition':0x99690,
 'peer_table_initialize':0x893c0, 'peer_chain_creation':0x89950,
 'peer_release':0x89660, 'peer_chain_release':0x89690,
 'peer_allocate':0x89c40, 'peer_create':0x89cc0,
 'match_dirty_publication':0xa7810, 'dirty_mask_publication':0xb58c0,
 'entry_flush_differences':0x8a090, 'periodic_flush_caller':0x8dfc0,
 'peer_pending_mask_publication':0x89710, 'king_world_update':0x2bd680,
 'entity_initialize_dispatch':0xa45e0, 'entity_difference_dispatch':0xa4650,
 'entity_apply_dispatch':0xa4730, 'entity_creation_dispatch':0xa4880,
 'entity_adopt_identity':0xa47a0, 'entity_release_identity':0xa4910,
 'stream_overflowed':0x1946f0,
}
# Resolve the changed allocation owner from its genuine table-constructor
# operand after the unchanged table methods have been independently located.
OWNERS['distribution_allocation']=OWNERS.pop('distribution_allocation')
SETUP_ENTRIES={
 'world_replication_setup':{'tu11':0x68e10,'u5':0x69ff0},
 'view_connection_setup':{'tu11':0x86120,'u5':0x89370},
}
CHANGED_SETUP=set(SETUP_ENTRIES)|{'distribution_allocation'}
CHANGED_LAYOUT={'entry_update_decode','entry_creation_apply','entry_update_apply','entry_delete',
                'dirty_mask_publication','entry_flush_differences','periodic_flush_caller','king_world_update'}
SPECIAL_ORDER=SETUP_ENTRIES.keys()|CHANGED_LAYOUT|{'distribution_allocation'}
for _key in list(OWNERS):
 if _key in SPECIAL_ORDER:OWNERS[_key]=OWNERS.pop(_key)
# Dependencies for actual tables and backwards direct-call discovery.
for _key in ('entry_flush_differences','periodic_flush_caller','distribution_allocation'):
 OWNERS[_key]=OWNERS.pop(_key)

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def call_target(i):
 return int(i.op_str,16) if i.mnemonic=='call' and i.op_str.startswith('0x') else None
def imm(i):
 return int(i.op_str.rsplit(', ',1)[1],16)

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT)
 args=ap.parse_args();args.output.mkdir(parents=True,exist_ok=False)
 indexed=read_rows(FUNCTIONS_CSV);images={};starts={};codes={};entries={};owners={};locators=[]
 for name,spec in SPECS.items():
  path=Path(spec['path']);assert sha(path)==spec['pin'];im=Xbe(path);images[name]=im
  starts[name]={int(r['address'],16) for r in csv.DictReader((ROOT/spec['tsv']).open(),delimiter='\t')}
  codes[name]={s.name:_Code(im,s.name) for s in im.sections if s.name in ('.text','D3D')}
  entries[name]={}
 old=images['retail']
 for key,entry in OWNERS.items():
  of,oi=literal(codes['retail'][section(old,entry).name],entry,starts['retail'])
  entries['retail'][key]=entry;owners['retail',key]=(of,oi)
  # The allocation owner also sizes unrelated pools; it is retained complete
  # even when a title update changes those sizes. Its constructor write, not
  # equivalence of that larger allocation routine, establishes this vtable.
  partial_locator=key=='distribution_allocation'
  pat=prefix(old,entry,64 if partial_locator else int(indexed[entry]['size']))
  for name in ('tu11','u5'):
   im=images[name];accepted=[];candidates=[]
   if key in SETUP_ENTRIES:
    e=SETUP_ENTRIES[key][name];f,ins=literal(codes[name][section(im,e).name],e,starts[name])
    entries[name][key]=e;owners[name,key]=(f,ins)
    locators.append({'owner':key,'image':name,'accepted':e,
     'reason':'Actual unique direct caller of independently matched peer initialization / connection callback setup; checked after all entries resolve.'})
    continue
   if key in ('entry_update_decode','entry_creation_apply','entry_update_apply','entry_delete'):
    needle=struct.pack('<I',entries[name]['entry_creation_decode']);virtual=[]
    for s in im.sections:
     if s.name!='.rdata':continue
     virtual.extend(s.va+m.start()-4 for m in re.finditer(re.escape(needle),im.section_bytes(s)))
    assert len(virtual)==1,(name,virtual)
    slot={'entry_update_decode':6,'entry_creation_apply':2,'entry_update_apply':7,'entry_delete':12}[key]
    e=int.from_bytes(im.read(virtual[0]+slot*4,4),'little');f,ins=literal(codes[name][section(im,e).name],e,starts[name])
    entries[name][key]=e;owners[name,key]=(f,ins)
    locators.append({'owner':key,'image':name,'accepted':e,'actual_table':virtual[0],'slot':slot,
      'reason':'Genuine entity replication vtable, uniquely anchored by unchanged creation decoder.'})
    continue
   if key=='dirty_mask_publication':
    calls=[call_target(i) for i in owners[name,'match_dirty_publication'][1] if call_target(i) is not None]
    assert len(calls)==1,(name,calls)
    e=calls[0];f,ins=literal(codes[name][section(im,e).name],e,starts[name]);entries[name][key]=e;owners[name,key]=(f,ins)
    locators.append({'owner':key,'image':name,'accepted':e,'reason':'Unique actual direct callee of unchanged match dirty publication.'})
    continue
   if key in ('entry_flush_differences','periodic_flush_caller'):
    callee='peer_pending_mask_publication' if key=='entry_flush_differences' else 'entry_flush_differences'
    target=entries[name][callee];edges=[]
    for s in im.sections:
     if s.name not in codes[name]:continue
     raw=im.section_bytes(s)
     for at in range(len(raw)-4):
      if raw[at]!=0xe8:continue
      site=s.va+at
      if (site+5+struct.unpack_from('<i',raw,at+1)[0])&0xffffffff!=target:continue
      ordered=sorted(starts[name]);e=ordered[bisect.bisect_right(ordered,site)-1]
      f,ins=literal(codes[name][s.name],e,starts[name])
      if any(i.address==site and call_target(i)==target for i in ins):edges.append((e,f,ins,site))
    assert len(edges)==1,(name,key,[(v[0],v[3]) for v in edges])
    e,f,ins,site=edges[0];entries[name][key]=e;owners[name,key]=(f,ins)
    locators.append({'owner':key,'image':name,'accepted':e,'actual_callee':target,'site':site,
      'reason':'Unique direct caller, containing original checked call instruction.'})
    continue
   if key=='king_world_update':
    path=ROOT/'analysis/multiplayer/game-variants/king-replication-source-20261012/complete/report.json'
    assert sha(path)=='44c4599ff00ed27dc3124555d98ca3e6c9f4a10565c9dfaf03fa9eb2fa1d6e2d'
    reg=next(r for r in json.loads(path.read_text())['registered_tables'] if r['image']==name)
    e=reg['vtable51'][18];f,ins=literal(codes[name][section(im,e).name],e,starts[name]);entries[name][key]=e;owners[name,key]=(f,ins)
    locators.append({'owner':key,'image':name,'accepted':e,'actual_table':reg['vtable'],'slot':18,
      'reason':'Pinned actual original registered King table, independently constructor-executed by prior research.'})
    continue
   if partial_locator:
    needle=struct.pack('<II',entries[name]['entry_creation_decode'],entries[name]['entry_creation_apply'])
    virtual=[]
    for s in im.sections:
     if s.name!='.rdata':continue
     for m in re.finditer(re.escape(needle),im.section_bytes(s)):
      virtual.append(s.va+m.start()-4)
    assert len(virtual)==1,(name,virtual)
    vt=virtual[0];needle=struct.pack('<I',vt)
    for s in im.sections:
     if s.name not in codes[name]:continue
     for m in re.finditer(re.escape(needle),im.section_bytes(s)):
      va=s.va+m.start();ordered=sorted(starts[name]);e=ordered[bisect.bisect_right(ordered,va)-1]
      candidates.append(e);f,ins=literal(codes[name][s.name],e,starts[name])
      # Teardown rewrites the same vtable. Only allocation also computes
      # the end of the genuinely larger TU distribution allocation.
      has_constructor=any(i.mnemonic=='mov' and re.fullmatch(r'dword ptr \[\w+ \+ 0x2098\], '+hex(vt),i.op_str) for i in ins)
      has_allocation_end=any(i.mnemonic=='lea' and '0xb0c4]' in i.op_str for i in ins)
      if has_constructor and has_allocation_end:accepted.append((e,f,ins))
    assert len(accepted)==1,(key,name,candidates,[v[0] for v in accepted])
    e,f,ins=accepted[0];entries[name][key]=e;owners[name,key]=(f,ins)
    locators.append({'owner':key,'image':name,'table_via_unchanged_methods':vt,'candidates':candidates,'accepted':e,
      'reason':'Actual vtable constructor write; unrelated complete allocation owner may differ.'})
    continue
   for s in im.sections:
    if s.name not in codes[name]:continue
    for m in re.finditer(pat,im.section_bytes(s),re.S):
     e=s.va+m.start()
     candidates.append(e);f,ins=literal(codes[name][s.name],e,starts[name])
     if not partial_locator and complete_tokens(oi,old)!=complete_tokens(ins,im):continue
     topology=table_pair(of,f,oi,ins,old,im)
     if partial_locator or (topology is not None and all(r['topology_equal'] for r in topology)):accepted.append((e,f,ins))
   assert len(accepted)==1,(key,name,candidates,[v[0] for v in accepted])
   e,f,ins=accepted[0];entries[name][key]=e;owners[name,key]=(f,ins)
   locators.append({'owner':key,'image':name,'candidates':candidates,'accepted':e,'prefix_only_locator':partial_locator})
 listings=[];pairs=[];later_pairs=[];tables=[];bindings=[]
 for name,im in images.items():
  for key,entry in entries[name].items():
   f,ins=owners[name,key];p=args.output/f'{name}-{key}-{entry:08x}.asm'
   p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins))
   jt=[]
   for address,width,count in f.tables:
    raw=im.read(address,width*count);jt.append({'address':address,'width':width,'count':count,'bytes_hex':raw.hex(),
      'targets':[int.from_bytes(raw[j*width:(j+1)*width],'little') for j in range(count)]})
   listings.append({'image':name,'owner':key,'entry':entry,'instructions':len(ins),'file':p.name,'sha256':sha(p),
    'indirect_calls':[{'site':i.address,'operand':i.op_str} for i in ins if i.mnemonic=='call' and call_target(i) is None],
    'direct_calls':[{'site':i.address,'target':call_target(i)} for i in ins if call_target(i) is not None], 'jump_tables':jt})
   if name=='retail':continue
   of,oi=owners['retail',key];ot=complete_tokens(oi,old);nt=complete_tokens(ins,im);equal=ot==nt
   if key not in CHANGED_SETUP|CHANGED_LAYOUT:assert equal,(key,name)
   topology=table_pair(of,f,oi,ins,old,im)
   if key not in CHANGED_SETUP|CHANGED_LAYOUT:assert topology is not None and all(r['topology_equal'] for r in topology)
   refs=references(oi,ins,old,im)
   # Virtual tables contain relocated executable pointers and are checked by
   # their actual slots below, not by raw pointer-byte equality.
   if key not in CHANGED_SETUP|{'storage_constructor'}:
    assert all(r['old']['bytes_hex']==r['later']['bytes_hex'] for r in refs if r['old']['section']=='.rdata'),(key,name)
   mapping=dict(zip(entries['retail'].values(),entries[name].values()));constraints=[]
   for x,y in (zip(oi,ins) if equal else []):
    t=call_target(x)
    if t in mapping:
     assert call_target(y)==mapping[t],(key,name,x.op_str,y.op_str)
     constraints.append({'old_site':x.address,'later_site':y.address,'old_target':t,'later_target':call_target(y)})
   pairs.append({'owner':key,'image':name,'complete_tokens_equal':equal,'switch_topology':topology,
    'old_instruction_count':len(oi),'later_instruction_count':len(ins),
    'token_differences':[{'index':j,'old':x,'later':y} for j,(x,y) in enumerate(zip(ot,nt)) if x!=y],
    'relocated_operands':refs,'direct_call_constraints':constraints})
  # Actual constructor immediates establish each virtual table. The preceding
  # five-entry table at retail450D4C is NOT the entity replication interface.
  ci=owners[name,'storage_constructor'][1]
  for setup,callee in [('world_replication_setup','peer_table_initialize'),('view_connection_setup','connection_callback_setup')]:
   sites=[i.address for i in owners[name,setup][1] if call_target(i)==entries[name][callee]]
   assert len(sites)==1,(name,setup,callee,sites)
   bindings.append({'image':name,'owner':setup,'checked_direct_service':callee,'target':entries[name][callee],'sites':sites})
  specs=[('aggregate',4,8,{'aggregate_decode_apply':5}),
         ('handles',0x30,9,{'handle_blocks_decode':5,'handle_block_apply':6}),
         ('events',0x5070,9,{}),('inputs',0x5098,9,{})]
  for kind,offset,count,checks in specs:
   operand=f'dword ptr [eax + {str(offset) if offset<10 else hex(offset)}], '
   found=[i for i in ci if i.mnemonic=='mov' and i.op_str.startswith(operand)]
   assert len(found)==1,(kind,name,operand)
   address=imm(found[0]);raw=im.read(address,4*count);ptrs=list(struct.unpack('<'+'I'*count,raw))
   for key,slot in checks.items():assert ptrs[slot]==entries[name][key],(name,key,slot)
   tables.append({'image':name,'kind':kind,'constructor_site':found[0].address,'object_offset':offset,'address':address,
    'bytes_hex':raw.hex(),'pointers':ptrs,'checked_slots':checks})
  ai=owners[name,'distribution_allocation'][1]
  found=[i for i in ai if i.mnemonic=='mov' and ', 0x' in i.op_str and re.search(r'\[\w+ \+ 0x2098\]',i.op_str)]
  assert len(found)==1,(name,'entity table constructor',[(i.address,i.op_str) for i in ai if '0x2098' in i.op_str])
  address=imm(found[0]);raw=im.read(address,19*4);ptrs=list(struct.unpack('<19I',raw))
  checks={'entry_creation_decode':1,'entry_creation_apply':2,'entry_update_decode':6,'entry_update_apply':7,
          'entry_delete':12}
  for key,slot in checks.items():
   if slot is not None:assert ptrs[slot]==entries[name][key],(name,key,slot)
  tables.append({'image':name,'kind':'entity_replication_owner','constructor_site':found[0].address,
   'object_offset':0x2098,'address':address,'bytes_hex':raw.hex(),'pointers':ptrs,'checked_slots':checks})
  # Entity-definition slots are a different table from the outer entry owner.
  pinned=ROOT/'analysis/multiplayer/game-variants/king-replication-source-20261012/complete/report.json'
  assert sha(pinned)=='44c4599ff00ed27dc3124555d98ca3e6c9f4a10565c9dfaf03fa9eb2fa1d6e2d'
  er=next(r for r in json.loads(pinned.read_text())['entity_definition_tables'] if r['image']==name)
  address=er['vtable'];raw=im.read(address,27*4);ptrs=list(struct.unpack('<27I',raw))
  assert ptrs==er['pointers27']
  checks={'entity_initialize_dispatch':19,'entity_difference_dispatch':20,'entity_apply_dispatch':23,
          'entity_creation_dispatch':22,'entity_adopt_identity':25,'entity_release_identity':24}
  for key,slot in checks.items():assert ptrs[slot]==entries[name][key],(name,key,slot)
  tables.append({'image':name,'kind':'king_entity_definition','address':address,'bytes_hex':raw.hex(),
    'pointers':ptrs,'checked_slots':checks,'reader_slot15':ptrs[15]})
  # Map every virtual dispatch site at the three key layers to literal slots.
  for owner,bank in [('aggregate_decode_apply','aggregate_children'),('handle_blocks_decode','entity_replication_owner'),
                     ('handle_block_apply','entity_replication_owner'),('entry_creation_decode','entity_definition'),
                     ('entry_update_decode','entity_definition'),('entry_creation_apply','entity_definition'),
                     ('entry_update_apply','entity_definition')]:
   for i in owners[name,owner][1]:
    if i.mnemonic=='call' and 'dword ptr [' in i.op_str:
     bindings.append({'image':name,'owner':owner,'site':i.address,'operand':i.op_str,'table_family':bank,
       'note':'Raw dispatch operand; allocator virtual calls also occur. Caller dataflow, not this family label alone, identifies the service.'})
 for key in OWNERS:
  of,oi=owners['tu11',key];nf,ni=owners['u5',key]
  equal=complete_tokens(oi,images['tu11'])==complete_tokens(ni,images['u5'])
  topology=table_pair(of,nf,oi,ni,images['tu11'],images['u5'])
  refs=references(oi,ni,images['tu11'],images['u5'])
  assert equal and topology is not None and all(r['topology_equal'] for r in topology),(key,'TU1.1 versus TU1.5')
  mapping=dict(zip(entries['tu11'].values(),entries['u5'].values()));constraints=[]
  for x,y in zip(oi,ni):
   t=call_target(x)
   if t in mapping:
    assert call_target(y)==mapping[t],(key,'TU named callee',x.op_str,y.op_str)
    constraints.append({'old_site':x.address,'later_site':y.address,'old_target':t,'later_target':call_target(y)})
  later_pairs.append({'owner':key,'old_image':'tu11','later_image':'u5','complete_tokens_equal':equal,
    'instructions':len(oi),'switch_topology':topology,'relocated_operands':refs,'direct_call_constraints':constraints})
 deps=[Path(__file__),ROOT/'tools/oracle_game_over.py',ROOT/'tools/oracle_mp_full_scoreboard_1_5.py',
       ROOT/'tools/research_mp_lobby_later_update.py',ROOT/'tools/inspect_render_primitives_title_updates.py',
       Path(FUNCTIONS_CSV),ROOT/'tools/reference/halo2-decompiled/tools/xbe.py',ROOT/'tools/reference/halo2-decompiled/tools/inventory.py',pinned]
 report={'schema':'king-replication-orchestration-static-source-v1','scope':__doc__,
  'images':{n:{'path':s['path'],'sha256':s['pin'],'function_index':s['tsv'],
               'function_index_sha256':sha(ROOT/s['tsv'])} for n,s in SPECS.items()},
  'entries':entries,'prefix_locators':locators,'listings':listings,'complete_comparisons':pairs,
  'complete_title_update_pairs':later_pairs,
  'actual_virtual_tables':tables,'raw_indirect_dispatch_sites':bindings,
  'entry_layouts':{
   'retail':{'row_bytes':0x20,'entry_array':0x14,'identifier':0,'type_i16':4,'created_u8':6,
    'object_identifier':8,'dirty_requested_mask':0xc,'data_size':0x10,'data_pointer':0x14,'state_size':0x18,'state_pointer':0x1c,
    'distribution_bytes':0xa0c4,'events_offset':0xa0ac},
   'tu11_and_u5':{'row_bytes':0x24,'entry_array':0x14,'identifier':0,'type_i16':4,'created_u8':6,
    'object_identifier':8,'dirty_requested_mask':0xc,'secondary_mask_producer_unresolved':0x10,
    'data_size':0x14,'data_pointer':0x18,'state_size':0x1c,'state_pointer':0x20,
    'distribution_bytes':0xb0c4,'events_offset':0xb0ac}},
  'instruction_patches':[],'supplied_returns':[],'packet_executions':0,'callback_executions':0,
  'dependencies':{str(p.relative_to(ROOT)):sha(p) for p in deps}}
 p=args.output/'report.json';p.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
 print(json.dumps({'owners':len(OWNERS),'listings':len(listings),'retail_pairs':len(pairs),
  'retail_equal_pairs':sum(r['complete_tokens_equal'] for r in pairs),'title_update_pairs':len(later_pairs),
  'virtual_tables':len(tables),'sha256':sha(p),'entries':entries}))

if __name__=='__main__':main()
