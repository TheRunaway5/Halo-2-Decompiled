//! Fixed provider GET IO. The shared Rust request owns interpretation/policy.
use curl::easy::Easy;
use halo2_online_protocol::matchmaking_model::{Error, Result};
use serde_json::{Value, json};
use std::time::Duration;

pub fn get(event: &Value, key: &str) -> Result<Value> {
    let url = event["url"]
        .as_str()
        .ok_or(Error(503, "Native Steam URL required"))?;
    if ![
        "https://api.steampowered.com/ISteamUser/GetFriendList/v1/",
        "https://api.steampowered.com/ISteamUser/GetPlayerSummaries/v2/",
    ]
    .contains(&url)
        || event["method"] != "GET"
        || event["allow_redirects"] != false
        || event["read_limit"] != 1048576
    {
        return Err(Error(503, "Invalid native Steam IO plan"));
    }
    let params = event["params"]
        .as_object()
        .ok_or(Error(503, "Native Steam parameters required"))?;
    if params.len() > 2 || params.contains_key("key") {
        return Err(Error(503, "Invalid native Steam parameters"));
    }
    let mut encoded = form_urlencoded::Serializer::new(String::new());
    for (name, value) in params {
        encoded.append_pair(
            name,
            value
                .as_str()
                .ok_or(Error(503, "Invalid native Steam parameter"))?,
        );
    }
    encoded.append_pair("key", key);
    let mut easy = Easy::new();
    let fail = |_| Error(502, "Steam provider unavailable");
    easy.url(&format!("{url}?{}", encoded.finish()))
        .map_err(fail)?;
    easy.proxy("").map_err(fail)?;
    easy.follow_location(false).map_err(fail)?;
    easy.ssl_verify_peer(true).map_err(fail)?;
    easy.ssl_verify_host(true).map_err(fail)?;
    easy.connect_timeout(Duration::from_secs(2)).map_err(fail)?;
    easy.timeout(Duration::from_secs(5)).map_err(fail)?;
    easy.accept_encoding("identity").map_err(fail)?;
    easy.http_content_decoding(false).map_err(fail)?;
    let mut bytes = Vec::new();
    let mut encoding = String::new();
    let mut length = None;
    let performed = {
        let mut transfer = easy.transfer();
        transfer
            .header_function(|line| {
                if line.starts_with(b"HTTP/") {
                    encoding.clear();
                    length = None;
                }
                if let Some(index) = line.iter().position(|c| *c == b':') {
                    let (name, value) = (&line[..index], &line[index + 1..]);
                    if name.eq_ignore_ascii_case(b"content-encoding") {
                        encoding = String::from_utf8_lossy(value).trim().to_owned();
                    }
                    if name.eq_ignore_ascii_case(b"content-length") {
                        length = Some(
                            String::from_utf8_lossy(value)
                                .trim()
                                .parse::<u64>()
                                .unwrap_or(u64::MAX),
                        );
                    }
                }
                true
            })
            .map_err(fail)?;
        transfer
            .write_function(|chunk| {
                if bytes.len() + chunk.len() > 1048576 {
                    return Ok(0);
                }
                bytes.extend_from_slice(chunk);
                Ok(chunk.len())
            })
            .map_err(fail)?;
        transfer.perform()
    };
    if performed.is_err() {
        return Ok(json!({"status":0,"encoding":"","length":null,"body":""}));
    }
    Ok(
        json!({"status":easy.response_code().map_err(fail)?,"encoding":encoding,"length":length,
        "body":bytes.iter().map(|b|format!("{b:02x}")).collect::<String>()}),
    )
}
