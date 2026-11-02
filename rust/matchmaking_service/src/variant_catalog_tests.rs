//! The actual startup/SQL owner must refuse a source-invalid full buffer.
use super::Runtime;
use serde_json::Value;
use std::path::Path;

#[test]
fn source_invalid_catalog_cannot_allocate_a_runtime_queue() {
    let mut catalog: Value = serde_json::from_str(include_str!(
        "../../../multiplayer/master_server/examples/matchmaking-online.json"
    ))
    .unwrap();
    let original = serde_json::to_vec(&catalog).unwrap();
    assert!(Runtime::open(Path::new(":memory:"), false, Some(&original)).is_ok());
    let game = catalog["playlists"]
        .as_array_mut()
        .unwrap()
        .iter_mut()
        .flat_map(|playlist| playlist["games"].as_array_mut().unwrap())
        .find(|game| &game["settings"].as_str().unwrap()[0x44 * 2..0x48 * 2] == "04000000")
        .unwrap();
    let mut settings = game["settings"].as_str().unwrap().to_owned();
    settings.replace_range(0xf4 * 2..0xf6 * 2, "ffff");
    game["settings"] = Value::String(settings);
    let invalid = serde_json::to_vec(&catalog).unwrap();
    let error = Runtime::open(Path::new(":memory:"), false, Some(&invalid))
        .err()
        .unwrap();
    assert_eq!(error.0, 400);
    assert_eq!(error.1, "Game variant settings fail original validation");
}
