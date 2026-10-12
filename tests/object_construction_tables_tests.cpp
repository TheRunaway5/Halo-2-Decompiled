#include "halo2/original_data.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::uint32_t word(const std::string &text){return std::stoul(text,nullptr,16);}
std::vector<std::byte> bytes(const std::string &text){
  require(text.size()%2==0,"Odd immutable field length");std::vector<std::byte> out;
  for(std::size_t i=0;i<text.size();i+=2)out.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
  return out;
}
template<class F> void unavailable(F call){
  bool stopped=false;try{call();}catch(const std::exception &){stopped=true;}
  require(stopped,"Unowned construction field/provider was admitted");
}
}
int main(int argc,char **argv){
  try{
    require(argc==5,"Supply provider fixture and base/1.1/u5 images");
    for(int i=1;i<argc;++i)if(!std::filesystem::exists(argv[i]))return 77;
    const auto base=halo2::OriginalData::load_xbe(argv[2]);
    require(!base->object_construction_tables&&!base->sound_admission_constants,"Unverified base providers imported");
    const std::array images{halo2::OriginalData::load_xbe(argv[3]),halo2::OriginalData::load_xbe(argv[4])};
    std::shared_ptr<const halo2::SourceObjectConstructionTables> tables;
    const halo2::SoundAdmissionConstants *admission=nullptr;
    std::ifstream input(argv[1]);unsigned image_count=0,type_count=0,range_count=0,query_count=0;
    std::size_t compared_bytes=0;unsigned image_ranges=0,image_types=0,image_queries=0;
    for(std::string line;std::getline(input,line);){
      std::istringstream row(line);char kind;std::string text;require(bool(row>>kind),"Empty provider row");
      if(kind=='I'){
        unsigned index;std::string bank,threshold;require(bool(row>>index>>bank>>threshold),"Incomplete image row");
        require(index==++image_count&&index<=2,"Incorrect image order");
        tables=images[index-1]->object_construction_tables;
        const auto &value=images[index-1]->sound_admission_constants;
        require(tables&&value,"Authenticated providers unavailable");admission=&*value;
        require(tables->bank==word(bank)&&admission->occlusion_threshold_bits==word(threshold),"Immutable provider identity differs");
        require(tables->ranges.size()==44,"Extra or missing imported construction ranges");
        image_ranges=image_types=image_queries=0;
      }else if(kind=='C'){
        require(admission,"Shared constant outside image");
        const auto &c=admission->shared;
        for(const auto value:{c.default_extra_gain_bits,c.minimum_gain_bits,c.maximum_float_bits,c.rng_scale_bits,
            c.unsigned_conversion_correction_bits,c.one_bits,c.upper_gain_bits,c.db_exponent_scale_bits,
            c.db_exponent_log_bits,c.linear_db_scale_bits,c.fade_epsilon_bits}){
          require(bool(row>>text)&&value==word(text),"Admission shared immutable word differs");
        }
      }else if(kind=='T'){
        unsigned type;std::string address;require(bool(row>>type>>address)&&tables&&type==image_types&&type<13,"Incorrect type row");
        require(tables->type_addresses[type]==word(address),"Imported type identity differs");++type_count;++image_types;
      }else if(kind=='R'){
        std::string address,data;require(bool(row>>address>>data)&&tables,"Incomplete field row");const auto expected=bytes(data);
        const auto at=word(address);const auto found=tables->ranges.find(at);
        require(found!=tables->ranges.end()&&found->second==expected,"Imported immutable field differs");
        const auto actual=tables->read(at,expected.size());
        require(std::equal(actual.begin(),actual.end(),expected.begin()),"Bounded field view differs");
        compared_bytes+=actual.size();++range_count;++image_ranges;
      }else if(kind=='Q'){
        std::string definition,group,result;int signed_index;require(bool(row>>definition>>signed_index>>group>>result)&&tables,"Incomplete original query");
        unsigned invoked=0;
        const auto metadata=halo2::source_object_construction_metadata(tables,[&](std::int16_t index){
          require(index==signed_index,"Original signed16 tag-index ABI differs");++invoked;return word(group);});
        const auto actual=halo2::source_object_definition_type(word(definition),metadata);
        require(actual==std::bit_cast<std::int16_t>(std::uint16_t(word(result)))&&invoked==1,"Imported original type query differs");
        ++query_count;++image_queries;
      }else if(kind=='E'){
        require(image_types==13&&image_ranges==44&&image_queries==12,"Incomplete original image coverage");
        const auto owned=tables->type_addresses[0];
        unavailable([&]{tables->read(owned+8,1);});
        unavailable([&]{tables->read(owned+7,2);});
        unavailable([&]{halo2::source_object_construction_metadata(tables,{});});
        unavailable([&]{halo2::source_object_construction_metadata({},[](std::int16_t){return 0u;});});
        auto sole_owner=std::make_shared<halo2::SourceObjectConstructionTables>(*tables);
        auto retained=halo2::source_object_construction_metadata(sole_owner,[](std::int16_t)->std::uint32_t{
          throw std::runtime_error("Actual tag group absent");});
        unavailable([&]{retained.type_address(-1);});unavailable([&]{retained.type_address(13);});
        unavailable([&]{halo2::source_object_definition_type(0,retained);});
        sole_owner.reset();tables.reset();require(retained.type_address(0)==owned,"Metadata owner lifetime lost");
        admission=nullptr;
      }else throw std::runtime_error("Unknown provider fixture row");
      require(!(row>>text),"Unexpected provider fixture operand");
    }
    require(image_count==2&&type_count==26&&range_count==88&&query_count==24&&compared_bytes==960,
        "Incomplete immutable provider comparison");
    std::cout<<"immutableConstructionBytes=960 extraAdmissionBytes=8 originalGroupQueries=24 baseUnknown=absent boundedFieldsAndOwners=pass\n";
    return 0;
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
