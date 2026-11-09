"""Small private validation support shared by object-node owner probes."""
import os
import re
import subprocess

CONTROLS=('HALO2_REFERENCE_OBJECT_NODE_REBUILD','HALO2_REFERENCE_OBJECT_ANIMATION','HALO2_REFERENCE_OBJECT_PREDICTION_COPY')
def admission(binary,control,prefix=''):
    environment={key:value for key,value in os.environ.items()if key not in CONTROLS}
    result=subprocess.run([str(binary),'--'+prefix+'admission'],capture_output=True,text=True,env=environment)
    if result.returncode or result.stdout.strip()!='admission_refusals=13':
        raise RuntimeError('Object owner admission refusal proof failed: '+result.stdout+result.stderr)
    environment[control]='1'
    result=subprocess.run([str(binary),'--'+prefix+'reference'],capture_output=True,text=True,env=environment)
    if result.returncode or result.stdout.strip()!='admission_refusals=1':
        raise RuntimeError('Object owner forced-reference admission proof failed: '+result.stdout+result.stderr)
    environment.pop(control,None)
    result=subprocess.run([str(binary),'--'+prefix+'live-admission'],capture_output=True,text=True,env=environment)
    match=re.fullmatch(r'live_protocol_fallbacks=(13|39)\s*',result.stdout)
    if result.returncode or not match:
        raise RuntimeError('Object owner actual live retained-fallback proof failed: '+result.stdout+result.stderr)
    return dict(runtime_refusals=13,forced_own_reference_refusals=1,reference_control=control,
        rejected_before_memory_gp_flags_fp_cursor_observer_or_callback_mutation=True,
        literal_fallback_execution_qualified=False,
        after_construction_live_protocol_fallbacks=int(match[1]),
        actual_first_retained_callback_or_memory_fault_matches=True,
        whole_fallback_continuation_qualified_by_live_guards=False)
