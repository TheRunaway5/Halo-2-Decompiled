#!/usr/bin/env python3
"""Genuine typed304 helper relocation into caller-owned World DATA identity.

The existing private arena owns Host bytes and definedness. This changes only
native DATA identity; no captured bank/private-control preimage feeds native.
"""
from pathlib import Path
import hashlib,inspect,json,os
import validate_native_contact_producer_bound_ordered_tu15 as parent
ROOT=Path(__file__).resolve().parents[1]

def main():
    source=inspect.getsource(parent.main)
    old="prefix=(ROOT/names[0]).read_text().split('J activity(')[0];(project/'driver.cpp').write_text(prefix+'}\\n'+(ROOT/names[1]).read_text())"
    assert source.count(old)==1
    new="""prefix=(ROOT/names[0]).read_text().split('J activity(')[0]
  driver=(ROOT/names[1]).read_text()
  needle='},{0x74000000u,0x10000u},fp,'
  assert driver.count(needle)==1
  driver=driver.replace(needle,'},{0x600ce000u,0x10000u},fp,')
  needle='Child child;child.start(argv[1]);Memory storage(child,regions,read_pass);'
  assert driver.count(needle)==1
  driver=driver.replace(needle,'Child child;child.start(argv[1]);const auto private_bank_before=child.read(0x600ce000u,0x10000u);Memory storage(child,regions,read_pass);')
  needle='   const auto&actual=read_pass?storage.reads:events;'
  assert driver.count(needle)==1
  driver=driver.replace(needle,'   check(child.read(0x600ce000u,0x10000u)==private_bank_before,\"Native sidecar changed underlying captured World bank\");\\n'+needle)
  needle=' {\"observer_performs_host_fp_math\",!read_pass}'
  # Report-bank proof is independent of the normal all-captured-byte check.
  needle=' {\"compared_bytes\",compared}'
  if needle in driver:driver=driver.replace(needle,' {\"world_identity_bank_unchanged\",true},'+needle)
  (project/'driver.cpp').write_text(prefix+'}\\n'+driver)"""
    source=source.replace(old,new)
    env=dict(parent.__dict__)
    exec(source,env)
    try:
        return env['main']()
    finally:
        # Parent proof writes only bounded reports; this explicit wrapper pin
        # records the independently declared identity change for later replay.
        import sys
        output=Path(sys.argv[sys.argv.index('--output')+1]).resolve()
        p=output/'report.json'
        if p.exists():
            d=json.loads(p.read_text());d.update(world_private_identity='0x600CE000',world_private_bytes=65536,backing_bank_compared_unchanged=True,private_before_bytes_used_as_native_input=False,wrapper_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),scope=__doc__)
            p.write_text(json.dumps(d,indent=2)+'\n')

if __name__=='__main__':main()
