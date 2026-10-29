use halo2_online_protocol::matchmaking_ratings::{self, RankMap, SearchEnvelope};
use serde_json::Value;

fn fixture() -> Value {
    serde_json::from_str(include_str!(
        "../../../tests/fixtures/matchmaking_ratings_original.json"
    ))
    .unwrap()
}

fn integers(value: &Value) -> Vec<i32> {
    value
        .as_array()
        .unwrap()
        .iter()
        .map(|v| v.as_i64().unwrap() as i32)
        .collect()
}

fn map(fixture: &Value, name: &Value) -> RankMap {
    let values: Vec<u8> = integers(&fixture["maps"][name.as_str().unwrap()])
        .into_iter()
        .map(|v| v as u8)
        .collect();
    RankMap::from_slice(&values).unwrap()
}

#[test]
fn original_raw_effective_and_advertisement_outputs() {
    let f = fixture();
    assert_eq!(
        f["original_sha256"],
        "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d"
    );
    assert_eq!(f["rosters"].as_array().unwrap().len(), 300);
    for case in f["rosters"].as_array().unwrap() {
        let raw = integers(&case["raw"]);
        let flags: Vec<u32> = integers(&case["flags"])
            .into_iter()
            .map(|n| n as u32)
            .collect();
        let stats = matchmaking_ratings::raw_statistics(&raw, &flags).unwrap();
        assert_eq!(
            [
                stats.average,
                stats.maximum,
                stats.minimum,
                stats.rated_count as i32
            ],
            integers(&case["statistics"]).as_slice(),
            "original raw producer: {case}"
        );
        let table = map(&f, &case["map"]);
        let adjusted = matchmaking_ratings::adjusted_ranks(
            &raw,
            &integers(&case["groups"]),
            case["catalog_index"].as_u64().unwrap() as usize,
            &table,
        )
        .unwrap();
        assert_eq!(
            adjusted,
            integers(&case["adjusted"]),
            "original effective producer: {case}"
        );
        assert_eq!(
            [
                stats.average,
                stats.maximum,
                stats.minimum,
                table
                    .advertised_minimum(stats.minimum, stats.maximum)
                    .unwrap()
            ],
            integers(&case["advertisement"]).as_slice(),
            "original advertisement: {case}"
        );
    }
}

#[test]
fn original_inverse_search_initialization_and_queries() {
    let f = fixture();
    assert_eq!(f["inverses"].as_array().unwrap().len(), 516);
    for case in f["inverses"].as_array().unwrap() {
        let actual = map(&f, &case["map"])
            .inverse(case["rank"].as_i64().unwrap() as i32)
            .unwrap();
        assert_eq!(
            actual,
            case["actual"].as_i64().unwrap() as i32,
            "original inverse: {case}"
        );
    }
    assert_eq!(f["searches"].as_array().unwrap().len(), 40);
    for case in f["searches"].as_array().unwrap() {
        let envelope = SearchEnvelope::new(
            case["minimum"].as_i64().unwrap() as i32,
            case["maximum"].as_i64().unwrap() as i32,
            &map(&f, &case["map"]),
        )
        .unwrap();
        assert_eq!(
            [
                envelope.inverse_maximum,
                envelope.mapped_minimum,
                envelope.ceiling,
                envelope.maximum_radius,
                envelope.initial_radius
            ],
            integers(&case["initialization"]).as_slice()
        );
        for query in case["queries"].as_array().unwrap() {
            assert_eq!(
                envelope
                    .bounds(query["radius"].as_i64().unwrap() as i32)
                    .unwrap(),
                (
                    query["minimum"].as_i64().unwrap() as i32,
                    query["maximum"].as_i64().unwrap() as i32
                )
            );
        }
    }
    for case in f["defaults"].as_array().unwrap() {
        assert_eq!(
            RankMap::retail_launch().values().map(i32::from).as_slice(),
            integers(&case["table"])
        );
    }
}

#[test]
fn distinct_raw_and_effective_owners_and_none_sentinels() {
    let plus_one: Vec<u8> = (0..128).map(|i| (i + 1).min(127)).collect();
    let map = RankMap::from_slice(&plus_one).unwrap();
    let raw = [1, 2, 8];
    assert_eq!(
        matchmaking_ratings::adjusted_ranks(&raw, &[0, 0, 0], 1, &map).unwrap(),
        [7, 7, 8]
    );
    assert_eq!(
        matchmaking_ratings::adjusted_ranks(&raw, &[0, 0, 0], 0, &map).unwrap(),
        raw
    );
    let stats = matchmaking_ratings::raw_statistics(&[20, 20, 40], &[0, 1, 0]).unwrap();
    assert_eq!(
        (
            stats.average,
            stats.minimum,
            stats.maximum,
            stats.rated_count
        ),
        (30, 20, 40, 2)
    );
    let absent = matchmaking_ratings::raw_statistics(&[1, 127], &[1, 2]).unwrap();
    assert_eq!(
        (
            absent.average,
            absent.minimum,
            absent.maximum,
            absent.rated_count
        ),
        (-1, -1, -1, 0)
    );
    assert_eq!(map.inverse(-1), Some(-1));
    assert_eq!(map.advertised_minimum(-1, -1), Some(-1));
}

#[test]
fn bounded_carriers_and_signed_rank_maps_are_checked() {
    let map = RankMap::retail_launch();
    assert!(RankMap::from_slice(&[0; 128]).is_none());
    assert!(RankMap::from_slice(&[255; 128]).is_none());
    assert!(RankMap::from_header(&[0; 0x613]).is_none());
    assert!(matchmaking_ratings::raw_statistics(&[128], &[0]).is_none());
    assert!(matchmaking_ratings::raw_statistics(&[0; 17], &[0; 17]).is_none());
    assert!(matchmaking_ratings::adjusted_ranks(&[0], &[16], 1, &map).is_none());
    assert!(matchmaking_ratings::adjusted_ranks(&[0], &[0], 16, &map).is_none());
    assert!(SearchEnvelope::new(-1, 0, &map).is_none());
    assert!(
        SearchEnvelope::new(20, 20, &map)
            .unwrap()
            .bounds(-1)
            .is_none()
    );
}
