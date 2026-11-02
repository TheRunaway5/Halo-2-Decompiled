#!/usr/bin/env python3
"""Exact35getter/twoJSON/gates, plus separately proved Host-witness admission."""
import inspect,json
from pathlib import Path
import validate_native_moving_shape_world_counters_tu15 as parent
ROOT=Path(__file__).resolve().parents[1]
def main():
 fields=json.loads((ROOT/'analysis/physics/native-moving-shape-retained-world-stage-20261102/source-pins.json').read_text())['fields'];source=inspect.getsource(parent.main)
 source=source.replace("end=world.index('    NativeTu15MovingShapeNativeServices native;',begin)","end=world.index('    NativeTu15RetainedStoreObserver moving_observer;',begin)")
 source=source.replace('#include "halo2/native_tu15_world_integration.hpp"','#include "halo2/native_tu15_world_integration.hpp"\n#include "halo2/native_tu15_world_retained_store_observation.hpp"')
 marker='void check(bool ok,const char*s)';assert source.count(marker)==1
 source=source.replace(marker,'struct ModelMemory:SourcePhysicsContactMemory{void read(std::uint32_t,std::span<std::byte>)const override{throw std::runtime_error("unexpected DATA");}void write(std::uint32_t,std::span<const std::byte>)override{throw std::runtime_error("unexpected DATA");}};\n'+marker)
 marker='NativeTu15WorldIntegrationStats activity;auto*stats=&activity;';assert source.count(marker)==1;source=source.replace(marker,marker+'ModelMemory m;const NativeTu15WorldRetainedStoreWitness*retained_stores=nullptr;')
 marker='setenv("HALO2_REQUIRE_NATIVE_MOVING_SHAPE_PROCESS","1",1);';assert source.count(marker)==1;source=source.replace(marker,'unsetenv("HALO2_TU15_PHANTOM_TRACE");'+marker)
 marker='for(unsigned mode=0;mode<4;++mode)';assert source.count(marker)==1
 source=source.replace(marker,'''setenv("HALO2_TU15_PHANTOM_TRACE","1",1);auto traced=good;traced.native_moving_process_retained_observer_calls=2;traced.native_moving_process_retained_observer_events=64;check(evaluate(traced),"traced positive gate");auto invalid=traced;invalid.native_moving_process_retained_observer_calls=1;check(!evaluate(invalid),"traced count mismatch admitted");invalid=traced;invalid.native_moving_process_retained_observer_events=0;check(!evaluate(invalid),"missing observed events admitted");unsetenv("HALO2_TU15_PHANTOM_TRACE");
'''+marker)
 source=source.replace('fields=33,json_scalar_fields=66','fields=35,json_scalar_fields=70').replace('negative_gates=12','negative_gates=14')
 env=dict(parent.__dict__,FIELDS=fields,__file__=__file__);exec(source,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
