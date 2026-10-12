#include "halo2/frontend.hpp"
#include <fstream>
#include <iostream>

int main(int argc, char **argv) {
  try {
    if (argc != 3)
      throw halo2::FormatError("usage: export_frontend_ui_inventory mainmenu.map output.tsv");
    const halo2::Cache cache(argv[1]);
    const auto content = cache.load_frontend();
    std::ofstream output(argv[2]);
    if (!output) throw halo2::FormatError("cannot create frontend image inventory");
    output << "H2UI_LOGICAL_IMAGES_V1\n";
    for (const auto &[datum, image] : content.images) {
      if (image.name.find_first_of("\t\r\n") != std::string::npos)
        throw halo2::FormatError("frontend image name cannot be represented in TSV");
      for (std::size_t page = 0; page < image.images.size(); ++page)
        output << image.name << '\t' << page << '\t' << image.images.size()
               << '\t' << image.images[page].width << '\t'
               << image.images[page].height << '\n';
    }
    if (!output.flush()) throw halo2::FormatError("cannot complete frontend image inventory");
    std::cout << "Exported " << content.images.size() << " original frontend image banks\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
