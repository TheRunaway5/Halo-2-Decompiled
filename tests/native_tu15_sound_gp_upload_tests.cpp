#include "halo2/source_runtime_image.hpp"
#include "halo2/source_runtime_dsp_section.hpp"
#include "halo2/source_runtime_sound_dma_resources.hpp"
#include "halo2/source_runtime_sound_sdk_resources.hpp"
#include "halo2/source_sound_dma_storage_tu15.hpp"
#include "halo2/source_sound_dsp_image_tu15.hpp"
#include "halo2/source_sound_sdk_tu15.hpp"
#if defined(HALO2_TEST_GP_CONSUMER)
#include "halo2/sound_gp_cpu_tu15.hpp"
#include "halo2/sound_gp_dma_tu15.hpp"
#endif
#include <SDL.h>
#include <openssl/evp.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
using namespace halo2;
namespace {
using Json=nlohmann::json;
constexpr std::uint32_t stack=0x2008000,fs=0x200a000,descriptors=0x2001000,out_pointer=0x2002000;
void check(bool v,const char* text){if(!v)throw std::runtime_error(text);}
void put(SourceRuntimeMemory& m,std::uint32_t at,std::uint32_t v){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8*i));m.write(at,b);}
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t at){std::array<std::byte,4>b{};m.read(at,b);std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<unsigned>(b[i])<<(8*i);return v;}
std::string hex(std::span<const std::byte> b){constexpr char d[]="0123456789abcdef";std::string s;s.reserve(b.size()*2);for(auto c:b){auto n=std::to_integer<unsigned>(c);s+=d[n>>4];s+=d[n&15];}return s;}
std::vector<std::byte> unhex(const std::string& s){check(s.size()%2==0,"Invalid upload fixture byte string");std::vector<std::byte>b;for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));return b;}
Json load(const std::filesystem::path& p){std::ifstream f(p);check(bool(f),"Upload reference is unavailable");Json j;f>>j;return j;}
void save(const std::filesystem::path& p,const Json& j){std::ofstream f(p);f<<j.dump(2)<<'\n';check(bool(f),"Cannot retain upload evidence");}
Json upload_reference(const std::filesystem::path& path){
    auto reference=load(path/"case.json");
    check(reference.at("xbe_sha256").get<std::string>()=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&
        reference.at("entry")==0x388383&&reference.at("boundary")==0x388470&&
        reference.at("instruction_patches").empty()&&reference.at("supplied_external_results").empty()&&
        reference.at("supplied_request_clear")==false,"Upload reference supplied code, results or acknowledgement");
    return reference;
}
struct Boundary{};
struct Fixture {
    SourceRuntimeMemory memory;
    std::shared_ptr<SdlSoundPlatform> output;
    std::unique_ptr<SourceRuntimeSoundDmaResources> pages;
    std::unique_ptr<SourceRuntimeSoundSdkResources> sdk;
    std::unique_ptr<SourceRuntimeDspSection> resource;
    std::uint32_t root{},stop{};
    bool tracking{},stopped{};
    const Json* expected_before{};
    std::set<std::uint32_t> sites;
    std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> digest{nullptr,EVP_MD_CTX_free};
    unsigned stores{};
    unsigned polls{};
    std::function<void()> at_upload_poll;
    struct Memory final:SourcePhysicsContactMemory,SourceSoundDspImageMmioTu15{
        Fixture& f;explicit Memory(Fixture& v):f(v){}
        void read(std::uint32_t at,std::span<std::byte> out)const override{
            if(at>=0xfe800000)throw std::runtime_error("Upload attempted an unowned MMIO read");f.memory.read(at,out);
        }
        void write(std::uint32_t at,std::span<const std::byte> in)override{
            if(at>=0xfe800000)throw std::runtime_error("Upload bypassed its native typed GP table store");f.memory.write(at,in);
        }
        void write_sound_dsp_mmio(std::uint32_t pc,std::uint32_t at,std::uint32_t value)override{
            check(f.root!=0,"GP publication precedes its real constructor object");
            f.pages->publish_gp_scratch(word(f.memory,f.root+8),pc,at,value);
        }
    } view{*this};
    explicit Fixture(const std::filesystem::path& xbe):memory(load_source_runtime_image(xbe,SourcePhysicsContactImage::TitleUpdate15)){
        memory.mount(stack-0x1000,std::vector<std::byte>(0x2000),SourceRuntimeMemory::Access::ReadWrite,"declared native upload caller stack");
        memory.mount(fs,std::vector<std::byte>(0x100),SourceRuntimeMemory::Access::ReadWrite,"actual passive native executor context");
        memory.mount(descriptors,std::vector<std::byte>(64),SourceRuntimeMemory::Access::ReadWrite,"declared upstream extra native DMA descriptors");
        memory.mount(out_pointer,std::vector<std::byte>(4),SourceRuntimeMemory::Access::ReadWrite,"actual upload caller output pointer");
        output=std::make_shared<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::Manual,64,{}});
        const auto* driver=SDL_GetCurrentAudioDriver();check(driver&&std::string(driver)!="dummy"&&std::string(driver)!="disk","Upload composition requires an actual native output backend");
        pages=std::make_unique<SourceRuntimeSoundDmaResources>(memory,output);
        sdk=std::make_unique<SourceRuntimeSoundSdkResources>(memory,fs);
        resource=std::make_unique<SourceRuntimeDspSection>(memory,xbe);
        call(0x387bb5,0,{});call(0x387baa,0,{});
        // A real native allocation for the caller-provided object extent.
        // The complete upstream APU factory remains a separate owner.
        root=call(0x387a32,0,{0x626f5344,0x3c,1}).gp[0];check(root!=0,"Root object has no native SDK allocation");
        call(0x388275,root,{});
        allocate(0x391420,0xc000,0x4000);allocate(0x391430,0x1060,0x4000);
        call(0x388294,root,{});
        for(unsigned i=0;i<4;++i)allocate(descriptors+i*16,4096,4096);
        call(0x38850a,root,{descriptors,4});
        check(pages->gp_scratch_binding()->entries==12,"Original upload constructor/append did not publish its prefix");
        resource->retain();
    }
    void allocate(std::uint32_t at,std::uint32_t bytes,std::uint32_t alignment){
        put(memory,at+8,bytes);check(call(0x38e71c,0,{at,alignment,4}).gp[0]==0,"Native upload DMA constructor failed");
    }
    SourcePhysicsWorldRuntimeAbi call(std::uint32_t entry,std::uint32_t self,std::initializer_list<std::uint32_t> args){
        SourcePhysicsWorldRuntimeAbi a{};a.gp[1]=self;a.gp[4]=stack;a.eflags=2;a.fs_base=fs;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
        for(unsigned i=0;i<8;++i)for(unsigned j=0;j<4;++j)a.xmm[i][j]=0x100*i+j;
        put(memory,stack,0x2100000);unsigned index=1;for(auto v:args)put(memory,stack+4*index++,v);
        SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto pc){
            if(pc==0x388470){
                check(++polls<=100,"Native CPU upload did not leave its actual poll within the host audit bound");
                if(at_upload_poll)at_upload_poll();
            }
            if(pc==stop){stopped=true;throw Boundary{};}
            if(pc==0x388383){if(expected_before)compare_memory(*expected_before,"Upload prefix differs from its declared original suffix input");tracking=true;}sites.insert(pc);
        };
        services.required_owner=[&](auto& bank,const auto& c,auto& abi,const auto& observe){
            check(&bank==&view,"Upload dispatch changed its retained memory view");
            if(pages->dispatch(memory,c,abi)||sdk->dispatch(memory,c,abi,observe))return;
            if(source_sound_sdk_tu15_owns_entry(c.target)){source_run_sound_sdk_tu15(view,c.target,abi,{0x27f,0x1f80},services,observe);return;}
            if(source_sound_dma_storage_tu15_owns_entry(c.target)){source_run_sound_dma_storage_tu15(view,c.target,abi,{0x27f,0x1f80},services,observe);return;}
            throw std::runtime_error("Original upload dependency has no native owner at "+std::to_string(c.instruction));
        };
        SourcePhysicsContactRecordObserver observe;
        if(digest)observe=[&](const auto& s){if(!tracking)return;std::array<unsigned char,12> h{};
            const std::array<std::uint32_t,3>w{s.instruction,s.address,std::uint32_t(s.written.size())};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<4;++j)h[i*4+j]=static_cast<unsigned char>(w[i]>>(8*j));
            check(EVP_DigestUpdate(digest.get(),h.data(),h.size())==1&&EVP_DigestUpdate(digest.get(),s.before.data(),s.before.size())==1&&EVP_DigestUpdate(digest.get(),s.written.data(),s.written.size())==1,"Cannot retain upload stores");++stores;
        };
        SourceRuntimeSoundDmaResources::CallScope page_scope(pages.get());SourceRuntimeSoundSdkResources::CallScope sdk_scope(sdk.get());
        try{
            if(source_sound_dma_storage_tu15_owns_entry(entry))source_run_sound_dma_storage_tu15(view,entry,a,{0x27f,0x1f80},services,observe);
            else if(source_sound_sdk_tu15_owns_entry(entry))source_run_sound_sdk_tu15(view,entry,a,{0x27f,0x1f80},services,observe);
            else source_run_sound_dsp_image_tu15(view,entry,a,{0x27f,0x1f80},services,observe);
        }catch(const Boundary&){}
        return a;
    }
    SourcePhysicsWorldRuntimeAbi upload(std::uint32_t boundary){
        stop=boundary;tracking=stopped=false;stores=polls=0;digest.reset(EVP_MD_CTX_new());check(bool(digest)&&EVP_DigestInit_ex(digest.get(),EVP_sha256(),nullptr)==1,"Cannot start upload store digest");
        auto a=call(0x388576,root,{resource->address(),std::uint32_t(resource->retained_bytes().size()),out_pointer});
        check(boundary?stopped:!stopped,"Upload missed its real source boundary/return");return a;
    }
    std::string store_digest(){std::array<std::byte,32>b{};unsigned size{};check(EVP_DigestFinal_ex(digest.get(),reinterpret_cast<unsigned char*>(b.data()),&size)==1&&size==32,"Cannot finalize upload stores");return hex(b);}
    void compare_memory(const Json& banks,const char* message){
        for(const auto&[address,value]:banks.items()){
            const auto at=std::stoul(address,nullptr,16);auto expected=unhex(value);std::vector<std::byte> actual(expected.size());memory.read(at,actual);check(actual==expected,message);
        }
    }
    Json abi(const SourcePhysicsWorldRuntimeAbi& a){return{{"gp",a.gp},{"xmm",a.xmm},{"eflags",a.eflags},{"fs_base",a.fs_base},{"x87_depth",a.x87_depth},{"x87_status",a.x87_status},{"x87_control",0x27f},{"mxcsr",0x1f80}};}
    Json snapshot(const std::filesystem::path& out,const SourcePhysicsWorldRuntimeAbi& a){
        Json regions=Json::array();unsigned index{};
        memory.visit_committed([&](std::uint32_t at,std::span<const std::byte> bytes){
            auto name="region-"+std::to_string(index++)+".bin";std::ofstream f(out/name,std::ios::binary);f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());check(bool(f),"Cannot retain actual upload input pages");
            regions.push_back({{"address",at},{"bytes",bytes.size()},{"read_only",memory.is_read_only(at,bytes.size())},{"file",name}});
        });
        const auto binding=*pages->gp_scratch_binding();Json rows=Json::array();
        const auto allocations=pages->allocations();const auto pools=pages->gp_pool_allocations();
        for(unsigned i=0;i<*binding.entries;++i){
            const auto physical=word(memory,binding.table_source+i*8);check(word(memory,binding.table_source+i*8+4)==0,"Exported SGE high word is unowned");
            std::optional<std::uint32_t> source;
            for(const auto& owner:allocations)if(physical>=owner.physical&&std::uint64_t(physical)-owner.physical+4096<=owner.capacity)
                source=owner.source+(physical-owner.physical);
            for(const auto& owner:pools)if(physical>=owner.physical&&std::uint64_t(physical)-owner.physical+4096<=owner.capacity){
                check(owner.locked&&owner.context==binding.context,"Exported GP page lost its locked context");source=owner.source+(physical-owner.physical);}
            check(source.has_value(),"Exported SGE has no actual retained owner");
            rows.push_back({{"offset",i*4096},{"physical",physical},{"source",*source},{"bytes",4096}});
        }
        return{{"scope","Declared native caller input at actual original388383; no original kernel or hardware claim"},{"pc",0x388383},{"root",root},{"abi",abi(a)},{"regions",regions},{"source_request_clear_supplied",false},
            {"gp_scratch",{{"context",binding.context},{"table_source",binding.table_source},{"entries",*binding.entries},{"rows",rows}}}};
    }
};
#if defined(HALO2_TEST_GP_CONSUMER)
void consume_gp(Fixture& f,const std::filesystem::path& context,const std::filesystem::path& out){
    const auto declared=load(context);
    check(declared.at("scope")=="Declared reference-model initial context and frame/timer inputs; not authenticated MCPX reset or hardware clock",
        "GP test requires its explicitly declared initial/peripheral context");
    auto& pages=*f.pages;
    const auto scratch_bytes=*pages.gp_scratch_binding()->entries*4096u;
    std::vector<std::byte> before(scratch_bytes),after(scratch_bytes);pages.read_gp_scratch(0,before);
    check(std::to_integer<unsigned>(before[0x810])==3,"Original CPU upload did not retain request3 before GP execution");
    const auto binary=[](const auto& name,std::span<const std::byte> bytes){std::ofstream file(name,std::ios::binary);
        file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());check(bool(file),"Cannot retain native GP consumer evidence");};
    std::filesystem::create_directory(out/"gp-input");binary(out/"gp-input/initial-scratch.bin",before);
    SoundGpCpuInputTu15 input{};input.registers=declared.at("registers").get<std::array<std::uint32_t,64>>();
    std::unique_ptr<SoundGpCpuTu15> cpu;std::unique_ptr<SoundGpDmaTu15> dma;
    bool idle=false;unsigned frame_status{},frame_events{},instructions{},reads{},writes{};std::uint32_t pc{};
    input.read_peripheral=[&](auto address)->std::uint32_t{
        if(address==0xffffd6)return dma->read(address);
        if(address==0xffffc5)return frame_status|(dma->state().eol?0x80u:0u);
        if(address==0xffffb3)return declared.at("timer_read").get<unsigned>();
        throw std::runtime_error("GP boot encountered an unowned peripheral read");
    };
    input.write_peripheral=[&](auto address,auto value){
        if(address>=0xffffd4&&address<=0xffffd7){dma->write(address,value);return;}
        if(address==0xffffc5){frame_status&=~value;if(value&0x80)dma->acknowledge_eol();return;}
        if(address==0xffffc4&&value==1){idle=true;return;}
        if((address==0xffffb0||address==0xffffb1)&&value==1)return;
        if(address==0xffffb2&&value==0xffffff)return;
        throw std::runtime_error("GP boot encountered an unowned peripheral write");
    };
    cpu=std::make_unique<SoundGpCpuTu15>(std::move(input));
    SoundGpDmaResourcesTu15 resources;
    resources.read_word=[&](auto area,auto at){return cpu->read(area,at);};
    resources.write_word=[&](auto area,auto at,auto value){cpu->write(area,at,value);};
    resources.read_scratch=[&](auto at,auto bytes){pages.read_gp_scratch(at,bytes);++reads;};
    resources.write_scratch=[&](auto at,auto bytes){
        check(pc==0x11c,"GP boot scratch store moved from its actual DMA instruction");
        pages.write_gp_scratch(at,bytes);++writes;
    };
    const auto& d=declared.at("dma");
    dma=std::make_unique<SoundGpDmaTu15>(std::move(resources),SoundGpDmaStateTu15{
        d.at("configuration"),d.at("control"),d.at("start"),d.at("next"),d.at("eol"),d.at("error")});
    std::array<std::uint32_t,0x800> boot{};
    for(unsigned i=0;i<boot.size();++i)for(unsigned b=0;b<4;++b)boot[i]|=std::to_integer<unsigned>(before[i*4+b])<<(8*b);
    cpu->bootstrap(boot);
    const auto request=[&]{std::array<std::byte,4> bytes{};pages.read_gp_scratch(0x810,bytes);
        unsigned value{};for(unsigned b=0;b<4;++b)value|=std::to_integer<unsigned>(bytes[b])<<(8*b);return value;};
    while(request()){
        check(instructions<200000,"Original GP did not clear the actual native request in its instruction bound");
        if(idle){check(frame_events==0,"Upload required an unexpected second declared frame event");
            frame_status|=declared.at("frame_status").get<unsigned>();++frame_events;idle=false;}
        pc=cpu->pc();cpu->step();++instructions;
    }
    pages.read_gp_scratch(0,after);
    unsigned changed{};for(unsigned i=0;i<before.size();++i)if(before[i]!=after[i]){
        check(i==0x810&&before[i]==std::byte{3}&&after[i]==std::byte{},"GP bootstrap changed unexpected native scratch bytes");++changed;}
    check(changed==1&&writes==1&&frame_events==1,"GP acknowledgement did not come from its actual single header transfer");
    const auto prefix=word(f.memory,0x391420);
    check(word(f.memory,prefix+0x810)==0,"Native GP used a detached scratch mirror for its acknowledgement");
    std::ofstream output(out/"gp-output.bin",std::ios::binary);
    const auto put_word=[&](std::uint32_t value){std::array<char,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=char(value>>(8*i));output.write(bytes.data(),4);};
    for(auto [area,count]:{std::pair{SoundGpSpaceTu15::program,0x1000u},std::pair{SoundGpSpaceTu15::x,0xc00u},std::pair{SoundGpSpaceTu15::y,0x800u}})
        for(unsigned i=0;i<count;++i)put_word(cpu->read(area,i));
    output.write(reinterpret_cast<const char*>(after.data()),after.size());check(bool(output),"Cannot retain actual GP RAM/scratch output");output.close();
    const auto state=cpu->execution_state();
    save(out/"gp-report.json",{{"passed",true},{"scope","Actual original GP consumes real source-published201-SGE pages; explicitly declared reference-model initial context/frame/timer"},
        {"instructions",instructions},{"pc",state.pc},{"cycles",state.cycles},{"registers",state.registers},{"stack",state.stack},{"repeating",state.repeating},{"pc_on_repeat",state.pc_on_repeat},
        {"scratch_reads",reads},{"scratch_writes",writes},{"declared_frame_events",frame_events},{"request_before",3},{"request_after",0},{"changed_scratch_bytes",changed},
        {"format2","unverified and unexecuted"},{"supplied_request_acknowledgement",false},{"hardware_clock_verified",false},{"source_upload_return_verified",false},{"whole_engine_verified",false}});
    std::cout<<"PASS actual original GP bootstrap consumes real201-SGE native pages and clears CPU request3 itself; "<<instructions<<" GP instructions; format2 remains unresolved\n";
}
#endif
}
int main(int argc,char** argv)try{
    check(argc==3||argc==4
#if defined(HALO2_TEST_GP_CONSUMER)
        ||((argc==6||argc==8)&&std::string(argv[4])=="--complete-gp")
        ||(argc==6&&std::string(argv[4])=="--consume-gp")
#endif
        ,"Supply XBE fresh-output [unchanged-original suffix fixture [--consume-gp declared-context]]");if(!std::filesystem::exists(argv[1]))return 77;
    const std::filesystem::path out=argv[2];check(std::filesystem::create_directory(out),"Preserve prior upload evidence");Fixture f(argv[1]);
    if(argc==3){
        auto a=f.upload(0x388383);check(f.stores==0,"Prefix capture supplied or executed decoder suffix work");save(out/"before.json",f.snapshot(out,a));
        std::cout<<"PASS actual native constructor/allocation/upload prefix reaches original388383; retained real source frames/pages for independent original suffix execution\n";
    }
#if defined(HALO2_TEST_GP_CONSUMER)
    else if(argc>=6&&std::string(argv[4])=="--complete-gp"){
        auto reference=upload_reference(argv[3]);Json returned;
        if(argc==8){
            check(std::string(argv[6])=="--return-reference","Unknown native upload return comparison option");
            returned=load(std::filesystem::path(argv[7])/"case.json");
            check(returned.at("entry")==0x388470&&returned.at("boundary")==0x2100000&&
                returned.at("xbe_sha256")==reference.at("xbe_sha256")&&returned.at("instruction_patches").empty()&&
                returned.at("supplied_external_results").empty()&&returned.at("supplied_request_clear")==false,
                "Return fixture supplied a source result or request clear");
        }
        f.expected_before=&reference.at("before");std::string prefix_digest;unsigned prefix_stores{};
        f.at_upload_poll=[&]{
            check(f.polls==1,"Upload reached its poll again after native GP execution");
            f.compare_memory(reference.at("after"),"Complete upload prefix differs before GP execution");
            prefix_digest=f.store_digest();prefix_stores=f.stores;
            check(prefix_digest==reference.at("store_sha256").get<std::string>()&&prefix_stores==reference.at("store_count").get<unsigned>(),"Complete upload prefix stores differ");
            // Release the large prefix fixture before starting the GP owner.
            f.expected_before=nullptr;reference=Json{};
            consume_gp(f,argv[5],out);
            if(!returned.is_null())f.compare_memory(returned.at("before"),"Original CPU return input differs after actual GP transfer");
            f.stores=0;f.digest.reset(EVP_MD_CTX_new());check(bool(f.digest)&&EVP_DigestInit_ex(f.digest.get(),EVP_sha256(),nullptr)==1,"Cannot retain original CPU return stores");
        };
        auto a=f.upload(0);const auto tail_digest=f.store_digest();
        check(f.polls==1&&a.gp[0]==0&&a.gp[4]==stack+16,"Original upload did not complete its actual result/stack cleanup");
        const auto prefix=word(f.memory,0x391420),table=word(f.memory,f.root+0x20);
        check(word(f.memory,prefix+0x810)==0&&word(f.memory,out_pointer)==table&&table!=0,"Original upload return did not retain the GP acknowledgement/output descriptor");
        check(std::ranges::equal(f.memory.view(f.root+0x24,24),f.memory.view(prefix+0x800,24)),"Original upload return did not copy its actual six-word header");
        if(!returned.is_null()){
            check(f.abi(a).at("gp")==returned.at("gp_after")&&f.abi(a).at("xmm")==returned.at("xmm_after"),"Original CPU return GP/XMM differs");
            check((a.eflags&0xcc5)==(returned.at("eflags_after").get<unsigned>()&0xcc5),"Original CPU return defined flags differ");
            for(const auto* field:{"fs_base","x87_depth","x87_status","x87_control","mxcsr"})
                check(f.abi(a).at(field)==returned.at("abi_after").at(field),"Original CPU return floating-point/executor state differs");
            f.compare_memory(returned.at("after"),"Original CPU return retained bytes differ");
            check(f.stores==returned.at("store_count").get<unsigned>()&&tail_digest==returned.at("store_sha256").get<std::string>(),"Original CPU return ordered stores differ");
        }
        save(out/"return-report.json",{{"passed",true},{"scope","Original native CPU upload return after actual GP DMA acknowledgement; declared startup/frame/timer and cooperative audit dispatch"},
            {"native_instruction_addresses",f.sites.size()},{"prefix_stores",prefix_stores},{"prefix_store_sha256",prefix_digest},{"return_stores",f.stores},{"return_store_sha256",tail_digest},
            {"abi",f.abi(a)},{"polls",f.polls},{"request_word",0},{"descriptor_table",table},{"original_return_compared",!returned.is_null()},{"supplied_request_acknowledgement",false},
            {"whole_engine_verified",false},{"hardware_clock_verified",false}});
        std::cout<<"PASS original native CPU upload exits its actual wait and returns after real GP DMA consumption; "<<f.sites.size()<<" CPU instruction addresses, "<<f.stores<<" return stores\n";
    }
#endif
    else{
        const auto reference=upload_reference(argv[3]);
        f.expected_before=&reference.at("before");auto a=f.upload(0x388470);
        check(f.abi(a).at("gp")==reference.at("gp_after")&&f.abi(a).at("xmm")==reference.at("xmm_after"),"Full original upload GP/XMM state differs");
        check((a.eflags&0xcc5)==(reference.at("eflags_after").get<unsigned>()&0xcc5),"Full original upload defined flags differ");
        f.compare_memory(reference.at("after"),"Original upload retained bytes differ");
        const auto digest=f.store_digest();check(digest==reference.at("store_sha256").get<std::string>()&&f.stores==reference.at("store_count").get<unsigned>(),"Full original upload ordered stores differ");
        const auto gp=word(f.memory,f.root+8),prefix=word(f.memory,0x391420),table=word(f.memory,f.root+0x20);
        check(word(f.memory,prefix+0x810)==3&&word(f.memory,gp+0x14)!=0&&table!=0&&f.pages->gp_scratch_binding()->entries==201,"Original upload did not retain its real request, data pages and relocated table");
        Json report{{"passed",true},{"original_boundary",0x388470},{"native_instruction_addresses",f.sites.size()},{"ordered_stores",f.stores},{"store_sha256",digest},{"request_word",3},{"descriptor_table",table},{"native_scratch_entries",201},{"supplied_request_acknowledgement",false},{"whole_engine_verified",false}};save(out/"report.json",report);
        std::cout<<"PASS original complete upload prefix and decoder/relocation/publication suffix: "<<f.sites.size()<<" native instruction addresses, "<<f.stores<<" exact ordered stores; request3 retained at original first poll\n";
    }
#if defined(HALO2_TEST_GP_CONSUMER)
    f.expected_before=nullptr;
    if(argc==6&&std::string(argv[4])=="--consume-gp")consume_gp(f,argv[5],out);
#endif
    std::cout<<"LIMIT declared root/upstream resources and processor/peripheral context; physical device clocks, live upload integration, final mix and audiovisual parity remain unverified\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
