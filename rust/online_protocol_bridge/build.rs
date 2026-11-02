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
    println!("cargo:rerun-if-changed=src/matchmaking_client.hpp");
    println!("cargo:rerun-if-changed=src/matchmaking_native_qos.hpp");
    println!("cargo:rerun-if-changed=src/matchmaking_native_session.hpp");
    println!("cargo:rerun-if-changed=src/matchmaking_native_peers.hpp");
    println!("cargo:rerun-if-changed=src/matchmaking_native_publication.hpp");
    println!("cargo:rerun-if-changed=src/matchmaking_native_preparation.hpp");
    println!("cargo:rerun-if-changed=src/matchmaking_native_exchange.hpp");
    println!("cargo:rerun-if-changed=src/peer_snapshot_candidate.hpp");
    println!("cargo:rerun-if-changed=src/matchmaking_native_receive.hpp");
    println!("cargo:rerun-if-changed=src/matchmaking_native_access.hpp");
    println!("cargo:rerun-if-changed=src/matchmaking_native_migration.hpp");
    println!("cargo:rerun-if-changed=src/matchmaking_native_api.hpp");
    println!("cargo:rerun-if-changed=src/matchmaking_account_client.hpp");
    println!("cargo:rerun-if-changed=src/matchmaking_native_results.hpp");
    println!("cargo:rerun-if-changed=src/matchmaking_frontend_search.hpp");
    println!("cargo:rerun-if-changed=src/matchmaking_frontend_presentation.hpp");
    println!("cargo:rerun-if-changed=src/matchmaking_frontend_party.hpp");
    println!("cargo:rerun-if-changed=src/matchmaking_frontend_party_flow.hpp");
    println!("cargo:rerun-if-changed=src/matchmaking_frontend_party_view.hpp");
    println!("cargo:rerun-if-changed=src/matchmaking_frontend_lobby_query.hpp");
    println!("cargo:rerun-if-changed=src/matchmaking_frontend_account_flow.hpp");
    println!("cargo:rerun-if-changed=src/matchmaking_frontend_friends.hpp");
    println!("cargo:rerun-if-changed=src/matchmaking_frontend_lobby_view.hpp");
    println!("cargo:rerun-if-env-changed=HALO2_EOS_SDK_ROOT");
}
