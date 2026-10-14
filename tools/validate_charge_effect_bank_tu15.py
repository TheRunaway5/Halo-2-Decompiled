"""Authenticate a captured charge callback against its actual cache and bank."""
import hashlib,json,struct
from pathlib import Path
from audit_rendering import Cache,file_sha

ROOT=Path(__file__).resolve().parents[1]
SCOPE=ROOT/'analysis/weapons-tu15-goal-20261013/authored-scope.json'
SCOPE_PIN='6855b4f33585b332ca82e4b688b9e29c6a7a04eca5757c1bc8fe9b20640c6a1b'

def validate(case,metadata,rows):
    assert file_sha(SCOPE)==SCOPE_PIN
    build=json.loads((case/'build.json').read_text())
    for name in ('before.bin','after.bin','native-input.bin','native-exit.bin','native-stores.bin','native.json'):
        assert file_sha(case/name)==build['files_sha256'][name],name
    path=Path(build['command'][2]);scope=json.loads(SCOPE.read_text())
    expected=next(m['sha256'] for m in scope['maps'] if Path(m['path'])==path)
    assert file_sha(path)==expected
    def read(address,size):
        for p,b in rows:
            if p<=address and address+size<=p+len(b):return b[address-p:address-p+size]
        raise AssertionError(('Native source range absent',hex(address),size))
    def u(address,size=4):return int.from_bytes(read(address,size),'little')
    def record(slot,datum,stride):
        pool=u(slot);index=datum&65535
        assert datum!=0xffffffff and u(pool+0x24)==stride and index<u(pool+0x38)
        address=u(pool+0x44)+index*stride
        assert u(address,2)==datum>>16 and u(address,2)
        return address
    def body(datum):return u(record(0x4ecbd0,datum,12)+8)
    cache=Cache(path)
    try:
        weapon=metadata['weapon'];weapon_body=body(weapon);definition=u(weapon_body)
        assert definition==metadata['definition']
        tag=cache.tags[definition];assert tag['group']=='weap'
        root=u(u(0x4f0414)+(definition&65535)*16+8);assert root==tag['address']
        assert u(root+0x2c8)==1
        trigger=read(u(root+0x2cc),0x40)
        raw_root=tag['address']-cache.magic
        raw_trigger=cache.u32(raw_root+0x2cc)-cache.magic
        assert trigger==bytes(cache.data[raw_trigger:raw_trigger+0x40])
        effect=struct.unpack_from('<I',trigger,0x34)[0];assert effect==metadata['effect']
        effect_tag=cache.tags[effect];assert effect_tag['group']=='effe'
        effect_root=u(u(0x4f0414)+(effect&65535)*16+8);assert effect_root==effect_tag['address']
        assert read(effect_root,0x18)==bytes(cache.data[effect_root-cache.magic:effect_root-cache.magic+0x18])
        assert u(effect_root)==metadata['effect_flags']
        behavior=struct.unpack_from('<h',trigger,6)[0]
        charging,discharging=struct.unpack_from('<2h',trigger,0x18)
        parents=[];parent=u(weapon_body+0x14)
        while parent!=0xffffffff:
            assert parent not in parents and len(parents)<64
            parents.append(parent);parent=u(body(parent)+0x14)
        if 'vehicle' in metadata:assert parents and parents[-1]==metadata['vehicle']
        raw=(case/'native-input.bin').read_bytes();assert len(raw)==168
        gp=struct.unpack_from('<8I',raw);assert gp[4]==0x600f0000 and u(gp[4])==0x60300000
        if metadata['entry']==0x108a70:
            assert gp[0]==effect and gp[1]==weapon and read(gp[4]+4,12)==struct.pack('<3I',0xffffffff,0,0)
        elif metadata['entry']==0x108db0:
            # Whole1030F0 passes the full weapon in ECX and trigger0 on the
            # caller stack. This explicit helper probe does not certify the
            # AI/player input owner that selects the production transition.
            assert gp[1]==weapon and u(gp[4]+4)==0
            assert behavior==2 and charging==1 and parents
        elif metadata['entry']==0x1030f0:
            assert u(gp[4]+4)==weapon and parents
            assert behavior==2 and charging==1 and u(weapon_body+0x20c,1)==1
            instance=metadata['effect_instance']
            assert u(weapon_body+0x214)==instance
            assert u(record(0x4f720c,instance,400)+4)==effect
        elif metadata['entry']==0x17a0f0:
            instance=metadata['effect_instance'];assert gp[7]==instance and u(gp[4]+4)==0
            assert u(record(0x4f720c,instance,400)+4)==effect
        else:raise AssertionError('Unadmitted charge consumer entry')
        return dict(map=str(path),map_sha256=expected,authored_scope_sha256=SCOPE_PIN,
            weapon=tag['name'],weapon_body=weapon_body,weapon_definition=definition,
            parent_chain=parents,effect=effect_tag['name'],effect_definition=effect,
            trigger_behavior=behavior,charging_type=charging,discharging_type=discharging,
            idle_charge_route=behavior==3 or (behavior==2 and (charging==1 or discharging==1)),
            validation_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    finally:cache.close()
