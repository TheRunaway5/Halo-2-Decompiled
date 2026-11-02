//! JSON-lines replay tool; modern C++ compatibility, not an Xbox Live oracle.
use halo2_online_protocol::matchmaking_account_client as account;
use serde_json::{Value, json};
use std::io::{self, BufRead};
fn replay(input: &Value) -> account::Result<Value> {
    let kind = input["kind"].as_str().ok_or(account::Error::Invalid)?;
    let root = &input["root"];
    if kind == "countdown" {
        return account::countdown(root);
    }
    let initial = account::empty_snapshot();
    let (snapshot, keys): (Value, &[&str]) = match kind {
        "party" => (
            account::party(root, &initial)?,
            &["party", "party_invitations"],
        ),
        "social" => (account::social(root, &initial)?, &["clan", "invitations"]),
        "friends" => (
            account::friends(root, &initial)?,
            &[
                "friend_tag",
                "friends",
                "friends_available",
                "friends_message",
                "friend_search",
            ],
        ),
        _ => return Err(account::Error::Invalid),
    };
    Ok(Value::Object(
        keys.iter()
            .map(|k| ((*k).into(), snapshot[*k].clone()))
            .collect(),
    ))
}
fn main() {
    for line in io::stdin().lock().lines() {
        let result = line
            .ok()
            .and_then(|s| serde_json::from_str::<Value>(&s).ok())
            .and_then(|v| replay(&v).ok());
        println!(
            "{}",
            match result {
                Some(value) => json!({"accepted":true,"value":value}),
                None => json!({"accepted":false}),
            }
        );
    }
}
