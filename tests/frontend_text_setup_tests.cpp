#include "halo2/frontend_text_style.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace halo2;
namespace {
void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
void original_setup(const std::filesystem::path &path) {
  std::ifstream file(path);
  require(bool(file), "missing original text setup fixture");
  unsigned cases{}, ordinary{}, small{}, buttons{}, flagged_buttons{};
  for (std::string line; std::getline(file,line);) {
    if (line.empty() || line.front()=='#') continue;
    std::istringstream input(line);
    char type{};
    std::string label, shared_word, expected_word;
    bool button{}, expected_shadow{};
    unsigned flags{};
    int authored_font{}, expected_font{};
    input >> type >> label >> button >> flags >> authored_font >> shared_word
          >> expected_font >> expected_word >> expected_shadow;
    require(!input.fail() && type=='S' && shared_word.size()==8 && expected_word.size()==8,
            "invalid original text setup row");
    std::string trailing;
    require(!(input>>trailing), "trailing original text setup input");
    const auto shared=std::uint32_t(std::stoul(shared_word,nullptr,16));
    const auto expected=std::uint32_t(std::stoul(expected_word,nullptr,16));
    const auto actual=frontend_text_setup(flags,authored_font,button,
                                          std::bit_cast<float>(shared));
    if (actual.font!=expected_font ||
        std::bit_cast<std::uint32_t>(actual.scale)!=expected ||
        actual.shadow!=expected_shadow)
      throw std::runtime_error("text setup differs from original253765/253CFF: "+label);
    if (button) {
      ++buttons;
      if (flags&8) ++flagged_buttons;
    } else if (flags&8) ++small;
    else ++ordinary;
    ++cases;
  }
  require(cases>=60 && ordinary && small>=52 && buttons && flagged_buttons,
          "original text setup fixture omits source definitions or caller controls");
  std::cout << "Original text setup: " << cases << " exact font/scale/shadow cases\n";
}
void actual_metadata(const std::filesystem::path &path) {
  const Cache cache(path);
  const auto content=cache.load_frontend();
  require(content.typewriter.interval_ms==50 && content.typewriter.cursor==0xdb &&
          std::bit_cast<std::uint32_t>(content.typewriter.scale)==0x3e99999a,
          "loaded mainmenu typewriter fields differ from actual wigl+48/+4A/+4C");
  unsigned small{}, ordinary{}, buttons{};
  for (const auto &[id,screen] : content.screens)
    for (const auto &pane : screen.panes) {
      for (const auto &text : pane.texts) {
        const auto setup=frontend_text_setup(text.flags,text.font,false,
                                              content.typewriter.scale);
        if (text.flags&8) {
          require(text.font==9 && setup.font==1 && !setup.shadow &&
                  std::bit_cast<std::uint32_t>(setup.scale)==0x3e99999a,
                  "actual flag8 text does not resolve original font1/shared scale");
          ++small;
        } else {
          require(setup.font==text.font && setup.scale==1 && setup.shadow,
                  "ordinary text loses its authored font/unit scale/shadow");
          ++ordinary;
        }
      }
      for (const auto &button : pane.buttons) {
        const auto setup=frontend_text_setup(button.text.flags,button.text.font,true,
                                              content.typewriter.scale);
        require(setup.font==button.text.font && setup.scale==1 && setup.shadow,
                "actual button loses its authored font/unit scale/shadow");
        ++buttons;
      }
    }
  require(small==52 && ordinary && buttons,
          "actual mainmenu text setup coverage differs from retained source inventory");
  const auto &labels=content.screens.at(19).panes.at(0).texts;
  require(labels.size()==8 && !(labels[0].flags&8),
          "Settings description/diagram inventory changed");
  for (unsigned i=1; i<labels.size(); ++i)
    require(labels[i].flags==9 && labels[i].font==9 && labels[i].delay==0,
            "Settings label source definition changed");
  std::cout << "Actual shared fields and " << small << " standard small-text definitions, "
            << ordinary << " ordinary texts, " << buttons << " buttons\n";
}
}
int main(int argc,char **argv) {
  if (argc!=3 || !std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))
    return 77;
  try {
    original_setup(argv[2]);
    actual_metadata(argv[1]);
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n'; return 1;
  }
}
