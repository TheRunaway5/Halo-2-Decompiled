# Native campaign session timers

The native `777E0` owner retires matching active timer records and stores the
original timestamp. Its typed algorithm and four exact caller transports are
qualified against unchanged TU1.5 execution in the
[component proof](../analysis/campaign-native-session-timers-20261029/proof-v2.json).

The [World integration proof](../analysis/campaign-native-session-timers-20261030/world-integration/proof.json)
now covers four real `77AB0` parent calls and 64 native timer calls in a prepared
MCC World. The test completes the public map/session lifecycle, verifies that
an unconfigured World rejects a clock anchor, then exercises adjusted clock
wrap, journal capture/replay and cached/inactive paths. Replay does not sample
the platform clock. The production clock-boundary guard remains unchanged.

The original complete parent selects 16 slots. The timer selects five groups
at stride `99C`, reading/writing state and timestamp DWORDs at slot stride `98`.
The [original storage audit](../analysis/campaign-native-session-timers-20261030/world-integration/cold-storage-primary-v2.json)
authenticates those 80 loader-zeroed pairs and the parent's two policy bytes:
642 bytes in total. The cold factory declares those exact fields. It leaves
group headers and other record payloads absent and runs no initializer.
The actual cold-factory regression verifies mutable ownership, native backing,
payload gaps, independent cold values and checkpoint preservation of live state.

The two previous generated `777E0` bodies are retired. The
[inverse-edit audit](../analysis/campaign-native-session-timers-20261030/world-integration/retirement-source-proof.json)
proves that their address mappings and unrelated bodies are unchanged and only
the authenticated callers now request the native owner.

The final frozen cohort passes the shared area regressions and timer World
test. Armory still fails at the next missing `1B0D1C -> 1BBEC0` callback after
the new `1BBB50` callback runs. This is bounded integration evidence; complete
training, campaign/co-op, native-only closure and launcher promotion remain open.
The frozen source/build and all completed cold-validation owners are removed;
the [cleanup receipt](../analysis/campaign-native-armory-20261030/cohort-v1/cleanup.json)
records unchanged launcher selections and no retained large artifacts.

```sh
python3 tools/run_test.py -- .tools/python/bin/python3 -B tools/audit_native_campaign_session_timer_storage_tu15.py --report analysis/timer-storage-primary.json
python3 tools/run_test.py -- python3 -B tools/validate_native_campaign_session_timer_storage_tu15.py --output analysis/timer-cold-validation
```
