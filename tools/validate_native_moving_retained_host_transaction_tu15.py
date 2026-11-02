#!/usr/bin/env python3
"""Exact retained setter transaction: injected allocation failures keep consent."""
from pathlib import Path
import inspect
import validate_native_moving_retained_host_witness_tu15 as parent
ROOT=Path(__file__).resolve().parents[1]
def main():
 source=inspect.getsource(parent.main).replace('native-moving-retained-host-witness-20261102-third','native-moving-retained-host-transaction-20261102-first')
 marker='using namespace halo2;';assert source.count(marker)==1
 source=source.replace(marker,'''#include <cstdlib>
#include <new>
static int fail_allocation_after=-1;
void*operator new(std::size_t n){if(fail_allocation_after==0){fail_allocation_after=-1;throw std::bad_alloc{};}if(fail_allocation_after>0)--fail_allocation_after;if(void*p=std::malloc(n?n:1))return p;throw std::bad_alloc{};}
void operator delete(void*p)noexcept{std::free(p);}void operator delete(void*p,std::size_t)noexcept{std::free(p);}
'''+marker)
 marker='for(unsigned mode=0;mode<10;++mode)';assert source.count(marker)==1
 test='''for(unsigned registration=0;registration<3;++registration)for(int fault=0;fault<2;++fault){
 PhysicsLiveWorldTu15 prior;unsigned old_calls{},new_calls{};const auto old_callback=[&](auto,const auto&){++old_calls;};
 if(registration==0)prior.observe_physics_stores(old_callback);else if(registration==1)prior.observe_physics_stores_read_only(old_callback);else prior.observe_physics_retained_stores(old_callback);
 const bool old_read_only=prior.impl_->physics_store_observer_read_only,old_retained=prior.impl_->physics_store_observer_retained;
 SourcePhysicsStoreObserverTu15 pending=[&](auto,const auto&){++new_calls;};bool refused=false;fail_allocation_after=fault;
 try{prior.observe_physics_retained_stores(std::move(pending));}catch(const std::bad_alloc&){refused=true;}
 const bool consumed=fail_allocation_after<0;fail_allocation_after=-1;
 check(refused&&consumed,"wrapping failure was not injected inside exact setter");check(prior.impl_->physics_store_observer_read_only==old_read_only&&prior.impl_->physics_store_observer_retained==old_retained,"setter failure granted wrong consent");prior.impl_->physics_store_observer(event);check(old_calls==1&&new_calls==0,"setter failure replaced previous callable");}
'''
 source=source.replace(marker,test+marker)
 source=source.replace('exact_world_preflight_models=10,','exact_world_preflight_models=10,exact_setter_allocation_failures=6,')
 env=dict(parent.__dict__,STAGE=ROOT/'analysis/physics/native-moving-shape-retained-world-stage-v2-20261102/staged',__file__=__file__);exec(source,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
