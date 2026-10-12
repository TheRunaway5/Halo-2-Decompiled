#!/usr/bin/env python3
"""Freeze the independently audited original grenade begin prefix."""
import argparse,json
from pathlib import Path
from oracle_mp_grenade_bounded_phases_1_1 import ROOT,OUT,motion

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert not a.output.exists()
    path=OUT/'audit-frozen/audit.json';audit=json.loads(path.read_text());files=dict(audit['source_files'])
    files[str(path.relative_to(ROOT))]=motion.digest(path);files['tools/freeze_mp_grenade_bounded_phases_1_1.py']=motion.digest(__file__)
    for collection in (files,audit['verified_prior_pins']):
        for name,digest in collection.items():assert motion.digest(ROOT/name)==digest,name
    value=dict(schema=1,source_files=files,verified_prior_pins=audit['verified_prior_pins'],
        previous_grenade_manifest_sha256=audit['previous_grenade_manifest_sha256'],independently_identical_pair=['measured-complete','repeat-complete'],
        original_xbe_sha256=audit['original_xbe_sha256'],source_instruction_rows=audit['source_instruction_rows'],
        source_state_write=audit['source_state_write'],selected_channel=audit['selected_channel'],genuine_stop=audit['genuine_stop'],
        scope=audit['scope'],native_gameplay_update_applied=False)
    motion.frozen_write(a.output,motion.encoded(value));print(json.dumps(dict(manifest_sha256=motion.digest(a.output),source_files=len(files),prior_pins=len(audit['verified_prior_pins']))))

if __name__=='__main__':main()
