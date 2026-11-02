#include "halo2/native_tu15_draw_units.hpp"
#include "halo2/native_tu15_sphere_rectangle.hpp"
#include "halo2/native_tu15_light_parent_biped.hpp"
#include "halo2/native_tu15_render_prepare.hpp"
#include "halo2/source_geometry_resource_request.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <iostream>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif
using namespace halo2;
using J=nlohmann::json;
namespace {
struct Memory:SourcePhysicsContactMemory {
    struct Bank {std::uint32_t address;std::vector<std::byte> bytes;};
    std::vector<Bank> banks;mutable J reads=J::array();J writes=J::array();
    std::span<const std::byte> view(std::uint32_t at,std::size_t size)const {
        for(const auto& b:banks)if(at>=b.address&&std::uint64_t(at-b.address)+size<=b.bytes.size())
            return std::span(b.bytes).subspan(at-b.address,size);
        throw std::runtime_error("Unmapped NativeRenderLighting read at "+std::to_string(at));
    }
    std::span<std::byte> view(std::uint32_t at,std::size_t size) {
        for(auto& b:banks)if(at>=b.address&&std::uint64_t(at-b.address)+size<=b.bytes.size())
            return std::span(b.bytes).subspan(at-b.address,size);
        throw std::runtime_error("Unmapped NativeRenderLighting write at "+std::to_string(at));
    }
    void read(std::uint32_t at,std::span<std::byte> bytes)const override {
        const auto from=view(at,bytes.size());std::copy(from.begin(),from.end(),bytes.begin());
        reads.push_back(J::array({at,bytes.size()}));
    }
    void write(std::uint32_t at,std::span<const std::byte> bytes)override {
        auto into=view(at,bytes.size());std::copy(bytes.begin(),bytes.end(),into.begin());
        writes.push_back(J::array({at,bytes.size()}));
    }
    void put(std::uint32_t at,std::uint32_t value,unsigned size=4) {
        std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));
        write(at,std::span(bytes).first(size));
    }
};
std::uint32_t get(const Memory& m,std::uint32_t at) {
    std::array<std::byte,4> bytes{};m.read(at,bytes);std::uint32_t value{};
    for(unsigned i=0;i<4;++i)value|=std::to_integer<unsigned>(bytes[i])<<(8*i);return value;
}
void check(bool v,const char* s){if(!v)throw std::runtime_error(s);}
struct Boundary {std::uint32_t entry;};
J run(const J& row) {
    Memory m;for(const auto& bank:row.at("banks")) {
        const auto h=bank.at("before").get<std::string>();std::vector<std::byte> bytes;
        for(std::size_t i=0;i<h.size();i+=2)bytes.push_back(std::byte(std::stoul(h.substr(i,2),nullptr,16)));
        m.banks.push_back({bank.at("address"),std::move(bytes)});
    }
    const auto before=row.at("before_fp").get<std::array<std::uint32_t,3>>();
    NativeTu15PhysicsFrameFp fp{std::uint16_t(before[0]),std::uint16_t(before[1]),before[2]};
    J calls=J::array(),stores=J::array();std::uint32_t result{},ecx{},edx{},return_flags{},triples{},return_word{};bool stopped{};
    SourcePhysicsContactRecordObserver draw_observer;
    const auto call=[&](auto entry,J arguments){calls.push_back(J{{"entry",entry},{"arguments",std::move(arguments)}});};
    SourceGeometryResourceMemory io;
    io.read=[&](auto at,auto size)->std::span<const std::byte>{m.reads.push_back(J::array({at,size}));return m.view(at,size);};
    io.write=[&](auto at,auto size){m.writes.push_back(J::array({at,size}));return m.view(at,size);};
    SourceGeometryResourceRequestServices resources;
    resources.acquire=[&](auto header,bool blocking,bool selective,std::int32_t priority) {
        call(0x130eb0,J::array({header,std::uint32_t(blocking),std::uint32_t(selective),std::bit_cast<std::uint32_t>(priority)}));
        throw Boundary{0x130eb0};
    };
    resources.promote=[&](auto job,auto priority){call(0x1236a0,J::array({job,priority}));throw Boundary{0x1236a0};};
    resources.yield=[&]{call(0x2dbf74,J::array());throw Boundary{0x2dbf74};};
    resources.pump=[&]{call(0x128870,J::array());throw Boundary{0x128870};};
    NativeTu15DrawUnitServices services;
    services.rectangle=[&](auto center,auto output,auto radius,auto& arithmetic) {
        call(0x30270,J::array({center,output,radius}));
        native_tu15_sphere_rectangle_30270(m,center,0x4c6634u,0x4c65c0u,output,radius,arithmetic,draw_observer);
    };
    if(row.value("native_rectangle_default",false))services.rectangle={};
    const auto append_queue=[&](auto queue,auto object,auto flags,auto value,auto association,auto entry_eax) {
        call(0x166b60,J::array({queue,object,flags,value,association,entry_eax}));
        return native_tu15_object_queue_append_166b60(m,queue,object,flags,value,association,entry_eax);
    };
    services.lighting.request=[&](auto header,auto&) {
        call(0x130980,J::array({3,header}));
        return source_geometry_resource_request(header,3,SourceGeometryResourceGlobals::title_update_1_5(),io,resources);
    };
    services.lighting.object=[&](auto owner,auto&) {
        call(0xbcdf0,J::array({owner,0xffffffffu}));return native_tu15_object_body_bcdf0(m,owner,0xffffffffu);
    };
    const auto gp=row.at("gp").get<std::array<std::uint32_t,8>>();
    if(row.value("argument_stack",false)) {
        m.put(gp[4],row.value("return_pc",0x63000000u));m.put(gp[4]+4u,row.at("matrix"));m.put(gp[4]+8u,row.at("source"));
        m.writes=J::array();
    }
    try {
        const auto entry=row.at("entry").get<std::uint32_t>();
        if(entry==0x2cb10)native_tu15_draw_units_begin_2cb10(m);
        else if(entry==0x166b60) {
            const auto args=row.at("queue_arguments").get<std::array<std::uint32_t,4>>();
            const auto appended=append_queue(gp[1],args[0],args[1],args[2],args[3],gp[0]);
            result=appended.eax;ecx=appended.ecx;edx=appended.edx;
        }
        else if(entry==0x31ec2) {
            NativeTu15LightParentServices parent;parent.append=append_queue;
            native_tu15_light_parent_biped_31ec2(m,row.at("definition_data"),row.at("shadow_input"),gp[3],gp[6],parent);
        }
        else if(entry==0x30270) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
            call(0x30270,J::array({gp[0],gp[6],row.at("matrix")}));
            const auto host_mxcsr=_mm_getcsr();std::uint16_t host_control;
            asm volatile("fnstcw %0":"=m"(host_control));_mm_setcsr(0x5f80);
            SourcePhysicsContactRecordObserver observer;
            if(row.value("observe",false))observer=[&](const auto& event) {
                std::uint16_t actual_control;asm volatile("fnstcw %0":"=m"(actual_control));
                check(_mm_getcsr()==0x5f80&&actual_control==host_control,"30270 kept its guest FP environment across an observer service");
                const auto hex=[](const auto& bytes){std::string h;const char* d="0123456789abcdef";
                    for(auto b:bytes){const auto v=std::to_integer<unsigned>(b);h+=d[v>>4];h+=d[v&15];}return h;};
                stores.push_back(J{{"instruction",event.instruction},{"address",event.address},{"before",hex(event.before)},{"written",hex(event.written)}});
                NativeTu15PhysicsFrameFp callback_fp{0x27f,0,0x1f80};
                check(native_tu15_floor_16ac0(m,0x40600000,callback_fp)==3,"30270 observer source service failed");
            };
            const auto residue=native_tu15_sphere_rectangle_30270(m,gp[0],gp[1],gp[2],gp[6],row.at("matrix"),fp,observer);
            result=residue.eax;ecx=residue.ecx;triples=residue.x87_triples;
            return_flags=native_tu15_sphere_rectangle_return_flags(gp[4],row.value("eflags",0x202u));
            check(_mm_getcsr()==0x5f80,"30270 did not restore the host arithmetic environment");_mm_setcsr(host_mxcsr);
#else
            throw std::runtime_error("Native30270 fixture needs its exact x86 arithmetic backend");
#endif
        }
        else if(entry==0x135e00)result=native_tu15_draw_flags_135e00(m,std::uint16_t(gp[2]),row.at("matrix"),fp);
        else if(entry==0x135ea0)native_tu15_draw_clusters_135ea0(m,gp[3],fp,services);
        else if(entry==0x136030)native_tu15_draw_instances_136030(m,row.at("matrix"),fp,services);
        else if(entry==0x16ac0)result=std::bit_cast<std::uint32_t>(native_tu15_floor_16ac0(m,row.at("matrix"),fp));
        else if(entry==0x135dc0)result=native_tu15_mask_arena_135dc0(m,gp[1],gp[2]).address;
        else {
            NativeTu15DrawUnitCall input{gp[0],gp[1],gp[2],row.at("arguments").get<std::array<std::uint32_t,21>>()};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
            const auto host_mxcsr=_mm_getcsr();std::uint16_t host_control;
            asm volatile("fnstcw %0":"=m"(host_control));_mm_setcsr(0x5f80);
            const auto hex=[](const auto& bytes){std::string h;const char* d="0123456789abcdef";
                for(auto b:bytes){const auto v=std::to_integer<unsigned>(b);h+=d[v>>4];h+=d[v&15];}return h;};
            if(row.value("observe",false))draw_observer=[&](const auto& event) {
                std::uint16_t actual_control;asm volatile("fnstcw %0":"=m"(actual_control));
                check(_mm_getcsr()==0x5f80&&actual_control==host_control,"2CBD0 kept guest FP across a store callback");
                stores.push_back(J{{"instruction",event.instruction},{"address",event.address},{"before",hex(event.before)},{"written",hex(event.written)}});
                NativeTu15PhysicsFrameFp callback_fp{0x27f,0,0x1f80};
                check(native_tu15_floor_16ac0(m,0x40600000,callback_fp)==3,"2CBD0 observer source service failed");
            };
            const auto write=[&](auto pc,auto at,std::span<const std::byte> bytes) {
                if(!draw_observer){m.write(at,bytes);return;}
                SourcePhysicsContactRecordStore event{pc,at,std::vector<std::byte>(bytes.size()),std::vector<std::byte>(bytes.begin(),bytes.end())};
                m.read(at,event.before);m.write(at,bytes);draw_observer(event);
            };
            if(row.value("argument_stack",false)) {
                services.argument_write=[&](auto pc,unsigned index,auto value) {
                    std::array<std::byte,4> bytes;for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8u*n));
                    write(pc,gp[4]+4u+4u*index,bytes);
                };
                io.store_tu15=write;
            }
            NativeTu15DrawUnitDetails details;
            try{result=std::uint16_t(native_tu15_draw_unit_2cbd0(m,input,fp,services,draw_observer,&details));}
            catch(...){check(_mm_getcsr()==0x5f80,"2CBD0 prefix did not restore host FP");_mm_setcsr(host_mxcsr);throw;}
            return_word=details.return_word;
            check(_mm_getcsr()==0x5f80,"2CBD0 did not restore host arithmetic environment");_mm_setcsr(host_mxcsr);
#else
            result=std::uint16_t(native_tu15_draw_unit_2cbd0(m,input,fp,services));
#endif
        }
    }catch(const Boundary& b) {
        const auto entries=row.at("stop_before_entries").get<std::vector<std::uint32_t>>();
        check(std::find(entries.begin(),entries.end(),b.entry)!=entries.end(),"2CFE0 reached an undeclared resource service");stopped=true;
    }
    J banks=J::array();const char* digits="0123456789abcdef";
    for(const auto& b:m.banks)if(b.address!=0x62000000u) {
        std::string h;for(auto byte:b.bytes){const auto v=std::to_integer<unsigned>(byte);h+=digits[v>>4];h+=digits[v&15];}
        banks.push_back(J{{"address",b.address},{"after",h}});
    }
    std::string stack_above;
    if(row.value("argument_stack",false)) {
        const char* d="0123456789abcdef";for(auto b:m.view(gp[4],0x62020000u-gp[4])) {
            const auto v=std::to_integer<unsigned>(b);stack_above+=d[v>>4];stack_above+=d[v&15];
        }
    }
    return {{"result",result},{"return_word",return_word},{"stack_above",stack_above},{"stopped",stopped},{"calls",calls},{"reads",m.reads},{"writes",m.writes},
        {"banks",banks},{"fp",J::array({fp.control_word,fp.status,fp.mxcsr})},{"ecx",ecx},{"edx",edx},{"eflags",return_flags},{"x87_triples",triples},{"stores",stores}};
}
}
int main()try {
    for(std::string line;std::getline(std::cin,line);)if(!line.empty())std::cout<<run(J::parse(line)).dump()<<'\n'<<std::flush;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
