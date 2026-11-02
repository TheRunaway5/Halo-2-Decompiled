//! Full source-buffer admission at the actual Rust catalog boundary.
use halo2_online_protocol::matchmaking_catalog;
use serde_json::Value;

fn bytes(encoded: &str) -> Vec<u8> {
    encoded
        .as_bytes()
        .chunks_exact(2)
        .map(|pair| u8::from_str_radix(std::str::from_utf8(pair).unwrap(), 16).unwrap())
        .collect()
}

#[test]
fn all_retained_original_full_buffer_validity_controls() {
    let mut count = 0;
    for line in
        include_str!("../../../tests/fixtures/game_variant_normalization_original.tsv").lines()
    {
        if !line.starts_with("N ") {
            continue;
        }
        let fields: Vec<_> = line.split_whitespace().collect();
        let raw = bytes(fields[3]);
        assert_eq!(
            halo2_online_protocol::matchmaking_variant::valid(&raw),
            fields[5] == "1",
            "{}",
            fields[2]
        );
        count += 1;
    }
    assert_eq!(count, 4249);
}

#[test]
fn variant_admission_has_exact_extent_and_preserves_all_authored_presets() {
    assert!(!halo2_online_protocol::matchmaking_variant::valid(&[]));
    assert!(!halo2_online_protocol::matchmaking_variant::valid(
        &[0; 303]
    ));
    assert!(!halo2_online_protocol::matchmaking_variant::valid(
        &[0; 305]
    ));
    let presets: Value =
        serde_json::from_str(include_str!("../data/tu15-result-presets.json")).unwrap();
    let rows = presets["variants"].as_array().unwrap();
    assert_eq!(rows.len(), 39);
    for row in rows {
        let raw = bytes(row["settings"].as_str().unwrap());
        let before = raw.clone();
        assert!(halo2_online_protocol::matchmaking_variant::valid(&raw));
        assert_eq!(raw, before);
    }
}

#[test]
fn catalog_rejects_negative_original_king_timer() {
    let mut catalog: Value = serde_json::from_str(include_str!(
        "../../../multiplayer/master_server/examples/matchmaking-online.json"
    ))
    .unwrap();
    assert!(matchmaking_catalog::parse(&serde_json::to_vec(&catalog).unwrap()).is_ok());
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
    assert!(
        matchmaking_catalog::parse(&serde_json::to_vec(&catalog).unwrap()).is_err(),
        "Rust admitted a King timer of signed-short -1; native original validation refuses it"
    );
}
