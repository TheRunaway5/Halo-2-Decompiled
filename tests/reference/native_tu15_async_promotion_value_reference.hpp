#pragma once
#include "halo2/source_async_jobs.hpp"
#include "native_tu15_async_queue_value_reference.hpp"
#include <stdexcept>
namespace halo2::async_promotion_value_reference {
void valid(unsigned index){if(index>=150)throw std::invalid_argument("source async node outside150-slot bank");}
bool reprioritize(SourceAsyncQueue &queue,std::uint32_t job,std::int32_t priority){
    const auto node=job&255u;valid(node);
    if(queue.nodes[node].job!=job)return false;
    //120CE0 unlinks without clearing its existing job identity.
    source_async_unlink(queue,node);queue.nodes[node].job=job;
    queue.nodes[node].priority=priority;async_queue_value_reference::insert(queue,node);return true;
}
}
