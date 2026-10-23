//! OS-protected, profile-scoped Steam handoff credentials. No plaintext fallback.
use sha2::{Digest, Sha256};

fn entry(origin: &str, namespace: &str, profile: &str) -> Result<keyring::Entry, String> {
    if namespace.is_empty() || profile.is_empty() || profile.len() > 256 {
        return Err("Select a local profile before remembering sign-in.".into());
    }
    let mut digest = Sha256::new();
    for value in [origin, namespace, profile] {
        digest.update((value.len() as u64).to_be_bytes());
        digest.update(value.as_bytes());
    }
    let account = format!("{:x}", digest.finalize());
    keyring::Entry::new("Halo2Forever.Steam.Refresh.v1", &account)
        .map_err(|_| "Secure sign-in storage is unavailable.".into())
}
fn valid(value: &str) -> bool {
    value.len() == 47 && value.starts_with("h2r_") && value[4..].bytes()
        .all(|c| c.is_ascii_alphanumeric() || c == b'_' || c == b'-')
}
pub fn steam_saved_login(origin: &str, namespace: &str, profile: &str) -> Result<String, String> {
    match entry(origin, namespace, profile)?.get_password() {
        Ok(value) if valid(&value) => Ok(value),
        Ok(_) | Err(keyring::Error::NoEntry) => Ok(String::new()),
        Err(_) => Err("Secure sign-in storage is unavailable.".into()),
    }
}
pub fn steam_save_login(origin: &str, namespace: &str, profile: &str, value: &str) -> Result<(), String> {
    if !valid(value) { return Err("Invalid saved sign-in credential.".into()); }
    entry(origin, namespace, profile)?.set_password(value)
        .map_err(|_| "Could not remember sign-in in secure storage.".into())
}
pub fn steam_forget_login(origin: &str, namespace: &str, profile: &str) -> Result<(), String> {
    match entry(origin, namespace, profile)?.delete_credential() {
        Ok(()) | Err(keyring::Error::NoEntry) => Ok(()),
        Err(_) => Err("Could not remove the saved sign-in.".into()),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    #[ignore = "requires the host OS credential service"]
    fn secure_store_profile_isolation_and_forget() {
        let namespace = format!("test-{}-{:?}",std::process::id(),std::time::SystemTime::now());
        let origin="https://credential-test.invalid";
        let secret=format!("h2r_{}","a".repeat(43));
        struct Cleanup<'a>(&'a str,&'a str);
        impl Drop for Cleanup<'_> {fn drop(&mut self){let _=steam_forget_login(self.0,self.1,"first");}}
        let _cleanup=Cleanup(origin,&namespace);
        assert!(steam_saved_login(origin,&namespace,"first").unwrap().is_empty());
        steam_save_login(origin,&namespace,"first",&secret).unwrap();
        assert!(steam_saved_login(origin,&namespace,"first").unwrap()==secret);
        assert!(steam_saved_login(origin,&namespace,"second").unwrap().is_empty());
        assert!(steam_saved_login("https://different.invalid",&namespace,"first").unwrap().is_empty());
        steam_forget_login(origin,&namespace,"first").unwrap();
        assert!(steam_saved_login(origin,&namespace,"first").unwrap().is_empty());
    }
}
