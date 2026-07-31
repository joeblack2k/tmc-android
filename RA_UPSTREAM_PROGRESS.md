# RA Upstream Progress Ledger

## Repository State

- Upstream repository: `https://github.com/samyost1/tmc-android.git`
- Upstream branch: `dual-screen-native`
- Upstream base SHA: `b03cbe5a690b52fe9ea27534fc43a14970a522af`
- Fork repository: not created or pushed
- Integration branch: `feature/native-retroachievements-port`
- Current HEAD: `b03cbe5a690b52fe9ea27534fc43a14970a522af`
- Donor snapshot: `zeldaTMC.zip`, SHA-256
  `c5e6e82554ecdf303290d75d981be3e6d449a44c7d79b544edee599576ee0295`,
  embedded donor revision `11b313dcfc51609e6a4fa9508cbb914eea2b5243`
- rcheevos pin for the donor port: `v12.3.0`,
  `e9ca3694c862b61235595176dac4b22677848c93`
- Android package ID: baseline `dev.picori.tmc`; planned RA side-by-side ID
  `dev.picori.tmc.ra`

## Security Inventory

- Secret scan: `gitleaks detect --source . --no-git --redact=100
  --max-archive-depth=1 --no-banner` scanned the complete TMC input set before
  target edits and found no leaks. The same scan passed on the edited target
  repository. A Git-history scan covered 2,992 inherited upstream commits and
  reported three redacted `generic-api-key` candidates in historical commits;
  they are absent from the current checkout. Do not push until those historical
  candidates have a documented provenance decision.
- Ignored private paths: `/private/`, `/artifacts/`, ROMs, saves, states,
  tokens, keystores, RA snapshots, provenance/validity captures,
  `requested.bin`, and `overworld.jpg`.
- ROM present in the target repository: no.
- Save/capture paths in the target repository: no.
- Credentials present in Git: no.

## Milestone Status

| Milestone | Status | Commit | Evidence | Open Gate |
|---|---|---|---|---|
| P0 baseline | ANDROID PACKAGED | Uncommitted | Host and both Android ABIs built; Debug and Release APKs packaged | No legal ROM or Thor observation; inherited-history scan needs a provenance decision before push |
| P1 native RA core | NOT STARTED | | | |
| P2 memory adapter | NOT STARTED | | | |
| P3 runtime wiring | NOT STARTED | | | |
| P4 Android secure login | NOT STARTED | | | |
| P5 RA panel | NOT STARTED | | | |
| P6 Spectator APK | NOT STARTED | | | |
| P7 Casual state/policy | NOT STARTED | | | |
| P8 outbox | NOT STARTED | | | |
| P9 address union | NOT STARTED | | | |
| P10 PR extraction | NOT STARTED | | | |

## Baseline Matrix

| Command | Result | Notes |
|---|---|---|
| `xmake f -y --game_version=USA` | PASS | macOS arm64 host configuration |
| `xmake build -y tmc_pc` | PASS | Host build completed in 25.817s; existing compiler warnings recorded below |
| `xmake f -y -p android -a arm64-v8a --ndk="$ANDROID_NDK_HOME" --game_version=USA --gpu_renderer=y --widescreen_width=384` | PASS | Android API 21, NDK r26 |
| `xmake build -y tmc_pc` | PASS | Android `arm64-v8a` native library completed in 32.222s |
| `xmake f -y -p android -a x86_64 --ndk="$ANDROID_NDK_HOME" --game_version=USA --gpu_renderer=y --widescreen_width=384` | PASS | Android API 21, NDK r26 |
| `xmake build -y tmc_pc` | PASS | Android `x86_64` native library completed in 29.850s |
| `ANDROID_HOME="$ANDROID_SDK_ROOT" ./gradlew assembleDebug` | PASS | Debug APK packages both ABIs |
| `ANDROID_HOME="$ANDROID_SDK_ROOT" ./gradlew assembleRelease` | PASS | Local fallback debug signer; no release key was supplied |
| `ANDROID_HOME="$ANDROID_SDK_ROOT" ./gradlew test` | PASS | Current upstream has no Android unit-test sources |

Existing baseline warnings:

- `include/region.h` has extraneous comparison parentheses.
- `port/port_offset_USA.h` redefines `offset_gAreaRoomMap_None`.
- Gradle cannot strip `libmain.so` and packages it unchanged.

## Test Evidence

| Test/build | Exact command | Result | Artifact/hash |
|---|---|---|---|
| Debug package | `./gradlew assembleDebug` | PASS | `app-debug.apk`: `eb96eda01189919d0d0c88a347f8db24e0be763927bd0f2f081c3a7fc82fb2ba` |
| Release package | `./gradlew assembleRelease` | PASS | `app-release.apk`: `7e7cab9a93a9b1fd58f11ef6e67bec7d9d2ceafdc89cd45680c2a3304436670d` |
| APK package/ABI | `aapt dump badging <apk>` | PASS | `dev.picori.tmc`, `arm64-v8a`, `x86_64` in Debug and Release |
| APK signer | `apksigner verify --verbose --print-certs <apk>` | PASS | Debug signer, SHA-256 `43566c58c3385d0fad832bffa6a4286fb1b69d4735c819228296fce00a2befcf` |

## Device Evidence

- Device serial/model: not checked in this milestone.
- Install method: not attempted.
- Top display result: not observed.
- Bottom display result: not observed.
- RA login result: not applicable; RA is not integrated.
- Token restore result: not applicable.
- Lifecycle matrix result: not observed.
- Submission mode: no RA client is present.

## Memory-Parity Evidence

No RA memory snapshot or requested-address evidence exists on this branch.

## Current Change Set

- Files changed: `.gitignore`, `RA_UPSTREAM_PROGRESS.md`.
- Behavior changed: none.
- Behavior intentionally unchanged: all upstream launcher, display, map,
  dungeon, widescreen, runtime, and Android behavior.
- Risks: no legal ROM is in this repository; no AYN Thor device evidence has
  been collected; the two private optional launcher submodules are unavailable
  but upstream gates them as optional. The inherited-history scan also reports
  three redacted upstream candidates: `src/fileScreen.c:378` in `6fcfb53d`,
  `src/chooseFile.c:393` in `3399e6e`, and
  `src/introSetTransition.c:319` in `df80390`.
- Rollback: reset this branch to
  `b03cbe5a690b52fe9ea27534fc43a14970a522af`.

## Known Limitations

- `TMC_RA_UPSTREAM_PORT_PLAN.zip` was not present in the input directory. The
  equivalent top-level plan Markdown files were present and read before code
  changes.
- Release packaging currently uses the local debug signer because no release
  signing material was provided. This is valid for the baseline only.
- The host build cannot be smoke-run without a user-supplied legal ROM.

## Next Smallest Objective Gate

Re-scan the edited target repository, review the staged P0 diff, commit the
baseline ledger, then inventory the donor RA core and the current upstream
build/runtime boundaries before porting the optional RA core.
