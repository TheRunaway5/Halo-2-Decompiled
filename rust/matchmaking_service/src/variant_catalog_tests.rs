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

#[test]
fn catalog_restart_refuses_a_reused_revision_and_allows_exact_rollback() {
    let original: Value = serde_json::from_str(include_str!(
        "../../../multiplayer/master_server/examples/matchmaking-online.json"
    ))
    .unwrap();
    let mut nonce = [0; 8];
    getrandom::getrandom(&mut nonce).unwrap();
    let root = std::path::PathBuf::from(
        std::env::var_os("HALO2_TEST_SCRATCH").expect("Run through tools/run_test.py"),
    )
    .join(format!("catalog-revisions-{}", u64::from_le_bytes(nonce)));
    std::fs::create_dir(&root).unwrap();
    let db = root.join("registry.sqlite3");
    let open =
        |catalog: &Value| Runtime::open(&db, false, Some(&serde_json::to_vec(catalog).unwrap()));
    drop(open(&original).unwrap());
    drop(
        Runtime::open(
            &db,
            false,
            Some(&serde_json::to_vec_pretty(&original).unwrap()),
        )
        .unwrap(),
    );

    let mut candidate = original.clone();
    candidate["playlists"][0]["revision"] = Value::from("atomic-catalog-update");
    let lab = candidate["playlists"]
        .as_array()
        .unwrap()
        .iter()
        .position(|row| row["id"] == "lab-test")
        .unwrap();
    let game = &mut candidate["playlists"][lab]["games"][0];
    game["weight"] = Value::from(game["weight"].as_u64().unwrap() + 1);
    let error = open(&candidate)
        .err()
        .expect("Reused playlist revision was accepted");
    assert_eq!(error.0, 409);
    let previous_count = original["playlists"].as_array().unwrap().len() as i64;
    let state = rusqlite::Connection::open(&db).unwrap();
    let pins = || {
        state
            .query_row(
                "SELECT count(*) FROM matchmaking_catalog_revisions",
                [],
                |row| row.get::<_, i64>(0),
            )
            .unwrap()
    };
    assert_eq!(
        pins(),
        previous_count,
        "Rejected startup left a partial binding"
    );

    candidate["playlists"][lab]["revision"] = Value::from("new-lab-rotation");
    drop(open(&candidate).unwrap());
    assert_eq!(pins(), previous_count + 2);
    drop(open(&original).unwrap());
    assert_eq!(pins(), previous_count + 2);
}
