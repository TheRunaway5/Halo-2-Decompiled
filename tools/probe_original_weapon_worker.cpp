#include "halo2/animation_async_worker.hpp"
#include "halo2/native_execution_gate.hpp"
#include "halo2/native_threads.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <fstream>
#include <future>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <vector>
using namespace halo2;
using J=nlohmann::json;
namespace {
struct Memory {
    std::vector<std::byte> bytes;
    std::uint32_t cr3,cr4;
    NativeExecutionGate gate;
    Memory(const char *path,const J &input):cr3(input["control"]["CR3"]),cr4(input["control"]["CR4"]) {
        std::ifstream file(path,std::ios::binary|std::ios::ate);
        if(!file || file.tellg()!=67108864)throw std::runtime_error("Actual original64MiB input unavailable");
        bytes.resize(67108864);file.seekg(0);file.read(reinterpret_cast<char *>(bytes.data()),bytes.size());
        if(!file || !(input["control"]["CR0"].get<std::uint32_t>()&0x80000000u) || cr4&0x20u)
            throw std::runtime_error("Original page mode differs");
    }
    std::uint32_t physical_word(std::uint32_t p) const {
        if(std::uint64_t(p)+4>bytes.size())throw std::out_of_range("Original physical word escapes");
        std::uint32_t value;std::memcpy(&value,bytes.data()+p,4);return value;
    }
    std::uint32_t physical(std::uint32_t address) const {
        const auto directory=physical_word((cr3&0xfffff000u)+(address>>22)*4u);
        if(!(directory&1))throw std::out_of_range("Original PDE absent");
        if(directory&128u){if(!(cr4&16u))throw std::runtime_error("Original PSE unavailable");return (directory&0xffc00000u)+(address&0x3fffffu);}
        const auto entry=physical_word((directory&0xfffff000u)+((address>>12)&1023u)*4u);
        if(!(entry&1))throw std::out_of_range("Original PTE absent");
        return (entry&0xfffff000u)+(address&4095u);
    }
    std::span<std::byte> span(std::uint32_t p,std::size_t n) {
        gate.require_owned();const auto start=physical(p);
        if(std::uint64_t(p)+n>0x100000000ull || std::uint64_t(start)+n>bytes.size())
            throw std::out_of_range("Original native backing escapes");
        for(std::size_t i=0;i<n;){
            if(physical(p+std::uint32_t(i))!=start+i)throw std::runtime_error("This borrowed raw span is not physically contiguous");
            i+=std::min<std::size_t>(n-i,4096-((p+i)&4095));
        }
        return std::span(bytes).subspan(start,n);
    }
};
}
int main(int argc,char **argv) {
    try {
        if(argc!=5)throw std::invalid_argument("Expected input JSON, original RAM, result and native RAM output");
        J input;std::ifstream(argv[1])>>input;
        Memory memory(argv[2],input);
        SourceAnimationResourceMemory raw;
        raw.read=[&](auto p,auto n){return std::span<const std::byte>(memory.span(p,n));};
        raw.write=[&](auto p,auto b){std::ranges::copy(b,memory.span(p,b.size()).begin());};
        NativeSynchronization synchronization({0xf3100001u,0xf3100004ull});
        const auto native=synchronization.services();
        auto create_mutex=[&](){const auto r=native.create_mutex(false);if(r.error)throw std::system_error(r.error);return r.handle;};
        const auto free_mutex=create_mutex(),work_mutex=create_mutex();
        const auto semaphore=native.create_semaphore(0,150);
        if(semaphore.error)throw std::system_error(semaphore.error);
        const auto &b=input["bindings"];
        const auto source_free=input["original_handles"]["free"].get<std::uint32_t>();
        const auto source_work=input["original_handles"]["work"].get<std::uint32_t>();
        const auto source_semaphore=input["original_handles"]["semaphore"].get<std::uint32_t>();
        const std::map<std::uint32_t,std::uint32_t> handles{{source_free,free_mutex},{source_work,work_mutex},{source_semaphore,semaphore.handle}};
        if(handles.size()!=3)throw std::runtime_error("Distinct source synchronization identities required");
        const auto file_token=input["file_token"].get<std::uint32_t>();
        SourceAsyncOwnedFiles files(std::array{SourceAsyncOwnedFile{file_token,input["owned_path"].get<std::string>()}});
        J stores=J::array(),reads=J::array();
        SourceAnimationAsyncObserver observe=[&](const auto &s){
            memory.gate.require_owned();
            const auto site=std::to_string(s.instruction);
            if(!input["instruction_map"].contains(site))throw std::runtime_error("Unproven original worker store site");
            stores.push_back({{"canonical_instruction",s.instruction},{"tu15_instruction",input["instruction_map"][site]},
                {"address",s.address},{"bytes",s.bytes},{"before",s.before},{"after",s.after}});
        };
        SourceAnimationAsyncWorkerServices base;base.memory=raw;
        base.bindings.queue.nodes=b["nodes"];base.bindings.queue.free_head=b["free_head"];base.bindings.queue.work_head=b["work_head"];
        base.bindings.queue.free_mutex_slot=b["free_mutex_slot"];base.bindings.queue.work_mutex_slot=b["work_mutex_slot"];
        base.bindings.queue.semaphore_slot=b["semaphore_slot"];base.bindings.counter=b["counter"];
        base.bindings.requests_added=b["requests_added"];base.bindings.current_node=b["current_node"];
        base.bindings.read_callback=b["read_callback"];
        base.release_mutex=[&](auto h){memory.gate.require_owned();if(auto e=native.release_mutex(handles.at(h)))throw std::system_error(e);};
        base.release_semaphore=[&](auto h,auto n){memory.gate.require_owned();const auto r=native.release_semaphore(handles.at(h),n);if(r.error)throw std::system_error(r.error);};
        base.yield=[&]{memory.gate.suspend([]{std::this_thread::yield();});};
        bool callback_completed=false;
        base.callback=[&](auto callback,auto payload){
            memory.gate.require_owned();
            if(callback!=base.bindings.read_callback || payload!=input["task_payload"].get<std::uint32_t>())
                throw std::runtime_error("Unexpected actual worker callback/task");
            const auto buffer=input["buffer"].get<std::uint32_t>(),size=input["bytes"].get<std::uint32_t>();
            const auto io=read_source_animation_async_file(payload,input["actual_chunk_limit"].get<std::uint32_t>(),
                {buffer,memory.span(buffer,size)},files,raw,observe);
            reads.push_back({{"file",io.step.file},{"offset",io.step.offset},{"buffer",io.step.buffer},{"requested",io.step.size},
                {"actual_bytes",io.io.bytes_read},{"actual_seek",io.io.seek_low},{"result",io.completion.callback_result}});
            callback_completed=io.completion.callback_result!=0;return io.completion.callback_result;
        };
        std::promise<void> retired;auto completion=retired.get_future();
        // Final-owner stop/join precedes every borrowed bank/file/observer lifetime.
        NativeThreads threads({0xf5100001u,0xf5100002ull});const auto workers=threads.services();
        const auto worker=workers.create([&](std::stop_token stop){
            auto owned=base;bool notified=false;
            owned.wait=[&](auto h,auto ms){memory.gate.suspend([&]{acquire_native_synchronization(native,handles.at(h),ms,stop,5);});};
            owned.release_mutex=[&](auto h){base.release_mutex(h);if(callback_completed&&!notified&&h==source_free){notified=true;retired.set_value();}};
            memory.gate.execute([&]{run_source_animation_async_worker(owned,observe);});
        });
        if(worker.error)throw std::system_error(worker.error);
        // Actual native signal at the observed original producer notification
        // boundary. No original kernel status or acquired permit is supplied.
        const auto signal=native.release_semaphore(semaphore.handle,input["observed_release_count"].get<int>());
        if(signal.error)throw std::system_error(signal.error);
        const auto status=completion.wait_for(std::chrono::seconds(10));
        if(auto e=workers.request_stop(worker.handle))throw std::system_error(e);
        if(auto e=workers.join(worker.handle))throw std::system_error(e);
        const auto outcome=workers.outcome(worker.handle);
        if(outcome.state==NativeThreadState::Failed&&outcome.failure)std::rethrow_exception(outcome.failure);
        if(status!=std::future_status::ready || outcome.state!=NativeThreadState::Stopped || reads.size()!=1)
            throw std::runtime_error("Actual worker retirement/stop did not complete");
        J result={{"completed",true},{"native_worker_stopped_and_joined",true},{"native_synchronization_objects",synchronization.object_count()},
            {"reads",reads},{"source_stores",stores},{"native_constructor_or_Xbox_scheduler_verified",false},
            {"expected_bytes_or_kernel_returns_supplied",false}};
        std::ofstream(argv[3])<<result.dump()<<'\n';
        std::ofstream output(argv[4],std::ios::binary);output.write(reinterpret_cast<const char *>(memory.bytes.data()),memory.bytes.size());
        if(!output)throw std::runtime_error("Cannot save disposable worker observation");
    } catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
