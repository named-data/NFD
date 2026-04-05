# Adaptive Multicast Suppression (AMS) for NFD

## Research Paper

This implementation is based on the following published work:

> **Reining in Redundant Traffic through Adaptive Duplicate Suppression in Multi-Access NDN Networks**
> Saurab Dulal, Lan Wang
> *10th ACM Conference on Information-Centric Networking (ACM ICN '23)*
> October 9–10, 2023, Reykjavik, Iceland
> [https://doi.org/10.1145/3623565.3623717](https://doi.org/10.1145/3623565.3623717)

**Abstract:**
Named Data Networking (NDN) provides native support for multiparty communication. However,
the current NDN forwarder lacks a duplicate suppression mechanism for multicasting in a
multi-access network, potentially leading to network congestion and significant degradation
in overall packet delivery performance. In this paper, we introduce Adaptive Duplicate
Suppression (ADS) for one-hop multicasting in multi-access NDN networks. ADS utilizes the
duplicate count per Interest and Data name observed in the network to dynamically adjust the
suppression time that a node waits before forwarding a packet. We have implemented ADS in
the NDN forwarding daemon (NFD) and assessed its performance using Mini-NDN. Our evaluation
demonstrates that ADS can effectively reduce redundant network traffic under various network
conditions, resulting in significantly improved application goodput and reduced transfer times.

**BibTeX:**
```bibtex
@inproceedings{dulal2023reining,
  author    = {Dulal, Saurab and Wang, Lan},
  title     = {Reining in Redundant Traffic through Adaptive Duplicate Suppression in Multi-Access NDN Networks},
  booktitle = {Proceedings of the 10th ACM Conference on Information-Centric Networking (ACM ICN '23)},
  year      = {2023},
  month     = {October},
  address   = {Reykjavik, Iceland},
  publisher = {ACM},
  doi       = {10.1145/3623565.3623717},
  isbn      = {979-8-4007-0403-1},
}
```

---

## Overview

Multicast forwarding in NDN can cause significant redundancy — when multiple nodes receive
the same Interest or Data, they may all attempt to forward it, flooding the network with
duplicate packets.

**Adaptive Multicast Suppression (AMS)** addresses this by making each node learn, on a
per-name-prefix basis, how many duplicates it typically sees and adaptively adjusting a
suppression delay before forwarding. Nodes that hear a neighbor already forwarded a packet
cancel their own pending forward, reducing unnecessary traffic.

The algorithm follows an **AIMD** (Additive Increase, Multiplicative Decrease) strategy:
- If duplicates are high (EMA > 1.3) → **multiply** the suppression delay by 1.3× (back off)
- If duplicates are low (EMA ≤ 1.3) → **decrease** the suppression delay by 5 ms (forward sooner)

AMS is **disabled by default** and has zero overhead when off — stock NFD behavior is fully
preserved.

---

## Prerequisites

- NFD built from source (see [`docs/INSTALL.rst`](docs/INSTALL.rst))
- `ndn-cxx` library installed
- Applies to **UDP multicast** and **Ethernet multicast** faces only

---

## Building

AMS is compiled into NFD by default. No additional build flags are needed.

```shell
# Standard build
./waf configure
./waf

# Build with unit tests (recommended during development)
./waf configure --with-tests
./waf
```

---

## Configuration

Edit your `nfd.conf` file to enable AMS. The knob can be set independently for UDP and
Ethernet multicast faces.

### Enable on UDP multicast faces

```
face_system
{
  udp
  {
    mcast yes                ; multicast must be enabled
    mcast_suppression yes    ; enable AMS (default: no)
  }
}
```

### Enable on Ethernet multicast faces

```
face_system
{
  ether
  {
    mcast yes                ; multicast must be enabled
    mcast_suppression yes    ; enable AMS (default: no)
  }
}
```

### Enable on both

```
face_system
{
  udp
  {
    mcast yes
    mcast_suppression yes
  }
  ether
  {
    mcast yes
    mcast_suppression yes
  }
}
```

Restart NFD after changing the configuration:

```shell
sudo nfd-stop && sudo nfd-start
```

---

## Verifying AMS is Active

AMS logs all suppression decisions through NFD's logging system. To see suppression activity
in real time, set the log level for the relevant modules:

```shell
# See suppression decisions on the LinkService
NDN_LOG=MulticastSuppression=INFO:LinkService=INFO nfd

# Full debug output
NDN_LOG=MulticastSuppression=DEBUG:LinkService=DEBUG nfd
```

Key log messages to look for:

| Message | Meaning |
|---|---|
| `Interest drop by suppression ... is in flight` | Duplicate Interest dropped immediately |
| `waiting Xms before forwarding` | Suppression delay applied before send |
| `overheard, duplicate forwarding dropped` | Scheduled forward cancelled after overhearing |
| `Suppression time updated with X` | EMA updated, suppression timer adjusted |
| `Moving average before: X after: Y` | EMA calculation result |

---

## How It Works

```
Node receives Interest/Data to forward
         │
         ▼
Is this a multicast face AND is AMS enabled?
         │ No → forward immediately (stock NFD behavior)
         │ Yes
         ▼
Is the same name already in-flight?
         │ Yes → drop (duplicate suppression)
         │ No
         ▼
Look up suppression timer from NameTree (longest prefix match)
         │
         ▼
Schedule delayed forward (random value in [0, 2×suppressionTime])
         │
    During wait...
         ├── Overhear same packet → cancel scheduled forward
         └── Timer fires → forward and record into EMA history
                               │
                               ▼
                    After 30ms window expires:
                    update EMA, adjust suppressionTime (AIMD)
                    store new time back into NameTree
```

---

## Tunable Parameters

All parameters are in `daemon/face/multicast-suppression.cpp`. Changing them requires
recompiling NFD.

| Parameter | Default | Description |
|---|---|---|
| `DISCOUNT_FACTOR` | 0.125 | EMA smoothing factor (α). Lower = slower adaptation |
| `DUPLICATE_THRESHOLD` | 1.3 | EMA above this triggers suppression time increase |
| `MULTIPLICATIVE_INCREASE` | 1.3× | Factor to multiply suppression time when over threshold |
| `ADATIVE_DECREASE` | 5 ms | Amount to subtract from suppression time when under threshold |
| `DEFAULT_INSTANT_LIFETIME` | 30 ms | Duplicate collection window (2× max propagation delay) |
| `MAX_MEASURMENT_INACTIVE_PERIOD` | 300 s | EMA record expires after this period of inactivity |
| `minSuppressionTime` | 15 ms | Minimum suppression delay floor |
| `maxSuppressionTime` | 15,000 ms | Maximum suppression delay ceiling |
| `MAX_IGNORE` | 3 | Consecutive rising duplicate counts to ignore before updating EMA |

---

## Running Unit Tests

```shell
# Build with test support first
./waf configure --with-tests
./waf

# Run all AMS unit tests
./build/unit-tests-daemon -t "Face/TestNameTree:Face/TestEMAMeasurements:Face/TestMulticastSuppressionClass"

# Run with verbose output
./build/unit-tests-daemon --log_level=test_suite -t "Face/TestNameTree:Face/TestEMAMeasurements:Face/TestMulticastSuppressionClass"

# Run a single test case
./build/unit-tests-daemon -t "Face/TestMulticastSuppressionClass/EntryExpiration"
```

Expected output: `*** No errors detected` with 26 tests across 3 suites.

| Suite | Tests | Coverage |
|---|---|---|
| `TestNameTree` | 9 | Trie insert, longest-prefix match, edge cases |
| `TestEMAMeasurements` | 6 | EMA construction, update, suppression time adaptation |
| `TestMulticastSuppressionClass` | 11 | Record, duplicate detection, in-flight tracking, expiration, delay timer |

---

## Source Files

| File | Description |
|---|---|
| `daemon/face/multicast-suppression.hpp` | `NameTree`, `EMAMeasurements`, `MulticastSuppression` declarations |
| `daemon/face/multicast-suppression.cpp` | Core AMS algorithm implementation |
| `daemon/face/link-service.hpp` | `m_suppressionEnabled` flag, `ScopedEventId` storage |
| `daemon/face/link-service.cpp` | Send/receive path integration |
| `daemon/face/generic-link-service.hpp` | `enableMulticastSuppression` option in `Options` struct |
| `daemon/face/generic-link-service.cpp` | Propagates option to `LinkService` |
| `daemon/face/udp-factory.cpp` | Parses `mcast_suppression` from config for UDP faces |
| `daemon/face/ethernet-factory.cpp` | Parses `mcast_suppression` from config for Ethernet faces |
| `tests/daemon/face/multicast-suppression.t.cpp` | Unit tests |
| `docs/multicast-suppression-status.md` | Implementation status and component detail |
| `docs/multicast-suppression-review.md` | Code review findings and fix tracking |

---

## Known Limitations

- **Dropped packet behavior** — suppressed packets do not explicitly notify the upstream PIT
  entry. Retransmission behavior depends on the forwarding strategy.
- **Data suppression asymmetry** — if multiple consumers sent Interests at different times,
  suppressing a Data reply could starve some consumers.
- **EMA granularity** — suppression timers are tracked at `name.getPrefix(-1)`. Very short
  names may cause unrelated prefixes to share a timer.
- **Nacks not suppressed** — Nack forwarding bypasses AMS entirely.

---

## License

AMS is part of NFD and is distributed under the GNU General Public License version 3.
See [`COPYING.md`](COPYING.md) for details.
