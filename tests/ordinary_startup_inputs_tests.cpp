#include "halo2/multiplayer_world.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_execution_calls.hpp"
#include "halo2/source_execution_inputs.hpp"
#include "ordinary_multiplayer_trace.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <span>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

using namespace halo2;
namespace {
using Json=nlohmann::json;
using Environment=ordinary_trace::Environment;
constexpr std::uint64_t bank_limit=256ull*1024*1024;
constexpr unsigned observed_steps=2;
void check(bool value,const char *message){if(!value)throw FormatError(message);}
std::string hex(std::span<const std::byte> bytes){constexpr char digits[]="0123456789abcdef";
    std::string out;out.reserve(bytes.size()*2);for(auto b:bytes){const auto n=std::to_integer<unsigned>(b);out+=digits[n>>4];out+=digits[n&15];}return out;}
struct Files {
    std::filesystem::path path;
    explicit Files(const std::filesystem::path &parent){path=parent/"ordinary-startup-inputs";check(std::filesystem::create_directory(path),"Startup scratch path already exists");}
    ~Files(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
struct File {
    int fd=-1;
    explicit File(const std::filesystem::path &path,bool writing){fd=::open(path.c_str(),writing?
        O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW:O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK,0600);
        check(fd>=0,"Cannot open owned startup bank file");}
    ~File(){if(fd>=0)::close(fd);}
    File(const File&)=delete;
    void write(std::span<const std::byte> bytes){while(!bytes.empty()){const auto n=::write(fd,bytes.data(),bytes.size());
        if(n<0&&errno==EINTR)continue;check(n>0,"Startup bank write failed");bytes=bytes.subspan(std::size_t(n));}}
    void read(std::span<std::byte> bytes){while(!bytes.empty()){const auto n=::read(fd,bytes.data(),bytes.size());
        if(n<0&&errno==EINTR)continue;check(n>0,"Startup bank file is truncated");bytes=bytes.subspan(std::size_t(n));}}
    void word(std::uint64_t value){const auto bytes=ordinary_trace::little(value);write(bytes);}
    std::uint64_t word(){std::array<std::byte,8> bytes{};read(bytes);std::uint64_t value{};
        for(unsigned i=0;i<8;++i)value|=std::uint64_t(std::to_integer<unsigned>(bytes[i]))<<(i*8);return value;}
};
struct Bank {std::filesystem::path path;std::uint64_t ranges{},bytes{},file_bytes{};std::string sha256;};
Bank save_bank(const SourceRuntimeMemory &memory,const std::filesystem::path &path){
    Environment preserved;File output(path,true);Bank out{path};ordinary_trace::Hash hash;
    std::uint64_t end{};
    memory.visit_committed([&](std::uint32_t address,std::span<const std::byte> bytes){
        check(!bytes.empty()&&address>=end&&bytes.size()<=bank_limit-out.bytes,"Startup committed-bank cap/order exceeded");
        end=std::uint64_t(address)+bytes.size();check(end<=0x100000000ull,"Startup source range wraps");
        output.word(address);output.word(bytes.size());output.write(bytes);
        hash.word(address);hash.word(bytes.size());hash.bytes(bytes);++out.ranges;out.bytes+=bytes.size();
    });
    check(out.ranges>0&&out.ranges<=65536,"Startup bank has no committed source ranges or too many ranges");
    output.word(out.ranges);output.word(out.bytes);hash.word(out.ranges);hash.word(out.bytes);
    out.sha256=hex(hash.finish());out.file_bytes=out.bytes+out.ranges*16+16;
    check(out.file_bytes<=bank_limit+65536*16+16,"Startup bank file exceeds its explicit bound");return out;
}
void compare_bank(const SourceRuntimeMemory &memory,const Bank &expected,Json &row){
    Environment preserved;File input(expected.path,false);struct stat info{};
    check(::fstat(input.fd,&info)==0&&S_ISREG(info.st_mode)&&std::uint64_t(info.st_size)==expected.file_bytes,"Startup reference bank extent/type changed");
    std::uint64_t count{},bytes{},end{};std::array<std::byte,65536> buffer{};ordinary_trace::Hash hash;
    memory.visit_committed([&](std::uint32_t address,std::span<const std::byte> value){
        check(count<expected.ranges&&!value.empty()&&address>=end&&value.size()<=bank_limit-bytes,"Replay committed-bank cap/order differs");
        const auto old_address=input.word(),old_size=input.word();
        if(old_address!=address||old_size!=value.size()){
            row["first_bank_difference"]={{"expected_address",old_address},{"actual_address",address},{"expected_bytes",old_size},{"actual_bytes",value.size()}};
            throw FormatError("Startup committed source range differs");
        }
        for(std::size_t offset=0;offset<value.size();){const auto n=std::min(buffer.size(),value.size()-offset);input.read(std::span(buffer).first(n));
            if(std::memcmp(buffer.data(),value.data()+offset,n)){
                std::size_t i{};while(i<n&&buffer[i]==value[offset+i])++i;
                row["first_bank_difference"]={{"address",std::uint64_t(address)+offset+i},{"expected_byte",std::to_integer<unsigned>(buffer[i])},{"actual_byte",std::to_integer<unsigned>(value[offset+i])}};
                const auto begin=i-i%4,window=std::min<std::size_t>(32,n-begin);
                row["first_bank_difference"]["window_address"]=std::uint64_t(address)+offset+begin;
                row["first_bank_difference"]["expected_window_hex"]=hex(std::span(buffer).subspan(begin,window));
                row["first_bank_difference"]["actual_window_hex"]=hex(value.subspan(offset+begin,window));
                // The inactive async queue retains real task payload history.
                // Report it when this exact node differs; never exclude it.
                if(std::uint64_t(address)+offset+i>=0x4ecc38u && std::uint64_t(address)+offset+i<0x4ecc78u) {
                    row["first_bank_difference"]["actual_first_async_job_hex"]=hex(memory.view(0x4ecc38u,0x40));
                    const auto first=std::uint64_t(address)+offset;
                    if(first<=0x4ecc38u && first+n>=0x4ecc78u)
                        row["first_bank_difference"]["expected_first_async_job_hex"]=hex(std::span(buffer).subspan(0x4ecc38u-first,0x40));
                }
                throw FormatError("Startup committed source byte differs");
            }offset+=n;
        }
        hash.word(address);hash.word(value.size());hash.bytes(value);++count;bytes+=value.size();end=std::uint64_t(address)+value.size();
    });
    check(count==expected.ranges&&bytes==expected.bytes&&input.word()==count&&input.word()==bytes,"Replay omitted a committed range or reference footer differs");
    std::byte trailing{};check(::read(input.fd,&trailing,1)==0,"Startup bank reference has trailing data");
    hash.word(count);hash.word(bytes);check(hex(hash.finish())==expected.sha256,"Startup complete bank digest differs");
    row["committed_bank_equal"]=true;row["committed_ranges"]=count;row["committed_bytes"]=bytes;row["committed_sha256"]=expected.sha256;
}
bool bound_finish_rejected(SourceExecutionInputs &tape){Environment preserved;
    try{(void)tape.finish();}catch(const std::logic_error &error){return std::string_view(error.what())=="Native execution input phase is still bound";}return false;
}
struct Observation {Bank bank;std::vector<std::byte> state;std::uint32_t tick{};};
Observation observe(MultiplayerPreparedWorld &world,const std::filesystem::path &path){
    // This is the actual public native state read. Only the following hashing
    // and file work is diagnostic and wrapped in Environment.
    const auto state=world.match.state();Environment preserved;
    return {save_bank(world.source_objects->memory(),path),source_execution_state_result(state),state.tick};
}
void verify_observation(MultiplayerPreparedWorld &world,const Observation &expected,Json &row){
    const auto state=world.match.state();Environment preserved;const auto digest=source_execution_state_result(state);
    row["tick"]=state.tick;row["state_sha256"]=hex(digest);row["expected_tick"]=expected.tick;row["expected_state_sha256"]=hex(expected.state);
    check(state.tick==expected.tick&&digest==expected.state,"Actual native Match state publication differs");
    compare_bank(world.source_objects->memory(),expected.bank,row);row["state_equal"]=true;
}
std::shared_ptr<MultiplayerPreparedWorld> make_world(MultiplayerOptions &options,const std::shared_ptr<SourceExecutionInputs> &tape){
    return std::make_shared<MultiplayerPreparedWorld>(options,std::optional<ScriptProgram>{},
        std::shared_ptr<PhysicsLiveClockSource>{},tape);
}
void close_world(std::shared_ptr<MultiplayerPreparedWorld> &world,MultiplayerOptions &options,
    const std::shared_ptr<SourceExecutionInputs> &tape,Json &row){
    const auto path=*world->source_savegames;std::weak_ptr<MultiplayerPreparedWorld> weak=world;
    auto alias=world;options.prepared_world=world;world.reset();
    check(!weak.expired()&&bound_finish_rejected(*tape),"A live PreparedWorld alias did not retain startup binding");
    alias.reset();options.prepared_world.reset();
    check(weak.expired(),"A PreparedWorld alias survived closure");
    check(!std::filesystem::exists(path),"Native startup private source files survived world closure");
    row["all_prepared_aliases_released"]=true;row["automatic_source_files_removed"]=true;
}
void step(MultiplayerPreparedWorld &world){
    std::array<Controls,multiplayer_capacity> controls{};
    // Read the real retained view angles; no gameplay/source result is assigned.
    for(const auto slot:world.local_slots){controls[slot].yaw=world.match.unit(slot).yaw;controls[slot].pitch=world.match.unit(slot).pitch;}
    world.match.step(controls);
}
std::string tape_hash(std::span<const SourceExecutionInput> values){Environment preserved;ordinary_trace::Hash hash;
    for(const auto &value:values){hash.word(std::uint8_t(value.kind));hash.word(value.operand);hash.word(value.value);}return hex(hash.finish());
}
void rejected_startup(MultiplayerOptions options,std::span<const SourceExecutionInput> values,const char *name,Json &report){
    auto tape=std::make_shared<SourceExecutionInputs>(values);std::string error;
    try{auto world=make_world(options,tape);(void)world;}
    catch(const std::runtime_error &failure){error=failure.what();}
    auto &control=report["negative_controls"].emplace_back(Json{{"name",name},{"observed_error",error},{"passed",false}});
    constexpr std::string_view input_error="Native execution input order or operand differs from the producer";
    // The genuine RNG constructor bridge retains this failure and appends its
    // actual root/PC and register/stack context. Accept that observed wrapper,
    // while still requiring the precise input-boundary failure.
    const auto native_error="TU1.5 live actual root 148d40 at 33a4d3: "+std::string(input_error)+" [gp ";
    check(error==input_error || error.starts_with(native_error),"Invalid first startup input did not fail at its actual native request");
    bool sealed_failure{};try{(void)tape->finish();}catch(const std::runtime_error &){sealed_failure=true;}
    check(sealed_failure,"Rejected startup input tape was not invalidated");
    control["rejected_before_supplying_wrong_value"]=true;control["binding_released_after_constructor_failure"]=true;
    control["passed"]=true;
}
void run(unsigned players,MultiplayerOptions options,const std::filesystem::path &directory,Json &report,
    std::vector<SourceExecutionInput> *captured_inputs){
    options.local_players=players;options.prepared_world.reset();
    auto &row=report["cases"].emplace_back(Json{{"players",players},{"observations",Json::array()},
        {"constructor_bound",false},{"clock_provider","actual NativeWorldClock"}});
    auto capture=std::make_shared<SourceExecutionInputs>();auto world=make_world(options,capture);
    check(world->cache->is_mcc()&&world->source_objects&&world->source_objects->owns_physics()&&world->local_slots.size()==players,"Actual MCC native PreparedWorld was not constructed");
    check(source_execution_inputs(world->source_objects->memory())==capture.get()&&bound_finish_rejected(*capture),"Startup tape is not retained at constructor return");
    row["constructor_bound"]=true;
    std::vector<Observation> expected;expected.reserve(observed_steps+1);
    for(unsigned n=0;n<=observed_steps;++n){if(n)step(*world);expected.push_back(observe(*world,directory/(std::to_string(players)+"p-bank-"+std::to_string(n)+".bin")));
        row["recorded_ticks"].push_back(expected.back().tick);
        check(!n || expected[n].tick==expected[n-1].tick+1,"Actual preparation step did not advance one native source tick");}
    close_world(world,options,capture,row["capture_cleanup"]);
    // Includes actual native exit callbacks and arena retirement, not merely
    // the reads observed before the public constructor returned.
    const auto sealed=capture->finish();check(!sealed.empty(),"Genuine startup consumed no recorded external inputs");
    const std::vector<SourceExecutionInput> original(sealed.begin(),sealed.end());const auto original_hash=tape_hash(original);
    if(captured_inputs)*captured_inputs=original;
    row["capture_input_count"]=original.size();row["capture_input_sha256"]=original_hash;
    row["capture_input_kind_counts"]=Json::object();
    for(const auto &value:original){const auto name=std::to_string(unsigned(value.kind));
        row["capture_input_kind_counts"][name]=row["capture_input_kind_counts"].value(name,0u)+1;}
    auto replay=std::make_shared<SourceExecutionInputs>(std::span<const SourceExecutionInput>(original));world=make_world(options,replay);
    check(source_execution_inputs(world->source_objects->memory())==replay.get()&&bound_finish_rejected(*replay),"Replay startup binding did not survive constructor return");
    for(unsigned n=0;n<=observed_steps;++n){if(n)step(*world);auto &observation=row["observations"].emplace_back(Json{{"ordinal",n}});
        verify_observation(*world,expected[n],observation);}
    close_world(world,options,replay,row["replay_cleanup"]);check(replay->finish().size()==original.size(),"Replay did not consume the complete startup-through-destruction input tape");
    check(tape_hash(original)==original_hash,"Original recorded startup tape changed");
    row["input_count"]=original.size();row["input_sha256"]=original_hash;row["kind_counts"]=Json::object();
    for(const auto &value:original){const auto name=std::to_string(unsigned(value.kind));row["kind_counts"][name]=row["kind_counts"].value(name,0u)+1;}
    row["sealed_after_native_destruction"]=true;row["replay_complete"]=true;
    for(const auto &value:expected)check(std::filesystem::remove(value.bank.path),"Owned raw startup bank file was not removed");
    row["raw_bank_files_removed"]=true;
}
}
int main(int argc,char **argv){Json report={{"schema","H2_ORDINARY_STARTUP_INPUTS_TEST_V1"},{"passed",false},
    {"actual_mcc_native_constructors",true},{"runtime_xbe_argument",false},{"synthetic_clock_or_callee",false},
    {"cases",Json::array()},{"negative_controls",Json::array()},{"state_comparison","existing complete named-field source_execution_state_result digest"},
    {"bank_comparison","all visit_committed ranges and raw bytes, streamed without normalization"},
    {"uncommitted_topology_or_resource_checkpoint_qualified",false},{"full_callback_trace_or_gpu_qualified",false},
    {"bank_bytes_per_observation_cap",bank_limit},{"bank_file_count_per_case_cap",observed_steps+1}};
    std::filesystem::path output,owned_files;
    try{check(argc==4,"Supply prepared MCC_MAP MCC_MAPS REPORT");output=argv[3];
        check(!std::filesystem::exists(output),"Startup report already exists; preserve the prior result");
        const auto *scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch&&*scratch,"Startup regression requires tools/run_test.py owned scratch");
        Files files(std::filesystem::canonical(scratch));owned_files=files.path;MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
        options.source_object_world=true;options.mute=true;options.starting_weapon=5;options.session_random_seed=0x15c071u;
        check(options.gameplay_xbe.empty(),"Startup regression must use compiled ordinary native operands");
        {Cache cache(options.map,options.shared_maps);check(cache.is_mcc(),"Startup regression requires genuine prepared MCC inputs");}
        bool complete=true;std::vector<SourceExecutionInput> captured_inputs;
        for(const unsigned players:{1u,4u}) {
            const auto parent=files.path/std::to_string(players);
            check(std::filesystem::create_directory(parent),"Startup case scratch already exists");
            const auto case_index=report["cases"].size();
            try {
                // Retire this case's raw references before the next case,
                // including after a mismatch; one failure must not hide4p.
                Files case_files(parent);run(players,options,case_files.path,report,players==1?&captured_inputs:nullptr);
            }catch(const std::exception &error) {
                complete=false;
                if(report["cases"].size()==case_index)report["cases"].push_back({{"players",players}});
                report["cases"].at(case_index)["error"]=error.what();
                if(!report.contains("error"))report["error"]=error.what();
            }
        }
        // Malformed-input controls use the actual sealed producer transcript
        // even when an independent cold-start bank comparison has failed.
        check(!captured_inputs.empty(),"Negative controls lack an actual sealed startup transcript");
        const auto original_hash=tape_hash(captured_inputs);
        for(const bool missing:{true,false})try {
            auto wrong=captured_inputs;
            if(missing)wrong.clear();else wrong.front()={wrong.front().kind==SourceExecutionInputKind::Milliseconds?
                SourceExecutionInputKind::TimestampCounter:SourceExecutionInputKind::Milliseconds,0,1};
            rejected_startup(options,wrong,missing?"missing-first-input":"wrong-first-input-kind",report);
        }catch(const std::exception &error) {
            complete=false;report["negative_control_errors"].push_back({
                {"name",missing?"missing-first-input":"wrong-first-input-kind"},{"error",error.what()}});
            if(!report.contains("error"))report["error"]=error.what();
        }
        check(tape_hash(captured_inputs)==original_hash,"Negative controls modified the genuine source transcript");
        auto recovery=std::make_shared<SourceExecutionInputs>();auto fresh=make_world(options,recovery);
        check(source_execution_inputs(fresh->source_objects->memory())==recovery.get(),"Fresh startup after rejected construction lost its binding");
        Json cleanup;close_world(fresh,options,recovery,cleanup);check(!recovery->finish().empty(),"Fresh capture after rejected constructor is empty");
        report["fresh_capture_after_rejected_startup"]=std::move(cleanup);
        report["passed"]=complete;
    }catch(const std::exception &error){report["error"]=error.what();}catch(...){report["error"]="Unknown startup regression failure";}
    if(!owned_files.empty()){report["owned_bank_files_removed"]=!std::filesystem::exists(owned_files);if(!report["owned_bank_files_removed"].get<bool>())report["passed"]=false;}
    if(!output.empty()&&!std::filesystem::exists(output)){std::ofstream file(output,std::ios::binary);file<<report.dump(2)<<'\n';if(!file)report["passed"]=false;}
    std::cout<<report.dump()<<'\n';return report["passed"].get<bool>()?0:1;
}
