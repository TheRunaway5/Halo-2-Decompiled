#include "halo2/source_bitmap_async_read.hpp"
#include <array>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <vector>

int main(int argc,char **argv) {
    using namespace halo2;
    using J=nlohmann::json;
    try {
        if(argc!=5)throw std::invalid_argument("Expected input, initial buffer, result and output buffer");
        J input;std::ifstream(argv[1])>>input;
        SourceAsyncTask task;
        for(unsigned i=0;i<10;++i)task.words[i]=input.at("observed_task").at(i).get<std::uint32_t>();
        std::vector<std::byte> buffer(input.at("bytes").get<std::size_t>());
        std::ifstream before(argv[2],std::ios::binary);
        before.read(reinterpret_cast<char *>(buffer.data()),std::streamsize(buffer.size()));
        if(std::size_t(before.gcount())!=buffer.size())throw std::runtime_error("Incomplete observed entry buffer");
        const std::array files{SourceAsyncOwnedFile{task.words[0],input.at("owned_path").get<std::string>()}};
        SourceAsyncOwnedFiles owner(files);
        const auto result=owner.read(task,input.at("declared_chunk_limit").get<std::uint32_t>(),
                                     {task.words[1],buffer});
        J output={{"task_after",task.words},
            {"step",{{"file",result.step.file},{"offset",result.step.offset},
                     {"buffer",result.step.buffer},{"size",result.step.size}}},
            {"actual_seek_low",result.io.seek_low},{"actual_bytes_read",result.io.bytes_read},
            {"callback_result",result.completion.callback_result},
            {"cumulative_write",result.completion.cumulative_write?J(*result.completion.cumulative_write):J(nullptr)},
            {"expected_buffer_or_kernel_result_input",false}};
        std::ofstream(argv[3])<<output.dump()<<'\n';
        std::ofstream data(argv[4],std::ios::binary);
        data.write(reinterpret_cast<const char *>(buffer.data()),std::streamsize(buffer.size()));
        if(!data)throw std::runtime_error("Cannot write native observation");
    } catch(const std::exception &error) {
        std::cerr<<error.what()<<'\n';return 1;
    }
}
