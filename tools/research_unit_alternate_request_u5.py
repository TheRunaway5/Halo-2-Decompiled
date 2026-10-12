#!/usr/bin/env python3
"""Complete alternate request/availability correspondence and direct coupling."""
import json
import research_mp_inventory_switch_u5 as original
original.ENTRIES=(0x1df670,0x1cfa90,0x1deb60,0x29c6a0,0x1e0f50,0x1e13c0,0x124e40)
save=original.save
def scoped_save(path,raw):
 if path.name=='report.json':
  report=json.loads(raw)
  report['comparator_dependency_sha256']=report['tool_sha256']
  report['tool_sha256']=original.sha(__file__)
  report['scope']='Complete alternate request/availability and direct resource coupling static correspondence only. No actual TU1.5 unit, queue, resource initialization, clock, transform sampling or IO success is asserted; external helpers not separately mapped remain explicit.'
  raw=original.encoded(report)
 save(path,raw)
original.save=scoped_save
if __name__=='__main__':original.main()
