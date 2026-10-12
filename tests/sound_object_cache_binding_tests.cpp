// Reuse the unchanged original fixture parser and explicit retained-world
// inputs. The separately named main below replaces only metadata providers.
#define main original_literal_sound_object_main
#include "sound_object_request_tests.cpp"
#undef main
#include "halo2/original_data.hpp"
#include "halo2/sound_metadata_store.hpp"

int main(int argc,char **argv) {
  try {
    check(argc==6,"Supply original world-owner fixture, owned Lockout and base/1.1/u5 images");
    for (int i=1;i<argc;++i) if (!std::filesystem::exists(argv[i])) return 77;
    const auto fixture=load(argv[1]);
    auto cache=std::make_shared<halo2::Cache>(argv[2]);
    std::size_t class_tags=0;
    for (const auto &tag:cache->tags()) class_tags+=tag.group=="sncl" && tag.address!=0;
    check(class_tags==1,"Owned original map sound classes are ambiguous");
    const auto metadata=halo2::SoundMetadataStore(cache).providers();
    check(!halo2::OriginalData::load_xbe(argv[3])->sound_object_request_constants,
          "Unverified base world-sound operands were supplied");
    std::size_t accepted=0,writes=0,cases=0;
    for (int image=4;image<6;++image) {
      const auto data=halo2::OriginalData::load_xbe(argv[image]);
      check(bool(data->sound_object_request_constants),"Original world-sound operands unavailable");
      const auto &constants=*data->sound_object_request_constants;
      const auto &expected=fixture.constants;
      check(constants.one_bits==expected.one_bits && constants.minus_one_bits==expected.minus_one_bits &&
            constants.z_scale_bits==expected.z_scale_bits && constants.xy_scale_bits==expected.xy_scale_bits &&
            constants.zero_vector==expected.zero_vector,"Imported whole world-sound immutable operands differ");
      for (const auto &c:fixture.cases) {
        check(metadata.definition(c.sound)==c.definition &&
              metadata.sound_class(std::bit_cast<std::int8_t>(c.definition[2]))==c.sound_class,
              "Owned Cache records differ from unchanged original world-owner metadata");
        check(c.points.empty(),"Real point-query ownership required by this original case");
        auto supplied=services(c);
        supplied.associations.sound_definition=metadata.definition;
        supplied.associations.sound_class=metadata.sound_class;
        // No point-location callback is supplied. The unchanged cases have no
        // positive activity query; that live producer remains unavailable.
        auto location=c.before;
        std::vector<halo2::SoundObjectRequestStore> observed;
        const auto before=c.memory;
        const auto result=halo2::source_sound_object_request(c.owner,c.sound,c.marker,location,
            constants,c.fpcw,supplied,[&](const auto &write){observed.push_back(write);});
        check(result==c.expected && location==c.after && observed==c.stores && c.memory==before,
              "Original world-sound owner differs with actual Cache metadata");
        accepted+=result;writes+=observed.size();++cases;
      }
    }
    check(cases==30 && accepted==24 && writes==240,"Incomplete original world-sound composition coverage");
    std::cout<<"actualCacheWorldOwnerCases=30 acceptedOwnerRequests=24 genuineRejections=6 "
                 "orderedStores=240 metadataFixtureCallbacks=0 pointQueryCallbacks=0 "
                 "retainedOriginalWorldAndLocalAssociations=explicitInputs "
                 "sourceOriginalExecutionImage=1.1 supportedOperandImages=2 "
                 "sourceVoiceAdmissionAndLiveReload=unverified\n";
  } catch (const std::exception &error) { std::cerr<<error.what()<<'\n';return 1; }
}
