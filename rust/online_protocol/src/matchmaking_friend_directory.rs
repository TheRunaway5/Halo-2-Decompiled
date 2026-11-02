//! Accepted friendships, bilateral blocks, public tags and transaction policy.
//! Modern native service; original Live friend procedures remain unrecovered.
use super::*;
use crate::matchmaking_model::s;
use crate::matchmaking_unicode::fold;
#[path = "matchmaking_friend_cloud.rs"]
mod cloud;

const TAG_SCHEMA: &str = "CREATE TABLE IF NOT EXISTS player_friend_tags(player TEXT PRIMARY KEY REFERENCES players(id),discriminator TEXT NOT NULL UNIQUE CHECK(length(discriminator)=8 AND discriminator NOT GLOB '*[^0-9]*'))";
const SCHEMA:&str="
CREATE TABLE IF NOT EXISTS player_friendships(low TEXT NOT NULL REFERENCES players(id),high TEXT NOT NULL REFERENCES players(id),requester TEXT NOT NULL REFERENCES players(id),accepted INTEGER NOT NULL CHECK(accepted IN (0,1)),CHECK(low<high AND requester IN (low,high)),PRIMARY KEY(low,high));
CREATE INDEX IF NOT EXISTS player_friendships_high ON player_friendships(high);
CREATE INDEX IF NOT EXISTS player_friend_pending_requester ON player_friendships(requester) WHERE accepted=0;
CREATE TABLE IF NOT EXISTS player_friend_blocks(player TEXT NOT NULL REFERENCES players(id),target TEXT NOT NULL REFERENCES players(id),CHECK(player!=target),PRIMARY KEY(player,target));
CREATE INDEX IF NOT EXISTS player_friend_blocks_target ON player_friend_blocks(target);
CREATE TABLE IF NOT EXISTS player_friend_commands(player TEXT NOT NULL REFERENCES players(id),request TEXT NOT NULL,payload TEXT NOT NULL,expires INTEGER NOT NULL,PRIMARY KEY(player,request));
CREATE INDEX IF NOT EXISTS player_friend_commands_expiry ON player_friend_commands(expires);
CREATE TABLE IF NOT EXISTS player_friend_block_epoch(id INTEGER PRIMARY KEY CHECK(id=1),revision INTEGER NOT NULL CHECK(typeof(revision)='integer' AND revision>=0));
INSERT OR IGNORE INTO player_friend_block_epoch VALUES(1,0);
";
pub(super) fn schema(c: &mut Cursor<'_>) -> Flow<Value> {
    let mut rows = SCHEMA
        .split(';')
        .map(str::trim)
        .filter(|s| !s.is_empty())
        .map(|s| statement(s, json!([]), None))
        .collect::<Vec<_>>();
    for (name, event) in [
        ("insert", "INSERT"),
        ("delete", "DELETE"),
        ("update", "UPDATE"),
    ] {
        rows.push(statement(&format!("CREATE TRIGGER IF NOT EXISTS player_friend_blocks_epoch_{name} AFTER {event} ON player_friend_blocks BEGIN UPDATE player_friend_block_epoch SET revision=revision+1 WHERE id=1; END"),json!([]),None));
    }
    c.batch(rows)?;
    cloud::schema(c)?;
    Ok(Value::Null)
}
pub(super) fn tag_schema(c: &mut Cursor<'_>) -> Flow<Value> {
    c.write(TAG_SCHEMA, json!([]), None)?;
    Ok(Value::Null)
}
pub(super) fn tag(c: &mut Cursor<'_>, who: &str) -> Flow<Value> {
    let row=c.one("SELECT p.screen_name,t.discriminator FROM player_profiles p JOIN player_friend_tags t ON t.player=p.player WHERE p.player=?",json!([who]))?;
    Ok(match row {
        Some(row) => json!(format!("{}#{}", txt(&row[0])?, txt(&row[1])?)),
        None => Value::Null,
    })
}
pub(super) fn assign(c: &mut Cursor<'_>, who: &str) -> Flow<Value> {
    if c.one(
        "SELECT 1 FROM player_friend_tags WHERE player=?",
        json!([who]),
    )?
    .is_some()
    {
        return Ok(Value::Null);
    }
    for index in 0..128 {
        let suffix = c.random_discriminator(&format!("friend-tag-{index}"))?;
        c.write(
            "INSERT OR IGNORE INTO player_friend_tags VALUES(?,?)",
            json!([who, suffix]),
            None,
        )?;
        if c.one(
            "SELECT 1 FROM player_friend_tags WHERE player=?",
            json!([who]),
        )?
        .is_some()
        {
            return Ok(Value::Null);
        }
    }
    Err(Error(503, "Friend tag allocation is busy; reconnect shortly").into())
}
pub(super) fn resolve(c: &mut Cursor<'_>, value: &Value) -> Flow<String> {
    let value = m::text(value, 73)?;
    let (name, suffix) = value
        .rsplit_once('#')
        .ok_or(Error(400, "Enter a friend tag such as EmkMage#12345678"))?;
    need(
        !name.is_empty() && suffix.len() == 8 && suffix.bytes().all(|c| c.is_ascii_digit()),
        400,
        "Enter a friend tag such as EmkMage#12345678",
    )?;
    let row=c.one("SELECT t.player,p.screen_name FROM player_friend_tags t JOIN player_profiles p ON p.player=t.player WHERE t.discriminator=?",json!([suffix]))?.ok_or(Error(404,"Player not found; check their current friend tag"))?;
    need(
        fold(txt(&row[1])?) == fold(name),
        404,
        "Player not found; check their current friend tag",
    )?;
    Ok(txt(&row[0])?.to_owned())
}
fn target(c: &mut Cursor<'_>, public: &Value) -> Flow<String> {
    let public = uuid(public)?;
    let row = c
        .one(
            "SELECT player FROM history_players WHERE public_id=?",
            json!([public]),
        )?
        .ok_or(Error(404, "Player not found"))?;
    Ok(txt(&row[0])?.to_owned())
}
fn public(c: &mut Cursor<'_>, who: &str) -> Flow<Value> {
    let row=c.one("SELECT h.public_id,p.screen_name,t.discriminator FROM history_players h JOIN player_profiles p ON p.player=h.player LEFT JOIN player_friend_tags t ON t.player=h.player WHERE h.player=?",json!([who]))?.ok_or(Error(404,"Player not found"))?;
    Ok(
        json!({"id":row[0],"name":row[1],"friend_tag":if row[2].is_null(){Value::Null}else{json!(format!("{}#{}",txt(&row[1])?,txt(&row[2])?))},"presence":"offline"}),
    )
}
fn party_enabled(value: &Value) -> Flow<bool> {
    if value.is_null() {
        return Ok(false);
    }
    let pairs = value
        .as_array()
        .ok_or(Error(400, "Friend query pairs required"))?;
    need(pairs.len() <= 1, 400, "Unexpected friend query")?;
    if pairs.is_empty() {
        return Ok(false);
    }
    let pair = pairs[0]
        .as_array()
        .ok_or(Error(400, "Invalid friend query"))?;
    need(
        pair.len() == 2 && pair[0] == "party" && ["0", "1"].iter().any(|s| pair[1] == *s),
        400,
        "Unexpected friend query",
    )?;
    Ok(pair[1] == "1")
}
fn roster(c: &mut Cursor<'_>, who: &str) -> Flow<Vec<Value>> {
    let mut rows = Vec::new();
    loop {
        let page=c.query("WITH relationships(target,state) AS (SELECT CASE WHEN low=?1 THEN high ELSE low END,CASE WHEN accepted=1 THEN 'friend' WHEN requester=?1 THEN 'outgoing' ELSE 'incoming' END FROM player_friendships WHERE low=?1 OR high=?1 UNION ALL SELECT target,'blocked' FROM player_friend_blocks WHERE player=?1) SELECT r.target,r.state,h.public_id,p.screen_name,t.discriminator FROM relationships r JOIN player_profiles p ON p.player=r.target JOIN history_players h ON h.player=r.target LEFT JOIN player_friend_tags t ON t.player=r.target WHERE r.state='blocked' OR NOT EXISTS(SELECT 1 FROM player_friend_blocks b WHERE (b.player=?1 AND b.target=r.target) OR (b.player=r.target AND b.target=?1)) ORDER BY r.target,r.state LIMIT 128 OFFSET ?2",json!([who,rows.len()]))?;
        let count = page.len();
        rows.extend(page);
        need(rows.len() <= 250, 409, "Friend list exceeds capacity")?;
        if count < 128 {
            break;
        }
    }
    Ok(rows)
}
fn view(c: &mut Cursor<'_>, who: &str, party: bool) -> Flow<Value> {
    let presence = c.provider("friend_presence", Value::Null)?;
    let observed = presence
        .as_object()
        .ok_or(Error(503, "Native friend presence required"))?;
    need(
        observed.len() <= 8192,
        503,
        "Friend presence exceeds bounds",
    )?;
    for (who, state) in observed {
        m::text(&json!(who), 64)?;
        need(
            state == "online" || state == "playing",
            503,
            "Invalid native friend presence",
        )?;
    }
    let rows = roster(c, who)?;
    let targets = rows
        .iter()
        .filter(|row| row[1] == "friend")
        .map(|row| row[0].clone())
        .collect::<Vec<_>>();
    let contexts = if party {
        c.provider(
            "friend_party_views",
            json!({"player":who,"targets":targets}),
        )?
    } else {
        json!([])
    };
    if party {
        need(
            contexts
                .as_array()
                .is_some_and(|r| r.len() == targets.len()),
            503,
            "Invalid native friend party views",
        )?;
    }
    let mut friends = Vec::new();
    let mut context_index = 0;
    for stored in rows {
        let cells = stored
            .as_array()
            .ok_or(Error(503, "Friend SQL row required"))?;
        need(cells.len() == 5, 503, "Friend SQL columns differ")?;
        let player = txt(&cells[0])?;
        let state = txt(&cells[1])?;
        let name = txt(&cells[3])?;
        let mut row = json!({"id":cells[2],"name":name,"friend_tag":if cells[4].is_null(){Value::Null}else{json!(format!("{name}#{}",txt(&cells[4])?))},"state":state,"presence":if state=="friend"{observed.get(player).cloned().unwrap_or(json!("offline"))}else{json!("offline")}});
        if party {
            row["party_joinable"] = json!(false);
            row["party_context"] = Value::Null;
            if state == "friend" {
                let context = &contexts[context_index];
                context_index += 1;
                m::fields(context, &["joinable", "context"])?;
                m::boolean(&context["joinable"])?;
                if !context["context"].is_null() {
                    m::text(&context["context"], 64)?;
                }
                row["party_joinable"] = context["joinable"].clone();
                row["party_context"] = context["context"].clone();
            }
        }
        friends.push((fold(name), row));
    }
    friends.sort_by(|(an, a), (bn, b)| {
        (
            a["state"] != "incoming",
            a["state"] == "blocked",
            a["presence"] == "offline",
            an,
            s(a, "id"),
        )
            .cmp(&(
                b["state"] != "incoming",
                b["state"] == "blocked",
                b["presence"] == "offline",
                bn,
                s(b, "id"),
            ))
    });
    let mut value = json!({"friend_tag":tag(c,who)?,"friends":friends.into_iter().map(|(_,r)|r).collect::<Vec<_>>()});
    let enabled = c.friend_cloud;
    value["cloud"] = cloud::status(c, who, enabled)?;
    Ok(value)
}
fn capacity(c: &mut Cursor<'_>, who: &str) -> Flow<()> {
    let row=c.one("SELECT (SELECT count(*) FROM player_friendships WHERE low=? OR high=?)+(SELECT count(*) FROM player_friend_blocks WHERE player=?)",json!([who,who,who]))?.ok_or(Error(503,"Friend count unavailable"))?;
    need(
        m::integer(&row[0], 0, i64::MAX)? < 250,
        409,
        "Friend list is full",
    )
}
fn count(
    c: &mut Cursor<'_>,
    sql: &str,
    args: Value,
    maximum: i64,
    message: &'static str,
    status: u16,
) -> Flow<()> {
    let row = c
        .one(sql, args)?
        .ok_or(Error(503, "Friend count unavailable"))?;
    need(m::integer(&row[0], 0, i64::MAX)? < maximum, status, message)
}
fn blocked(c: &mut Cursor<'_>, a: &str, b: &str) -> Flow<Value> {
    let mut plan = Value::Null;
    if c.friends && c.transaction_owned {
        let epoch = c
            .one(
                "SELECT revision FROM player_friend_block_epoch WHERE id=1",
                json!([]),
            )?
            .ok_or(Error(503, "Friend block epoch unavailable"))?;
        plan = c.friend_cache(json!({"op":"lookup","a":a,"b":b,"revision":epoch[0]}))?;
        if !plan["value"].is_null() {
            return Ok(plan["value"].clone());
        }
    }
    let value = c.blocked(a, b)?;
    if !plan["token"].is_null() {
        c.friend_cache(json!({"op":"stage","token":plan["token"],"value":value}))?;
    }
    Ok(json!(value))
}
fn command(c: &mut Cursor<'_>, r: &Value, who: &str, party: bool) -> Flow<Value> {
    let body = &r["body"];
    m::fields_optional(body, &["request_id", "action"], &["tag", "target"])?;
    let rid = uuid(&body["request_id"])?;
    let action = m::text(&body["action"], 64)?;
    need(
        [
            "search", "request", "accept", "decline", "cancel", "remove", "block", "unblock",
        ]
        .contains(&action),
        400,
        "Unknown friend action",
    )?;
    m::fields(
        body,
        &[
            "request_id",
            "action",
            if action == "search" { "tag" } else { "target" },
        ],
    )?;
    if action == "search" {
        let target = resolve(c, &body["tag"])?;
        need(target != who, 404, "Player not found")?;
        let mut result = view(c, who, party)?;
        result["search_result"] = public(c, &target)?;
        return Ok(result);
    }
    let floor = c
        .one("SELECT value FROM clock_floor WHERE id=1", json!([]))?
        .ok_or(Error(503, "Friend clock unavailable"))?;
    let now = m::integer(&r["wall"], 0, 9007199254650000)?.max(m::integer(
        &floor[0],
        0,
        9007199254740991,
    )?);
    need(now <= 9007199254650000, 503, "Friend clock exhausted")?;
    c.write(
        "UPDATE clock_floor SET value=? WHERE id=1 AND value<=?",
        json!([now, now]),
        Some(1),
    )?;
    c.write(
        "DELETE FROM player_friend_commands WHERE expires<=?",
        json!([now]),
        None,
    )?;
    let payload = ascii_json(body);
    if let Some(previous) = c.one(
        "SELECT payload FROM player_friend_commands WHERE player=? AND request=?",
        json!([who, rid]),
    )? {
        need(
            previous[0] == payload,
            409,
            "Friend command ID reused differently",
        )?;
        return view(c, who, party);
    }
    count(
        c,
        "SELECT count(*) FROM player_friend_commands WHERE player=?",
        json!([who]),
        256,
        "Friend command limit reached; try again later",
        429,
    )?;
    count(
        c,
        "SELECT count(*) FROM player_friend_commands",
        json!([]),
        100000,
        "Friend service is full",
        503,
    )?;
    let target = target(c, &body["target"])?;
    need(target != who, 400, "Choose another player")?;
    let (a, b) = if who < target.as_str() {
        (who, target.as_str())
    } else {
        (target.as_str(), who)
    };
    let relation = c.one(
        "SELECT requester,accepted FROM player_friendships WHERE low=? AND high=?",
        json!([a, b]),
    )?;
    match action {
        "block" => {
            c.write(
                "DELETE FROM player_friendships WHERE low=? AND high=?",
                json!([a, b]),
                None,
            )?;
            if c.one(
                "SELECT 1 FROM player_friend_blocks WHERE player=? AND target=?",
                json!([who, target]),
            )?
            .is_none()
            {
                capacity(c, who)?;
                count(
                    c,
                    "SELECT count(*) FROM player_friend_blocks",
                    json!([]),
                    100000,
                    "Friend service is full",
                    503,
                )?;
                c.write(
                    "INSERT INTO player_friend_blocks VALUES(?,?)",
                    json!([who, target]),
                    Some(1),
                )?;
            }
            c.batch(vec![statement("DELETE FROM party_invites WHERE (inviter=? AND target=?) OR (inviter=? AND target=?)",json!([who,target,target,who]),None),statement("DELETE FROM clan_invites WHERE (inviter=? AND target=?) OR (inviter=? AND target=?)",json!([who,target,target,who]),None)])?;
        }
        "unblock" => {
            c.write(
                "DELETE FROM player_friend_blocks WHERE player=? AND target=?",
                json!([who, target]),
                None,
            )?;
        }
        _ => {
            need(!c.blocked(who, &target)?, 404, "Friend action unavailable")?;
            if action == "request" {
                need(
                    relation.is_none(),
                    409,
                    "Friend request or friendship already exists",
                )?;
                count(
                    c,
                    "SELECT count(*) FROM player_friendships WHERE requester=? AND accepted=0",
                    json!([who]),
                    32,
                    "Too many outgoing friend requests",
                    429,
                )?;
                count(
                    c,
                    "SELECT count(*) FROM player_friendships WHERE (low=? OR high=?) AND requester!=? AND accepted=0",
                    json!([target, target, target]),
                    32,
                    "Player is not accepting more friend requests",
                    429,
                )?;
                capacity(c, who)?;
                capacity(c, &target)?;
                count(
                    c,
                    "SELECT count(*) FROM player_friendships",
                    json!([]),
                    100000,
                    "Friend service is full",
                    503,
                )?;
                c.write(
                    "INSERT INTO player_friendships VALUES(?,?,?,0)",
                    json!([a, b, who]),
                    Some(1),
                )?;
            } else {
                let relation = relation.ok_or(Error(
                    409,
                    "Friend request or friendship no longer available",
                ))?;
                let accepted = relation[1] == 1;
                need(
                    (action == "remove" && accepted)
                        || (["accept", "decline"].contains(&action)
                            && !accepted
                            && relation[0] != who)
                        || (action == "cancel" && !accepted && relation[0] == who),
                    409,
                    "Friend action unavailable",
                )?;
                if action == "accept" {
                    c.write(
                        "UPDATE player_friendships SET accepted=1 WHERE low=? AND high=?",
                        json!([a, b]),
                        Some(1),
                    )?;
                } else {
                    c.write(
                        "DELETE FROM player_friendships WHERE low=? AND high=?",
                        json!([a, b]),
                        Some(1),
                    )?;
                }
            }
        }
    }
    c.write(
        "INSERT INTO player_friend_commands VALUES(?,?,?,?)",
        json!([who, rid, payload, now + 86400]),
        Some(1),
    )?;
    let enabled = c.friend_cloud;
    let wake = cloud::enqueue(c, who, enabled)? | cloud::enqueue(c, &target, enabled)?;
    let mut result = view(c, who, party)?;
    result["_native_friend_wake"] = json!(wake);
    Ok(result)
}
pub(super) fn operation(r: &Value, c: &mut Cursor<'_>) -> Flow<Value> {
    let mode = txt(&r["mode"])?;
    if mode == "schema" {
        return schema(c);
    }
    if mode == "cloud_schema" {
        return cloud::schema(c);
    }
    if mode == "cloud_next" {
        return cloud::next(c, r);
    }
    if mode == "cloud_finish" {
        return cloud::finish(c, r);
    }
    if mode == "tag_schema" {
        return tag_schema(c);
    }
    if mode == "resolve_tag" {
        return Ok(json!(resolve(c, &r["body"])?));
    }
    if mode == "target" {
        return Ok(json!(target(c, &r["body"])?));
    }
    let who = m::text(&r["player"], 64)?;
    match mode {
        "cloud_status" => cloud::status(c, who, r["friend_cloud"] == true),
        "cloud_enqueue" => {
            Ok(json!({"_native_friend_wake":cloud::enqueue(c,who,r["friend_cloud"]==true)?}))
        }
        "tag" => tag(c, who),
        "assign_tag" => assign(c, who),
        "public_player" => {
            let mut value = public(c, who)?;
            if !r["body"].is_null() {
                m::fields(&r["body"], &["presence", "state"])?;
                need(
                    ["online", "offline", "playing"]
                        .iter()
                        .any(|s| r["body"]["presence"] == *s),
                    400,
                    "Invalid friend presence",
                )?;
                value["presence"] = r["body"]["presence"].clone();
                if !r["body"]["state"].is_null() {
                    need(
                        ["friend", "incoming", "outgoing", "blocked"]
                            .iter()
                            .any(|s| r["body"]["state"] == *s),
                        400,
                        "Invalid friend state",
                    )?;
                    value["state"] = r["body"]["state"].clone();
                }
            }
            Ok(value)
        }
        "blocked" => blocked(c, who, m::text(&r["target"], 64)?),
        "accepted" => Ok(json!(c.accepted(who, m::text(&r["target"], 64)?)?)),
        "view" | "get" => view(c, who, party_enabled(&r["features"])?),
        "command" => command(c, r, who, party_enabled(&r["features"])?),
        _ => Err(Error(400, "Unknown friend directory operation").into()),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn collision_retry_keeps_native_entropy_and_sql_replay_stable() {
        let mut d=Directory::new(&json!({"mode":"assign_tag","source":null,"player":"new","wall":0,"clock":0,"family":"friends"})).unwrap();
        d.entropy.insert("friend-tag-0".into(), "00000001".into());
        d.entropy.insert("friend-tag-1".into(), "99999999".into());
        let mut e = d.execute(&json!({"op":"begin"})).unwrap();
        let mut inserted = Vec::new();
        for (rows, changes) in [
            (json!([]), -1),
            (json!([]), 0),
            (json!([]), -1),
            (json!([]), 1),
            (json!([[1]]), -1),
        ] {
            assert_eq!(e["kind"], "sql");
            if e["mode"] == "batch" {
                inserted.push(e["statements"][0]["params"][1].clone());
            }
            e = d
                .execute(&json!({"op":"reply","frame":e["frame"],"rows":rows,"changes":[changes]}))
                .unwrap();
        }
        assert_eq!(e["kind"], "done");
        assert_eq!(
            inserted,
            json!(["00000001", "99999999"]).as_array().unwrap().clone()
        );
    }
}
