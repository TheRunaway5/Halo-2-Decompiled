#!/usr/bin/env python3
"""Read-only original evidence checks and fresh contract audit output."""
import argparse,hashlib,json,platform,shutil,subprocess,sys
from pathlib import Path
import unicorn,capstone
from oracle_mp_title_update_display import ROOT,OLD,NEW,PINS,Xbe

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p,x):
 raw=(json.dumps(x,sort_keys=True,indent=2)+'\n').encode();p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==raw,p
 else:p.write_bytes(raw)
def compare(a,b):
 first={p.relative_to(a):p for p in a.rglob('*')if p.is_file()};second={p.relative_to(b):p for p in b.rglob('*')if p.is_file()}
 assert first.keys()==second.keys(),(a,b)
 for rel,p in first.items():assert p.read_bytes()==second[rel].read_bytes(),rel
 return len(first)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();out=a.directory.resolve()
 prior=ROOT/'analysis/multiplayer/integration/sound-object-request-native/final-manifest.json'
 assert sha(prior)=='ac6fafa092a2efc81d927f92a93b815adbeb1de8b4f8b295eaa085efde29b433'
 pins=json.loads(prior.read_text())['files']
 for rel,h in pins.items():assert sha(ROOT/rel)==h,rel
 paired=compare(out/'paired',out/'paired-repeat');scalar=compare(out/'scalar-measured',out/'scalar-repeat')
 report=json.loads((out/'paired/report.json').read_text());images={'old':Xbe(OLD),'u5':Xbe(NEW)}
 for path in(OLD,NEW):assert sha(path)==PINS[str(path)]
 count=0
 for row in report['owners']:
  for key,x in images.items():
   listing=out/'paired/instructions'/f'{row["old"]}-{key}.asm'
   lines=listing.read_text().splitlines();assert len(lines)==row['rows']
   for line in lines:
    address,raw,*_=line.split();data=bytes.fromhex(raw);assert x.read(int(address,16),len(data))==data
   count+=len(lines)
 providers=json.loads((out/'paired/providers.json').read_text())
 for q in providers['constants']:
  assert images['old'].read(int(q['old_va'],16),4).hex()==q['raw4_hex']
  assert images['u5'].read(int(q['u5_va'],16),4).hex()==q['raw4_hex']
 scalars=json.loads((out/'scalar-measured/report.json').read_text());rows=scalars['images']
 for key in('normalize','pitch_slices','gain_slices','relative_rates'):assert rows[0][key]==rows[1][key]
 command=[sys.executable,str(ROOT/'tools/export_sound_platform_parameters.py'),'--report',str(out/'scalar-measured/report.json'),'--output',str(out/'scalar-repeat-fixtures.tsv')]
 result=subprocess.run(command,cwd=ROOT,text=True,capture_output=True);assert result.returncode==0,(result.stdout,result.stderr)
 assert(out/'scalar-fixtures.tsv').read_bytes()==(out/'scalar-repeat-fixtures.tsv').read_bytes()
 snapshot=out/'source-snapshot';assert not snapshot.exists()
 own=['tools/research_sound_platform_voice_contract.py','tools/oracle_sound_platform_parameters.py','tools/export_sound_platform_parameters.py','tools/audit_sound_platform_voice_contract.py','docs/MP_SOUND_PLATFORM_VOICE_CONTRACT.md','include/halo2/audio.hpp','src/audio.cpp']
 dependencies={str(Path(m.__file__).resolve().relative_to(ROOT))for m in tuple(sys.modules.values())if getattr(m,'__file__',None)and Path(m.__file__).resolve().is_relative_to(ROOT)and '.tools'not in Path(m.__file__).resolve().parts}
 dependencies.update(own);observed={rel:sha(ROOT/rel)for rel in dependencies}
 for rel,h in observed.items():
  dest=snapshot/rel;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/rel,dest);assert sha(dest)==h
 for rel,h in observed.items():assert sha(ROOT/rel)==h,rel
 # Preserve the primary compatibility ABI declaration with provenance; it is
 # only corroborating field naming, never retail capabilities or DSP results.
 abi=ROOT/'analysis/multiplayer/map-pack/smg-audio-startup/reference/cxbx-585c49a/XbDSoundTypes.h'
 shutil.copy2(abi,out/'XbDSoundTypes.h')
 for rel,h in pins.items():assert sha(ROOT/rel)==h,rel
 audit=dict(schema='source-sound-platform-voice-contract-audit-v1',prior_manifest_sha256=sha(prior),prior_pins_verified=len(pins),
  independent_paired_files=paired,independent_scalar_files=scalar,complete_paired_bodies=len(report['owners']),verified_instruction_rows_both_images=count,
  immutable_words=len(providers['constants']),scalar_counts_per_image={k:len(rows[0][k])for k in('normalize','pitch_slices','gain_slices','relative_rates')},scalar_rows_per_image=sum(len(rows[0][k])for k in('normalize','pitch_slices','gain_slices','relative_rates')),
  fixture_reexport=dict(argv=command,cwd=str(ROOT),returncode=result.returncode,stdout=result.stdout,stderr=result.stderr),source_snapshots=observed,
  observed_compatibility_abi=dict(path=str(abi.relative_to(ROOT)),sha256=sha(abi),commit='585c49a',scope='Field-name/ABI corroboration only; retail capability/DSP semantics not imported'),
  environment=dict(python=sys.version,python_executable=sys.executable,platform=platform.platform(),unicorn=unicorn.__version__,capstone=capstone.__version__),
  limitations=scalars['boundaries'],native_scheduler_or_device_execution=False,production_edits=[],
  preliminary_observations={'preliminary-preserved':'Preliminary packet extent98h was not proved and is excluded from canonical field contract. Retained unchanged as investigation history.','scalar-v2-preserved':'Gain XMM0 after scaling was initially labeled normalized_bits; canonical observer now records actual normalization return separately. Retained unchanged.'})
 save(out/'audit.json',audit);print(json.dumps(dict(audit_sha256=sha(out/'audit.json'),paired_files=paired,scalar_files=scalar,complete_bodies=len(report['owners']),instructions_both=count,scalar_rows_per_image=audit['scalar_rows_per_image'],prior_pins=len(pins))))
if __name__=='__main__':main()
