#include "halo2/unit_scope_camera.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::uint32_t hex(const std::string &s) {return std::stoul(s,nullptr,16);}
float real(const std::string &s) {return std::bit_cast<float>(hex(s));}
void check(bool ok,const std::string &s) {if(!ok)throw std::runtime_error(s);}
void check_float(float value,const std::string &expected,const std::string &context) {
  if(std::bit_cast<std::uint32_t>(value)!=hex(expected)) {
    std::ostringstream error;
    error<<context<<": expected "<<expected<<", received "<<std::hex<<std::bit_cast<std::uint32_t>(value);
    throw std::runtime_error(error.str());
  }
}
void packet(const halo2::UnitScopeCameraPacket &p,const std::vector<std::string> &r,
            std::size_t n,const std::string &where) {
  check(p.flags==hex(r[n]),where+" packet flags");
  check_float(p.fov,r[n+1],where+" packet FOV");
  check_float(p.global_seconds,r[n+2],where+" packet global seconds");
  check(p.fov_flags==std::stoi(r[n+3]),where+" packet group4 flags");
  check_float(p.fov_seconds,r[n+4],where+" packet group4 seconds");
}
void state(const halo2::UnitScopeCameraState &s,const std::vector<std::string> &r,
           std::size_t n,const std::string &where) {
  check(s.registered==bool(std::stoi(r[n])),where+" registered");
  if(s.registered)check(s.registered_flags==hex(r[n+1]),where+" registered packet flags");
  check(s.flags==hex(r[n+2]),where+" adopted packet flags");
  check(s.fov_flags==std::stoi(r[n+3]),where+" adopted group flags");
  const float values[]={s.current,s.target,s.remaining,s.velocity,s.acceleration,s.delta};
  const char *labels[]={"current","target","remaining","velocity","acceleration","delta"};
  for(unsigned i=0;i<6;++i)check_float(values[i],r[n+4+i],where+" "+labels[i]);
  for(unsigned i=0;i<6;++i)check_float(s.coefficients[i],r[n+10+i],where+" coefficient "+std::to_string(i));
  check_float(s.rendered_fov,r[n+16],where+" render FOV");
}
}
int main(int argc,char **argv) {
 try {
  if(argc!=2)throw std::runtime_error("Supply frozen original camera fixture export");
  std::ifstream input(argv[1]);if(!input)return 77;
  halo2::UnitScopeCameraConstants c{};
  halo2::UnitScopeCameraState s{};
  halo2::UnitScopeCameraProducer producer{};
  halo2::UnitScopeCameraPacket p{};
  float dt=0.0f;
  unsigned frames=0,states=0,leaves=0,initial=0;
  for(std::string line;std::getline(input,line);) {
   std::istringstream in(line);std::vector<std::string> r;
   for(std::string v;in>>v;)r.push_back(v);
   if(r.empty())continue;
   if(r[0]=="C") {
    check(r.size()==25,"Exact source constant fixture size");
    c={real(r[1]),real(r[2]),real(r[3]),real(r[4]),real(r[5]),real(r[6]),real(r[7]),real(r[8]),
       real(r[9]),real(r[10]),real(r[11]),real(r[12]),real(r[13]),real(r[14]),real(r[15]),
       real(r[16]),real(r[17]),real(r[18]),real(r[19]),real(r[20]),real(r[21]),real(r[22]),
       real(r[23]),std::bit_cast<double>(std::stoull(r[24],nullptr,16))};
   }else if(r[0]=="I") {
    s=halo2::source_scope_camera_initial(c);state(s,r,1,"Original fresh constructor");++initial;
   }else if(r[0]=="L") {
    check_float(halo2::source_scope_camera_fov(real(r[2]),real(r[3]),c),r[4],r[1]);++leaves;
   }else if(r[0]=="F") {
    const std::string where="Original frame "+r[1];
    const float fov=halo2::source_scope_camera_fov(real(r[3]),real(r[4]),c);
    check_float(fov,r[5],where+" authored magnified FOV");
    dt=halo2::source_scope_camera_dt(real(r[6]),c);check_float(dt,r[7],where+" source dt writer");
    // Normal23DE70 supplied the original packet flagword; the FOV fields are
    // cleared by the executed caller contract before23E210 changes its target.
    p={hex(r[9]),0.0f,real(r[11]),0,0.0f};
    halo2::source_scope_camera_target(producer,p,fov,c);
    check_float(producer.persistent_fov,r[8],where+" persistent target");packet(p,r,9,where+" produced");++frames;
   }else if(r[0]=="S") {
    const std::string where="Original frame "+r[1]+" "+r[2];
    if(r[2]=="registration")halo2::source_scope_camera_register(s,p);
    else if(r[2]=="adoption")check(halo2::source_scope_camera_adopt(s,p,c),where+" packet admitted");
    else if(r[2]=="motion")check(!halo2::source_scope_camera_motion(s,dt,c),where+" no coupled cap branch");
    else if(r[2]=="render_packet")halo2::source_scope_camera_render_fov(s,c);
    else throw std::runtime_error("Unknown original camera phase");
    state(s,r,3,where);packet(p,r,20,where);++states;
   }else throw std::runtime_error("Unknown original camera fixture row");
  }
  check(initial==1 && leaves==4 && frames==28 && states==112,"Complete frozen source camera coverage");
  std::cout<<frames<<" source frame transitions, "<<states<<" exact phase states and "<<leaves<<" authored FOV leaves matched\n";
 }catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
