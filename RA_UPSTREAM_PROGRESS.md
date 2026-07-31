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
  candidates have a documented provenance decision. P1 adds one exact
  `.gitleaksignore` fingerprint for the official pinned `rcheevos` AES source;
  it is outside the P1 source list and is not a string, hex literal, or
  initializer. No broad path or rule exclusion is used.
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
| P1 native RA core | HOST TESTED | `5d2fc97f` | ASan/UBSan/TSan host tests pass; arm64-v8a and x86_64 test binaries link | P2 adapter, memory, and game-loop wiring are not started |
| P2 memory adapter | CHECKPOINTED | `d8a6bb25` | Fail-closed memory and adapter tests pass under ASan/UBSan; enabled host and Android game targets link | Canonical-ROM memory parity and requested-address coverage are not available without a legal ROM |
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
| P1 outbox classifier | `xmake build -y native_ra_outbox_test && xmake run native_ra_outbox_test` | PASS | Host-only classifier test |
| P1 lifecycle/core | `xmake f -y --mode=debug --pc_sanitize=y --enable_retroachievements=n && xmake build -y native_ra_tests && xmake run native_ra_tests` | PASS | ASan/UBSan lifecycle and concurrency test |
| P1 lifecycle/core | `xmake f -y --mode=debug --pc_sanitize=n --pc_tsan=y --enable_retroachievements=n && xmake build -y native_ra_tests && xmake run native_ra_tests` | PASS | ThreadSanitizer lifecycle and concurrency test |
| P1 Android arm64 | `xmake f -y -p android -a arm64-v8a --enable_retroachievements=n && xmake build -y native_ra_tests` | PASS | Native RA test binary links |
| P1 Android x86_64 | `xmake f -y -p android -a x86_64 --enable_retroachievements=n && xmake build -y native_ra_tests` | PASS | Native RA test binary links |
| P2 manifest | `python3 tools/ra/validate_ra_manifest.py --self-test` | PASS | Rejects malformed overlap, size, and linker-boundary declarations |
| P2 manifest | `python3 tools/ra/validate_ra_manifest.py && python3 tools/ra/generate_tmc_memory_map.py --check` | PASS | Generated manifest hash `ac01bf3b7e31c38eb0cb38a4cf8deadc587d71a99c394005a631c64fba2dae5b`; unresolved entries stay release-invalid |
| P2 host memory | `xmake f -y --mode=debug --pc_sanitize=y --enable_retroachievements=n --game_version=USA` plus the P2 test targets | PASS | `tmc_ra_memory_test` and `tmc_ra_adapter_test` pass under ASan/UBSan |
| P2 host game | `xmake build -y tmc_pc` with `enable_retroachievements=n`, then `=y` | PASS | Default-off build excludes P2 game code; enabled build compiles snapshot publication without the RA runtime |
| P2 Android tests | arm64-v8a and x86_64 P2 test targets with `enable_retroachievements=n` | PASS | Both ABI test binaries link |
| P2 Android game | arm64-v8a and x86_64 `tmc_pc` with `enable_retroachievements=y` | PASS | Both `libmain.so` builds include the snapshot publication bridge only |
| P2 checkpoint | Read-only memory-contract review | PASS AFTER CORRECTION | `gSave.fillerCC` and `gSave.figurines` are now invalid snapshot bytes, matching the unresolved manifest status |

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

- The P2 snapshot has the fixed 0x58000-byte GBA layout: IWRAM at 0x00000,
  EWRAM at 0x08000, and EEPROM at 0x48000. Raw IWRAM/EWRAM stays invalid
  unless an explicit reviewed packer writes it; EEPROM is exported from the
  in-memory save only, with no per-frame file I/O.
- `read_memory_snapshot` returns zeroed bytes and false for every unvalidated
  or out-of-range request. The adapter always reports `fully_validated=false`;
  no achievement may treat this P2 snapshot as live-ready. In particular,
  `gSave.fillerCC` and `gSave.figurines` are intentionally not packed until
  canonical parity proves their GBA offsets.
- No canonical ROM, title checkpoint, mGBA parity capture, or requested-address
  union has been collected. Those are required future gates, not inferred from
  the donor packers.

## Current Change Set

- Files changed: `.gitignore`, `RA_UPSTREAM_PROGRESS.md`, `.gitmodules`,
  `libs/rcheevos`, `libs/native_ra/**`, `port/ra/**`, `port/port_save.{c,h}`,
  `src/main.c`, `tools/ra/**`, and `xmake.lua`.
- Behavior changed: an optional P2 build publishes a fail-closed canonical
  snapshot after `AudioMain`; independent native RA and P2 bridge tests are
  available. `tmc_pc` is still not linked to an RA runtime or network client.
- Behavior intentionally unchanged: all upstream launcher, display, map,
  dungeon, widescreen, RA login, RA UI, RA runtime, submission, and Android
  account behavior.
- Risks: no legal ROM is in this repository; no AYN Thor device evidence has
  been collected; the two private optional launcher submodules are unavailable
  but upstream gates them as optional. The inherited-history scan also reports
  three redacted upstream candidates: `src/fileScreen.c:378` in `6fcfb53d`,
  `src/chooseFile.c:393` in `3399e6e`, and
  `src/introSetTransition.c:319` in `df80390`. P1 is deliberately not linked
  to the game yet, so it cannot identify ROMs, read memory, or submit data.
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

Complete the P2 checkpoint review and evidence commit. Then wire the RA
runtime behind the same optional build gate without loosening the fail-closed
snapshot contract.
