#include "halo2/native_execution_accounting.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <mutex>
#include <string>
#include <vector>

namespace halo2::execution_accounting {
namespace {
constexpr std::size_t report_limit=1u<<20;
struct Registry { std::mutex mutex;std::vector<Counter*> counters; };
std::atomic<std::uint64_t> registration_failures{};
std::atomic<std::uint64_t> orphan_literal_events{};
thread_local Scope* current_scope{};
// Deliberately process-lifetime storage: generated calls made by later static
// destructors still register valid counters. The pointer remains reachable.
std::atomic<Registry*>& registry_pointer() { static std::atomic<Registry*> value{};return value; }
Registry& registry() {
    static Registry* const instance=[] {auto* value=new Registry;registry_pointer().store(value,std::memory_order_release);return value;}();
    return *instance;
}
void quoted(std::string& out,const char* value) {
    constexpr char hex[]="0123456789abcdef";out+='"';
    for(const auto* p=reinterpret_cast<const unsigned char*>(value);*p;++p){
        if(*p=='"'||*p=='\\'){out+='\\';out+=char(*p);}
        else if(*p<32){out+="\\u00";out+=hex[*p>>4];out+=hex[*p&15];}
        else out+=char(*p);
    }
    out+='"';
}
void number(std::string& out,std::uint64_t value) {
    std::array<char,24> bytes{};const auto result=std::to_chars(bytes.data(),bytes.data()+bytes.size(),value);
    out.append(bytes.data(),result.ptr);
}
bool unit_less(const UnitIdentity* a,const UnitIdentity* b) {
    const int source=std::strcmp(a->source,b->source);if(source)return source<0;
    const int canonical=std::strcmp(a->canonical_sha256,b->canonical_sha256);if(canonical)return canonical<0;
    return std::strcmp(a->input_sha256,b->input_sha256)<0;
}
bool unit_equal(const UnitIdentity* a,const UnitIdentity* b) {return !unit_less(a,b)&&!unit_less(b,a);}
void at_exit_report() noexcept {
    const char* path=std::getenv("HALO2_NATIVE_EXECUTION_REPORT");
    if(path&&*path&&!write_report(path))std::fputs("Native execution accounting report failed\n",stderr);
}
#if defined(__GNUC__) || defined(__clang__)
// Run after ordinary C++ static destruction. No iostream/locale state is used.
__attribute__((destructor(101))) void late_report() {at_exit_report();}
#endif
}
Counter::Counter(const UnitIdentity* u,const char* s,std::uint32_t sites,std::uint32_t dynamic) noexcept:unit(u),symbol(s),literal_sites(sites),dynamic_forwarders(dynamic) {
    const int saved=errno;
    try {
    auto& r=registry();std::lock_guard guard(r.mutex);r.counters.push_back(this);
#if !defined(__GNUC__) && !defined(__clang__)
    static const bool registered=[] {std::atexit(at_exit_report);return true;}();(void)registered;
#endif
    }catch(...) {registration_failures.fetch_add(1,std::memory_order_relaxed);}
    errno=saved;
}
Scope::Scope(Counter& counter,unsigned image) noexcept:counter_(counter.images[image<3?image:2]),owner_(&counter),previous_(current_scope),exceptions_(std::uncaught_exceptions()) {
    current_scope=this;
    counter_.entries.fetch_add(1,std::memory_order_relaxed);counter_.active.fetch_add(1,std::memory_order_relaxed);
}
Scope::~Scope() noexcept {
    current_scope=previous_;
    if(std::uncaught_exceptions()>exceptions_){
        counter_.unwinds.fetch_add(1,std::memory_order_relaxed);
        if(!body_)counter_.unwinds_without_body.fetch_add(1,std::memory_order_relaxed);
    }else{
        counter_.returns.fetch_add(1,std::memory_order_relaxed);
        if(!body_)counter_.returns_without_body.fetch_add(1,std::memory_order_relaxed);
    }
    counter_.active.fetch_sub(1,std::memory_order_relaxed);
}
void Scope::literal() noexcept {
    if(!body_){body_=true;counter_.body_entries.fetch_add(1,std::memory_order_relaxed);}
    counter_.statements.fetch_add(1,std::memory_order_relaxed);
}
void record_literal(Counter& counter) noexcept {
    for(auto* scope=current_scope;scope;scope=scope->previous_)
        if(scope->owner_==&counter){scope->literal();return;}
    orphan_literal_events.fetch_add(1,std::memory_order_relaxed);
}
bool write_report(const char* path) noexcept try {
    const int saved=errno;
    auto* r=registry_pointer().load(std::memory_order_acquire);if(!r)return false;
    std::vector<Counter*> records;
    {std::lock_guard guard(r->mutex);records=r->counters;}
    std::sort(records.begin(),records.end(),[](const auto* a,const auto* b){
        if(!unit_equal(a->unit,b->unit))return unit_less(a->unit,b->unit);
        return std::strcmp(a->symbol,b->symbol)<0;
    });
    std::vector<const UnitIdentity*> units;
    for(const auto* record:records)
        if(units.empty()||!unit_equal(units.back(),record->unit))units.push_back(record->unit);
    std::string out="{\"schema\":\"native-execution-accounting-v1\",\"units\":[";
    for(std::size_t i=0;i<units.size();++i){
        if(i)out+=',';out+='[';quoted(out,units[i]->source);out+=',';quoted(out,units[i]->canonical_sha256);
        out+=',';quoted(out,units[i]->input_sha256);out+=',';quoted(out,units[i]->image_scope);out+=']';
    }
    out+="],\"image_names\":[\"map-pack11\",\"tu15\",\"unknown\"],\"row_fields\":[\"unit\",\"symbol\",\"image\",\"literal_sites\",\"dynamic_forwarders\",\"entries\",\"body_entries\",\"literal_statements\",\"returns\",\"unwinds\",\"returns_without_body\",\"unwinds_without_body\",\"active\"],\"rows\":[";
    std::size_t published=0,omitted=0;std::uint64_t active=0;
    for(const auto* record:records)for(unsigned image=0;image<3;++image){
        const auto& stats=record->images[image];
        if(!stats.entries.load(std::memory_order_relaxed))continue;
        std::string row="[";const auto unit=std::lower_bound(units.begin(),units.end(),record->unit,unit_less)-units.begin();
        number(row,unit);row+=',';quoted(row,record->symbol);row+=',';number(row,image);row+=',';number(row,record->literal_sites);
        row+=',';number(row,record->dynamic_forwarders);
        const std::array values{stats.entries.load(std::memory_order_relaxed),stats.body_entries.load(std::memory_order_relaxed),
            stats.statements.load(std::memory_order_relaxed),stats.returns.load(std::memory_order_relaxed),
            stats.unwinds.load(std::memory_order_relaxed),stats.returns_without_body.load(std::memory_order_relaxed),
            stats.unwinds_without_body.load(std::memory_order_relaxed),stats.active.load(std::memory_order_relaxed)};
        active+=values.back();for(auto value:values){row+=',';number(row,value);}row+=']';
        if(out.size()+row.size()+1024u>report_limit){++omitted;continue;}
        if(published++)out+=',';out+=row;
    }
    out+="],\"omitted_records\":";number(out,omitted);out+=" ,\"active_scopes\":";number(out,active);
    out+=" ,\"registered_records\":";number(out,records.size());
    out+=" ,\"registration_failures\":";number(out,registration_failures.load(std::memory_order_relaxed));
    out+=" ,\"orphan_literal_events\":";number(out,orphan_literal_events.load(std::memory_order_relaxed));
    out+=" ,\"counter_scope\":\"source-qualified literal-instruction functions; statement sites independent of service callbacks\",\"native_closure_proved\":false,\"snapshot\":\"process-exit or explicit caller synchronization\"}\n";
    if(out.size()>report_limit){errno=saved;return false;}
    auto* file=std::fopen(path,"wb");if(!file){errno=saved;return false;}
    const bool written=std::fwrite(out.data(),1,out.size(),file)==out.size();const bool closed=std::fclose(file)==0;
    errno=saved;return written&&closed;
} catch(...) {return false;}
}
