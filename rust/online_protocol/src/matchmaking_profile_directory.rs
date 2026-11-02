//! Native current-profile/customization SQL and career aggregates. Modern
//! account API policy; the nine-byte profile and sixteen-byte PCR appearance
//! remain separate source formats. Accepted-result namespaces are not widened.
use super::*;

const SCHEMA: &str = r#"
CREATE TABLE IF NOT EXISTS eos_players(player TEXT PRIMARY KEY REFERENCES players(id),puid TEXT NOT NULL UNIQUE CHECK(length(puid)=32));
CREATE TABLE IF NOT EXISTS eos_profile_outbox(player TEXT PRIMARY KEY REFERENCES players(id),revision INTEGER NOT NULL);
CREATE TABLE IF NOT EXISTS player_customization(player TEXT PRIMARY KEY REFERENCES players(id),appearance BLOB NOT NULL CHECK(length(appearance)=9),service_tag TEXT NOT NULL DEFAULT '',updated_at INTEGER NOT NULL,revision INTEGER NOT NULL);
"#;
fn cells(row: &Value, count: usize) -> Flow<&[Value]> {
    let row = row
        .as_array()
        .ok_or(Error(503, "Profile database row required"))?;
    need(row.len() == count, 503, "Profile database columns differ")?;
    Ok(row)
}
fn raw_appearance(value: &Value) -> Result<Vec<u8>> {
    let bytes = m::hex(value, 9)?;
    m::check(bytes[4] <= 1, "Player species must be Spartan or Elite")?;
    Ok(bytes)
}
fn decoded(value: &Value) -> Flow<Value> {
    if value.is_null() {
        return Ok(Value::Null);
    }
    m::fields(value, &["blob"])?;
    let bytes = raw_appearance(&value["blob"])?;
    Ok(
        json!({"format":"halo2-profile-appearance-v1","bytes":value["blob"],
        "species":if bytes[4]==1{"elite"}else{"spartan"},"colors":bytes[..4].iter().map(|b|i32::from(*b as i8)).collect::<Vec<_>>(),
        "emblem":{"foreground":bytes[5],"background":bytes[6],"flags":bytes[7],"reserved":bytes[8]}}),
    )
}
fn aggregate(c: &mut Cursor<'_>, who: &str) -> Flow<Value> {
    let summary=c.one("SELECT count(*),coalesce(sum(placement=0),0),coalesce(sum(kills),0),coalesce(sum(deaths),0),coalesce(sum(assists),0) FROM match_result_players WHERE player=?",json!([who]))?.ok_or(Error(503,"Profile summary unavailable"))?;
    let accuracy=c.one("SELECT coalesce(sum(shots),0),coalesce(sum(hits),0),count(*) FROM carnage_player_accuracy WHERE player=?",json!([who]))?.ok_or(Error(503,"Profile accuracy unavailable"))?;
    let a = cells(&summary, 5)?;
    let b = cells(&accuracy, 3)?;
    let value = json!({"matches":a[0],"wins":a[1],"kills":a[2],"deaths":a[3],"assists":a[4],
        "shots":b[0],"hits":b[1],"accuracy_matches":b[2],"accuracy_complete":b[2]==a[0]});
    crate::matchmaking_profiles::statistics(&value)?;
    Ok(value)
}
pub(super) fn statistics(c: &mut Cursor<'_>, who: &str) -> Flow<Value> {
    // Never read/publish shared cached data inside somebody else's unfinished
    // transaction. Such a view must observe its own writes without poisoning
    // a cache if the outer caller later rolls back.
    if !c.profiles || !c.transaction_owned {
        return aggregate(c, who);
    }
    let revision = c.one(
        "SELECT revision FROM eos_profile_outbox WHERE player=?",
        json!([who]),
    )?;
    let revision = match revision {
        Some(row) => cells(&row, 1)?[0].clone(),
        None => Value::Null,
    };
    let plan = c.profile(json!({"op":"lookup","player":who,"revision":revision}))?;
    if !plan["value"].is_null() {
        return Ok(plan["value"].clone());
    }
    let value = aggregate(c, who)?;
    if !plan["token"].is_null() {
        c.profile(json!({"op":"stage","token":plan["token"],"value":value}))?;
    }
    Ok(value)
}
pub(super) fn view(c: &mut Cursor<'_>, who: &str) -> Flow<Value> {
    let row=c.one("SELECT p.screen_name,p.joined_at,c.appearance,c.service_tag,c.updated_at,c.revision FROM player_profiles p LEFT JOIN player_customization c ON c.player=p.player WHERE p.player=?",json!([who]))?.ok_or(Error(404,"Player profile not found"))?;
    let row = cells(&row, 6)?;
    let tag=c.one("SELECT p.screen_name,t.discriminator FROM player_profiles p JOIN player_friend_tags t ON t.player=p.player WHERE p.player=?",json!([who]))?;
    let tag = match tag {
        Some(row) => {
            let row = cells(&row, 2)?;
            json!(format!(
                "{}#{}",
                m::text(&row[0], 64)?,
                m::text(&row[1], 8)?
            ))
        }
        None => Value::Null,
    };
    Ok(
        json!({"screen_name":row[0],"friend_tag":tag,"joined_at":row[1],"appearance":decoded(&row[2])?,
        "service_tag":row[3],"updated_at":row[4],"revision":if row[5].is_null(){json!(0)}else{row[5].clone()},
        "statistics":statistics(c,who)?}),
    )
}
fn enqueue(c: &mut Cursor<'_>, who: &str) -> Flow<()> {
    match c.one(
        "SELECT revision FROM eos_profile_outbox WHERE player=?",
        json!([who]),
    )? {
        Some(row) => {
            let revision = m::integer(&cells(&row, 1)?[0], 0, i64::MAX - 1)?;
            c.write(
                "UPDATE eos_profile_outbox SET revision=? WHERE player=? AND revision=?",
                json!([revision + 1, who, revision]),
                Some(1),
            )?;
        }
        None => {
            c.write(
                "INSERT INTO eos_profile_outbox VALUES(?,1)",
                json!([who]),
                Some(1),
            )?;
        }
    }
    Ok(())
}
// Private verified-provider fact. HTTP profile updates cannot select this mode.
// The caller revalidates its actual account/room capability after SDK IO and
// keeps the directory plan in the same owned SQL/native transaction.
fn eos_bind(c: &mut Cursor<'_>, who: &str, body: &Value) -> Flow<Value> {
    m::fields(body, &["account", "puid"])?;
    need(
        body["account"] == who,
        403,
        "EOS identity belongs to another account",
    )?;
    m::hex(&body["puid"], 16)?;
    let puid = m::text(&body["puid"], 32)?;
    let old = c.one("SELECT puid FROM eos_players WHERE player=?", json!([who]))?;
    need(
        old.as_ref().is_none_or(|r| r[0] == puid),
        409,
        "EOS identity migration requires explicit recovery",
    )?;
    let linked = c.one("SELECT player FROM eos_players WHERE puid=?", json!([puid]))?;
    need(
        linked.as_ref().is_none_or(|r| r[0] == who),
        409,
        "EOS identity already linked",
    )?;
    if old.is_none() {
        c.write(
            "INSERT INTO eos_players VALUES(?,?)",
            json!([who, puid]),
            Some(1),
        )?;
        enqueue(c, who)?;
    }
    Ok(json!({"linked":true}))
}
fn update(c: &mut Cursor<'_>, who: &str, body: &Value, wall: i64) -> Flow<()> {
    m::fields_optional(body, &["appearance"], &["service_tag"])?;
    let bytes = raw_appearance(&body["appearance"])?;
    let tag = if body["service_tag"].is_null() {
        None
    } else {
        let text = body["service_tag"]
            .as_str()
            .ok_or(Error(400, "Service tag text required"))?;
        need(
            text.len() <= 4
                && text
                    .bytes()
                    .all(|b| b.is_ascii_uppercase() || b.is_ascii_digit()),
            400,
            "Service tag must contain up to four uppercase letters or digits",
        )?;
        Some(text)
    };
    let previous = c.one(
        "SELECT appearance,service_tag,revision FROM player_customization WHERE player=?",
        json!([who]),
    )?;
    let old_tag = previous
        .as_ref()
        .map(|r| cells(r, 3))
        .transpose()?
        .map(|r| {
            r[1].as_str()
                .ok_or(Error(503, "Stored service tag differs"))
        })
        .transpose()?
        .unwrap_or("");
    let tag = tag.unwrap_or(old_tag).to_owned();
    if let Some(row) = &previous {
        let row = cells(row, 3)?;
        if m::hex(&row[0]["blob"], 9)? == bytes && row[1].as_str() == Some(tag.as_str()) {
            return Ok(());
        }
    }
    // Preserve the original adapter's durable wall-clock floor, but advance it
    // inside this atomic customization transaction, never during a read.
    let floor = c
        .one("SELECT value FROM clock_floor WHERE id=1", json!([]))?
        .ok_or(Error(503, "Profile clock floor unavailable"))?;
    let now = wall.max(m::integer(&cells(&floor, 1)?[0], 0, 9007199254740991)?);
    c.write(
        "UPDATE clock_floor SET value=? WHERE id=1",
        json!([now]),
        Some(1),
    )?;
    let blob = json!({"blob":body["appearance"]});
    if let Some(row) = previous {
        let row = cells(&row, 3)?;
        let revision = m::integer(&row[2], 1, i64::MAX - 1)?;
        c.write("UPDATE player_customization SET appearance=?,service_tag=?,updated_at=?,revision=? WHERE player=? AND revision=?",json!([blob,tag,now,revision+1,who,revision]),Some(1))?;
    } else {
        c.write(
            "INSERT INTO player_customization VALUES(?,?,?,?,1)",
            json!([who, blob, tag, now]),
            Some(1),
        )?;
    }
    enqueue(c, who)
}
pub(super) fn operation(r: &Value, c: &mut Cursor<'_>) -> Flow<Value> {
    let mode = txt(&r["mode"])?;
    if mode == "schema" {
        c.batch(
            SCHEMA
                .split(';')
                .map(str::trim)
                .filter(|s| !s.is_empty())
                .map(|s| statement(s, json!([]), None))
                .collect(),
        )?;
        return Ok(Value::Null);
    }
    let who = txt(&r["player"])?;
    match mode {
        "view" | "get" => view(c, who),
        "statistics" => statistics(c, who),
        "totals" => aggregate(c, who),
        "eos_bind" => eos_bind(c, who, &r["body"]),
        "enqueue" => {
            enqueue(c, who)?;
            Ok(Value::Null)
        }
        "update" => {
            update(
                c,
                who,
                &r["body"],
                m::integer(&r["wall"], 0, 9007199254650000)?,
            )?;
            view(c, who)
        }
        _ => Err(Error(400, "Unknown native profile operation").into()),
    }
}
