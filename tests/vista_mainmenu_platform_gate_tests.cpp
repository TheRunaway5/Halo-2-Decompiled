#include "halo2/vista_mainmenu_platform_gate.hpp"
#include <nlohmann/json.hpp>
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

using nlohmann::json;
namespace {
void require(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
std::string hex(std::span<const std::byte> bytes) {
  constexpr char digits[]="0123456789abcdef";std::string output;
  for(auto byte:bytes) {const auto v=std::to_integer<unsigned>(byte);output+=digits[v>>4];output+=digits[v&15];}
  return output;
}
void assign(std::span<std::byte> bank,const std::string& value) {
  require(value.size()==bank.size()*2,"Fixture bank length differs");
  for(std::size_t i=0;i<bank.size();++i)bank[i]=std::byte(std::stoul(value.substr(i*2,2),nullptr,16));
}
}
int main(int argc,char** argv)try {
  require(argc==2,"Expected original platform gate fixture path");
  std::ifstream input(argv[1]);json fixture;input>>fixture;std::size_t count{};
  for(const auto& row:fixture.at("rows")) {
    const auto& q=row.at("input");
    std::array<std::byte,12> bank;assign(bank,row.at("before").get<std::string>());
    std::array<std::byte,2> pbuffer{std::byte{0xa5},std::byte{0xa5}};
    const auto pointer=row.at("pbuffer_pointer").get<std::uint32_t>();
    json events=json::array();halo2::VistaMainmenuPlatformGateServices s;
    s.allocate_pbuffer=[&](std::uint32_t size,std::uint32_t& output) {
      events.push_back({"allocate",size});require(size==2,"Original PBuffer allocation size differs");
      output=pointer;return 0;
    };
    s.set_pbuffer_byte=[&](std::uint32_t p,std::uint32_t index,std::uint8_t value) {
      require(p==pointer && index<pbuffer.size(),"PBuffer setter received a different owner");
      events.push_back({"set_byte",index,value});pbuffer[index]=std::byte(value);return 0;
    };
    s.initialize_licensing=[&] {events.push_back({"SLDLInitialize"});};
    s.create_event=[&] {
      const auto handle=q.at("event_handle").get<std::uint32_t>();
      events.push_back({"CreateEventA",0,0,0,0,handle});
      if(q.at("event_cancel").get<bool>())bank[4]=std::byte{};
      return handle;
    };
    s.close_event=[&](std::uint32_t handle) {events.push_back({"CloseHandle",handle});};
    s.open_license=[&] {
      const auto hr=q.at("open").get<std::int32_t>();events.push_back({"SLDLOpen",hr});
      return halo2::VistaLicenseOpenResult{hr,q.at("open_handle").get<std::uint32_t>()};
    };
    s.consume_right=[&](std::uint32_t handle,std::span<const std::byte,16> right) {
      require(hex(right)==row.at("right").get<std::string>(),"Original complete licensing GUID bytes differ");
      const auto hr=q.at("consume").get<std::int32_t>();
      events.push_back({"SLDLConsumeRight",hr,handle,0x79bb20,0,0,0});return hr;
    };
    s.query_license_status=[&](std::uint32_t handle) {
      require(handle==q.at("open_handle").get<std::uint32_t>(),"Licensing query consumed another handle");
      const auto hr=q.at("query").get<std::int32_t>();events.push_back({"SLDLGetLicensingStatusInformation",hr});
      return halo2::VistaLicenseStatusResult{hr,std::uint32_t(q.at("statuses").size()),
        q.at("status_pointer").get<bool>()?0x1a00000u:0u};
    };
    s.close_license=[&](std::uint32_t handle) {events.push_back({"SLDLClose",handle});};
    s.read_status=[&](std::uint32_t p,std::uint32_t index) {
      require(p==0x1a00000 && index<q.at("statuses").size(),"Licensing array row read differs");
      return q.at("statuses").at(index).get<std::uint32_t>();
    };
    s.free_status=[&](std::uint32_t p) {events.push_back({"LocalFree",p});};
    const auto licensed=halo2::vista_mainmenu_platform_gate_initialize(bank,s);
    if(hex(bank)!=row.at("owner").get<std::string>() || hex(pbuffer)!=row.at("bytes").get<std::string>() ||
        events!=row.at("events")) {
      std::cerr<<"Input "<<q<<"\nExpected owner "<<row.at("owner")<<" bytes "<<row.at("bytes")
        <<"\nActual owner "<<hex(bank)<<" bytes "<<hex(pbuffer)<<"\nExpected "<<row.at("events")
        <<"\nActual "<<events<<'\n';throw std::runtime_error("Original startup PBuffer producer differs");
    }
    require(licensed==(pbuffer[0]==std::byte{}),"Licensed result differs from original gate byte");
    ++count;
  }
  std::array<std::byte,12> absent{};
  bool rejected=false;
  try {halo2::vista_mainmenu_platform_gate_initialize(absent,{});}catch(const std::invalid_argument&) {rejected=true;}
  require(rejected,"Unavailable native provider must not become a successful platform no-op");
  require(count==647,"Original platform gate fixture count differs");
  std::cout<<"PASS original startup PBuffer/license/event producer cases="<<count<<" complete owner/PBuffer bytes and service order\n";
  return 0;
}catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
