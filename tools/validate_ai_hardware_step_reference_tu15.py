"""Admit observed original hardware traces without hiding Unicorn differences."""
import json
import shutil
from pathlib import Path
from audit_rendering import file_sha
from research_mp_sandbox_update_functions import NEW_PIN
from oracle_charged_effect_lifetime_bank_tu15 import stores

def publish(reference,step):
    original=json.loads((reference/'report.json').read_text())
    hardware=json.loads((reference/'hardware/report.json').read_text())
    report=json.loads((step/'report.json').read_text())
    assert report['passed'] and report['hardware_trace_equals_unicorn'] and report['hardware_store_layout_equals_unicorn']
    assert report['estimate_boundaries'] or report.get('fp_comparison_flags')
    if report.get('fp_comparison_flags'):
        flags=report['fp_comparison_flags']
        assert original['entry']==0x26f260 and flags['instruction']==0x26f2c3
        assert flags['difference_except_IF']==0x80 and flags['all_other_CPU_FP_bytes_equal']
    assert not report['source_instruction_patches'] and not report['supplied_game_services']
    assert report['original_report_sha256']==file_sha(reference/'report.json')
    target=reference/'hardware-step-reference.json';assert not target.exists()
    published={}
    for name in ('hardware-trace.bin','hardware-stores.bin'):
        destination=reference/'hardware'/name;assert not destination.exists()
        assert file_sha(step/name)==report['files_sha256'][name]
        shutil.copy2(step/name,destination);published[name]=file_sha(destination)
    target.write_text(json.dumps(dict(hardware_step_directory=str(step),hardware_step_report_sha256=file_sha(step/'report.json'),
        published_files_sha256=published,unicorn_disagreement_preserved=True,xbox_cpu_estimate_bits_verified=False),indent=2,sort_keys=True)+'\n')
    return validate(reference,original,hardware)

def validate(reference,original,hardware):
    if hardware['emulator_retained_state_matches_hardware']:
        return None
    manifest=reference/'hardware-step-reference.json';record=json.loads(manifest.read_text())
    step=Path(record['hardware_step_directory']);report=json.loads((step/'report.json').read_text())
    assert file_sha(step/'report.json')==record['hardware_step_report_sha256']
    assert report['passed'] and report['original_xbe_sha256']==NEW_PIN
    assert report['original_report_sha256']==file_sha(reference/'report.json')
    assert report['ordinary_hardware_report_sha256']==file_sha(reference/'hardware/report.json')
    assert report['ordinary_hardware_full_cpu_exit_equal'] and report['ordinary_hardware_retained_bytes_equal']==hardware['retained_bytes_compared']
    assert report['hardware_trace_equals_unicorn'] and report['hardware_store_layout_equals_unicorn']
    assert not report['source_instruction_patches'] and not report['supplied_game_services']
    if report.get('fp_comparison_flags'):
        flags=report['fp_comparison_flags']
        assert original['entry']==0x26f260 and flags['instruction']==0x26f2c3
        assert flags['difference_except_IF']==0x80 and flags['all_other_CPU_FP_bytes_equal']
    for name in ('hardware-trace.bin','hardware-stores.bin','hardware-exit.bin','hardware-after.bin'):
        assert file_sha(step/name)==report['files_sha256'][name]
    assert (step/'hardware-trace.bin').read_bytes()==(reference/'original-trace.bin').read_bytes()
    assert (step/'hardware-exit.bin').read_bytes()==(reference/'hardware/hardware-exit.bin').read_bytes()
    for name in ('hardware-trace.bin','hardware-stores.bin'):
        assert file_sha(reference/'hardware'/name)==record['published_files_sha256'][name]==report['files_sha256'][name]
    layout=lambda p:[(pc,address,len(after)) for pc,address,before,after in stores(p)]
    assert layout(step/'hardware-stores.bin')==layout(reference/'original-stores.bin')
    return dict(manifest_sha256=file_sha(manifest),hardware_step_directory=str(step),
        hardware_step_report_sha256=file_sha(step/'report.json'),
        unicorn_retained_state_matches_hardware=False,observed_original_hardware_reference=True,
        cpu_estimate_reference='Actual host CPU SSE instructions; Xbox estimate lookup remains unverified' if report['estimate_boundaries'] else None,
        cpu_comparison_flag_reference=report.get('fp_comparison_flags'),
        source_instruction_patches=[],supplied_game_services=[])
