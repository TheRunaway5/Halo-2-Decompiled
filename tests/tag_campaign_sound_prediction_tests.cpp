#include "halo2/residency.hpp"
#include <algorithm>
#include <iostream>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
}
int main(int argc,char **argv) {
  try {
    if(argc!=2)throw FormatError("Supply the original Cairo cache");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    Cache cache(argv[1]);NativeResidency residency(cache);
    residency.predict_sound(null_datum);
    check(residency.sound_count()==0,"NONE sound prediction allocated residency");
    std::vector<std::uint32_t> sounds;
    // Exact first-chunk extents requested by unmodified retail125F10 for
    // these three actual Cairo definitions (mode2, recorded original oracle).
    for(const auto &[token,extent]:std::vector<std::pair<std::string_view,std::size_t>>{
            {"x02_01_mus",0x7ff8},{"x02_01_fol",0x7ff8},{"x02_0010_mas",0xbed4}}) {
      const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto &t){
        return t.group=="snd!" && t.name.ends_with(token);
      });
      check(tag!=cache.tags().end(),"Original sound prediction fixture missing");
      const auto first=cache.load_sound_prediction_chunk(tag->datum);
      const auto whole=cache.load_sound(tag->datum);
      check(first && first->size()==extent && first->size()<=whole.data.size() &&
          std::equal(first->begin(),first->end(),whole.data.begin()),
          "Prediction did not retain the actual first compressed chunk");
      residency.predict_sound(tag->datum);residency.predict_sound(tag->datum);
      sounds.push_back(tag->datum);
      check(residency.sound_count()==sounds.size() && residency.decoded_sound_count()==0,
          "Sound prediction decoded PCM or duplicated its chunk");
    }
    // Playback still uses the existing complete decoder and reuses the same
    // PCM owner. Promoting one prediction must preserve the other two chunks.
    const auto pcm=residency.sound(sounds.back());
    check(pcm && pcm->frames()>0 && pcm->sound_class==48 &&
          residency.sound(sounds.back())==pcm && residency.decoded_sound_count()==1 &&
          residency.sound_count()==3,"Predicted sound promotion changed playback ownership");
    residency.predict_sound(sounds.back());
    check(residency.sound_count()==3 && residency.decoded_sound_count()==1,
          "Prediction duplicated an existing playback owner");
    bool stale=false;
    try{residency.predict_sound(sounds.front()^0x10000u);}catch(const FormatError &){stale=true;}
    check(stale && residency.sound_count()==3,"Stale prediction salt changed residency");
    std::cout<<"Original first sound chunks retained without PCM; playback promotion and stale salts passed\n";
  }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
