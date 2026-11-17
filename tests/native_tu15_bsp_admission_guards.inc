// Compiled actual raw/prepared owner fallback checks; no supplied source outcomes.
unsigned guards(bool forced){
    unsigned count{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u},changed{0x38846eu,0x167efbu,0x33b595u};constexpr std::array extra{0x38846eu,0x21f590u,0x33b595u,0x100u};
    for(unsigned route=0;route<4;++route)for(unsigned policy=0;policy<(forced?1u:25u);++policy){
        Memory memory;unsigned callbacks{},stores{};bool first{},second{};SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto){++callbacks;};services.instruction_cursor={&memory.pc,&first,&second,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
        SourcePhysicsContactRecordObserver observer;if(policy==18||policy==19)observer=[&](const auto&){++stores;};
        InstrumentedState s{memory,observer,services,!forced&&policy==0?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};
        SourcePhysicsWorldRuntimeAbi input{};input.gp={1,2,3,4,0x62001000,6,7,8};input.eflags=0x246;input.floating_point=SourcePhysicsFactoryFloatingPoint{0x37f,0x1f80};s.load(input);
        if(!forced){
            if(policy==1){s.active_mmx=SourcePhysicsMmxAlias{};s.fp.count=1;}if(policy==2)s.fp.count=9;if(policy==24)s.fp.count=4;if(policy==3)s.fp.top=8;
            if(policy==4){const std::uint16_t cw=0x37e;asm volatile("fldcw %0"::"m"(cw));}if(policy==5)_mm_setcsr(0x1f00);
            if(policy==6||policy==19)services.instruction={};
            if(policy==7){const auto old=services.instruction;services.instruction=[old](auto pc){old(pc);};}
            if(policy==8)services.instruction_cursor.executor={};if(policy==9)services.instruction_cursor.callback_type=nullptr;
            if(policy==10)services.instruction_cursor.callback_type=&typeid(int);if(policy==11)services.instruction_cursor.position=nullptr;
            if(policy==12)s.instruction_cursor=nullptr;if(policy==13)services.instruction_cursor.literal_service_sites=false;
            if(policy==14)services.instruction_cursor.service_sites={};if(policy==15)services.instruction_cursor.service_sites=changed;
            if(policy==16)first=true;if(policy==17)second=true;if(policy==18)services.instruction_cursor.observation_only_stores=false;
            if(policy==20)s.selected_observation_pcs=false;if(policy==21)s.fp.status=0x80;if(policy==22)s.fp.status=0x8000;
            if(policy==23)services.instruction_cursor.service_sites=extra;
        }
        const auto gp=s.gp;const auto flags=s.flags.bits();const auto depth=s.fp.count,top=s.fp.top;const auto status=s.fp.status;const auto host=native_raw_x87_detail::capture();std::array<std::byte,128>xmm{};std::memcpy(xmm.data(),s.xmm,128);memory.pc=0xdeadbeefu;
        retained_statements=0;stop_at_retained=true;bool stopped{};try{if(route==0)copies::bound::run_136ea0(s);else if(route==1)copies::prepared::run_136ea0(s);else if(route==2)copies::other_bound::run_136ea0(s);else copies::other_prepared::run_136ea0(s);}catch(const RetainedStop&){stopped=true;}stop_at_retained=false;
        check(stopped&&retained_statements==1,"Unqualified policy skipped untouched first retained statement");
        check(s.gp==gp&&s.flags.bits()==flags&&s.fp.count==depth&&s.fp.top==top&&s.fp.status==status&&!std::memcmp(xmm.data(),s.xmm,128),"Admission fallback changed CPU state");
        check(memory.pc==0xdeadbeefu&&!callbacks&&!stores&&memory.reads.empty()&&memory.writes.empty()&&native_raw_x87_detail::equivalent(host,native_raw_x87_detail::capture()),"Admission fallback executed source I/O/callbacks");++count;
        asm volatile("fninit":::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(0x1f80);
    }
    return count;
}
