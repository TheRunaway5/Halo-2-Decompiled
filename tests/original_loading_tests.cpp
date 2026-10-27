#include "halo2/original_loading_assets.hpp"
#include "halo2/original_loading_progress.hpp"
#include "halo2/original_loading_wait.hpp"
#include "halo2/source_loading_assets.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {void check(bool value,const char*message){if(!value)throw FormatError(message);}}
int main(int argc,char**argv)try {
  if(argc!=3 && argc!=4)return 77;
  const Cache source(argv[1]);check(!source.is_mcc() && !source.is_vista(),"Original loading reference requires its authenticated Xbox cache");
  const auto &banks=original_loading_bitmaps();unsigned pages{};
  for(const auto &bank:banks) {
    const auto*owner=source.find_tag({bank.source_datum});
    check(owner && owner->name==bank.name && owner->group=="bitm","Original loading bitmap reference changed");
    for(unsigned page=0;page<bank.frames.size();++page) {
      const auto actual=source.load_bitmap(bank.source_datum,page);const auto &prepared=bank.frames[page];
      check(actual.width==prepared.width && actual.height==prepared.height && actual.format==prepared.format &&
          actual.layers==prepared.layers && actual.depth==prepared.depth && actual.mips.size()==prepared.mips.size() &&
          actual.pixels==prepared.pixels,"Original loading pixels differ from actual native Cache preparation");
      for(unsigned mip=0;mip<actual.mips.size();++mip) {
        const auto&a=actual.mips[mip],&b=prepared.mips[mip];
        check(a.width==b.width && a.height==b.height && a.depth==b.depth && a.offset==b.offset && a.size==b.size,
            "Original loading resident mip extent changed");
      }
      ++pages;
    }
  }
  check(pages==11,"Original loading source bank is incomplete");
  if(argc==4) {
    const Cache admitted(argv[3]);const auto actual=load_source_loading_bitmap_assets(admitted);
    for(unsigned bank=0;bank<banks.size();++bank) {
      check(actual[bank].name==banks[bank].name && actual[bank].frames.size()==banks[bank].frames.size(),
          "MCC source loading role/page identity changed");
      for(unsigned page=0;page<banks[bank].frames.size();++page) {
        const auto&a=actual[bank].frames[page],&b=banks[bank].frames[page];
        check(a.width==b.width && a.height==b.height && a.format==b.format && a.pixels==b.pixels && a.mips.size()==b.mips.size(),
            "Admitted MCC source loading resident pixels differ from offline original control");
        for(unsigned mip=0;mip<a.mips.size();++mip)
          check(a.mips[mip].offset==b.mips[mip].offset && a.mips[mip].size==b.mips[mip].size,
              "Admitted MCC source loading resident mip boundaries differ");
      }
    }
  }
  std::ifstream fixture(argv[2]);check(bool(fixture),"Original loading byte-control fixture is unavailable");
  const auto records=nlohmann::json::parse(fixture);
  check(records.at("schema")=="H2_TU15_LOADING_ARITHMETIC_V1" && records.at("original_sha256")==
      "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Original loading fixture source changed");
  unsigned cases{};
  for(const auto&row:records.at("cases")) {
    const auto&v=row.at("input"),&expected=row.at("output");
    if(row.at("kind")==0) {
      OriginalLoadingProgress state{v[0],v[1],v[2]};
      original_loading_progress_update(state,{v[3],v[4],v[5],v[6]},{0x027f,0x1f80});
      check(state.baseline_bits==expected[0] && state.progress_bits==expected[1] && state.previous_clock==expected[2],
          "Native Xbox loading progress differs from unchanged TU1.5 control");
    } else {
      OriginalLoadingFilter state{v[0],v[1]};const auto result=original_loading_filtered_progress(state,v[2],{0x027f,0x1f80});
      check(state.delta_bits==expected[0] && result==expected[1],"Native Xbox loading filter differs from unchanged TU1.5 control");
    }
    ++cases;
  }
  check(cases==944,"Original loading arithmetic controls are incomplete");
  const auto wait_path=std::filesystem::path(__FILE__).parent_path()/"fixtures/tu15_loading_wait_original.json";
  std::ifstream wait_stream(wait_path);check(bool(wait_stream),"Original loading wait controls are unavailable");
  const auto waits=nlohmann::json::parse(wait_stream);unsigned wait_cases{};
  check(waits.at("schema")=="H2_TU15_LOADING_WAIT_V1" && waits.at("original_sha256")==records.at("original_sha256"),
        "Original loading wait source identity changed");
  for(const auto &row:waits.at("cases")) {
    const auto &input=row.at("input"),&expected=row.at("output");
    const auto actual=original_loading_wait_text(input[0],input[1],
        {input[2].get<std::int16_t>(),0,480,640},{0x27f,0x1f80});
    check(actual.percent==expected[0] && actual.rectangle[0]==expected[1] && actual.font==expected[2] &&
        actual.style==expected[3] && actual.justification==expected[4] && actual.flags==expected[5],
        "Original loading wait numeric/setup tuple differs from unchanged instructions");
    check(actual.text.size()+6==expected.size(),"Original loading wait localized caption extent changed");
    for(unsigned i=0;i<actual.text.size();++i)check(unsigned(actual.text[i])==expected[i+6],
        "Original loading wait differs from its unchanged localized caption/percentage");
    ++wait_cases;
  }
  check(wait_cases==340,"Original loading wait controls are incomplete");
  std::cout<<"PASS original loading "<<pages<<" complete source pages/mips, "<<cases
      <<" unchanged TU1.5 arithmetic controls and "<<wait_cases<<" original MP wait controls\n";
  return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
