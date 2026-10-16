"""Authenticate a declared 1164D0 component input from a real failed object tick."""
import json,re,struct
from pathlib import Path
from audit_rendering import Cache,file_sha
from oracle_charged_effect_lifetime_bank_tu15 import regions
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from validate_charge_effect_bank_tu15 import SCOPE,SCOPE_PIN

def validate(case,metadata,rows):
    assert metadata['entry'] in (0x1164d0,0x116440,0x116400) and metadata['declared_component_caller']
    transition=metadata['entry'] in (0x116440,0x116400)
    build=json.loads((case/'build.json').read_text());origin=Path(build['origin_case'])
    assert file_sha(origin/'build.json')==build['origin_build_sha256']
    parent=json.loads((origin/'build.json').read_text());assert parent['returncode']==1
    observation=json.loads((origin/'native.json').read_text())
    assert observation['public_campaign_placement'] and observation['placement_completed']
    callsite=0xc7ec8 if metadata['entry']==0x116400 else 0xc7ee1 if transition else 0xc7ed1
    returned=callsite+5
    assert f'at {callsite:x}:' in observation['failure'] and f'-> {metadata["entry"]:x} return {returned:x}' in observation['failure']
    gp=[int(v,16) for v in re.search(r'\[gp ([0-9a-f ]+)\]',observation['failure']).group(1).split()]
    assert gp[0]==metadata['unit']==observation['hunter']
    assert file_sha(origin/'failure-bank.bin')==parent['files_sha256']['failure-bank.bin']
    assert file_sha(origin/'native.json')==parent['files_sha256']['native.json']
    assert file_sha(Path(parent['command'][0]))==parent['binary_sha256']
    for p,digest in parent['linked_files_sha256'].items():assert file_sha(Path(p))==digest
    baseline=list(regions(origin/'failure-bank.bin'))
    def read(p,n):
        for q,data in baseline:
            if q<=p and p+n<=q+len(data):return data[p-q:p-q+n]
        raise AssertionError(hex(p))
    def u(p,n=4):return int.from_bytes(read(p,n),'little')
    unit=metadata['unit'];headers=u(u(0x4ecbd0)+0x44);header=headers+(unit&65535)*12
    assert u(header,2)==unit>>16
    body=u(header+8);offset=int.from_bytes(read(body+0x33e,2),'little',signed=True);record=body+offset
    assert record==metadata['query_record']
    admitted={0x600f0000:struct.pack('<I',0x60300000)}
    caller_unit=metadata.get('caller_unit',unit)
    assert caller_unit==unit or (transition and caller_unit==0xffffffff)
    if transition:
        original_stack=[int(v,16) for v in re.search(r'\[stack ([0-9a-f ]+)\]',observation['failure']).group(1).split()]
        assert original_stack[1]==0x3f000000
        assert 0<=metadata['duration_bits']<=0xffffffff
        admitted[0x600f0004]=struct.pack('<I',metadata['duration_bits'])
        if 'declared_transition_bytes' in metadata:
            raw=bytes.fromhex(metadata['declared_transition_bytes']);assert len(raw)==4
            admitted[record+0x88]=raw
    if 'declared_enabled' in metadata:
        assert metadata['declared_enabled'] in (0,1) and metadata['declared_flags'] in range(4)
        admitted[record+0x89]=bytes([metadata['declared_enabled']])
        admitted[record+0x8b]=bytes([metadata['declared_flags']])
    assert len(rows)==len(baseline)
    for (p,data),(q,prior) in zip(rows,baseline):
        assert p==q and len(data)==len(prior);expected=bytearray(prior)
        for at,raw in admitted.items():
            if p<=at and at+len(raw)<=p+len(prior):expected[at-p:at-p+len(raw)]=raw
        assert data==expected
    for name in ('before.bin','native-input.bin','native.json'):
        assert file_sha(case/name)==build['files_sha256'][name]
    caller=(case/'native-input.bin').read_bytes()
    assert len(caller)==168 and struct.unpack_from('<8I',caller)==(caller_unit,0,0,0,0x600f0000,0,0,0)
    assert caller[32:160]==bytes(128) and struct.unpack_from('<II',caller,160)==(2,0x60200000)
    assert file_sha(SCOPE)==SCOPE_PIN and file_sha(NEW_DEFAULT)==NEW_PIN
    call=_Code(Xbe(NEW_DEFAULT),'.text').at(callsite)
    assert call.mnemonic=='call' and call.operands[0].imm==metadata['entry']
    path=Path(parent['command'][2]).resolve();scope=json.loads(SCOPE.read_text())
    pin=next(m['sha256'] for m in scope['maps'] if Path(m['path'])==path);assert file_sha(path)==pin
    cache=Cache(path)
    try:
        tag=cache.tags[u(body)]
        assert tag['group']=='bipd' and tag['name']=='objects\\characters\\hunter\\hunter'
        return dict(map=str(path),map_sha256=pin,actual_failed_object_caller=callsite,
            declared_component_caller=True,unit=unit,query_record=record,
            declared_enabled=metadata.get('declared_enabled'),declared_flags=metadata.get('declared_flags'),
            caller_unit=caller_unit,duration_bits=metadata.get('duration_bits'),declared_transition_bytes=metadata.get('declared_transition_bytes'),
            full_object_update_verified=False,validation_tool_sha256=file_sha(Path(__file__)))
    finally:cache.close()
