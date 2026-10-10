# hunt

A Qt6 desktop client for [uhunt.onlinejudge.org](https://uhunt.onlinejudge.org),
the statistics service for the UVa Online Judge.

It re-creates the main uHunt features as a native app, talking directly to the
public uHunt JSON API (`https://uhunt.onlinejudge.org/api`) over HTTPS.

## Features

- **Account / sign in** – the toolbar defaults to the username **`gmwRetired`**
  and auto-loads it on launch. You can type any other UVa username in the toolbar
  field and press **Enter**; `uname2uid` resolves the account, then submissions
  and the ranklist are loaded. The account is remembered (username only); use
  **Account → Sign out** to clear it for the current session. The toolbar's
  **Refresh** button (or **F5**) reloads everything from uHunt: the problem
  catalogue and the current account's submissions and ranklist.
- **Dashboard** – a profile section with the **global ranking** as the headline
  stat, plus **solved problems** (with the percentage of the loaded problem
  catalogue solved and a progress bar) and **total submissions**; followed by the
  **Competitive Programming Exercises** (CP4 book) tree with **Level**,
  **Solved**, **Total** and **%** columns (★ still marks starred, must-try
  problems in a problem's title), expandable into sections → topics → problems,
  each problem marked solved (✔), tried, or unsolved in the Solved column and
  showing uhunt's difficulty **Level** (from `dacu`); unfinished (unsolved)
  problems carry a **Start** button. Checkboxes filter the tree to
  **only solved** or **only unsolved** problems, and a **Level** dropdown shows
  only problems of a chosen level. Double-click a problem for its
  statistics. No book images.
- **Problems** – the full ~5000-problem catalog with search (number/title),
  solved/unsolved filter, and a filter by Competitive Programming book exercise
  category. Solved problems are highlighted. Double-click for full statistics
  and a link to the problem on onlinejudge.org.
- **Levels** – every loaded problem grouped by its uHunt difficulty **level**
  (Level 0–10, 10 = hardest), sorted by problem number, with **Chapter** and
  **Section** columns showing where each problem sits in the CP book (blank for
  problems outside it). Only the level headings are shown — no chapter/section
  grouping. Checkboxes filter to **only solved** or **only unsolved** problems.
  Double-click for full statistics.
- **My Submissions** – full submission history or just the last N, with verdicts,
  language, runtime, rank and submit time. Verdicts are colour-coded.
- **Ranklist** – users around you (5 or 25 each side), or the global top 100
  or top 500.
- **Live** – a live submission feed driven by the `/api/poll` long-polling
  endpoint.

## Build

Requires Qt 6 (Core, Gui, Widgets, Network) and CMake 3.16+.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
./build/hunt
```

If CMake picks up the wrong Qt (for example an Anaconda Qt in `PATH`), point it
at the system install:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/usr/lib/cmake/Qt6
```

On this machine Qt 6.11 is installed system-wide via the `qt6-base` package.

## Notes / limitations

- The app is a read-only client: it only exposes what the public uHunt API
  provides. There is **no chat** feature and no password handling. "Sign in"
  here means "remember and auto-load this UVa username"; it does not create a
  server-side login session.
- Live events only cover the last 100 submissions; if the feed is not polled
  for a while some events are missed (server-side behaviour).
- The **Competitive Programming 4 (CP4)** book exercises are shown. The uhunt
  API only serves editions 1–3 (`/api/cpbook/{edition}`; edition 4 returns
  `invalid`), so the CP4 exercise tree is bundled with the app as
  `resources/cpbook4.json` (extracted from uhunt's own client-side data) and
  loaded via a Qt resource.
- There is no caching: every launch reloads the problem list from uHunt.

## Account storage

The signed-in username is stored with `QSettings` (on Linux:
`~/.config/hunt/hunt.conf`, key `account/username`). Only the username is
persisted — never a password or any secret. When no account is saved, the app
falls back to the built-in default username; sign-out is per-session (the
default returns on the next launch).
