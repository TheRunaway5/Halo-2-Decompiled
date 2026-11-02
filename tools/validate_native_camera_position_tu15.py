#!/usr/bin/env python3
"""CD4A0 parent/native children composition with genuine query entry stops.

Parent ordered operands/stores and complete declared parent DATA are strict.
Native child operand/fault proofs are separate, never replaced by supplied
outcomes. Child-private scratch is independent native arithmetic DATA and
is not represented as a serialized original stack/control frame.
"""
import copy,inspect,struct,sys
from pathlib import Path
import validate_native_camera_adjustment_tu15 as base
physical=base.physical
CPP='src/physics/native_tu15_camera_position.cpp';HPP='include/halo2/native_tu15_camera_position.hpp'
UNITS=(CPP,*base.UNITS,'src/physics/native_tu15_camera_basis.cpp','src/physics/native_tu15_look_context.cpp','src/physics/native_tu15_constraint_auxiliary_math.cpp','src/campaign/ai/native_campaign_object_world_point_tu15.cpp','src/campaign/ai/native_campaign_object_marker_request_tu15.cpp','src/campaign/ai/native_campaign_object_markers_tu15.cpp','src/campaign/ai/native_campaign_object_motion_refs_tu15.cpp')
BACKEND='unchanged-tu15-native-camera-position-parent-v1'
DATA=physical.SP-0xa4

def metadata(image):
 source=inspect.getsource(base.base.metadata).replace('assert counts==[18,62]','assert counts==[287]')
 env=dict(base.base.__dict__,OWNERS=(0xcd4a0,));exec(source,env);meta=env['metadata'](image);code=base._Code(image,'.text');meta['boundaries']=[0x16c620]
 for row in meta['sites']:
  i=code.at(row['instruction'])
  if i.mnemonic=='call'and i.op_str!='0x16c620':row.update(opaque_call=True,instruction_size=i.size)
 return meta

def cases(image):
 out=[]
 for seed in base.cases(image):
  c=copy.deepcopy(seed)
  c.update(entry=0xcd4a0,receiver=0,entry_registers={'eax':base.POSITION},arguments=[],object=0,destination=base.POSITION,scratch=DATA,name='parent_'+seed['name'])
  banks={b['address']:bytearray.fromhex(b['before'])for b in c['banks']if b['address']!=base.DATA}
  raw=banks[base.BASE]
  def put(at,n,width=4):raw[at-base.BASE:at-base.BASE+width]=(n&((1<<(width*8))-1)).to_bytes(width,'little')
  put(base.UNIT+0x14,0xffffffff);put(base.UNIT+0xaa,0,1);put(base.UNIT+0x10a,0,1);put(base.UNIT+0x3dc,0,1);put(base.UNIT+0x3f8,0,1);put(base.UNIT+0xa0,base.bits(1.));put(base.UNIT+0x2c4,base.bits(.25));put(base.DEFINITION+0x218,base.bits(1.));put(base.DEFINITION+0x21c,base.bits(.5))
  raw[base.UNIT+0x64-base.BASE:base.UNIT+0x70-base.BASE]=struct.pack('<fff',2.,3.,4.)
  raw[base.UNIT+0x168-base.BASE:base.UNIT+0x174-base.BASE]=raw[base.FORWARD-base.BASE:base.FORWARD-base.BASE+12]
  banks[DATA]=bytearray((n*17+23)&255 for n in range(164))
  c['banks']=[dict(address=at,before=b.hex())for at,b in sorted(banks.items())];out.append(c)
 # Declared coherent parent/seat/marker fallback trees. Marker records are
 # computed by genuine original/native BAD60, never supplied or zeroed.
 for attached,seat,kind,marker,special,related in((True,0xffff,1,0,False,False),(True,0,1,0,False,False),(True,0,1,123,False,False),(True,0,0,0,False,False),(False,0xffff,0,0,True,False),(False,0xffff,1,0,False,False),(False,0xffff,0,0,True,True)):
  c=copy.deepcopy(out[0]);c['name']=f'marker_parent_{attached}_{seat}_{kind}_{marker}_{special}_{related}';raw=bytearray.fromhex(next(b for b in c['banks']if b['address']==base.BASE)['before']);parent_unit=base.BASE+0x1800;parent_definition=base.BASE+0x3800;seats=base.BASE+0x2800
  def put(at,n,width=4):raw[at-base.BASE:at-base.BASE+width]=(n&((1<<(width*8))-1)).to_bytes(width,'little')
  put(base.HEADERS+20,parent_unit);put(base.TAGS+24,parent_definition);put(parent_unit,1);put(parent_unit+0x14,0xffffffff);put(parent_unit+0xb4,0xffffffff);put(parent_unit+0xaa,kind,1);put(parent_unit+0x116,0x400,2);put(parent_unit+0x1fc,0,2);put(parent_definition+0x38,0xffffffff);put(parent_definition+0x1cc,seats);put(base.DEFINITION+0x1cc,seats);put(base.DEFINITION+0x38,0xffffffff);put(base.DEFINITION+2,0,1);put(seats+0x60,marker);put(seats+8,777)
  put(base.UNIT+0x14,1 if attached else 0xffffffff);put(base.UNIT+0x1fc,seat,2);put(base.UNIT+0xaa,0 if special else kind,1);put(base.UNIT+0x10a,4 if special else 0,1);put(base.UNIT+0x24c,1 if related else 0xffffffff);put(base.UNIT+4,0);put(base.UNIT+0x18,0,1)
  for record in(base.UNIT,parent_unit):
   for lane,value in enumerate((1.,1.,0.,0.,0.,1.,0.,0.,0.,1.,5.,6.,7.)):put(record+0x400+lane*4,base.bits(value))
  next(b for b in c['banks']if b['address']==base.BASE)['before']=raw.hex();out.append(c)
 return out

def boundary(c,actual):
 if actual['actual_stop']==physical.STOP or'fault'in c or'observer_fault'in c:return None
 assert actual['actual_stop']==0x16c620
 return dict(owner=0x16c620,arguments=actual['boundary_stack'][1:7])

def main():
 source=inspect.getsource(base.main)
 source=source.replace("p=argparse.ArgumentParser(description=__doc__)","p=argparse.ArgumentParser(description=__doc__)")
 source=source.replace("assert all(c['boundary_gp'][4]==physical.SP+16 for c in rows)","assert all(c['boundary_gp'][4]==physical.SP+4 for c in rows if c['boundary']is None and 'fault'not in c and 'observer_fault'not in c)")
 source=source.replace("physical.boundary=lambda c,o:None","physical.boundary=boundary")
 source=source.replace("native_tu15_camera_adjustment.hpp","native_tu15_camera_position.hpp").replace("NativeTu15CameraAdjustmentStats stats;","NativeTu15CameraPositionStats stats;")
 source=source.replace("'bool boundary_hit{};\\n'", "'''bool boundary_hit{};\nNativeTu15CameraPositionServices services;\nservices.native_child=[&](auto,bool entering){m.inside_child=entering;};\nservices.segment=[&](const NativeTu15CameraSegment&call,auto&){const auto&b=c.at(\"boundary\");check(!b.is_null()&&b.at(\"owner\")==0x16c620u&&b.at(\"arguments\")==J::array({call.mask,call.origin,call.displacement,call.ignore_first,call.ignore_second,call.result}),name+\" actual camera query input differs\");boundary_hit=true;throw std::runtime_error(\"actual-island-owner-boundary\");return false;};\n'''" )
 source=source.replace('native_tu15_adjust_unit_camera_position(m,c.at("object"),c.at("position"),c.at("forward"),c.at("perpendicular"),{c.at("scratch")},fp,0,observer,&stats);','native_tu15_compute_unit_camera_position(m,c.at("object"),c.at("destination"),{c.at("scratch"),{0x06000000u}},fp,0,services,observer,&stats);')
 # Child-private arithmetic uses its own native allocation. The reference
 # has no corresponding bank: no original control lifetime is serialized.
 source=source.replace("(project/'driver.cpp').write_text(driver)",'''driver=driver.replace('bool log_reads{},log_writes{};','bool log_reads{},log_writes{},inside_child{};').replace('if(log_reads)reads.push_back','if(log_reads&&!inside_child)reads.push_back').replace('if(!fault.is_null()&&','if(!inside_child&&!fault.is_null()&&').replace('if(!write_fault.is_null()&&','if(!inside_child&&!write_fault.is_null()&&').replace('if(log_writes)raw_stores.push_back','if(log_writes&&!inside_child)raw_stores.push_back').replace('stores.push_back(store);','if(m.inside_child)return;stores.push_back(store);').replace('for(bool read_pass:{false,true}){','before.banks.emplace(0x06000000u,std::vector<std::byte>(40,std::byte{0xa5}));for(bool read_pass:{false,true}){');(project/'driver.cpp').write_text(driver)''')
 a=source.index("mutants={");b=source.index('\n  for name,(before,after)',a)
 source=source[:a]+'''mutants={'wrong_height':('on_foot_tag+0x218u','on_foot_tag+0x21cu'),'wrong_position_z':('m.word(0xcd565u,destination+8u','m.word(0xcd565u,destination+4u'),'missing_cached_tag':('m.word(0xcd4d9u,data+48u,definition);','/*cache write removed*/')}'''+source[b:]
 source=source.replace("complete_body(_Code(image,'.text'),0xe18d0)","complete_body(_Code(image,'.text'),0xcd4a0)")
 source=source.replace("private_scratch_provenance='Declared40B arithmetic DATA at original private allocation only; no GP-save/control field overlaps'","private_scratch_provenance='Parent164B DATA exact; child-private arithmetic independently owned, no serialized GP/control lifetime'")
 namespace=dict(base.__dict__,metadata=metadata,cases=cases,boundary=boundary,CPP=CPP,HPP=HPP,UNITS=UNITS,BACKEND=BACKEND,__file__=__file__,__doc__=__doc__);exec(source,namespace);return namespace['main']()
if __name__=='__main__':sys.exit(main())
