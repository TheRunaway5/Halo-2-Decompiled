#!/usr/bin/env python3
"""Read-only next generic body/type constructor boundary, complete originals."""
import json
import research_unit_pose_requests_u5 as original
original.ENTRIES=(0xb8a30,0x10cbd0,0x10cc20,0xc0890,0x1cea90)
save=original.save
def scoped_save(path,raw):
 if path.name=='report.json':
  value=json.loads(raw);value['comparator_dependency_sha256']=value['tool_sha256'];value['tool_sha256']=original.sha(__file__)
  value['scope']='Read-only next complete generic body/type constructor boundary. Full original paired byte/call/table correspondence is static evidence only; none of these bodies is newly implemented or supplied as a successful native service.'
  raw=original.encoded(value)
 save(path,raw)
original.save=scoped_save
if __name__=='__main__':original.main()
