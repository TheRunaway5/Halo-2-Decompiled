#include "halo2/motion_sensor.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace halo2;
namespace {
void require(bool b,const std::string &m){if(!b)throw std::runtime_error(m);}
float real(std::istream &s){std::string v;s>>v;return std::bit_cast<float>(std::uint32_t(std::stoul(v,nullptr,16)));}
Vec3 vector(std::istream &s){return {real(s),real(s),real(s)};}
std::uint32_t raw_word(const std::string &hex,unsigned byte) {
  std::uint32_t value=0;
  for(unsigned n=0;n<4;++n)value|=std::uint32_t(std::stoul(hex.substr((byte+n)*2,2),nullptr,16))<<(n*8);
  return value;
}
void same(float actual,std::uint32_t expected,const std::string &where) {
  require(std::bit_cast<std::uint32_t>(actual)==expected,where+" expected="+
      std::to_string(expected)+" actual="+std::to_string(std::bit_cast<std::uint32_t>(actual)));
}
}
int main(int argc,char **argv){try{
  require(argc==2,"Supply unchanged original motion-sensor fixture");
  std::ifstream in(argv[1]);
  if(!in){std::cout<<"Original motion-sensor fixture unavailable; run tools/oracle_motion_sensor.py\n";return 77;}
  unsigned moving=0,samples=0,pulses=0,renders=0;
  for(std::string line;std::getline(in,line);){
    std::istringstream row(line);char kind;row>>kind;
    if(kind=='M'){
      MotionSensorObject o;o.velocity=vector(row);
      unsigned type,control,action,mp,force,m,v;
      row>>type>>control>>action>>o.player_13c>>o.flags_134>>o.estimate_mode;
      o.type=std::uint8_t(type);o.control_c1=std::uint8_t(control);o.control_14a=std::uint8_t(action);
      const auto minimum=real(row),sneak=real(row);row>>mp>>force>>m>>v;
      require(source_motion_sensor_moving(o,minimum,sneak)==bool(m),"Original radar movement case "+std::to_string(moving));
      require(source_motion_sensor_visible(o,minimum,sneak,mp,force)==bool(v),"Original radar visibility case "+std::to_string(moving));
      ++moving;
    }else if(kind=='S'){
      unsigned unit,count,present,expected;row>>unit;
      const auto origin=vector(row);const auto yaw=real(row),range=real(row);
      const auto position=vector(row);row>>count>>present>>expected;std::string hex;row>>hex;
      std::vector<MotionSensorCandidate> objects(count,{position,bool(present),true,2,1});
      const auto result=source_motion_sensor_sample(unit,origin,yaw,range,objects);
      const auto where="Original radar sample "+std::to_string(samples);
      require(result.count==expected,where+" count");
      require((expected==0 && hex=="-") || hex.size()==expected*8,where+" fixture size");
      for(unsigned n=0;n<expected;++n){
        const auto byte=[&](unsigned b){return std::uint8_t(std::stoul(hex.substr(n*8+b*2,2),nullptr,16));};
        require(result.blips[n]==MotionSensorBlip{byte(0),byte(1),byte(2),byte(3)},where+" original quantized blip");
      }
      ++samples;
    }else if(kind=='P') {
      std::int32_t tick;row>>tick;const auto rate=real(row),expected=real(row);
      same(source_motion_sensor_pulse(tick,rate),std::bit_cast<std::uint32_t>(expected),
          "Original radar pulse "+std::to_string(pulses));++pulses;
    }else if(kind=='R') {
      unsigned view,index,count;std::int16_t mode;std::int32_t tick;
      std::array<std::int16_t,2> origin,viewport;
      row>>view>>mode>>origin[0]>>origin[1]>>viewport[0]>>viewport[1]>>tick;
      const auto rate=real(row);std::string history,draws,composite;
      row>>index>>history>>count>>draws>>composite;
      require(history.size()==10*68*2,"Incomplete original radar history input");
      MotionSensorHistory h;h.sample_index=index;
      for(unsigned sample=0;sample<10;++sample) {
        auto &s=h.players.at(view)[sample];s.count=raw_word(history,sample*68+64);
        for(unsigned k=0;k<16;++k) {
          const auto bits=raw_word(history,sample*68+k*4);
          s.blips[k]={std::uint8_t(bits),std::uint8_t(bits>>8),std::uint8_t(bits>>16),std::uint8_t(bits>>24)};
        }
      }
      const auto plan=source_motion_sensor_render_plan(h,view,mode,origin,viewport,tick,rate);
      const auto where="Original radar history "+std::to_string(renders);
      require(plan.visible==(composite!="-") && plan.blips.size()==count,where+" visibility/count");
      if(plan.visible) {
        require(composite.size()==24,where+" composite ABI size");
        same(plan.center.x,raw_word(composite,0),where+" center X");
        same(plan.center.y,raw_word(composite,4),where+" center Y");
        same(plan.pulse,raw_word(composite,8),where+" pulse");
      }
      require((count==0 && draws=="-") || draws.size()==count*29*2,where+" draw ABI size");
      for(unsigned k=0;k<count;++k) {
        const auto &b=plan.blips[k];const std::array<float,7> values{
          b.point.x,b.point.y,b.scale,b.intensity,b.color.x,b.color.y,b.color.z};
        for(unsigned q=0;q<values.size();++q)
          same(values[q],raw_word(draws,k*29+q*4),where+" blip "+std::to_string(k)+" field "+std::to_string(q));
        require(b.custom==bool(std::stoul(draws.substr((k*29+28)*2,2),nullptr,16)),where+" custom bitmap");
      }
      ++renders;
    }else throw std::runtime_error("Unknown motion-sensor fixture record");
    require(!row.fail(),"Truncated original motion-sensor fixture");std::string extra;
    require(!(row>>extra),"Trailing original motion-sensor fixture field");
  }
  require(moving==25920 && samples==300 && pulses==260 && renders==50,"Incomplete original motion-sensor fixture");
  std::cout<<moving<<" original movement/visibility pairs and "<<samples
           <<" complete original quantized samples, "<<pulses<<" sweep phases and "<<renders
           <<" complete history submissions matched; world and graphics owners remain separate\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
