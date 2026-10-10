# AGENTS.md

`hunt` is a Qt6 Widgets desktop client for the **uhunt** API
(`https://uhunt.onlinejudge.org/api`), the statistics service for the UVa Online
Judge. Read-only: it only uses public JSON endpoints. See `README.md` for the
feature list.

## Build & run

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
./build/hunt
```

- Requires system Qt6 (`qt6-base`: Core/Gui/Widgets/Network) + CMake 3.16+.
- **Gotcha:** an Anaconda `qmake`/Qt may shadow the system one in `PATH`. CMake
  config still resolves the right Qt here, but if it doesn't, pass
  `-DCMAKE_PREFIX_PATH=/usr/lib/cmake/Qt6`.
- No tests, linter or CI exist. Verify changes by building and running the app
  (`QT_QPA_PLATFORM=offscreen ./build/hunt` works for a headless smoke test).
- `#include` new sources in `CMakeLists.txt`; `AUTOMOC` is on, so new `Q_OBJECT`
  classes are moc'd automatically.

## Architecture

- `src/UhuntClient.*` – all HTTP + JSON parsing. Async only; results are emitted
  via signals. One method per endpoint. Start here when adding API calls.
- `src/Types.*` – `Problem`/`Submission`/`UserRank`/`LiveEvent`/`Category`
  structs, the shared `ProblemCatalog` lookup (`byPid`, `pidByNum`) and
  verdict/language/time formatting helpers. `Category` stores a flattened
  `Chapter / Section / Topic` path, the exercise problem `nums` (absolute), and
  a `starred` subset (the book's harder, must-try problems; the API marks these
  with negative numbers).
- `src/*TableModel.*` – `QAbstractTableModel` subclasses. They hold a
  `const ProblemCatalog *` (owned by `MainWindow`) to resolve problem titles.
  Return the raw value for `Qt::UserRole` so sorting is numeric.
- `src/ProblemFilterProxyModel.*` – search / solved-status / CP-book category
  filtering of the problem list.
- `src/MainWindow.*` – builds the tabs and wires signals to slots. All UI state
  (current uid, solved/tried pid sets, catalog) lives here. The toolbar's
  **Refresh** button (and File → Refresh, `F5`) calls `onRefreshAll()`, which
  reloads the problem catalogue and -- when signed in -- the account's
  submissions and ranklist, honouring the current tab selections; the
  `reloadSubmissions()` / `reloadRanklist()` helpers are shared with the tab
  combo boxes. The Dashboard tab's
  profile section shows the solved count with the percentage of the loaded
  problem catalogue solved (`updateSolvedProgress()`) and a `QProgressBar`. It
  also shows the **Competitive Programming 4 (CP4) Exercises** `QTreeWidget`
  (`rebuildCpBookTree()`), mirroring uhunt's CP-book page: chapters → sections →
  topics → problems with separate **Level / Solved / Total / %** columns and a
  **Start** button on each unfinished (unsolved) problem (currently inert; the
  problem id is stashed on the button's `pid` property). The **Level** column
  shows uhunt's per-problem difficulty, derived from `dacu` (distinct accepted
  users): `10 - floor(min(10, ln(dacu)))`, i.e. "Lev N" as on uhunt
  (10 = hardest, 0 = easiest). Two mutually-exclusive checkboxes
  ("Only solved" / "Only unsolved") and a **Level** combo box ("All levels" /
  level 0–10) filter the tree to matching problems, hiding branches with
  nothing left to show. Problem rows put their ✔/tried
  status in the Solved column. ★ still marks starred problems in the title. No
  book images.
- The **Levels** tab (`buildLevelsTab()` / `rebuildLevelsTree()`) lists every
  loaded catalogue problem grouped under "Level 0"…"Level 10" headings (only the
  non-empty levels, problems sorted by number). It has **Problem / Chapter /
  Section / Solved** columns; Chapter/Section come from a first-occurrence
  problem-number → chapter/section map built from `m_categories` (blank for
  problems outside the CP book, and only the first listing for the handful of
  problems in more than one chapter/section). No chapter/section grouping
  headings are shown. Two mutually-exclusive checkboxes ("Only solved" / "Only
  unsolved", `m_levelsFilterSolved` / `m_levelsFilterUnsolved`) filter the tree,
  dropping non-matching problems and any level heading left empty. The level is
  the same `dacu`-derived value as the CP tree.
- CP-book data: `resources/cpbook4.json`, embedded via `resources.qrc`. The
  uhunt API does not serve edition 4 (only 1–3), so the CP4 tree is extracted
  from uhunt's own client-side `cpbook_4ed` data and loaded with
  `UhuntClient::loadCpBook()` at startup. `UhuntClient::fetchCpBook()` still
  covers the API editions 1–3. The JSON nests `{title, arr:[...]}` chapters →
  sections → groups, where a group is `[topicTitle, num, num, ...]` and negative
  `num` means a starred problem.
- Account: `MainWindow` persists the signed-in username in `QSettings`
  (`account/username`) and auto-signs-in on launch via `onHunt()`; `onSignOut()`
  clears the account and all per-user models. Only the username is stored. The
  default username (when nothing is saved) is the `kDefaultUsername` constant in
  `src/MainWindow.cpp` (currently `gmwRetired`).
- **No chat:** uhunt's "sign in" is actually a chat login (`/chat/*`, uHunt
  code), and it was deliberately left out. Do not add `/chat/*` or
  `/poll/{sesid}` session polling; the app uses only the public `/api/*`
  endpoints plus the `/api/poll` live feed.
- `src/ProblemDetailsDialog.*` – statistics popup; data comes from the already
  loaded catalog, so it does no network I/O.

## uhunt API notes (verified against the live service)

Base URL `https://uhunt.onlinejudge.org/api`. Add a `User-Agent`; responses are
JSON (some are JSON arrays of arrays).

| Endpoint | Returns |
| --- | --- |
| `/uname2uid/{uname}` | integer uid, `0` if unknown |
| `/p` | array of **21-element** problem arrays (see mapping in `UhuntClient::parseProblems`) |
| `/p/id/{pid}`, `/p/num/{num}` | one problem object |
| `/subs-user/{uid}[/{min-sid}]` | `{name, uname, subs:[[sid,pid,ver,run,sbt,lan,rank]]}` |
| `/subs-user-last/{uid}/{count}` | same, last N |
| `/ranklist/{uid}/{nabove}/{nbelow}` | array of `{rank,old,userid,name,username,ac,nos,activity[5]}` – **three** path args |
| `/rank/{pos}/{count}` | global ranklist |
| `/p/rank/{pid}/{rank}/{count}`, `/p/ranklist/{pid}/{uid}/{nabove}/{nbelow}` | per-problem ranklists |
| `/cpbook/{edition}` | nested CP-book exercise categories; **editions 1–3 only** (4 returns `invalid cpbook version`) |
| `/poll/{poll-id}` | live events, long-polling (holds up to 60s). Start with `0` |
| `/contests`, `/contests/id/{id}`, `/subs-pids`, `/subs-nums`, `/solved-bits` | not yet used |

- Verdict ids: `90` accepted, `70` WA, `50` TLE, `30` CE, `40` RE, `20` in queue.
- Submission tuple order is `[sid, pid, ver, run, sbt, lan, rank]`; problem id
  (`pid`) is **not** the problem number (`num`). The catalog maps between them.
- Live event shape: `{id, type, msg:{sid,uid,pid,ver,lan,run,mem,rank,sbt,name,uname}}`.
  `UhuntClient::startLivePolling` re-arms itself with the last seen `id`.
- Full API reference: `https://uhunt.onlinejudge.org/api`.
