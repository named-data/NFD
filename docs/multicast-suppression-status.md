# Adaptive Multicast Suppression (AMS) — Current Status

## Overview

AMS reduces redundant packet forwarding on NDN multicast faces (UDP and Ethernet). Each node
learns per-name-prefix how many duplicates it typically sees, and adaptively adjusts how long
to wait before forwarding — suppressing more when the network is over-forwarding, less when it
is under-forwarding.

---

## Key Parameters

| Parameter | Value | Description |
|---|---|---|
| `DISCOUNT_FACTOR` (α) | 0.125 | EMA smoothing factor |
| `DUPLICATE_THRESHOLD` | 1.3 | EMA above this → increase suppression |
| `MULTIPLICATIVE_INCREASE` | 1.3× | Suppression time multiplier when EMA > threshold |
| `ADATIVE_DECREASE` | 5 ms | Additive decrease when EMA ≤ threshold |
| `DEFAULT_INSTANT_LIFETIME` | 30 ms | Window to collect duplicates (2× max propagation delay) |
| `MAX_MEASURMENT_INACTIVE_PERIOD` | 300 s | EMA record expires after 5 min of inactivity |
| `minSuppressionTime` | 15 ms | Floor for suppression timer |
| `maxSuppressionTime` | 15,000 ms | Ceiling for suppression timer |
| `MAX_IGNORE` | 3 | Consecutive rising duplicate counts ignored before EMA update |

---

## Component Summary

### `NameTree`
A trie keyed by NDN name components. Stores the learned suppression time per name prefix.
On send, a longest-prefix match gives the best known suppression time for any name.
If no match exists, falls back to `minSuppressionTime`.

### `EMAMeasurements`
Tracks the duplicate rate for a name prefix using an exponential moving average.
- EMA rises when duplicates increase → suppression time is **multiplied** by 1.3
- EMA falls below threshold → suppression time is **decreased** by 5 ms
- Updates are ignored for up to 3 consecutive cycles if the duplicate count is still rising
  (avoids reacting to transient spikes before the network stabilizes)

### `MulticastSuppression`
Per-face state machine. Maintains:
- `m_interestHistory` / `m_dataHistory` — 30 ms window tracking in-flight names and duplicate counts
- `m_EMA_interest` / `m_EMA_data` — long-lived EMA records per name prefix
- `m_interestNameTree` / `m_dataNameTree` — suppression timers indexed by prefix
- `m_objectExpirationTimer` — scheduler events for entry expiration

### `LinkService` (send/receive path)
The suppression gate sits here, guarded by `m_suppressionEnabled`:

**Send path:**
1. If interest/data already in-flight → drop immediately
2. Otherwise, look up suppression timer from `NameTree` → schedule delayed send
3. If a duplicate is overheard during the wait → cancel the scheduled send

**Receive path:**
1. Overheard interest → cancel any pending scheduled forward of the same interest
2. Overheard data → cancel pending data forward *and* any pending interest forward for same name
3. Record into history for duplicate counting

---

## Integration Points

| File | Role |
|---|---|
| `daemon/face/multicast-suppression.hpp/.cpp` | Core AMS logic |
| `daemon/face/link-service.hpp/.cpp` | Send/receive path with suppression gate |
| `daemon/face/generic-link-service.hpp/.cpp` | `enableMulticastSuppression` option, propagates to `LinkService` |
| `daemon/face/udp-factory.cpp/.hpp` | Parses `mcast_suppression` from config, sets option on UDP multicast faces |
| `daemon/face/ethernet-factory.cpp/.hpp` | Same for Ethernet multicast faces |
| `nfd.conf.sample.in` | Documents the config knob |

---

## Configuration

Suppression is **disabled by default**. To enable, add to `nfd.conf`:

```
face_system {
  udp {
    mcast_suppression yes   ; enable AMS on UDP multicast faces
  }
  ether {
    mcast_suppression yes   ; enable AMS on Ethernet multicast faces
  }
}
```

---

## Test Coverage

26 unit tests in `tests/daemon/face/multicast-suppression.t.cpp`, all passing:

| Suite | Tests | What is covered |
|---|---|---|
| `TestNameTree` | 9 | Insert, longest-prefix match, edge cases (empty name, no match, special chars) |
| `TestEMAMeasurements` | 6 | Default/custom construction, EMA update, suppression time increase/decrease |
| `TestMulticastSuppressionClass` | 11 | Record interest/data, duplicate detection, in-flight tracking, entry expiration, delay timer |

---

## Known Limitations / Open Questions

- **Dropped packet behavior** — when a forwarding is suppressed, the upstream PIT entry is not
  explicitly notified. Whether this causes retransmissions or silent drops depends on the
  forwarding strategy and is not yet handled.
- **Data suppression asymmetry** — Interest suppression uses a full delay+cancel loop. Data uses
  the same loop but the comment in the code notes this may cause issues if multiple consumers
  sent Interests at different times (risk of suppressing legitimate Data replies).
- **EMA granularity** — EMA is tracked at `name.getPrefix(-1)` (one component above the leaf).
  Very flat name spaces (short names) may cause unrelated prefixes to share a suppression timer.
- **No per-face isolation** — `MulticastSuppression` is a member of `LinkService` (one instance
  per face), so state is correctly isolated per face.
- **`cancelIfSchdeuled` typo** — function name has a spelling error; low priority but worth fixing.
- **No suppression for Nacks** — Nack forwarding bypasses the suppression logic entirely.
