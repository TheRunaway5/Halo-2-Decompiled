//! Pinned native matchmaking endpoints over actual URI-parser observations.
//! URI parsing is IO; authorization of the resulting origin is Rust policy.
#[derive(Clone, Debug, Default)]
pub struct UrlParts {
    pub parsed: bool,
    pub normalized: String,
    pub scheme: String,
    pub host: String,
    pub user: String,
    pub password: String,
    pub options: String,
    pub path: String,
}

pub fn pinned_origin(raw: &str, parts: &UrlParts, loopback: bool) -> Result<String, &'static str> {
    if raw.is_empty()
        || raw.len() > 2048
        || raw.bytes().any(|v| v <= 32 || v == 127 || v == b'\\')
        || raw.contains(['@', '?', '#'])
        || !parts.parsed
        || parts.host.is_empty()
        || !parts.user.is_empty()
        || !parts.password.is_empty()
        || !parts.options.is_empty()
        || parts.path != "/"
        || parts.normalized.is_empty()
        || !(parts.scheme == "https"
            || (loopback
                && parts.scheme == "http"
                && matches!(parts.host.as_str(), "127.0.0.1" | "[::1]")))
    {
        return Err("Invalid matchmaking origin");
    }
    Ok(parts.normalized.trim_end_matches('/').to_owned())
}
