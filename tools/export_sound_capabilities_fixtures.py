#!/usr/bin/env python3
"""Export literal source buffers/stores and real allocation/lock requests."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,sha,save,encoded,Xbe,OLD
BINDINGS=('free_2d','free_3d','free_sge','pool_bytes','contiguous_bytes','fault','critical_section','statistics_pointer','private_pool_pointer','device_context_pointer')
CONSTANTS=('reference_vtable','apu_vtable','apu_interface_vtable','sge_vtable','base_vtable','reference_add_method')
OPS={'00381bee':'caps','003802bf':'allocate','00384c7c':'apu','003868dc':'sge'}
def export(report,providers,out):
 r=json.loads(report.read_text());p=json.loads(providers.read_text());image=Xbe(OLD)
 lines=['SOUND_CAPABILITIES_V1','\t'.join(('BINDINGS',*(p['bindings'][k]['old']for k in BINDINGS))),'\t'.join(('CONSTANTS',*(p['constructor_constants'][k]['old']for k in CONSTANTS)))]
 context_path=ROOT/'analysis/multiplayer/map-pack/smg-audio-pool-prefix/frozen-measurement/raw-host-context.json'
 assert sha(context_path)==r['input_pins']['raw-host-context.json']
 context=json.loads(context_path.read_text());irql=bytes.fromhex(context['kpcr_hex'])[0x24];lines.append(f'IRQL\t{irql:02x}')
 vtable=int(p['constructor_constants']['reference_vtable']['old'],16);lines.append('\t'.join(('IMMUTABLE',f'{vtable:08x}',image.read(vtable,16).hex())))
 source=platform=total=0
 for c in r['cases']:
  lines.append('\t'.join(('CASE',c['name'],OPS[c['entry']],c['ecx']or'00000000',c['return_eax'],*c['arguments'])))
  for q in c['initial_regions']:lines.append('\t'.join(('BEFORE',q['address'],q['raw_hex'])))
  for q in c['actual_raw_pool_services']:
   if q['kernel_ordinal']==15:lines.append('\t'.join(('ALLOC',q['resulting_guest_alias'],str(q['requested_bytes']),str(q['raw_capacity_bytes']),q['allocation_tag_recorded_not_interpreted'])))
  for q in c['stores']:
   if int(q['instruction'],16)>=0x400000:platform+=1;continue
   lines.append('\t'.join(('STORE',q['instruction'],q['address'],str(q['bytes']),q['before_hex'],q['after_hex'])));source+=1
  for q in c['actual_critical_section_services']:lines.append('\t'.join(('LOCK',str(q['ordinal']),q['pointer'])))
  for q in c['output_regions']:lines.append('\t'.join(('AFTER',q['address'],q['raw_hex'])));total+=len(bytes.fromhex(q['raw_hex']))
  lines.append('ENDCASE')
 save(out/'fixtures.tsv',('\n'.join(lines)+'\n').encode())
 save(out/'fixtures.json',encoded(dict(schema='source-sound-capabilities-fixtures-v1',exporter_sha256=sha(__file__),original_report=dict(path=str(report),sha256=sha(report)),providers=dict(path=str(providers),sha256=sha(providers)),fixture_sha256=sha(out/'fixtures.tsv'),cases=len(r['cases']),source_stores=source,compatibility_platform_stores_separately_retained_not_native_representation=platform,whole_output_bytes=total,source_initialized_78=r['source_initialized_78'],expected_from_native=False)))
 print(json.dumps(dict(cases=len(r['cases']),source_stores=source,platform_stores=platform,bytes=total,fixture_sha256=sha(out/'fixtures.tsv'))))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--providers',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();export(a.report.resolve(),a.providers.resolve(),a.output.resolve())
