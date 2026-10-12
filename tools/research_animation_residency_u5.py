#!/usr/bin/env python3
"""Complete raw resource activation/capacity/cache-owner correspondence."""
import json
import research_unit_pose_requests_u5 as original
original.ENTRIES=(0x124290,0x1244b0,0x1248d0,0x124980,0x124a30,0x124af0,
 0x124e40,0x124f30,0x124bf0,0x124ca0,0x124d00,0x1250c0,0x125120,
 0x13e8e0,0x13f020,0x13eae0,0x1908e0,0x190930,0x2d7a1a)
save=original.save
def scoped_save(path,raw):
 if path.name=='report.json':
  value=json.loads(raw);value['comparator_dependency_sha256']=value['tool_sha256'];value['tool_sha256']=original.sha(__file__)
  value['scope']='Complete static resource initialization/activation/capacity/demand/cache/IO caller bodies. Unseeded124980/124A30 entries are recovered from original raw bytes and actual45AD78 callback pointers; not annotation truth. No runtime activebyte/residency/IO result is supplied by this comparator.'
  raw=original.encoded(value)
 save(path,raw)
original.save=scoped_save
if __name__=='__main__':original.main()
