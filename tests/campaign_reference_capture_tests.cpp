#include "app/campaign_reference_capture.hpp"
#include <fstream>
#include <iostream>

using namespace halo2;
namespace capture=halo2::campaign_reference_capture;
static_assert(std::is_same_v<decltype(CampaignSourceRuntime::ReferenceState::random_address),std::uint32_t>);
static_assert(std::is_same_v<decltype(CampaignSourceRuntime::ReferenceState::clock_address),std::uint32_t>);
namespace {
void need(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
template<class Fn> void reject(Fn fn) {
  bool failed=false;try{fn();}catch(const FormatError&){failed=true;}
  need(failed,"Invalid capture invocation was admitted");
}
void admission() {
  capture::Options options{1.f/30.f,.5f,0,"/trace.jsonl"};
  capture::Invocation invocation{true,true,false,false,true,true,false,60,"/frame.ppm"};
  capture::validate(options,invocation);
  const auto bad=[&](auto change){auto copy=invocation;change(copy);reject([&]{capture::validate(options,copy);});};
  bad([](auto &v){v.campaign=false;});bad([](auto &v){v.play=false;});
  bad([](auto &v){v.multiplayer=true;});bad([](auto &v){v.menu=true;});
  bad([](auto &v){v.map_supplied=false;});bad([](auto &v){v.mute=false;});
  bad([](auto &v){v.incompatible=true;});bad([](auto &v){v.frames=0;});
  bad([](auto &v){v.frames=601;});bad([](auto &v){v.screenshot.clear();});
  for(float interval:{0.f,-1.f,.0501f,std::numeric_limits<float>::infinity()}) {
    auto copy=options;copy.interval=interval;reject([&]{capture::validate(copy,invocation);});
  }
  for(float fraction:{-.001f,1.001f,std::numeric_limits<float>::quiet_NaN()}) {
    auto copy=options;copy.fraction=fraction;reject([&]{capture::validate(copy,invocation);});
  }
  for(const auto text:{"","nan","inf","1junk","-inf"})reject([&]{(void)capture::real(text);});
  for(const auto text:{"","-1","4294967296","1junk"})reject([&]{(void)capture::seed(text);});
  need(capture::seed("0")==0 && capture::seed("4294967295")==UINT32_MAX,"Seed endpoints changed");
}
void environment() {
  std::fenv_t original{};need(std::fegetenv(&original)==0,"Save initial FP environment");
  const int original_errno=errno;
  for(unsigned rounding=0;rounding<4;++rounding)for(unsigned controls=0;controls<4;++controls)
    for(unsigned sticky:{0u,1u,0x20u,0x3fu}) {
      const unsigned before=0x1f80u|(rounding<<13)|(controls&1u?0x40u:0u)|(controls&2u?0x8000u:0u)|sticky;
      _mm_setcsr(before);errno=EDOM;
      std::fenv_t saved{};need(std::fegetenv(&saved)==0,"Save diagnostic test FP environment");
      for(bool unwind:{false,true}) {
        try {
          capture::Environment guard;
          (void)std::feraiseexcept(FE_INVALID|FE_OVERFLOW|FE_INEXACT);
          (void)std::fesetround(FE_DOWNWARD);errno=ERANGE;
          if(unwind)throw std::runtime_error("diagnostic unwind");
        }catch(const std::runtime_error&){need(unwind,"Unexpected diagnostic exception");}
        std::fenv_t after{};need(std::fegetenv(&after)==0,"Read diagnostic FP receipt");
        need(_mm_getcsr()==before && errno==EDOM && !std::memcmp(&saved,&after,sizeof(saved)),
            "Diagnostic normal/unwind path leaked full FP environment or errno");
      }
    }
  need(std::fesetenv(&original)==0,"Restore initial FP environment");errno=original_errno;
}
void materialized_geometry() {
  capture::Environment guard;
  auto geometry=std::make_shared<ModelSkinGeometry>();
  geometry->vertices.resize(3);geometry->bindings.resize(3);geometry->palette.resize(1);
  geometry->palette.front()={1,0,0,2,0,1,0,3,0,0,1,4};
  for(unsigned i=0;i<3;++i) {
    auto &v=geometry->vertices[i];v.position={float(i),-0.f,1};v.normal={0,1,-0.f};
    v.tangent={1,0,0};v.binormal={0,0,1};v.material=i+5;
    geometry->bindings[i].weights=i==2?std::array<float,4>{-0.f,0,0,0}:std::array<float,4>{1,0,0,0};
  }
  const auto immutable=capture::sha256(std::as_bytes(std::span(geometry->vertices)));
  std::vector<Vertex> input(8);std::copy(geometry->vertices.begin(),geometry->vertices.end(),input.begin()+2);
  ModelSkinSubmission submission{2,geometry};
  for(bool post:{false,true}) {
    geometry->post_identity_placement=post;
    auto expected=input;
    auto output=std::span(expected).subspan(2,3);
    skin_model_vertices(output,geometry->bindings,geometry->palette);
    if(post)for(auto &v:output) {
      const Transform identity{};
      const auto rotate=[&](Vec3 value) {
        return (identity.basis[0]*value.x+identity.basis[1]*value.y)+identity.basis[2]*value.z;
      };
      v.position=identity.position+rotate(v.position)*identity.scale;
      v.normal=rotate(v.normal);v.tangent=rotate(v.tangent);v.binormal=rotate(v.binormal);
    }
    need(capture::geometry(input,std::span(&submission,1))==capture::sha256(std::as_bytes(std::span(expected))),
        "Selected complete Vertex geometry digest changed");
    need(capture::sha256(std::as_bytes(std::span(geometry->vertices)))==immutable,"Diagnostic mutated its source owner");
  }
  const std::array duplicates{submission,submission};
  reject([&]{(void)capture::geometry(input,duplicates);});
  auto invalid=submission;invalid.first=7;reject([&]{(void)capture::geometry(input,std::span(&invalid,1));});
  invalid.geometry={};reject([&]{(void)capture::geometry(input,std::span(&invalid,1));});
  need(capture::nodes(std::span<const Transform>{}).empty(),"Empty WORLD-node bank changed");
}
void trace_storage() {
  const auto *scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch && *scratch,"Owned scratch required");
  const auto path=std::filesystem::path(scratch)/"campaign-reference-unit.jsonl";
  {
    capture::Trace trace(path);
    trace.record([]{return nlohmann::json{{"event","first"}};});
    trace.record([]{return nlohmann::json{{"event","second"}};});
  }
  std::ifstream input(path);std::string line;
  for(unsigned i=0;i<2;++i) {
    need(bool(std::getline(input,line)),"Missing ordered trace event");
    need(nlohmann::json::parse(line).at("sequence")==i,"Trace event order changed");
  }
  reject([&]{capture::Trace overwrite(path);});
  std::filesystem::remove(path);
}
}
int main() {
  try {admission();environment();materialized_geometry();trace_storage();
    std::cout<<"Campaign reference admission, 128 full-FP diagnostic exits, immutable materialized geometry and owned trace gates passed\n";return 0;
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
