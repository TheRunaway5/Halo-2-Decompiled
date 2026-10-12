#!/usr/bin/env python3
"""Export expected scalar results only from original instruction execution."""
import argparse,hashlib,json
from pathlib import Path
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def write(p,b):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==b,p
 else:p.write_bytes(b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 report=json.loads(a.report.read_text());one,two=report['images']
 for key in('normalize','pitch_slices','gain_slices','relative_rates'):assert one[key]==two[key],key
 rows=['# source-sound-platform-scalar-fixtures-v1']
 for r in one['normalize']:rows.append('\t'.join(map(str,('normalize',r['source_fpcw'],r['source_mxcsr'],r['lower_bits'],r['upper_bits'],r['value_bits'],r['cl'],r['result_bits']))))
 for r in one['pitch_slices']:rows.append('\t'.join(map(str,('pitch',r['source_fpcw'],r['source_mxcsr'],r['rate_index'],r['source_pitch_bits'],r['clamped_float_bits'],r['driver_pitch_bits']))))
 for r in one['gain_slices']:rows.append('\t'.join(map(str,('gain',r['source_fpcw'],r['source_mxcsr'],r['value_bits'],r['normalized_bits'],r['scaled_driver_volume_float_bits'],r['driver_volume_bits']))))
 for r in one['relative_rates']:rows.append('\t'.join(map(str,('rate',r['source_fpcw'],r['source_mxcsr'],r['rate_index'],r['driver_pitch'],r['result_bits']))))
 write(a.output,('\n'.join(rows)+'\n').encode())
 meta=dict(schema='source-sound-platform-scalar-fixture-export-v1',tool_sha256=sha(__file__),source_report_sha256=sha(a.report),fixture_sha256=sha(a.output),rows=len(rows)-1,
  counts={k:len(one[k])for k in('normalize','pitch_slices','gain_slices','relative_rates')},source_images=[r['source_sha256']for r in report['images']],
  boundaries=report['boundaries'],instruction_patches=[],native_scheduler_or_playback_expected_values=False)
 write(a.output.with_suffix('.json'),(json.dumps(meta,sort_keys=True,indent=2)+'\n').encode());print(json.dumps(meta))
if __name__=='__main__':main()
