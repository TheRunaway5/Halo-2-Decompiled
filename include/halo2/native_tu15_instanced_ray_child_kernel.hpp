#pragma once
#include <cstdint>

namespace halo2::native_instanced_ray_child_detail {
enum class Field {scenario,count,flags,table,key};
struct Progress {std::uint32_t child{},scenario{},key{};};
// The output view owns its alias lifetime. In particular, key() is lazy and
// follows the scenario write: retained caller arguments can share that store.
template<class Frame>std::uint32_t resolve(Frame&frame,Progress&d){
    d={};d.child=frame.child();frame.test_child(d.child);
    if(!d.child){frame.null_child();return 0u;}
    d.scenario=frame.scenario();frame.store(Field::scenario,d.scenario,d);
    d.key=frame.key();frame.store(Field::count,1u,d);frame.store(Field::flags,0u,d);
    frame.store(Field::table,0x46625cu,d);frame.store(Field::key,d.key,d);return d.child;
}
}
