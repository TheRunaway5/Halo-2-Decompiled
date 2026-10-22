fn main() {
    let mut build = cxx_build::bridge("src/lib.rs");
    build.std("c++20").include(".").file("src/eos_sdk.cpp");
    if let Some(root) = std::env::var_os("HALO2_EOS_SDK_ROOT") {
        build.include(std::path::Path::new(&root).join("Include"));
        build.define("HALO2_EOS_SDK", "1");
    }
    build.compile("halo2-online-protocol-bridge");
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/eos_sdk.hpp");
    println!("cargo:rerun-if-changed=src/eos_sdk.cpp");
    println!("cargo:rerun-if-env-changed=HALO2_EOS_SDK_ROOT");
}
