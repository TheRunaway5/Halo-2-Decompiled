#include "halo2/native_tu15_input_devices.hpp"
#include "halo2/source_input_startup_tu15.hpp"
#include "halo2/source_runtime_image.hpp"
#include <SDL.h>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>

namespace {
using namespace halo2;using J=nlohmann::json;using M=SourceRuntimeMemory;using A=SourcePhysicsWorldRuntimeAbi;
void check(bool value,const std::string& why){if(!value)throw std::runtime_error(why);}
void put(M& m,std::uint32_t p,std::uint32_t value) {
    std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(value>>(i*8));m.write(p,b);
}
std::uint32_t word(const M& m,std::uint32_t p) {
    auto b=m.view(p,4);std::uint32_t value=0;for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(b[i])<<(i*8);return value;
}
std::string hex(std::span<const std::byte>b) {
    constexpr char digits[]="0123456789abcdef";std::string out;
    for(auto v:b){const auto u=std::to_integer<unsigned>(v);out.push_back(digits[u>>4]);out.push_back(digits[u&15]);}return out;
}
J cpu(const A& a) {
    J xmm=J::array();for(const auto& lane:a.xmm)xmm.push_back(hex(std::as_bytes(std::span(lane))));
    return {{"gp",a.gp},{"eflags",a.eflags},{"xmm",xmm},{"x87_control",a.floating_point->x87_control_word},
        {"x87_status",a.x87_status},{"mxcsr",a.floating_point->mxcsr},{"x87_depth",a.x87_depth}};
}
J call_json(const NativeTu15InputCall& c) {
    J writes=J::array();for(const auto& w:c.writes)writes.push_back({{"address",w.address},{"bytes",hex(w.bytes)}});
    return {{"instruction",c.instruction},{"target",c.target},{"entry_esp",c.entry_esp},{"return_esp",c.return_esp},
        {"return_eax",c.return_eax},{"arguments",c.arguments},{"writes",writes}};
}
struct Pads {
    struct Pad {SDL_Joystick* joystick=nullptr;SDL_JoystickID id=-1;unsigned rumble_calls=0;Uint16 left=0,right=0;int rumble_success=0;};
    std::array<Pad,4> pads{};
    Pads() {check(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)==0,SDL_GetError());}
    static int SDLCALL rumble(void* data,Uint16 left,Uint16 right) {
        auto& p=*static_cast<Pad*>(data);++p.rumble_calls;p.left=left;p.right=right;return p.rumble_success;
    }
    void attach() {
        for(auto& p:pads) {
            // This exact installed SDL2-compat revision forwards the SDL2 int
            // callback as an SDL3 bool without conversion (upstream source
            // release-2.32.74, VirtualJoystick_Rumble, lines11174-11178).
            // Adapt only this virtual test fixture; production still requires
            // an actual successful SDL_GameControllerRumble return value.
            if(std::string_view(SDL_GetRevision())=="SDL-release-2.32.74-0-ge4df8a5")p.rumble_success=1;
            SDL_VirtualJoystickDesc d{};d.version=SDL_VIRTUAL_JOYSTICK_DESC_VERSION;d.type=SDL_JOYSTICK_TYPE_GAMECONTROLLER;
            d.naxes=SDL_CONTROLLER_AXIS_MAX;d.nbuttons=SDL_CONTROLLER_BUTTON_MAX;d.name="TU1.5 native input source comparison";
            d.userdata=&p;d.Rumble=rumble;
            const auto index=SDL_JoystickAttachVirtualEx(&d);check(index>=0,SDL_GetError());
            p.joystick=SDL_JoystickOpen(index);check(p.joystick,SDL_GetError());p.id=SDL_JoystickInstanceID(p.joystick);
            check(SDL_IsGameController(index),"SDL virtual device lacks native game controller mapping");
            for(int a=0;a<SDL_CONTROLLER_AXIS_MAX;++a)check(SDL_JoystickSetVirtualAxis(p.joystick,a,a>=4?-32768:0)==0,SDL_GetError());
        }
        SDL_GameControllerUpdate();
    }
    void detach(unsigned at) {
        auto& p=pads[at];if(p.joystick){SDL_JoystickClose(p.joystick);p.joystick=nullptr;}
        for(int i=0;i<SDL_NumJoysticks();++i)if(SDL_JoystickGetDeviceInstanceID(i)==p.id){check(SDL_JoystickDetachVirtual(i)==0,SDL_GetError());break;}
    }
    void sample(unsigned sample) {
        constexpr std::array<std::array<Sint16,6>,4> values{{
            {32767,-32768,-32768,32767,-32768,32767},
            {9000,-9000,9001,-9001,0,16384},
            {-9001,9001,-32768,-32768,32767,-32768},
            {0,0,0,0,-32768,-32768}}};
        for(unsigned p=0;p<4;++p)if(pads[p].joystick) {
            for(int a=0;a<6;++a)check(SDL_JoystickSetVirtualAxis(pads[p].joystick,a,values[(sample+p)%4][a])==0,SDL_GetError());
            for(int b=0;b<SDL_CONTROLLER_BUTTON_MAX;++b)check(SDL_JoystickSetVirtualButton(pads[p].joystick,b,
                sample==3?0:((b+p+sample)%3==0))==0,SDL_GetError());
        }
        SDL_GameControllerUpdate();
    }
    ~Pads(){for(unsigned i=0;i<4;++i)detach(i);SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);}
};
J run_case(bool virtual_devices,bool host,const J& expected,const std::filesystem::path& reference) {
    Pads pads;if(virtual_devices)pads.attach();
    NativeTu15InputSelection selection;selection.restricted=!host;
    if(virtual_devices)for(const auto& p:pads.pads)selection.instances.push_back(p.id);
    M m;mount_native_tu15_input_startup_data(m);
    if(!reference.empty()) {
        auto image=load_source_runtime_image(reference,SourcePhysicsContactImage::TitleUpdate15);
        m.visit_committed([&](auto p,auto b){check(std::ranges::equal(b,image.view(p,b.size())),"Typed input declarations differ from authored TU1.5 data");});
    }
    m.mount(0x60000000,std::vector<std::byte>(0x10000),M::Access::ReadWrite,"native input executor stack");
    m.mount(0x4f3218,std::array<std::byte,4>{},M::Access::ReadOnly,"source game options binding before game startup");
    m.mount(0x60212000,std::array<std::byte,4>{},M::Access::ReadWrite,"owned input monotonic clock cell");
    constexpr std::array<std::byte,4> tick_binding{std::byte{0},std::byte{0x20},std::byte{0x21},std::byte{0x60}};
    m.mount(0x41b774,tick_binding,M::Access::ReadOnly,"native input imported monotonic clock binding");
    NativeTu15InputDevices owner(m,selection);J report={{"mode",host?"host":virtual_devices?"four-sdl-virtual":"empty-selected"},{"phases",J::array()},
        {"sdl_revision",SDL_GetRevision()},{"virtual_rumble_callback_success",virtual_devices?pads.pads[0].rumble_success:0}};
    J stores=J::array();std::vector<std::uint32_t> sites,ticks;std::size_t tick_index=0;
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto p) {
        if(p==0x33b595u) {
            const auto tick=expected.is_null()?std::uint32_t(SDL_GetTicks64()):expected.at("ticks").at(tick_index++).get<std::uint32_t>();
            put(m,0x60212000,tick);ticks.push_back(tick);
        }
        sites.push_back(p);
    };
    services.required_owner=[&](auto& bank,const auto& call,auto& abi,const auto& observe) {
        if(owner.try_call(bank,call,abi))return;
        if(source_input_startup_tu15_owns_entry(call.target)) {source_run_input_startup_tu15(bank,call.target,abi,{0x27f,0x1f80},services,observe);return;}
        throw std::runtime_error("Unowned source input call "+std::to_string(call.target));
    };
    const auto observe=[&](const SourcePhysicsContactRecordStore& s) {
        if(s.address>=0x60000000u && s.address<0x60010000u)return;
        stores.push_back({{"instruction",s.instruction},{"address",s.address},{"before",hex(s.before)},{"written",hex(s.written)}});
    };
    const auto run=[&](std::uint32_t entry,const std::string& name) {
        sites.clear();stores=J::array();const auto first_call=owner.calls().size();
        A a{};a.gp[4]=0x6000f000;a.eflags=2;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};put(m,a.gp[4],0x60200000);
        source_run_input_startup_tu15(m,entry,a,{0x27f,0x1f80},services,observe);
        check(a.gp[4]==0x6000f004,"Complete input source routine did not return to its real caller");
        J banks=J::array(),calls=J::array();M declarations;mount_native_tu15_input_startup_data(declarations);
        declarations.visit_committed([&](auto p,auto b){banks.push_back({{"address",p},{"bytes",hex(m.view(p,b.size()))}});});
        for(const auto& c:owner.calls().subspan(first_call))calls.push_back(call_json(c));
        J row={{"name",name},{"entry",entry},{"cpu",cpu(a)},{"instructions",sites},{"stores",stores},{"banks",banks},{"native_calls",calls}};
        if(!expected.is_null()) {
            const auto& original=expected.at("phases").at(report["phases"].size());
            for(auto key:{"name","entry","instructions","stores","banks","native_calls"})check(row[key]==original.at(key),std::string("Input original comparison differs: ")+name+" "+key);
            auto original_cpu=original.at("cpu");auto native_cpu=row["cpu"];
            const auto difference=native_cpu.at("eflags").get<unsigned>()^original_cpu.at("eflags").get<unsigned>();
            check(!(difference&0xcc5u),"Input original defined arithmetic flags differ");
            native_cpu.erase("eflags");original_cpu.erase("eflags");check(native_cpu==original_cpu,"Input original GP/XMM/FP state differs");
        }
        report["phases"].push_back(std::move(row));
    };
    run(0x1273c0,"initialize");
    if(virtual_devices) {
        check(owner.live_controllers()==4,"Actual source initialization did not open all four selected SDL devices");
        check(std::ranges::count_if(owner.calls(),[](const auto& c){return c.target==0x413cbfu && !c.return_eax;})==4,
            "Source initialization did not submit all four zero-rumble requests");
        for(unsigned i=0;i<4;++i)check(word(m,0x4f2a9c+i*4)!=0 && !pads.pads[i].left && !pads.pads[i].right,
            "Source handle publication or actual zero-rumble transport missing");
        for(unsigned sample=0;sample<4;++sample){pads.sample(sample);run(0x127460,"sample-"+std::to_string(sample));}
        run(0x127460,"unchanged-sample");
        pads.detach(1);run(0x127460,"remove-port-one");
        check(owner.live_controllers()==3 && word(m,0x4f2aa0)==0,"Actual removal did not close and clear the source binding");
    }
    const auto addresses=owner.native_controller_addresses();
    for(auto p:addresses)check(sizeof(void*)==8 && p>0xffffffffu,"Native input controller pointer narrowed to the source handle domain");
    report["native_controller_addresses"]=addresses;report["native_controller_address_bits"]=sizeof(void*)*8;
    run(0x21e5d0,"preset-constructor");
    for(unsigned i=0;i<4;++i)check(m.view(0x52d65c+i,1)[0]==std::byte(word(m,0x4f2a9c+i*4)?1:0),"Source input preset presence differs from live native handle publication");
    check(m.view(0x52d660,1)[0]==std::byte{1},"Source preset constructor did not publish completion");
    if(virtual_devices) {
        // Exercise the native motor boundary independently of the source
        // pre-game suppression path. Restore these test-owned inputs before
        // the next original routine; no gameplay motor result is supplied.
        const auto saved_feedback=m.view(0x4f2970,0x46);const std::vector<std::byte> saved(saved_feedback.begin(),saved_feedback.end());
        constexpr std::uint32_t test_sp=0x6000e000;A motors{};motors.gp[4]=test_sp;
        put(m,test_sp,0x127a81);put(m,test_sp+4,word(m,0x4f2a9c));put(m,test_sp+8,0x4f2970);
        put(m,0x4f29b2,32123u|(12345u<<16));
        check(owner.try_call(m,{0x127a7c,0x413cbf,false},motors) && !motors.gp[0] && pads.pads[0].left==32123 && pads.pads[0].right==12345,
            "Native input positive rumble did not reach the actual SDL virtual motor callback");
        put(m,0x4f29b2,0);motors.gp[4]=test_sp;
        check(owner.try_call(m,{0x127a7c,0x413cbf,false},motors) && !pads.pads[0].left && !pads.pads[0].right,"Native input motors did not stop");
        m.write(0x4f2970,saved);report["actual_native_positive_motor_transport_verified"]=true;
    }
    run(0x127430,"close");check(owner.live_controllers()==0,"Input teardown retained native resources");
    for(unsigned i=0;i<4;++i)check(word(m,0x4f2a9c+i*4)==0,"Source input teardown retained a published binding");
    // Rejected boundary calls must neither mutate ABI/bank nor touch devices.
    const auto found=std::ranges::find_if(owner.calls(),[](const auto& c){return c.instruction==0x1275cbu;});check(found!=owner.calls().end(),"No source device-change call");
    const auto saved=*found;A probe{};probe.gp[4]=saved.entry_esp;probe.eflags=0x246;put(m,probe.gp[4],saved.instruction+5);
    for(unsigned i=0;i<saved.arguments.size();++i)put(m,probe.gp[4]+4+i*4,saved.arguments[i]);
    const auto before=probe;const auto count=owner.calls().size();bool bank_rejected=false,thread_rejected=false,frame_rejected=false;
    M foreign;
    try{owner.try_call(foreign,{saved.instruction,saved.target,false},probe);}catch(const std::invalid_argument&){bank_rejected=true;}
    std::thread other([&]{try{owner.try_call(m,{saved.instruction,saved.target,false},probe);}catch(const std::invalid_argument&){thread_rejected=true;}});other.join();
    put(m,probe.gp[4],0xbad0bad0);
    try{owner.try_call(m,{saved.instruction,saved.target,false},probe);}catch(const std::invalid_argument&){frame_rejected=true;}
    check(bank_rejected && thread_rejected && frame_rejected && probe.gp==before.gp && probe.eflags==before.eflags && owner.calls().size()==count,
        "Input owner admitted a foreign bank/executor/return frame or mutated a rejected call");
    put(m,probe.gp[4],saved.instruction+5);
    for(unsigned fault=0;fault<3;++fault) {
        auto invalid=SourcePhysicsContactRuntimeCall{saved.instruction,saved.target,false};
        if(fault==0)++invalid.instruction;if(fault==1)invalid.tail=true;if(fault==2)put(m,probe.gp[4]+4,0xdeadbeef);
        const auto bank_before=hex(m.view(0x60000000,0x10000));bool rejected=false;
        try{owner.try_call(m,invalid,probe);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected && probe.gp==before.gp && probe.eflags==before.eflags && owner.calls().size()==count &&
            hex(m.view(0x60000000,0x10000))==bank_before,"Input owner admitted a foreign site/tail/type or changed rejected outputs");
    }
    report["ticks"]=ticks;report["foreign_bank_thread_frame_rejected"]=true;report["foreign_site_tail_type_rejected"]=true;report["closed_live_controllers"]=owner.live_controllers();
    if(!expected.is_null())check(tick_index==expected.at("ticks").size() && report["phases"].size()==expected.at("phases").size(),"Input original clock/phase receipt not fully consumed");
    return report;
}
}
int main(int argc,char** argv)try {
    std::filesystem::path capture,fixture,reference;bool host=false;
    for(int i=1;i<argc;i+=2) {
        check(i+1<argc,"Missing input startup test argument");const std::string_view option=argv[i];
        if(option=="--capture")capture=argv[i+1];else if(option=="--original-fixture")fixture=argv[i+1];
        else if(option=="--reference-xbe")reference=argv[i+1];else if(option=="--devices") {check(std::string_view(argv[i+1])=="host","Unknown input device test mode");host=true;}
        else throw std::runtime_error("Unknown input startup test option");
    }
    check(fixture.empty()==reference.empty() && !(host && !fixture.empty()),"Original input fixture/reference arguments differ");
    if(!fixture.empty() && (!std::filesystem::is_regular_file(fixture) || !std::filesystem::is_regular_file(reference)))return 77;
    J expected;if(!fixture.empty()) {std::ifstream in(fixture);expected=J::parse(in);check(expected.at("schema")=="H2_NATIVE_TU15_INPUT_ORIGINAL_V1" &&
        expected.at("source_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" && expected.at("instruction_patches").empty() &&
        expected.at("supplied_gameplay_results").empty(),"Input comparison is not unchanged TU1.5 execution");}
    J report={{"schema","H2_NATIVE_TU15_INPUT_NATIVE_V1"},{"cases",J::array()},{"runtime_active",false},{"execution_abi_bits",32}};
    const auto original_case=[&](unsigned i)->J{return expected.is_null()?J():expected.at("cases").at(i);};
    report["cases"].push_back(run_case(!host,host,original_case(0),reference));
    if(!host)report["cases"].push_back(run_case(false,false,original_case(1),reference));
    report["original_execution_compared"]=!expected.is_null();
    if(!capture.empty()){std::ofstream out(capture);out<<report.dump(2)<<'\n';}
    std::cout<<"Native input initialization, state/presets, removal and teardown passed"<<(!expected.is_null()?" with original TU1.5 comparison":"")<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
