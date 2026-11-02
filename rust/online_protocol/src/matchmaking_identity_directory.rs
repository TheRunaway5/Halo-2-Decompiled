//! Canonical private identities and dual-provider adoption transactions.
//! Provider proof IO is a temporary trusted boundary; names never identify players.
use super::*;
use crate::security_policy::PLAYER_CAPACITY;
use sha2::{Digest, Sha256};

fn schema(c: &mut Cursor<'_>) -> Flow<Value> {
    c.batch(vec![
        statement("CREATE TABLE IF NOT EXISTS players(id TEXT PRIMARY KEY,provider TEXT NOT NULL,subject TEXT NOT NULL,UNIQUE(provider,subject))",json!([]),None),
        statement("CREATE TABLE IF NOT EXISTS playlist_ranks(player TEXT NOT NULL REFERENCES players(id),playlist TEXT NOT NULL,rank INTEGER NOT NULL CHECK(rank BETWEEN 1 AND 50),PRIMARY KEY(player,playlist))",json!([]),None),
        statement("CREATE TABLE IF NOT EXISTS player_profiles(player TEXT PRIMARY KEY REFERENCES players(id),screen_name TEXT NOT NULL,discord_username TEXT NOT NULL DEFAULT '')",json!([]),None),
        statement("CREATE UNIQUE INDEX IF NOT EXISTS discord_usernames ON player_profiles(discord_username) WHERE discord_username!=''",json!([]),None),
        statement("CREATE TABLE IF NOT EXISTS player_identities(provider TEXT NOT NULL,subject TEXT NOT NULL,player TEXT NOT NULL REFERENCES players(id),PRIMARY KEY(provider,subject),UNIQUE(player,provider))",json!([]),None),
        statement("INSERT OR IGNORE INTO player_identities SELECT provider,subject,id FROM players",json!([]),None),
        statement(&format!("CREATE TRIGGER IF NOT EXISTS player_capacity BEFORE INSERT ON players WHEN (SELECT count(*) FROM players)>={PLAYER_CAPACITY} BEGIN SELECT RAISE(ABORT,'Player capacity reached'); END"),json!([]),None),
    ])?;
    let columns = c.query("PRAGMA table_info(player_profiles)", json!([]))?;
    for (name, ddl) in [
        (
            "steam_name",
            "ALTER TABLE player_profiles ADD COLUMN steam_name TEXT NOT NULL DEFAULT ''",
        ),
        (
            "joined_at",
            "ALTER TABLE player_profiles ADD COLUMN joined_at INTEGER",
        ),
    ] {
        if !columns.iter().any(|row| row[1] == name) {
            c.write(ddl, json!([]), None)?;
        }
    }
    // Existing registration dates remain NULL; migration never invents one.
    friends::tag_schema(c)
}
fn owner(v: &Value) -> Flow<(&str, &str)> {
    let rows = v
        .as_array()
        .ok_or(Error(401, "Verified account identity required"))?;
    need(rows.len() == 2, 401, "Verified account identity required")?;
    Ok((m::text(&rows[0], 64)?, m::text(&rows[1], 4096)?))
}
fn username<'a>(provider: &str, v: &'a Value) -> Flow<&'a str> {
    let name = v
        .as_str()
        .ok_or(Error(403, "Invalid verified account name"))?;
    let valid = if provider == "steam" {
        name.len() <= 128 && name.chars().all(|c| c >= ' ' && c != '\x7f')
    } else {
        name.is_empty()
            || ((2..=32).contains(&name.len())
                && name
                    .bytes()
                    .all(|b| b.is_ascii_lowercase() || b.is_ascii_digit() || b"_.".contains(&b)))
    };
    need(valid, 403, "Invalid verified account name")?;
    Ok(name)
}
fn write_lock(c: &mut Cursor<'_>) -> Flow<()> {
    // Serialize identity lookup, quota checks and writes before any read can
    // acquire a stale deferred-transaction snapshot on a second connection.
    c.write("UPDATE players SET id=id WHERE 0", json!([]), None)?;
    Ok(())
}
fn allocate(c: &mut Cursor<'_>, provider: &str, subject: &str) -> Flow<String> {
    write_lock(c)?;
    let count = c
        .one("SELECT count(*) FROM players", json!([]))?
        .ok_or(Error(503, "Canonical player count unavailable"))?;
    need(
        m::integer(&count[0], 0, i64::MAX)? < (PLAYER_CAPACITY as i64),
        503,
        "Player capacity reached",
    )?;
    let who = c.random(
        &format!("canonical-player-{}", json!([provider, subject])),
        false,
    )?;
    c.batch(vec![
        statement(
            "INSERT INTO players VALUES(?,?,?)",
            json!([who, provider, subject]),
            Some(1),
        ),
        statement(
            "INSERT INTO player_identities VALUES(?,?,?)",
            json!([provider, subject, who]),
            Some(1),
        ),
    ])?;
    Ok(who)
}
fn registration_time(c: &mut Cursor<'_>, observed: i64) -> Flow<i64> {
    let floor = c
        .one("SELECT value FROM clock_floor WHERE id=1", json!([]))?
        .ok_or(Error(503, "Canonical registration clock unavailable"))?;
    let joined = observed.max(m::integer(&floor[0], 0, 9007199254650000)?);
    c.write(
        "UPDATE clock_floor SET value=? WHERE id=1",
        json!([joined]),
        Some(1),
    )?;
    Ok(joined)
}
fn resolve(c: &mut Cursor<'_>, body: &Value, now: i64) -> Flow<Value> {
    m::fields(body, &["owner", "screen_name", "username"])?;
    let (provider, subject) = owner(&body["owner"])?;
    let name = m::text(&body["screen_name"], 64)?;
    let username = username(provider, &body["username"])?;
    write_lock(c)?;
    let current = c.one(
        "SELECT player FROM player_identities WHERE provider=? AND subject=?",
        json!([provider, subject]),
    )?;
    let (who, joined) = match current {
        Some(row) => (txt(&row[0])?.to_owned(), Value::Null),
        None => (allocate(c, provider, subject)?, json!(now)),
    };
    let discord = if provider == "discord" { username } else { "" };
    let steam = if provider == "steam" { username } else { "" };
    if !discord.is_empty() {
        c.write(
            "UPDATE player_profiles SET discord_username='' WHERE discord_username=? AND player!=?",
            json!([discord, who]),
            None,
        )?;
    }
    c.write("INSERT INTO player_profiles(player,screen_name,discord_username,steam_name,joined_at) VALUES(?,?,?,?,?) ON CONFLICT(player) DO UPDATE SET screen_name=excluded.screen_name,discord_username=CASE WHEN excluded.discord_username!='' THEN excluded.discord_username ELSE player_profiles.discord_username END,steam_name=CASE WHEN excluded.steam_name!='' THEN excluded.steam_name ELSE player_profiles.steam_name END",json!([who,name,discord,steam,joined]),Some(1))?;
    friends::assign(c, &who)?;
    Ok(json!(who))
}
fn lookup(c: &mut Cursor<'_>, value: &Value) -> Flow<Value> {
    let (provider, subject) = owner(value)?;
    Ok(c.one(
        "SELECT player FROM player_identities WHERE provider=? AND subject=?",
        json!([provider, subject]),
    )?
    .map(|row| row[0].clone())
    .unwrap_or(Value::Null))
}
fn resolve_seats(c: &mut Cursor<'_>, body: &Value, now: i64) -> Flow<Value> {
    let seats = body
        .as_array()
        .ok_or(Error(400, "Canonical seat roster required"))?;
    need(
        (1..=4).contains(&seats.len()) && !seats[0].is_null(),
        400,
        "Canonical seat roster exceeds bounds",
    )?;
    let mut result = Vec::new();
    for seat in seats {
        result.push(if seat.is_null() {
            Value::Null
        } else {
            resolve(c, seat, now)?
        });
    }
    Ok(json!(result))
}
fn admission_identities(c: &mut Cursor<'_>, body: &Value) -> Flow<Value> {
    let owners = body
        .as_array()
        .ok_or(Error(400, "Canonical seat identities required"))?;
    need(
        (1..=4).contains(&owners.len()),
        400,
        "Canonical seat count exceeds bounds",
    )?;
    let mut result = Vec::new();
    for account in owners {
        if account.is_null() {
            result.push(Value::Null);
            continue;
        }
        let who = lookup(c, account)?;
        let separate = if let Some(who) = who.as_str() {
            c.party(who)?.is_some()
        } else {
            false
        };
        result.push(json!({"owner":account,"player":who,"separate_party":separate}));
    }
    Ok(json!(result))
}
fn primary(c: &mut Cursor<'_>, r: &Value, now: i64) -> Flow<(String, String)> {
    m::fields(
        &r["features"],
        &["header", "steam_enabled", "registration_now"],
    )?;
    need(
        r["features"]["steam_enabled"] == true,
        503,
        "Steam sign-in is not configured",
    )?;
    let header = r["features"]["header"]
        .as_str()
        .ok_or(Error(401, "Bearer credential required"))?;
    let token = header
        .strip_prefix("Bearer ")
        .ok_or(Error(401, "Bearer credential required"))?;
    need(
        token.starts_with("h2s_"),
        403,
        "Sign in with Steam before linking Discord",
    )?;
    let suffix = token.strip_prefix("h2s_").unwrap();
    need(
        suffix.len() == 43
            && suffix
                .bytes()
                .all(|b| b.is_ascii_alphanumeric() || b"_-".contains(&b)),
        401,
        "Steam identity rejected",
    )?;
    let hash = format!("{:x}", Sha256::digest(token.as_bytes()));
    let row = c
        .one(
            "SELECT subject,name FROM steam_login_sessions WHERE token_hash=? AND expires>?",
            json!([hash, now]),
        )?
        .ok_or(Error(401, "Steam sign-in expired"))?;
    Ok((
        m::text(&row[0], 4096)?.to_owned(),
        username("steam", &row[1])?.to_owned(),
    ))
}
fn quote(value: &str) -> String {
    format!("\"{}\"", value.replace('"', "\"\""))
}
fn sql_name(value: &Value) -> Flow<&str> {
    let name = value
        .as_str()
        .ok_or(Error(503, "SQLite schema name required"))?;
    need(
        !name.is_empty() && name.len() <= 4096 && !name.contains('\0'),
        503,
        "SQLite schema name exceeds bounds",
    )?;
    Ok(name)
}
fn persistent_references(c: &mut Cursor<'_>, who: &str) -> Flow<()> {
    let mut after = String::new();
    loop {
        let tables = c.query(
            "SELECT name FROM sqlite_master WHERE type='table' AND name>? ORDER BY name LIMIT 128",
            json!([after]),
        )?;
        if tables.is_empty() {
            break;
        }
        for row in &tables {
            let table = sql_name(&row[0])?;
            if [
                "player_profiles",
                "player_identities",
                "history_players",
                "player_friend_tags",
                "friend_cloud_outbox",
            ]
            .contains(&table)
            {
                continue;
            }
            let table_sql = quote(table);
            for fk in c.query(&format!("PRAGMA foreign_key_list({table_sql})"), json!([]))? {
                if fk[2] == "players" {
                    let column = quote(sql_name(&fk[3])?);
                    need(
                        c.one(
                            &format!("SELECT 1 FROM {table_sql} WHERE {column}=? LIMIT 1"),
                            json!([who]),
                        )?
                        .is_none(),
                        409,
                        "Steam account already has ONLINE data; a data migration is required",
                    )?;
                }
            }
        }
        after = sql_name(&tables.last().unwrap()[0])?.to_owned();
    }
    Ok(())
}
fn link(c: &mut Cursor<'_>, r: &Value, now: i64) -> Flow<Value> {
    need(
        c.transaction_owned,
        409,
        "Identity linking requires an owned SQL transaction",
    )?;
    m::fields(&r["body"], &["discord_token"])?;
    m::text(&r["body"]["discord_token"], 2048)?;
    write_lock(c)?;
    let (subject, steam_name) = primary(c, r, now)?;
    if r["mode"] == "link_preflight" {
        return Ok(Value::Null);
    }
    need(
        r["verified_owner"].is_object(),
        401,
        "Verified Discord identity required",
    )?;
    m::fields(&r["verified_owner"], &["provider", "subject", "username"])?;
    need(
        r["verified_owner"]["provider"] == "discord",
        401,
        "Verified Discord identity required",
    )?;
    let social = m::text(&r["verified_owner"]["subject"], 4096)?;
    let name = username("discord", &r["verified_owner"]["username"])?;
    need(!name.is_empty(), 403, "Invalid verified Discord username")?;
    let steam = c.one(
        "SELECT player FROM player_identities WHERE provider='steam' AND subject=?",
        json!([subject]),
    )?;
    let discord = c.one(
        "SELECT player FROM player_identities WHERE provider='discord' AND subject=?",
        json!([social]),
    )?;
    let old = steam
        .as_ref()
        .map(|r| txt(&r[0]).map(str::to_owned))
        .transpose()?;
    let target = discord
        .as_ref()
        .map(|r| txt(&r[0]).map(str::to_owned))
        .transpose()?;
    if old.is_some() && old == target {
        return Ok(json!({"state":"linked"}));
    }
    if let Some(target) = &target {
        need(
            c.one(
                "SELECT 1 FROM player_identities WHERE player=? AND provider='steam'",
                json!([target]),
            )?
            .is_none(),
            409,
            "This Discord account is already linked to another Steam account",
        )?;
    }
    if let Some(old) = &old {
        need(
            c.one(
                "SELECT 1 FROM player_identities WHERE player=? AND provider='discord'",
                json!([old]),
            )?
            .is_none(),
            409,
            "This Steam account is already linked to another Discord account",
        )?;
    }
    if let (Some(old), Some(_)) = (&old, &target) {
        let active = c.core(json!({"op":"session_identity_in_use","player":old}))?;
        need(
            active == false,
            409,
            "Leave matchmaking before linking accounts",
        )?;
        persistent_references(c, old)?;
    }
    let adopted = target.is_some() && old.is_some();
    let who = if let Some(target) = target {
        if let Some(old) = &old {
            c.batch(
                [
                    "friend_cloud_outbox",
                    "player_friend_tags",
                    "history_players",
                    "player_identities",
                    "player_profiles",
                    "players",
                ]
                .iter()
                .map(|table| {
                    statement(
                        &format!(
                            "DELETE FROM {table} WHERE {}=?",
                            if *table == "players" { "id" } else { "player" }
                        ),
                        json!([old]),
                        None,
                    )
                })
                .collect(),
            )?;
        }
        c.batch(vec![
            statement(
                "INSERT INTO player_identities VALUES('steam',?,?)",
                json!([subject, target]),
                Some(1),
            ),
            statement(
                "UPDATE player_profiles SET steam_name=? WHERE player=?",
                json!([steam_name, target]),
                Some(1),
            ),
        ])?;
        target
    } else {
        let who = if let Some(old) = &old {
            old.clone()
        } else {
            allocate(c, "steam", &subject)?
        };
        if old.is_none() {
            let observed = m::integer(&r["features"]["registration_now"], 0, 9007199254650000)?;
            let joined = registration_time(c, observed)?;
            c.batch(vec![
                statement("INSERT INTO player_profiles(player,screen_name,steam_name,joined_at) VALUES(?,?,?,?)",json!([who,name,steam_name,joined]),Some(1)),
            ])?;
        }
        c.batch(vec![
            statement("UPDATE player_profiles SET discord_username='' WHERE discord_username=? AND player!=?",json!([name,who]),None),
            statement("UPDATE player_profiles SET discord_username=? WHERE player=?",json!([name,who]),Some(1)),
            statement("INSERT INTO player_identities VALUES('discord',?,?)",json!([social,who]),Some(1)),
        ])?;
        who
    };
    friends::assign(c, &who)?;
    Ok(json!({"state":"linked","_native_identity_revoke":if adopted{old}else{None}}))
}
pub(super) fn operation(r: &Value, c: &mut Cursor<'_>) -> Flow<Value> {
    let mode = txt(&r["mode"])?;
    let now = m::integer(&r["wall"], 0, 9007199254650000)?;
    match mode {
        "schema" => schema(c),
        "lookup" => lookup(c, &r["body"]),
        "registration_clock" => Ok(json!(registration_time(c, now)?)),
        "admission_identities" => admission_identities(c, &r["body"]),
        "resolve" => resolve(c, &r["body"], now),
        "resolve_seats" => resolve_seats(c, &r["body"], now),
        "allocate" => {
            let (provider, subject) = owner(&r["body"])?;
            Ok(json!(allocate(c, provider, subject)?))
        }
        "link" | "link_preflight" => link(c, r, now),
        _ => Err(Error(400, "Unknown canonical identity operation").into()),
    }
}
