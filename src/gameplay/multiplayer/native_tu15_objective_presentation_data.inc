    // TU1.5 21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd.
    // Individually declared waypoint/meter numeric operands, never code bytes.
    for(const auto [address,bits]:std::array<std::pair<std::uint32_t,std::uint32_t>,4>{{
        {0x4500b4u,0x41800000u},{0x46a310u,0x41c00000u},
        {0x46a388u,0x3d000000u},{0x46a448u,0xc0000000u}}})
        share_immutable_word(memory,address,bits,"native TU1.5 objective waypoint/meter scalar operand");
    share_immutable_words(memory,0x46aa10u,std::array{0u,0x40800000u},
        "native TU1.5 waypoint binary64 screen scale");
    share_immutable_words(memory,0x46aa18u,std::array{0x66666666u,0x3fd66666u},
        "native TU1.5 waypoint binary64 distance-height scale");
    share_immutable_words(memory,0x44ac40u,std::array{0u,0u,0x3f800000u},
        "native TU1.5 authored world-up vector");
    for(const auto [address,bits]:std::array<std::pair<std::uint32_t,std::uint32_t>,3>{{
        {0x474d10u,0x44ac40u},{0x48c7b4u,0x3fc00000u},{0x474c74u,0x44aaecu}}}) {
        const auto bytes=encode(std::array{bits});bool present=false;
        try {
            const auto prior=memory.view(address,4);present=true;
            if(!std::ranges::equal(prior,bytes))throw std::invalid_argument("Objective initialized scalar differs from its existing owner");
        }catch(const std::out_of_range&){}
        if(!present)memory.mount(address,bytes,SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 initialized objective world-up/fade binding");
    }
    for(const auto& [address,words]:std::array<std::pair<std::uint32_t,std::array<std::uint32_t,3>>,2>{{
        {0x4751e4u,{0x3eeaeaebu,0x3f3ababbu,0x3f800000u}},
        {0x4751f4u,{0x3f4ececfu,0x3f0f8f90u,0x3f5ededfu}}}}) {
        const auto bytes=encode(words);bool present=false;
        try {
            std::array<std::byte,12> prior;memory.read(address,prior);present=true;
            if(prior!=bytes)throw std::invalid_argument("Objective species palette differs from its original owner");
        }catch(const std::out_of_range&){}
        if(!present)memory.mount(address,bytes,SourceRuntimeMemory::Access::ReadWrite,
            "native TU1.5 initialized Spartan/Elite waypoint RGB palette");
    }
    share_immutable_words(memory,0x44aaecu,std::array{0x3f000000u,0x3f000000u,0x3f000000u},
        "native TU1.5 authored neutral objective RGB");
    // Original164FD0 friendly-player warning/relationship colors.
    // Authentication: original-waypoint-palettes.json, two named RGB operands.
    share_immutable_word(memory,0x474c7cu,0x44ab0cu,
        "native TU1.5 friendly waypoint red warning RGB binding");
    share_immutable_words(memory,0x44ab0cu,std::array{0x3f800000u,0u,0u},
        "native TU1.5 friendly waypoint red warning RGB");
    share_immutable_word(memory,0x474c84u,0x44ab2cu,
        "native TU1.5 friendly waypoint blue relationship RGB binding");
    share_immutable_words(memory,0x44ab2cu,std::array{0u,0u,0x3f800000u},
        "native TU1.5 friendly waypoint blue relationship RGB");
