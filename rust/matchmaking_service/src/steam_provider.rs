//! Fixed Valve HTTPS endpoints; this IO produces private verified receipts.
use super::*;
use curl::easy::{Easy, List};
use std::time::Duration;
pub fn verify(plan: &Plan, key: &str) -> Result<Owner> {
    let fields = plan.values.iter().map(|(k, v)| {
        (
            k.as_str(),
            if k == "openid.mode" {
                "check_authentication"
            } else {
                v.as_str()
            },
        )
    });
    let body = form_urlencoded::Serializer::new(String::new())
        .extend_pairs(fields)
        .finish();
    let raw = request(OP, Some(body.as_bytes()), 4096)?;
    let text =
        std::str::from_utf8(&raw).map_err(|_| Error(502, "Invalid Steam verification response"))?;
    let mut values = BTreeMap::new();
    for line in text.lines() {
        let (k, v) = line
            .split_once(':')
            .ok_or(Error(502, "Invalid Steam verification response"))?;
        need(
            values.insert(k, v).is_none(),
            502,
            "Invalid Steam verification response",
        )?;
    }
    need(
        values.get("ns") == Some(&NS) && values.get("is_valid") == Some(&"true"),
        401,
        "Steam signature rejected",
    )?;
    let name = if key.is_empty() {
        "Steam Player".into()
    } else {
        let url = format!(
            "https://api.steampowered.com/ISteamUser/GetPlayerSummaries/v2/?{}",
            form_urlencoded::Serializer::new(String::new())
                .extend_pairs([("key", key), ("steamids", plan.subject.as_str())])
                .finish()
        );
        let bytes = request(&url, None, 16384)?;
        let value = halo2_online_protocol::matchmaking_json::parse(&bytes, 16384)
            .map_err(|_| Error(502, "Invalid Steam profile response"))?;
        let rows = value["response"]["players"]
            .as_array()
            .ok_or(Error(502, "Invalid Steam profile response"))?;
        need(
            rows.len() == 1 && rows[0]["steamid"] == plan.subject,
            502,
            "Steam returned a different profile",
        )?;
        m::text(&rows[0]["personaname"], 128)
            .map_err(|_| Error(502, "Invalid Steam profile name"))?
            .to_owned()
    };
    Owner::stored(plan.subject.clone(), name)
}
fn request(url: &str, body: Option<&[u8]>, limit: usize) -> Result<Vec<u8>> {
    let fail = |_| Error(502, "Steam verification unavailable");
    let mut easy = Easy::new();
    easy.url(url).map_err(fail)?;
    easy.proxy("").map_err(fail)?;
    easy.follow_location(false).map_err(fail)?;
    easy.ssl_verify_peer(true).map_err(fail)?;
    easy.ssl_verify_host(true).map_err(fail)?;
    easy.connect_timeout(Duration::from_secs(2)).map_err(fail)?;
    easy.timeout(Duration::from_secs(5)).map_err(fail)?;
    easy.accept_encoding("identity").map_err(fail)?;
    easy.http_content_decoding(false).map_err(fail)?;
    if let Some(body) = body {
        easy.post(true).map_err(fail)?;
        easy.post_fields_copy(body).map_err(fail)?;
        let mut headers = List::new();
        headers
            .append("Content-Type: application/x-www-form-urlencoded")
            .map_err(fail)?;
        easy.http_headers(headers).map_err(fail)?;
    }
    let mut bytes = Vec::new();
    let mut encoded = false;
    {
        let mut transfer = easy.transfer();
        transfer
            .header_function(|line| {
                if line
                    .get(..17)
                    .is_some_and(|b| b.eq_ignore_ascii_case(b"content-encoding:"))
                {
                    encoded = !line[17..].iter().all(u8::is_ascii_whitespace);
                }
                true
            })
            .map_err(fail)?;
        transfer
            .write_function(|data| {
                if bytes.len() + data.len() > limit {
                    return Ok(0);
                }
                bytes.extend_from_slice(data);
                Ok(data.len())
            })
            .map_err(fail)?;
        transfer.perform().map_err(fail)?;
    }
    need(
        easy.response_code().map_err(fail)? == 200 && !encoded,
        502,
        "Steam verification unavailable",
    )?;
    Ok(bytes)
}
