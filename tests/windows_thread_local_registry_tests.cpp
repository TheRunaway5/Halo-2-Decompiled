#include "windows_thread_local_registry_fixture.hpp"
#include <iostream>
#include <thread>

namespace halo2::tests::tls {
std::atomic<unsigned> constructed{},destroyed{};
namespace {thread_local LifetimeMarker marker(47);}
}
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
}
int main()try {
    using namespace halo2;using namespace halo2::tests::tls;
    check(producer_marker()==31&&marker.value==47,"Different TU thread-local initializers were coalesced");
    const auto main_addresses=registry_addresses();
    check(main_addresses==producer_addresses(),"Registry has a different instance in another translation unit");
    for(std::size_t i=0;i<main_addresses.size();++i)
        for(std::size_t j=0;j<i;++j)
            check(main_addresses[i]!=main_addresses[j],"Different registries share one TLS instance");
    check(registries_empty(),"Main thread registries started with foreign bindings");
    Memory memory;unsigned calls{},nested{};
    const auto emit=[&]{SourceObjectAttachmentScopeTu15 scope(memory);
        scope.inputs(9,10,11,12);scope.complete(13);};
    auto binding=producer_bind(memory,[&](const auto& event){
        check(event.parent==9&&event.caller_return==13,"Cross-TU observer changed its event");++calls;
    });
    emit();check(calls==1,"Cross-TU binding was not visible");
    {
        SourceObjectAttachmentBindingTu15 inner(memory,[&](const auto&){++nested;});
        producer_emit(memory);check(nested==1&&calls==1,"Nested binding did not replace the producer callback");
    }
    emit();check(calls==2,"Nested binding did not restore the producer callback");
    binding.reset();check(registries_empty(),"Binding retirement leaked a registry entry");
    auto admission=std::make_shared<source_script_sound_detail::Admission>();
    registry(source_script_sound_detail::admissions).emplace(nullptr,admission);
    check(producer_admission()==admission.get(),"Script sound admission differed between translation units");
    registry(source_script_sound_detail::admissions).clear();admission.reset();

    std::weak_ptr<unsigned> retained;std::exception_ptr failure;
    std::thread worker([&]{try {
        const auto addresses=registry_addresses();
        check(addresses==producer_addresses(),"Worker registry differs across translation units");
        for(std::size_t i=0;i<addresses.size();++i)
            check(addresses[i]!=main_addresses[i],"Registry state leaked across threads");
        check(registries_empty(),"Worker inherited main-thread observer state");
        check(producer_marker()==31&&marker.value==47,"Worker lost a TU-specific initializer");
        auto owner=std::make_shared<unsigned>(73);retained=owner;
        registry(source_object_attachment_handlers_tu15).emplace(&memory,[owner](const auto&){
            check(*owner==73,"Worker callback lost its captured owner");
        });
        owner.reset();check(!retained.expired(),"Live TLS callback released its owner early");
        producer_emit(memory);
    }catch(...){failure=std::current_exception();}});
    worker.join();if(failure)std::rethrow_exception(failure);
    check(retained.expired(),"Thread exit did not destroy the TLS registry's callback");
    check(constructed==4&&destroyed==2,"TU-specific TLS constructor/destructor ownership differs");
    check(registries_empty(),"Worker teardown changed the main thread's bindings");
    std::cout<<"11 thread-local registries: cross-TU identity, nested callbacks, thread isolation and destruction passed\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
