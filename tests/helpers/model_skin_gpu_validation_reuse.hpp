#pragma once
// Test-private matrix extracted from the qualified retained-input trial.
// Include after the original GPU fixture types/oracle in model_skin_gpu_tests.cpp.
namespace {
constexpr const char *finite_error="model skin GPU palette leaves its finite arithmetic domain";
constexpr const char *normal_error="model skin GPU referenced palette leaves its normal arithmetic domain";
struct ValidationEvidence {
    std::uint64_t calls{},visits{},hits{},gpu_jobs{},complete_vertices{},hard{},refusals{},empties{};
    std::vector<std::string> semantic_events;
};
ValidationEvidence validation_cases(Gpu &gpu,bool expect_full) {
    ValidationEvidence evidence;
    struct Restore {std::fenv_t env{};int error=errno;
        Restore(){need(std::fegetenv(&env)==0,"Private FP owner capture failed");}
        ~Restore(){if(std::fesetenv(&env)!=0)std::terminate();errno=error;}
    } restore;
    ModelSkinGpu skin(gpu.device,gpu.physical,VK_API_VERSION_1_2,HALO2_MODEL_SKIN_GPU_SHADER);
    std::vector<Vertex> input(12);
    for(std::size_t i=0;i<input.size()*sizeof(Vertex);++i)
        std::as_writable_bytes(std::span{input})[i]=std::byte((i*29+73)%256);
    for(auto &vertex:input)for(auto *value:{&vertex.position,&vertex.normal,&vertex.tangent,&vertex.binormal})
        *value={-0.f,2.f,-1.f};
    Buffer destination(gpu,input.size()*sizeof(Vertex)),other(gpu,input.size()*sizeof(Vertex));
    std::uint64_t generation=7;
    std::vector<std::array<float,12>> palette{
        {1,0,0,1,0,1,0,2,0,0,1,3}, {0,1,0,4,1,0,0,5,0,0,-1,6},
        {1,0,0,-1,0,-1,0,-2,0,0,1,-3}, {1,0,0,7,0,1,0,8,0,0,1,9}};
    std::vector<ModelSkinGpuRecord> records(3);
    records[0].destination=1;records[0].palette_count=2;records[0].weights={1,0,0,0};
    records[0].bones={0,65535,65535,65535};
    records[1].destination=4;records[1].palette=1;records[1].palette_count=2;
    records[1].weights={.5f,-0.f,.5f,0};records[1].bones={0,1,0,65535};
    records[2].destination=7;records[2].post_identity_placement=1;
    records[2].weights={0,-0.f,0,-0.f};records[2].bones.fill(0xffffffffu);
    const auto preserve=[&](auto &&action) {
#if defined(__GLIBC__) && defined(__x86_64__)
        // Same declared stable metadata baseline as the unchanged original
        // service tests. Every control/status/tag/MXCSR bit remains intact.
        std::fenv_t stable{};need(std::fegetenv(&stable)==0,"Private FP baseline capture failed");
        stable.__eip=0;stable.__cs_selector=0;stable.__opcode=0;
        stable.__data_offset=0;stable.__data_selector=0;
        need(std::fesetenv(&stable)==0,"Private FP baseline restore failed");
#endif
        errno=EDOM;std::fenv_t before{},after{};
        need(std::fegetenv(&before)==0,"Private before FP capture failed");action();const int error=errno;
        need(std::fegetenv(&after)==0,"Private after FP capture failed");
        need(error==EDOM && !std::memcmp(&before,&after,sizeof(before)),"Private validation changed complete FP environment or errno");
    };
    const auto collect=[&] {const auto s=skin.prepare_stats();++evidence.calls;
        evidence.visits+=s.record_validation_visits;evidence.hits+=s.record_validation_cache_hits;return s;};
    const auto work=[&](const auto &stats,std::size_t count,bool eligible) {
        if(eligible && !expect_full)
            need(stats.record_validation_visits==0 && stats.record_validation_cache_hits==1,"Private validation work gate: expected actual reuse");
        else need(stats.record_validation_visits==count && !stats.record_validation_cache_hits,"Private validation work gate: expected full record validation");
    };
    const auto execute=[&] {
        checked(vkResetCommandBuffer(gpu.command,0));VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        checked(vkBeginCommandBuffer(gpu.command,&begin));skin.record(gpu.command);checked(vkEndCommandBuffer(gpu.command));
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&gpu.command;
        checked(vkQueueSubmit(gpu.queue,1,&submit,{}));checked(vkQueueWaitIdle(gpu.queue));
    };
    const auto valid=[&](const auto &job,const auto &rows,std::size_t extent,bool reuse) {
        auto expected=input;for(const auto &record:job)materialize(expected[record.destination],record,rows);
        std::memcpy(destination.mapped,input.data(),input.size()*sizeof(Vertex));
        preserve([&]{skin.prepare(job,rows,destination.buffer,extent,generation);});
        need(skin.vertices()==job.size(),"Private validation omitted GPU work");work(collect(),job.size(),reuse);
        execute();need(!std::memcmp(destination.mapped,expected.data(),input.size()*sizeof(Vertex)),"Private validation GPU complete bytes differ");
        std::uint64_t hash=1469598103934665603ull;for(auto byte:std::as_bytes(std::span{expected})){hash^=std::to_integer<unsigned>(byte);hash*=1099511628211ull;}
        evidence.semantic_events.push_back("ok:"+std::to_string(job.size())+":"+std::to_string(hash));
        ++evidence.gpu_jobs;evidence.complete_vertices+=input.size();
    };
    const auto prime=[&] {valid(records,palette,input.size(),false);valid(records,palette,input.size(),true);};
    const auto hard=[&](const auto &job,const auto &rows,std::size_t extent,const char *message) {
        std::string actual;
        std::memcpy(destination.mapped,input.data(),input.size()*sizeof(Vertex));
        preserve([&]{try{skin.prepare(job,rows,destination.buffer,extent,generation);}catch(const FormatError &error){actual=error.what();}});
        need(actual==message && !skin.vertices(),"Private validation changed exact programming error or retained work");
        collect();execute();need(!std::memcmp(destination.mapped,input.data(),input.size()*sizeof(Vertex)),"Private malformed job dispatched stale records");
        evidence.semantic_events.push_back(actual);++evidence.hard;
    };
    valid(records,palette,input.size(),false);valid(records,palette,input.size(),true);
    auto relocated=records;valid(relocated,palette,input.size(),true);
    palette[0][3]=11;valid(records,palette,input.size(),true);
    auto larger=std::vector<ModelSkinGpuRecord>(8,records[0]);
    for(unsigned i=0;i<larger.size();++i)larger[i].destination=i;
    {
        PrepareFailureControl control("map");bool refused=false;
        preserve([&]{try{skin.prepare(larger,palette,destination.buffer,input.size(),generation);}catch(const ModelSkinGpuPrepareRefused &e){
            refused=e.reason()==ModelSkinGpuPrepareRefusalReason::input_mapping && e.result()==VK_ERROR_OUT_OF_HOST_MEMORY;}});
        const auto failed=collect();
        need(refused && !skin.vertices() && failed.record_validation_visits==larger.size() &&
             !failed.record_validation_cache_hits && failed.input_allocations==1 && !failed.input_maps,
             "Private failed growth changed refusal/actual traffic or reused a stale proof");
        evidence.semantic_events.push_back("growth-map-refusal");++evidence.refusals;
    }
    prime();valid(larger,palette,input.size(),false);valid(larger,palette,input.size(),true);prime();
    auto shorter_owner=records;shorter_owner[0].palette_count=1;
    valid(shorter_owner,palette,input.size(),false);valid(shorter_owner,palette,input.size(),true);prime();
    // Every case starts with a successful warm proof, but the first two
    // failures must occur before any record validation or proof reuse.
    for(unsigned phase=0;phase<3;++phase) {
        if(phase)prime();else valid(records,palette,input.size(),true);
        auto bad_records=records;bad_records.back().post_identity_placement=2;
        auto bad_palette=palette;
        if(phase<2)bad_palette.back().back()=std::bit_cast<float>(0x7f800001u);
        const auto buffer=phase?destination.buffer:VK_NULL_HANDLE;
        const char *expected=phase==0?"model skin GPU job has no destination buffer":
            phase==1?finite_error:"model skin GPU has an unknown post-placement mode";
        std::string actual;
        preserve([&]{try{skin.prepare(bad_records,bad_palette,buffer,input.size(),generation);}
            catch(const FormatError &error){actual=error.what();}});
        const auto stats=collect();
        need(actual==expected && !skin.vertices(),"Private warm mixed-invalid precedence changed");
        need(stats.record_validation_visits==(phase==2?records.size():0) &&
             !stats.record_validation_cache_hits,"Private pre-record failure performed record work");
        evidence.semantic_events.push_back(std::string("precedence:")+actual);++evidence.hard;
    }
    prime();
    // Byte-identical topology cannot retain a stale palette normal proof.
    for(const auto row:{0u,2u}) {
        const auto original=palette[row][0];palette[row][0]=std::bit_cast<float>(1u);
        hard(records,palette,input.size(),normal_error);palette[row][0]=original;prime();
    }
    palette[3][0]=std::bit_cast<float>(1u);valid(records,palette,input.size(),true);
    for(auto bits:{0x7f800001u,0x7f800000u,0x49800001u}) {
        palette[3][0]=std::bit_cast<float>(bits);hard(records,palette,input.size(),finite_error);
        palette[3][0]=0;prime();
    }
    const auto changed=[&](auto mutate,const char *message) {auto job=records;mutate(job);hard(job,palette,input.size(),message);prime();};
    changed([](auto &r){r.back().weights[3]=std::bit_cast<float>(0x7f800001u);},"model skin GPU weight leaves its finite arithmetic domain");
    changed([](auto &r){r[1].palette_count=0;},"model skin GPU bone escapes its completed palette");
    changed([](auto &r){r[1].palette_count=4;},"model skin GPU palette base escapes its bank");
    changed([](auto &r){r.back().post_identity_placement=2;},"model skin GPU has an unknown post-placement mode");
    changed([](auto &r){r.back().destination=r.front().destination;},"model skin GPU destinations escape or alias different jobs");
    auto unordered=records;std::swap(unordered[0].destination,unordered[1].destination);
    valid(unordered,palette,input.size(),false);valid(unordered,palette,input.size(),true);prime();
    auto weight=records;weight[0].weights[0]=-1;valid(weight,palette,input.size(),false);valid(weight,palette,input.size(),true);prime();
    valid(records,palette,input.size()-1,false);valid(records,palette,input.size()-1,true);
    hard(records,palette,records.back().destination,"model skin GPU destinations escape or alias different jobs");prime();
    palette.push_back({});valid(records,palette,input.size(),false);valid(records,palette,input.size(),true);
    palette.pop_back();valid(records,palette,input.size(),false);valid(records,palette,input.size(),true);
    for(const unsigned rows:{128u,4095u,4096u,4097u}) {
        std::vector<std::array<float,12>> bank(rows);for(auto &r:bank)r={1,0,0,1,0,1,0,2,0,0,1,3};
        for(const unsigned selected:rows==128?std::vector<unsigned>{0,63,64}:std::vector<unsigned>{rows-1}) {
            auto job=records;for(auto &r:job){r.palette=selected;r.palette_count=1;r.bones.fill(0);r.weights={1,0,0,0};}
            valid(job,bank,input.size(),false);valid(job,bank,input.size(),rows<=4096);
            bank[selected][11]=std::bit_cast<float>(1u);hard(job,bank,input.size(),normal_error);bank[selected][11]=3;
        }
    }
    prime();
    struct Failure{const char *name;ModelSkinGpuPrepareRefusalReason reason;VkResult result;};
    using R=ModelSkinGpuPrepareRefusalReason;
    const Failure failures[]={{"quota",R::device_quota,VK_SUCCESS},{"packing",R::input_allocation,VK_SUCCESS},
        {"create",R::input_allocation,VK_ERROR_OUT_OF_HOST_MEMORY},{"allocation",R::input_allocation,VK_ERROR_OUT_OF_HOST_MEMORY},
        {"device-allocation",R::input_allocation,VK_ERROR_OUT_OF_DEVICE_MEMORY},{"map",R::input_mapping,VK_ERROR_OUT_OF_HOST_MEMORY},
        {"map-range",R::input_mapping,VK_ERROR_MEMORY_MAP_FAILED}};
    for(const auto &f:failures) {
        valid(records,palette,input.size(),true);
        {
            PrepareFailureControl control(f.name);
            palette[2][0]=std::bit_cast<float>(1u);hard(records,palette,input.size(),normal_error);palette[2][0]=1;
        }
        prime();
        {
            PrepareFailureControl control(f.name);bool refused=false;
            preserve([&]{try{skin.prepare(records,palette,destination.buffer,input.size(),generation);}catch(const ModelSkinGpuPrepareRefused &e){refused=e.reason()==f.reason && e.result()==f.result;}});
            need(refused && !skin.vertices(),"Private validation bypassed typed warm refusal");work(collect(),records.size(),true);
            evidence.semantic_events.push_back(std::string("refusal:")+f.name);++evidence.refusals;
        }
        prime();
    }
    for(const char *name:{"device-lost","unknown-mode"}) {
        PrepareFailureControl control(name);std::string actual;
        preserve([&]{try{skin.prepare(records,palette,destination.buffer,input.size(),generation);}catch(const FormatError &e){actual=e.what();}});
        need(!actual.empty() && !skin.vertices(),"Private validation swallowed hard injected failure");work(collect(),records.size(),true);
        evidence.semantic_events.push_back(actual);++evidence.hard;
        // Restore the environment before rebuilding the proof next iteration.
        PrepareFailureControl clear(nullptr);prime();
    }
    preserve([&]{skin.prepare({}, {},VK_NULL_HANDLE,0);});const auto empty=collect();
    need(!skin.vertices() && !empty.record_validation_visits && !empty.record_validation_cache_hits &&
         !empty.record_cache_hits && !empty.palette_upload_bytes,"Private empty reset performed validation/work");
    ++evidence.empties;valid(records,palette,input.size(),true);
    ++generation;valid(records,palette,input.size(),true);need(skin.prepare_stats().descriptor_writes==2,"Lease replacement did not refresh descriptors");
    generation=0;valid(records,palette,input.size(),true);need(skin.prepare_stats().descriptor_writes==2,"Unqualified destination suppressed descriptor refresh");
    constexpr int rounds[]{FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
    for(unsigned denorm=0;denorm<4;++denorm)for(unsigned r=0;r<4;++r)for(unsigned sticky:{0u,1u,2u,4u,8u,16u,32u,63u}) {
        need(std::fesetround(rounds[(r+1)%4])==0,"Private x87 rounding failed");
        _mm_setcsr(0x1f80u|(r<<13)|sticky|((denorm&1)<<6)|((denorm>>1)<<15));
        preserve([&]{skin.prepare(records,palette,destination.buffer,input.size(),generation);});work(collect(),records.size(),true);
    }
    return evidence;
}
}
