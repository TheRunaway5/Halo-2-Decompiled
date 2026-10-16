#!/usr/bin/env python3
"""Recover matching current-material environment values from primary2003 caches.

Only exact resident default environment response programs, implementation0 and static
normal/glancing/midrange values are admitted. Disagreeing caches, overlays and
other pass families stay unresolved. This never edits any original asset.
"""
from pathlib import Path
import hashlib,json,struct,sys
ROOT=Path(__file__).resolve().parents[2]
HERE=ROOT/'analysis/pstencil/2003-presentation'
sys.path.insert(0,str(HERE))
import extract_resident_environment as resident

def field(row,name):return next(f for f in row['fields'] if f['name']==name)
def value(row,name):return field(row,name)['value']
def run():
 candidates=json.loads((ROOT/'analysis/pstencil/environment-response-test/material-candidates.json').read_text())
 names={r['name'] for r in candidates['materials']} | {r['path'] for r in json.loads((ROOT/'analysis/pstencil/2003-cache-inventory/current-path-matches.json').read_text())['shader_matches']}
 inventory=ROOT/'analysis/pstencil/2003-cache-inventory'
 results={};unresolved=[]
 for m in json.loads((inventory/'summary.json').read_text())['maps']:
  metadata=json.loads((inventory/m['report']).read_text());selected=[t for t in metadata['tags'] if t['group']=='shad' and t['name'] in names and t['payload_file_offset']]
  if not selected:continue
  path=Path(metadata['path']);data=path.read_bytes();assert hashlib.sha256(data).hexdigest()==metadata['sha256']
  resident.DATA=data;resident.INDEX=int(metadata['tag_data_offset'],16);resident.MAGIC=metadata['pointer_bias']
  resident.TAGS={int(t['datum'],16):{**t,'payload_file_offset':int(t['payload_file_offset'],16) if t['payload_file_offset'] else None} for t in metadata['tags']}
  build=data[0x38:0x58].split(b'\0')[0].decode('ascii')
  for t in selected:
   try:
    lighting=resident.block(int(t['payload_file_offset'],16)+0x30,0x8112d0)
    for row in lighting:
     lighting_model=value(row,'lighting model')
     layers=field(row,'runtime layers*')['rows']
     if len(layers)<=12:continue
     for p in field(layers[12],'passes*')['rows']:
      ref=field(p,'pass*').get('reference')
      if not ref or ref['name'].split('\\')[-1] not in ('default_environment_map','default_environment_map_tangent_space'):continue
      pass_root=ref['payload_file_offset'];pass_defs=resident.block(pass_root+0x30,0x811760)
      assert len(pass_defs)==1,'multiple source program implementations'
      program_offset=int(pass_defs[0]['file_offset'],16)+0x14
      program_sha=hashlib.sha256(data[program_offset:program_offset+240]).hexdigest()
      assert program_sha=='40c792b9a3b656e2c1e83bc5ca27fe3e319c7404dc95b5d1dd4cdd5732cd4cac','different source environment program'
      implementations=field(p,'implementations*')['rows'];assert len(implementations)==1,'multiple implementations'
      impl=implementations[0];parameters={};sources=[]
      for c in field(impl,'VS constants*')['rows']:
       bank,index=value(c,'register bank*'),value(c,'register index*')
       if bank!=0 or index not in (10,11):continue
       assert not field(c,'overlays*')['rows'],'animated VS values'
       parameters['facing' if index==10 else 'glancing']=[value(c,k+'*') for k in ('x','y','z','w')]
       sources.append(c['file_offset'])
      for c in field(impl,'PS constants*')['rows']:
       stage,index=value(c,'register bank*'),value(c,'register index*')
       if index!=0 or stage not in (3,4):continue
       assert not field(c,'overlays*')['rows'],'animated PS values'
       assert value(c,'pass constant index*')==255,'indirect PS value'
       parameters['midrange_factor_argb' if stage==3 else 'midrange_color_argb']=value(c,'data*')
       sources.append(c['file_offset'])
      assert set(parameters)=={'facing','glancing','midrange_factor_argb','midrange_color_argb'},'missing explicit parameter role'
      results.setdefault(t['name'],[]).append({'map':m['name'],'cache_sha256':metadata['sha256'],'cache_build':build,'shader_file_offset':t['payload_file_offset'],'lighting_model':lighting_model,'pass':ref['name'],'program_sha256':program_sha,'implementation_file_offset':impl['file_offset'],'constant_file_offsets':sources,'parameters':parameters})
   except (AssertionError,KeyError,StopIteration,ValueError) as error:
    unresolved.append({'map':m['name'],'shader':t['name'],'error':str(error)})
 consensus=[];disagreements=[]
 for name,records in sorted(results.items()):
  distinct={json.dumps(r['parameters'],sort_keys=True) for r in records}
  entry={'shader':name,'parameters':records[0]['parameters'],'sources':records}
  (consensus if len(distinct)==1 else disagreements).append(entry)
 output={'scope':__doc__,'reference_schema_tool_sha256':hashlib.sha256(resident.defs.TOOL.read_bytes()).hexdigest(),'static_consensus':consensus,'disagreements':disagreements,'unresolved':unresolved}
 dest=ROOT/'analysis/pstencil/reference-environment-parameters';dest.mkdir(exist_ok=True)
 (dest/'evidence.json').write_text(json.dumps(output,indent=2)+'\n')
 if '--generate' in sys.argv:
  rows=[]
  def vector(values):
   return '{'+','.join('std::bit_cast<float>(0x%08xu)'%struct.unpack('<I',struct.pack('<f',v))[0] for v in values)+'}'
  for row in consensus:
   d=row['parameters']
   rows.append('    {'+json.dumps(row['shader'])+',{'+vector(d['facing'])+','+vector(d['glancing'])+',0x%08xu,0x%08xu'%(d['midrange_factor_argb'],d['midrange_color_argb'])+'}},')
  (ROOT/'src/pstencil/reference_environment.inc').write_text('// Generated from unchanged primary2003 resident values; exact path matches only.\n// Evidence: analysis/pstencil/reference-environment-parameters/evidence.json\n'+'\n'.join(rows)+'\n')
 print(json.dumps({'consensus':len(consensus),'disagreeing_paths':len(disagreements),'unresolved_records':len(unresolved)}))
 if '--verbose' in sys.argv:
  for row in consensus:print(row['shader'],row['parameters'],'records',len(row['sources']))
if __name__=='__main__':run()
