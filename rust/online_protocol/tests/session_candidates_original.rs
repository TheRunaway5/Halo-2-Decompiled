use halo2_online_protocol::session_candidates::{
    self as core, BANK_SIZE, COUNT, Candidates, Configuration, STRIDE, Search,
};
use serde_json::Value;
use sha2::{Digest, Sha256};

fn fixture() -> Value {
    let compressed = include_bytes!(
        "../../../multiplayer/master_server/tests/fixtures/retail-session-candidates.json.gz"
    );
    assert_eq!(
        format!("{:x}", Sha256::digest(compressed)),
        "20ef2f951fdac45ebec6501c95dd3b372c706392961f89b36b51bf1acb8f649a"
    );
    // This pinned numerical fixture has a plain ten-byte gzip header. Decode
    // only that trusted deflate stream; no general gzip parser is introduced.
    assert_eq!(&compressed[..4], &[0x1f, 0x8b, 8, 0]);
    let bytes =
        miniz_oxide::inflate::decompress_to_vec(&compressed[10..compressed.len() - 8]).unwrap();
    let value: Value = serde_json::from_slice(&bytes).unwrap();
    assert_eq!(
        value["xbe_sha256"],
        "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d"
    );
    assert_eq!(
        value["oracle_sha256"],
        "29133faa0b9a6a08868a3477f1f3a16bebc8e4c0660f8f2d11e9f4cf0a05e59e"
    );
    value
}

fn decode_hex(source: &str) -> Vec<u8> {
    source
        .as_bytes()
        .chunks_exact(2)
        .map(|pair| u8::from_str_radix(std::str::from_utf8(pair).unwrap(), 16).unwrap())
        .collect()
}

fn bank(value: &Value) -> Vec<u8> {
    let mut bank = vec![0; BANK_SIZE];
    for row in value.as_array().unwrap() {
        let index = row[0].as_u64().unwrap() as usize;
        let data = decode_hex(row[1].as_str().unwrap());
        assert!(index < COUNT);
        assert_eq!(data.len(), STRIDE);
        bank[index * STRIDE..(index + 1) * STRIDE].copy_from_slice(&data);
    }
    bank
}

fn ordinary_search() -> Search {
    Search {
        player_count: 1,
        average_rank: 10,
        language: 0,
        quality_first: 0,
        quality_second: -1,
        minimum_rank: None,
        maximum_rank: None,
        own_address: None,
        clan: None,
    }
}

#[test]
fn all_original_banks_preserve_complete_order_opaque_bytes_and_cache() {
    let fixture = fixture();
    let cases = fixture["banks"].as_array().unwrap();
    assert_eq!(cases.len(), 264);
    for (index, case) in cases.iter().enumerate() {
        let mut candidates = Candidates::new(
            Search::from_json(&case["search"]).unwrap(),
            Configuration::from_json(&case["config"]).unwrap(),
        )
        .unwrap();
        candidates.restore_cache(&bank(&case["previous"])).unwrap();
        assert_eq!(
            candidates.update(&bank(&case["bank"])).unwrap().as_slice(),
            bank(&case["expected"]),
            "bank {index}"
        );
        let cached = candidates.bank().to_vec();
        let changed = Configuration {
            rank_high: 9999,
            ..Configuration::default()
        };
        candidates.configure(ordinary_search(), changed).unwrap();
        assert_eq!(
            candidates.update(&cached).unwrap().as_slice(),
            cached,
            "byte cache {index}"
        );
    }
}

#[test]
fn original_admission_and_complete_selected_rows() {
    let fixture = fixture();
    let cases = fixture["admission"].as_array().unwrap();
    assert_eq!(cases.len(), 27);
    for (index, case) in cases.iter().enumerate() {
        let mut candidates = Candidates::new(
            Search::from_json(&case["search"]).unwrap(),
            Configuration::default(),
        )
        .unwrap();
        let mut input = vec![0; BANK_SIZE];
        let row = decode_hex(case["record"].as_str().unwrap());
        assert_eq!(row.len(), STRIDE);
        input[..STRIDE].copy_from_slice(&row);
        let result = candidates.update(&input).unwrap();
        let output = result.chunks_exact(STRIDE).find(|row| row[0] != 0).unwrap();
        assert_eq!(
            output[1] != 0,
            case["expected"].as_bool().unwrap(),
            "admission {index}"
        );
    }
    let cases = fixture["selection"].as_array().unwrap();
    assert_eq!(cases.len(), 124);
    for (index, case) in cases.iter().enumerate() {
        let mut candidates = Candidates::new(
            ordinary_search(),
            Configuration::from_json(&case["config"]).unwrap(),
        )
        .unwrap();
        let input = bank(&case["bank"]);
        candidates.restore_cache(&input).unwrap();
        let minimum = i32::try_from(case["minimum_score"].as_i64().unwrap()).unwrap();
        let elapsed = i32::try_from(case["elapsed_query_ms"].as_i64().unwrap()).unwrap();
        let selected = candidates.select(minimum, elapsed, case["active"].as_bool().unwrap());
        let id = selected
            .as_ref()
            .map(|row| i32::from_le_bytes(row[0xbc..0xc0].try_into().unwrap()));
        assert_eq!(
            id,
            case["expected_id"].as_i64().map(|n| n as i32),
            "selection {index}"
        );
        if let Some(row) = selected {
            assert!(
                input.chunks_exact(STRIDE).any(|original| original == row),
                "complete selected row {index}"
            );
        }
        assert!(candidates.select(minimum, 6000, false).is_none());
    }
}

#[test]
fn all_original_thresholds_and_rust_default_configuration() {
    let fixture = fixture();
    let cases = fixture["thresholds"].as_array().unwrap();
    assert_eq!(cases.len(), 100);
    for case in cases {
        assert_eq!(
            core::threshold(
                case["radius"].as_i64().unwrap() as i32,
                case["maximum_radius"].as_i64().unwrap() as i32,
                &Configuration::default()
            )
            .unwrap(),
            case["expected"].as_i64().unwrap() as i32
        );
    }
    // Every retained default field originated in the executable initializer.
    let original = &fixture["selection"][0]["config"];
    assert_eq!(
        Configuration::default(),
        Configuration::from_json(original).unwrap()
    );
    assert_eq!(
        Configuration::default(),
        Configuration::from_json(&Configuration::default().as_json()).unwrap()
    );
}

#[test]
fn invalid_records_do_not_publish_partly_scored_cache_and_queries_are_isolated() {
    let fixture = fixture();
    let case = &fixture["banks"][0];
    let mut first = Candidates::new(
        Search::from_json(&case["search"]).unwrap(),
        Configuration::from_json(&case["config"]).unwrap(),
    )
    .unwrap();
    let second = Candidates::new(ordinary_search(), Configuration::default()).unwrap();
    first.update(&bank(&case["bank"])).unwrap();
    let before = first.bank().to_vec();
    let mut invalid = vec![0; BANK_SIZE];
    invalid[0] = 1;
    invalid[1] = 1;
    invalid[0x70..0x74].copy_from_slice(&(-1_i32).to_le_bytes());
    invalid[0x5c..0x60].copy_from_slice(&128_i32.to_le_bytes());
    assert!(first.update(&invalid).is_err());
    assert_eq!(first.bank().as_slice(), before);
    assert!(second.bank().iter().all(|&n| n == 0));
    let invalid = Configuration {
        rank_radius: i32::MAX,
        ..Configuration::default()
    };
    assert!(first.configure(ordinary_search(), invalid).is_err());
    assert_eq!(first.bank().as_slice(), before);
    for source in [b"{}".as_slice(), b"[]", b"{\"search\":true}"] {
        assert!(core::request(source).is_err());
    }
    assert!(core::request(&vec![0; core::REQUEST_LIMIT + 1]).is_err());
}
