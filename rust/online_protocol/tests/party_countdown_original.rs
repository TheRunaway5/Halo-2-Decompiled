//! Complete unchanged original writer and bounded timer/press controls.
use halo2_online_protocol::party_countdown::{self, Parameter};
use serde_json::{Value, json};
use sha2::{Digest, Sha256};

fn requester(value: &Value) -> [u8; 12] {
    let text = value.as_str().unwrap();
    assert_eq!(text.len(), 24);
    std::array::from_fn(|i| u8::from_str_radix(&text[2 * i..2 * i + 2], 16).unwrap())
}

#[test]
fn unchanged_retail_and_tu15_complete_parameter_writers() {
    let bytes =
        include_bytes!("../../../tests/fixtures/matchmaking_countdown_writer_original.json");
    assert_eq!(
        format!("{:x}", Sha256::digest(bytes)),
        "0fb4d1f349e341a834ecb5cab320cc8ca9bd1a10e421c4a97d3c20d178d32d92"
    );
    let fixture: Value = serde_json::from_slice(bytes).unwrap();
    for (epoch, pin) in [
        (
            "retail",
            "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d",
        ),
        (
            "tu15",
            "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
        ),
    ] {
        assert_eq!(fixture["epochs"][epoch]["xbe_sha256"], pin);
        assert_eq!(fixture["epochs"][epoch]["calls"], 768);
        assert_eq!(
            fixture["epochs"][epoch]["covered_instructions"],
            fixture["epochs"][epoch]["instructions"]
        );
        assert_eq!(
            fixture["epochs"][epoch]["all_executed_instruction_bytes_verified"],
            true
        );
    }
    let cases = fixture["cases"].as_array().unwrap();
    assert_eq!(cases.len(), 768);
    for (index, case) in cases.iter().enumerate() {
        let input = &case["input"];
        let mut value = Parameter {
            active: input["active"].as_u64().unwrap() as u8,
            seconds: input["seconds"].as_i64().unwrap() as i32,
            mode: input["mode"].as_i64().unwrap() as i32,
            watermark: input["watermark"].as_i64().unwrap() as i32,
            updates: input["updates"].as_u64().unwrap() as u32,
            requester: requester(&input["requester"]),
        };
        let changed = value.publish(
            input["leader"].as_bool().unwrap(),
            input["start"].as_u64().unwrap() as u8,
            input["next_seconds"].as_i64().unwrap() as i32,
            input["next_mode"].as_i64().unwrap() as i32,
            &requester(&input["next_requester"]),
        );
        let requester = value
            .requester
            .iter()
            .map(|b| format!("{b:02x}"))
            .collect::<String>();
        assert_eq!(
            json!({"active":value.active,"seconds":value.seconds,"mode":value.mode,
            "watermark":value.watermark,"updates":value.updates,"requester":requester,"changed":changed}),
            case["expected"],
            "original pair {index}: {input}"
        );
    }
}

#[test]
fn unchanged_tu15_press_and_timer_controls() {
    let fixture: Value = serde_json::from_str(include_str!(
        "../../../tests/fixtures/tu15_party_countdown_original.json"
    ))
    .unwrap();
    assert_eq!(
        fixture["source_sha256"],
        "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"
    );
    assert_eq!(fixture["cases"].as_array().unwrap().len(), 40);
    for row in fixture["cases"].as_array().unwrap() {
        match row["kind"].as_str().unwrap() {
            "timer" => assert_eq!(
                party_countdown::tick(
                    row["seconds"].as_i64().unwrap() as i32,
                    row["elapsed"].as_u64().unwrap() as u32
                ),
                row["value"].as_i64().unwrap() as i32,
                "{row}"
            ),
            "press" => assert_eq!(
                party_countdown::press(row["seconds"].as_i64().map(|n| n as i32).unwrap_or(-1)),
                row["value"].as_i64().unwrap() as i32,
                "{row}"
            ),
            "parameter" => {}
            _ => panic!("Unknown original control {row}"),
        }
    }
}
