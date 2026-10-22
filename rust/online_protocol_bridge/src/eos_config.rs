//! Shared release bootstrap. Only the untrusted game client belongs here.
use crate::ffi;
use std::{collections::BTreeMap, path::{Path, PathBuf}};

type Error = Box<dyn std::error::Error>;
const SETTINGS: &[&str] = &[
    "H2_EOS_PRODUCT_ID", "H2_EOS_SANDBOX_ID", "H2_EOS_DEPLOYMENT_ID",
    "H2_EOS_CLIENT_ID", "H2_EOS_CLIENT_SECRET", "H2_EOS_CACHE_DIR",
    "H2_EOS_ENABLED", "H2_EOS_PROFILES",
];

fn parse(contents: &str) -> Result<BTreeMap<String, String>, Error> {
    if contents.len() > 16384 { return Err("EOS configuration is too large".into()); }
    let mut values = BTreeMap::new();
    for line in contents.lines().map(str::trim).filter(|s| !s.is_empty() && !s.starts_with('#')) {
        let (name, value) = line.split_once('=').ok_or("Invalid EOS configuration line")?;
        if !SETTINGS.contains(&name) || value.is_empty() || value.len() > 4096
            || value.bytes().any(|b| b < 32 || b == 127)
            || values.insert(name.to_owned(), value.to_owned()).is_some() {
            return Err("Invalid EOS configuration setting".into());
        }
    }
    Ok(values)
}

fn settings() -> Result<BTreeMap<String, String>, Error> {
    let explicit = std::env::var_os("H2_EOS_CONFIG");
    let path = match &explicit {
        Some(path) => PathBuf::from(path),
        None => std::env::current_exe()?.parent().ok_or("Missing executable directory")?.join("eos-client.env"),
    };
    let mut values = match std::fs::read_to_string(path) {
        Ok(contents) => parse(&contents)?,
        Err(error) if explicit.is_none() && error.kind() == std::io::ErrorKind::NotFound => BTreeMap::new(),
        Err(_) => return Err("Unable to read EOS client configuration".into()),
    };
    for &name in SETTINGS {
        if let Ok(value) = std::env::var(name) { values.insert(name.to_owned(), value); }
    }
    Ok(values)
}

fn flag(values: &BTreeMap<String, String>, name: &str) -> Result<bool, Error> {
    match values.get(name).map(String::as_str) {
        None | Some("0") => Ok(false), Some("1") => Ok(true),
        _ => Err(format!("Invalid {name}").into()),
    }
}

pub fn eos_runtime_options() -> Result<ffi::EosRuntimeOptions, Error> {
    let values = settings()?;
    let transport = match std::env::var("H2_ONLINE_TRANSPORT").ok().as_deref() {
        Some("eos") => true, Some("direct") => false, None => flag(&values, "H2_EOS_ENABLED")?,
        _ => return Err("Invalid H2_ONLINE_TRANSPORT".into()),
    };
    Ok(ffi::EosRuntimeOptions { transport, profiles: flag(&values, "H2_EOS_PROFILES")? })
}

fn cache_directory(values: &BTreeMap<String, String>) -> Result<PathBuf, Error> {
    if let Some(path) = values.get("H2_EOS_CACHE_DIR") { return Ok(PathBuf::from(path)); }
    #[cfg(target_os = "windows")]
    let base = std::env::var_os("LOCALAPPDATA").map(PathBuf::from);
    #[cfg(target_os = "macos")]
    let base = std::env::var_os("HOME").map(|p| PathBuf::from(p).join("Library/Caches"));
    #[cfg(not(any(target_os = "windows", target_os = "macos")))]
    let base = std::env::var_os("XDG_DATA_HOME").map(PathBuf::from)
        .or_else(|| std::env::var_os("HOME").map(|p| PathBuf::from(p).join(".local/share")));
    Ok(base.ok_or("Unable to locate persistent EOS device storage")?.join("halo2-native/eos"))
}

pub fn configuration() -> Result<ffi::EosConfig, Error> {
    let values = settings()?;
    let get = |name: &str| -> Result<String, Error> {
        let value = values.get(name).ok_or_else(|| format!("Missing {name}"))?;
        if value.is_empty() || value.len() > 4096 || value.bytes().any(|b| b < 32 || b == 127) {
            return Err(format!("Invalid {name}").into());
        }
        Ok(value.clone())
    };
    let cache = cache_directory(&values)?;
    if !cache.is_absolute() { return Err("EOS device storage must be an absolute persistent directory".into()); }
    let config = ffi::EosConfig {
        product: get("H2_EOS_PRODUCT_ID")?, sandbox: get("H2_EOS_SANDBOX_ID")?,
        deployment: get("H2_EOS_DEPLOYMENT_ID")?, client: get("H2_EOS_CLIENT_ID")?,
        secret: get("H2_EOS_CLIENT_SECRET")?, cache: cache.to_string_lossy().into_owned(),
    };
    for id in [&config.product, &config.sandbox, &config.deployment] {
        if id.len() != 32 || !id.bytes().all(|b| b.is_ascii_hexdigit() && !b.is_ascii_uppercase()) {
            return Err("Invalid EOS product identifiers".into());
        }
    }
    std::fs::create_dir_all(Path::new(&config.cache))?;
    Ok(config)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn refuses_trusted_credentials_duplicates_and_invalid_release_flags() {
        for input in ["H2_EOS_SERVER_CLIENT_SECRET=never", "H2_EOS_CLIENT_ID=a\nH2_EOS_CLIENT_ID=b", "H2_EOS_ENABLED=1\0"] {
            assert!(parse(input).is_err());
        }
        assert!(flag(&parse("H2_EOS_ENABLED=eos").unwrap(), "H2_EOS_ENABLED").is_err());
        assert!(flag(&parse("H2_EOS_ENABLED=1").unwrap(), "H2_EOS_ENABLED").unwrap());
        assert!(!flag(&BTreeMap::new(), "H2_EOS_ENABLED").unwrap());
    }
    #[test]
    fn settings_are_data_not_shell_input() {
        let values = parse("# shared untrusted client\nH2_EOS_CLIENT_SECRET=$(never-execute)=literal\n").unwrap();
        assert_eq!(values["H2_EOS_CLIENT_SECRET"], "$(never-execute)=literal");
    }
}
