#!/usr/bin/env python3
"""Freeze the separate audited grenade admission/resource dependency result."""
import argparse,json
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from oracle_mp_grenade_action_1_1 import motion
from audit_mp_grenade_action_1_1 import BASE

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert not a.output.exists()
    audit_path=BASE/'audit-frozen/audit.json';audit=json.loads(audit_path.read_text())
    files=dict(audit['source_files'])
    files[str(audit_path.relative_to(ROOT))]=motion.digest(audit_path)
    files['tools/freeze_mp_grenade_action_1_1.py']=motion.digest(__file__)
    for collection in [files,audit['verified_prior_pins']]:
        for name,digest in collection.items():assert motion.digest(ROOT/name)==digest,name
    report=dict(schema=1,source_files=files,verified_prior_pins=audit['verified_prior_pins'],previous_geometry_manifest_sha256=audit['previous_geometry_manifest_sha256'],
                independently_identical_pairs=[['exploratory','repeat'],['texture-context','texture-repeat']],
                actual_admission=audit['actual_admission'],texture_reset_stop=audit['texture_reset_stop'],
                original_xbe_sha256=audit['original_xbe_sha256'],source_instruction_rows=audit['source_instruction_rows'],
                scope=audit['scope'],native_gameplay_update_applied=False)
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(manifest_sha256=motion.digest(a.output),source_files=len(files),prior_pins=len(audit['verified_prior_pins']),doc_sha256=files['docs/MP_GRENADE_ACTION_SOURCE_RESEARCH.md'])))

if __name__=='__main__':main()
