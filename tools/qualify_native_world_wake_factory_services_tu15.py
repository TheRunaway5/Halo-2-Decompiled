#!/usr/bin/env python3
"""Seal same-factory actual1473 proof, complete dispatch-table audit and seam."""
import hashlib,json,lzma,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-world-wake-factory-services-stage-v4-20261102'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save(p,x):p.write_text(json.dumps(x,indent=2,sort_keys=True)+'\n')
def main():
 names=['native-wake-list-factory-actual1473-tu15-20261102-v8','native-collection-type-switch-tu15-20261102-first','native-activation-factory-services-interface-tu15-20261102-v3','native-world-wake-factory-services-model-gcc-20261102-v2','native-world-wake-factory-services-model-clang-20261102-first','native-world-wake-factory-counters-20261102-first']
 reports={name:ROOT/'analysis/physics'/name/'report.json'for name in names};data={name:json.loads(path.read_text())for name,path in reports.items()}
 assert all(r['passed']for r in data.values());pins_path=STAGE/'source-pins.json';pins=json.loads(pins_path.read_text())
 for name,digest in pins['staged_hashes'].items():assert sha(STAGE/'staged'/name)==digest,name
 world=(STAGE/'staged/src/physics/native_tu15_world_integration_residuals.cpp').read_text();start=world.index('    create.agents.classify_shape=');end=world.index('    create.agents.motion_kind=',start)
 actual=data[names[0]];assert hashlib.sha256(world[start:end].encode()).hexdigest()==actual['world_factory_lambda_sha256']
 assert actual['actual_before_bytes']==114429109 and actual['captured_bytes_excluded']==0 and not actual['expected_after_consumed']and not actual['original_gp_supplied']
 for name in names[3:5]:assert data[name]['stage_hashes']==pins['staged_hashes']
 archive=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-agent-wake-conveyor-original-v1473-20261102/iteration-01-native-pair-wake-factory-activation-call-actual-before-vehicle-camera/required-native-contact-agent-factory-before/actual-native-before.tar.zst'
 assert sha(archive)=='ca0cbb613bfb6a751fef2bf9b94ec48da056de8aaae3fec6684664ace78a889f'and sha(NEW_DEFAULT)==NEW_PIN
 manifest=json.loads(archive.with_name('capture-native-before.json').read_text());table_rows=[];image=Xbe(NEW_DEFAULT)
 members=subprocess.check_output(['tar','--zstd','-tf',str(archive)],text=True).splitlines()
 for address,size in((0x185044,8),(0x18504c,24)):
  row=next(r for r in manifest['regions']if r['address']==address and r['size']==size);member=next(m for m in members if Path(m).name==row['before_file'])
  raw=subprocess.check_output(['tar','--zstd','-xOf',str(archive),member]);assert raw==image.read(address,size)
  table_rows.append(dict(address=address,bytes=size,hex=raw.hex(),sha256=hashlib.sha256(raw).hexdigest(),captured_before_member=member,matches_immutable_tu15_XBE=True))
 save(STAGE/'dispatch-classification-qualification.json',dict(verified=True,captured_tables=table_rows,
  opcode_and_complete_uint32_AL_proof=str(reports[names[1]].relative_to(ROOT)),proof_sha256=sha(reports[names[1]]),
  excluded_instruction_dispatch_fetch_operands=[0x18502e,0x185035],physics_or_callback_DATA_reads_excluded=False,original_CPU_executes_unchanged=True,
  scope='Exactly185020 code dispatch fetches, not an input waiver. All captured table bytes match the pinned immutable source; native privateALswitch exacttrue/default set covers entireuint32,261compiled values percompiler. Source raw read-ledger failureV7 retained.'))
 corrected=dict(actual);corrected['underlying_template_scope']=corrected['scope'];corrected['scope']='Actual1473 Wake2F8030 using exact stagedWorldfactorylambda and typed native factory supplier; native303C10 child closure, four originalFProundings. All114429109 captured bytes, return/FP/stores and every semanticDATA read; only two authenticated185020 instruction-dispatch fetch operands excluded. No GP/afterimages or child results supplied; source private CPU-control ledger unclaimed.';corrected['raw_report_sha256']=sha(reports[names[0]]);save(STAGE/'actual-factory-qualified-report.json',corrected)
 pins.update(qualified=True,qualification_reports={str(path.relative_to(ROOT)):sha(path)for path in reports.values()},
  actual_input_archive_sha256=sha(archive),actual_factory_lambda_sha256=actual['world_factory_lambda_sha256'],
  actual_factory_profiles_per_compiler=4,actual_before_bytes=114429109,captured_bytes_excluded=0,
  dispatch_qualification='dispatch-classification-qualification.json',interface_old_and_new_profiles_per_compiler=32,
  coherent_world_cpp_count=data[names[3]]['compiled_cpp_count'],coherent_world_models=['default10','factory-prefix','genuine09-merge-prefix','native-pair-wake-queue-prefix'],
  native_factory_option_positive_scope='Actual native callback supplier/direct exact World factory fully qualified; shared option ABI regression proves old/newnil same original caller. Live whole-frame comparison remains required.',
  production_applied=False,live_qualified=False,remaining=['Current1473 guarded narrow merge with independently qualifiedWrappedFactory/PairDiscrete','Visible multiplayer actualnativeWake8030+303C10 calls/fulloriginalframe','New unsupportedfactory/shape/depth/lifetime owners remain countedrefusals, neverCPUfallback'])
 save(pins_path,pins);print(dict(qualified=True,stage=str(STAGE),cpp_count=pins['coherent_world_cpp_count'],source_pins_sha256=sha(pins_path)))
if __name__=='__main__':main()
