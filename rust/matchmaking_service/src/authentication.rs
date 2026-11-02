//! Verified provider receipts are private values, never deserialized from HTTP.
use curl::easy::{Easy, List};
use halo2_online_protocol::{
    matchmaking_json,
    matchmaking_model::{Error, Result},
};
use serde_json::Value;
use sha2::{Digest, Sha256};
use std::time::Duration;

pub struct Identity {
    pub provider: String,
    pub subject: String,
    pub username: String,
    pub steam_digest: Option<String>,
}
pub fn bearer(header: &str) -> Result<&str> {
    let token = header
        .strip_prefix("Bearer ")
        .ok_or(Error(401, "Bearer credential required"))?;
    if token.is_empty() || token.len() > 2048 || token.chars().any(|c| c < ' ' || c == '\x7f') {
        return Err(Error(401, "Invalid bearer token"));
    }
    if token.starts_with("h2r_") {
        return Err(Error(
            401,
            "Saved sign-in is only valid for session renewal",
        ));
    }
    Ok(token)
}
pub fn steam_digest(token: &str) -> Result<String> {
    let suffix = token
        .strip_prefix("h2s_")
        .ok_or(Error(401, "Steam identity rejected"))?;
    if suffix.len() != 43
        || !suffix
            .bytes()
            .all(|c| c.is_ascii_alphanumeric() || b"_-".contains(&c))
    {
        return Err(Error(401, "Steam identity rejected"));
    }
    Ok(format!("{:x}", Sha256::digest(token.as_bytes())))
}
fn get(url: &str, token: &str) -> Result<Value> {
    let mut easy = Easy::new();
    let fail = |_| Error(502, "Identity provider unavailable");
    easy.url(url).map_err(fail)?;
    easy.proxy("").map_err(fail)?;
    easy.follow_location(false).map_err(fail)?;
    easy.ssl_verify_peer(true).map_err(fail)?;
    easy.ssl_verify_host(true).map_err(fail)?;
    easy.connect_timeout(Duration::from_secs(2)).map_err(fail)?;
    easy.timeout(Duration::from_secs(5)).map_err(fail)?;
    easy.accept_encoding("identity").map_err(fail)?;
    easy.http_content_decoding(false).map_err(fail)?;
    let mut headers = List::new();
    headers
        .append(&format!("Authorization: Bearer {token}"))
        .map_err(fail)?;
    easy.http_headers(headers).map_err(fail)?;
    let mut bytes = Vec::new();
    let mut encoded = false;
    {
        let mut transfer = easy.transfer();
        transfer
            .header_function(|line| {
                if line
                    .get(..17)
                    .is_some_and(|v| v.eq_ignore_ascii_case(b"content-encoding:"))
                {
                    encoded = !line[17..].iter().all(u8::is_ascii_whitespace);
                }
                true
            })
            .map_err(fail)?;
        transfer
            .write_function(|data| {
                if bytes.len() + data.len() > 16384 {
                    return Ok(0);
                }
                bytes.extend_from_slice(data);
                Ok(data.len())
            })
            .map_err(fail)?;
        transfer.perform().map_err(fail)?;
    }
    if easy.response_code().map_err(fail)? != 200 {
        return Err(Error(401, "Discord authorization rejected"));
    }
    if encoded {
        return Err(Error(502, "Compressed upstream responses are not accepted"));
    }
    matchmaking_json::parse(&bytes, 16384)
        .map_err(|_| Error(502, "Invalid identity provider response"))
}
pub fn discord(token: &str, client_id: &str) -> Result<Identity> {
    let authorization = get("https://discord.com/api/v10/oauth2/@me", token)?;
    if authorization["application"]["id"] != client_id
        || !authorization["scopes"]
            .as_array()
            .is_some_and(|s| s.iter().any(|v| v == "identify"))
    {
        return Err(Error(403, "Wrong Discord application or scope"));
    }
    let user = get("https://discord.com/api/v10/users/@me", token)?;
    let id = user["id"]
        .as_str()
        .ok_or(Error(403, "Invalid Discord owner"))?;
    if !(17..=20).contains(&id.len())
        || !id.bytes().all(|b| b.is_ascii_digit())
        || user["bot"] == true
    {
        return Err(Error(403, "Invalid Discord owner"));
    }
    let name = user["username"]
        .as_str()
        .ok_or(Error(403, "Invalid Discord username"))?;
    if !(2..=32).contains(&name.len())
        || !name
            .bytes()
            .all(|b| b.is_ascii_lowercase() || b.is_ascii_digit() || b"_.".contains(&b))
    {
        return Err(Error(403, "Invalid Discord username"));
    }
    Ok(Identity {
        provider: "discord".into(),
        subject: id.into(),
        username: name.into(),
        steam_digest: None,
    })
}
