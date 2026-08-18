# Adaptive Multicast Suppression (AMS) — Code Review

> Reviewed: 2026-04-05  
> Files: `multicast-suppression.hpp/.cpp`, `link-service.hpp/.cpp`, `multicast-suppression.t.cpp`

---

## Critical Bugs

### C1 — `cancelIfSchdeuled` mutates the caller's Name
**File:** `link-service.cpp:159`  
**Status:** [x] Fixed

```cpp
// WRONG — appendNumber returns *this, entry_name is just an alias to the mutated name
auto entry_name = name.appendNumber(type);

// CORRECT
Name entry_name = name;
entry_name.appendNumber(type);
```

`appendNumber()` modifies the Name in-place and returns a reference to `*this`. Every call to
`cancelIfSchdeuled` permanently corrupts the name passed in. This breaks any subsequent lookup
using that name.

> **Fix applied:** Changed to `Name entry_name = name; entry_name.appendNumber(type);` — explicit
> copy makes type `Name` (not `Name&`) and the intent unambiguous.

---

### C2 — Lambda captures `this` with no lifetime guarantee
**File:** `link-service.cpp:79, 127` | `multicast-suppression.cpp:295, 337, 353`  
**Status:** [x] Fixed

The lambdas scheduled for delayed forwarding capture `this` raw. If the face is torn down before
the timer fires, accessing `this->nOutInterests` or calling `doSendInterest` is a use-after-free.

Same problem in `multicast-suppression.cpp`: `vec` and `nameTree` are raw pointers to member
variables captured inside scheduler lambdas. If `MulticastSuppression` is destroyed before the
timer fires, they dangle.

**Fix:** Use `shared_from_this()` or ensure all scheduled events are cancelled in the destructor
before the object is destroyed.

> **Fix applied:** Changed `m_scheduledEntry` to `std::map<Name, scheduler::ScopedEventId>`,
> `m_expirationId` in `EMAMeasurements` to `scheduler::ScopedEventId`, and
> `m_objectExpirationTimer` to `std::map<Name, scheduler::ScopedEventId>`. All EventIds are now
> stored via `std::move`. `ScopedEventId` auto-cancels on destruction, so all pending timers are
> cancelled when the face or suppression object is torn down.

---

## High Severity

### H1 — `UNSET = -1234` assigned to `unsigned int`
**File:** `multicast-suppression.cpp:35`  
**Status:** [ ] Open

```cpp
unsigned int UNSET = -1234; // wraps to 4294966062 on 32-bit
```

This giant value propagates into `longestPrefixMatch` return, then into `2 * suppressionTime`
cast to `int` — causing overflow and undefined behavior.

**Fix:** Use `const double UNSET = -1.0;` or `std::optional<double>` to represent "no value".

---

### H2 — Global variables are not `const`
**File:** `multicast-suppression.cpp:35–37`  
**Status:** [ ] Open

```cpp
unsigned int UNSET = -1234;   // should be constexpr
int CHARACTER_SIZE = 126;     // unused and mutable
int MAX_IGNORE = 3;           // mutable global
```

Mutable globals cause linkage issues if the file is ever included from multiple translation units,
and allow accidental modification.

**Fix:** `static constexpr` for all three. Remove `CHARACTER_SIZE` if it is unused.

---

### H3 — EMA constructor ignores its `lastDuplicateCount` parameter
**File:** `multicast-suppression.cpp:140`  
**Status:** [ ] Open

```cpp
EMAMeasurements::EMAMeasurements(double expMovingAverage, int lastDuplicateCount, ...)
  : m_lastDuplicateCount(1)  // always hardcoded — parameter silently discarded
```

The `lastDuplicateCount` argument passed by callers (e.g. tests) has no effect.

**Fix:** Change to `: m_lastDuplicateCount(lastDuplicateCount)` or remove the parameter.

---

### H4 — `getRandomNumber(0)` is undefined behavior
**File:** `multicast-suppression.cpp:42`  
**Status:** [ ] Open

```cpp
return ndn::random::generateWord32() % upperBound; // UB if upperBound == 0
```

When `suppressionTime` rounds to 0, `2 * suppressionTime = 0`, and `% 0` is undefined behavior
in C++. No bounds check exists anywhere in the call chain.

**Fix:**
```cpp
int getRandomNumber(int upperBound) {
  if (upperBound <= 0) return 0;
  return ndn::random::generateWord32() % upperBound;
}
```

---

### H5 — Interest and Data expiration timer keys may collide
**File:** `multicast-suppression.cpp:273–274` and `link-service.cpp:78, 126`  
**Status:** [ ] Open

Interests are keyed with `name.appendNumber(0)` and Data with `name.appendNumber(1)` in
`m_objectExpirationTimer`. However, `recordData` does `name_cop.appendNumber(0)` when looking
up whether a pending interest timer should be cancelled. If the interest and data share the exact
same base name the lookup is correct, but if naming conventions drift between send and receive
paths the keys will not match and timers will be orphaned.

**Fix:** Document and enforce a strict naming convention for timer keys, or use a dedicated
`struct TimerKey { Name name; char type; }` as the map key.

---

### H6 — `setUpdateExpiration` overwrites EventId without checking if already fired
**File:** `multicast-suppression.cpp:308–311`  
**Status:** [ ] Open

```cpp
itr_timer->second.cancel();  // may be a no-op if already fired
itr_timer->second = eventId; // assignment of EventId may not be safe
```

Calling `cancel()` on an already-fired event is implementation-defined in the NDN scheduler.
Safer to erase the old entry and insert fresh.

**Fix:**
```cpp
m_objectExpirationTimer[name] = eventId; // handles create + update safely
```

---

### H7 — Forwarded status lost after entry expiration
**File:** `multicast-suppression.cpp:232`  
**Status:** [ ] Open

`getForwardedStatus` looks up `m_interestHistory` which only lives for 30ms. Once the entry
expires and is erased, a subsequent `recordInterest` call for the same name always sees
`isForwarded = false`, losing the history of whether this node previously forwarded. This can
cause the algorithm to make incorrect suppression decisions for recurring names.

**Fix:** Persist the forwarded status inside `EMAMeasurements` so it survives the 30ms window.

---

## Medium Severity

### M1 — EMA ignore-counter logic is ambiguous
**File:** `multicast-suppression.cpp:159–165`  
**Status:** [ ] Open

```cpp
m_ignoreDuplicateRecoring = (duplicateCount > m_lastDuplicateCount)
                            ? (m_ignoreDuplicateRecoring + 1) : 0;

if (m_ignoreDuplicateRecoring > 0 && m_ignoreDuplicateRecoring < MAX_IGNORE) {
    return; // skip EMA update
}
```

This skips the first 2 consecutive increases but lets the 3rd through. The intent is not
documented. Why 3? Why not skip all of them until the count stabilizes?

**Fix:** Add a comment explaining the design intent, or simplify to
`if (m_ignoreDuplicateRecoring < MAX_IGNORE) return;`.

---

### M2 — EMA is only updated on expiration, not in real-time
**File:** `multicast-suppression.cpp:295–303`  
**Status:** [ ] Open

Duplicate counts accumulate during the 30ms window but the EMA is fed only when the entry timer
fires. The algorithm is always at least 30ms behind actual network conditions.

**Fix:** Consider updating EMA immediately when a duplicate threshold is crossed, or document
this as an intentional design trade-off.

---

### M3 — Data suppression may drop legitimate replies
**File:** `link-service.cpp:111–120`  
**Status:** [ ] Open

The comment in `sendData` acknowledges this: if multiple consumers sent Interests at slightly
different times, suppressing a Data reply could starve consumers whose Interest was not served
by the unsuppressed copy. No solution is implemented.

**Fix:** This requires a design decision — either track per-consumer PIT entries or document
as a known limitation with a recommendation to use unicast for consumer-sensitive traffic.

---

### M4 — `ssthresh` misspelled as `ssthress`
**File:** `multicast-suppression.hpp:81–83`  
**Status:** [ ] Open

```cpp
void setSSthress(double val, int factor = 2) { m_ssthress = val/factor; }
double m_ssthress;
```

Also `setSSthress` is defined but never called anywhere in the codebase — dead code.

**Fix:** Rename to `ssthresh` and remove or wire up the dead method.

---

## Low Severity / Typos

### L1 — Spelling errors in identifiers
**Status:** [ ] Open

| Location | Current | Correct |
|---|---|---|
| `multicast-suppression.cpp:18` | `MAX_PROPOGATION_DELAY` | `MAX_PROPAGATION_DELAY` |
| `multicast-suppression.hpp:95` | `m_ignoreDuplicateRecoring` | `m_ignoreDuplicateRecording` |
| `link-service.hpp/cpp` | `cancelIfSchdeuled` | `cancelIfScheduled` |
| `multicast-suppression.hpp:81` | `setSSthress` / `m_ssthress` | `setSSthresh` / `m_ssthresh` |

---

### L2 — Lambda captures should be explicit
**File:** `multicast-suppression.cpp:295, 337, 353`  
**Status:** [ ] Open

`[=]` captures everything by value including unintended variables.

**Fix:** Explicitly list captures: `[this, vec, nameTree, name]`.

---

## Test Coverage Gaps

### T1 — No test for `cancelIfSchdeuled` name mutation
The critical C1 bug has no regression test. A name passed in should not be modified.

### T2 — No test for `UNSET` sentinel in `NameTree`
`getSuppressionTimer` with no prior insert should return a value in `[0, 2*minSuppressionTime)`,
not something derived from the unsigned-wrapped `UNSET`.

### T3 — No test for `getRandomNumber(0)` edge case
When suppression time is 0, the modulo by zero path is never exercised.

### T4 — No test for EMA ignore-counter boundary
No test verifies behavior at exactly `MAX_IGNORE` consecutive increases.

### T5 — No test for forwarded-status persistence across expiration
`EntryExpiration` test only checks `interestInflight` goes false — does not verify that EMA
received the correct `wasForwarded` value after the entry expired.

### T6 — No test for timer key collision (interest vs data same name)
No test verifies that scheduling an interest timer and a data timer for the same name do not
interfere with each other.

---

## Fix Priority

| ID | Severity | Fix Effort | Status |
|---|---|---|---|
| C1 | Critical | Trivial | ✅ Fixed |
| C2 | Critical | Medium | ✅ Fixed |
| H1 | High | Trivial | [ ] Open |
| H4 | High | Trivial | [ ] Open |
| H2 | High | Easy | [ ] Open |
| H3 | High | Easy | [ ] Open |
| H6 | High | Easy | [ ] Open |
| H7 | High | Medium | [ ] Open |
| H5 | High | Medium | [ ] Open |
| M4 | Medium | Trivial | [ ] Open |
| L1 | Low | Trivial | [ ] Open |
| M1 | Medium | Easy | [ ] Open |
| M2 | Medium | Design | [ ] Open |
| M3 | Medium | Design | [ ] Open |
| L2 | Low | Trivial | [ ] Open |
| T1–T6 | — | Medium | [ ] Open |
