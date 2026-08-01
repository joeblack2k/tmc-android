# RA Upstream Progress Ledger

## Repository State

- Upstream repository: `https://github.com/samyost1/tmc-android.git`
- Upstream branch: `dual-screen-native`
- Upstream base SHA: `b03cbe5a690b52fe9ea27534fc43a14970a522af`
- User fork (`joeblack2k/tmc-android`): created, pushed, and used as the PR head
- Integration branch: `feature/native-retroachievements-port`
- Latest published milestone: `13a0838d` (RA parity tooling and publication evidence)
- Current working tree: Casual-only admission/UI corrections are being verified on
  `feature/native-retroachievements-port`
- Donor snapshot: `zeldaTMC.zip`, SHA-256
  `c5e6e82554ecdf303290d75d981be3e6d449a44c7d79b544edee599576ee0295`,
  embedded donor revision `11b313dcfc51609e6a4fa9508cbb914eea2b5243`
- rcheevos pin for the donor port: `v12.3.0`,
  `e9ca3694c862b61235595176dac4b22677848c93`
- Android package ID: baseline `dev.picori.tmc`; current RA side-by-side ID
  `dev.picori.tmc.ra`
- Re-audit 2026-08-01: `origin/dual-screen-native` still resolves to
  `b03cbe5a690b52fe9ea27534fc43a14970a522af`; local `artifacts/` and `private/`
  roots were recreated and remain Git-ignored. The private roots now contain the
  legal USA ROM and an isolated title save for local parity work; no emulator
  state or credentials were added. The current upstream
  `samyost1/tmc-android` is itself a fork of `Raekwon1603/tmc-android`; the user
  fork `joeblack2k/tmc-android` carries the published branch.

## Security Inventory

- Secret scan: `gitleaks detect --source . --no-git --redact=100
  --max-archive-depth=1 --no-banner` scanned the complete TMC input set before
  target edits and found no leaks. The same scan passes on the edited target
  repository. A Git-history scan covered 2,992 inherited upstream commits and
  reported three redacted `generic-api-key` candidates; context review on
  2026-07-31 classified all three as false positives: assignments involving
  upstream gameplay input state (`keys`/`newKeys`) rather than credential
  literals. They are absent from the current checkout. P1 adds one exact
  `.gitleaksignore` fingerprint for the official pinned `rcheevos` AES source;
  it is outside the P1 source list and is not a string, hex literal, or
  initializer. No broad path or rule exclusion is used.
- Ignored private paths: `/private/`, `/artifacts/`, ROMs, saves, states,
  tokens, keystores, RA snapshots, provenance/validity captures,
  `requested.bin`, and `overworld.jpg`.
- Tracked ROM in the target repository: no. Ignored local input:
  `private/canonical-usa.gba`, 16,777,216 bytes, mode 600, MD5
  `a104896da0047abe8bee2a6e3f4c7290`, SHA-1
  `b4bd50e4131b027c334547b4524e2dbbd4227130`.
- Tracked save/capture paths in the target repository: no. The ignored local
  title save is mode 600; its contents and fingerprint are not recorded in this
  ledger. The ignored mGBA metadata retains a SHA-256 provenance field only.
- Credentials present in Git: no.
- GitHub auth recheck 2026-08-01: `gh auth status` confirms the active
  `joeblack2k` account with `repo` and `workflow` scopes; the target fork
  `joeblack2k/tmc-android` is the `origin` push target; draft PR #16 targets
  `samyost1/tmc-android:dual-screen-native`. Casual admission remains blocked
  until the parity gates are accepted.

## Milestone Status

| Milestone | Status | Commit | Evidence | Open Gate |
|---|---|---|---|---|
| P0 baseline | ANDROID PACKAGED | Uncommitted | Host and both Android ABIs built; vanilla and RA APKs packaged and installed side-by-side | Full S0-S14 gameplay matrix and parity remain open |
| P1 native RA core | HOST TESTED | `5d2fc97f` | Current native lifecycle, classifier and concurrency coverage passes under ASan/UBSan; Android ABI link gates pass | Real service/game evidence remains downstream |
| P2 memory adapter | IMPLEMENTED, PARITY-BLOCKED | `d8a6bb25` | Fail-closed memory/adapter tests and manifest validators pass; unresolved bytes remain invalid | Canonical-ROM memory parity and requested-address coverage are unavailable without a legal ROM/capture |
| P3 runtime wiring | HOST/ANDROID BUILT | `0173fbcb` | Owner-thread runtime, snapshot publication, foreign-thread no-ops, monotonic time and lifecycle cleanup pass; both Android ABIs link | Submission proof, canonical parity, and full gameplay matrix remain open |
| P4 Android secure login | SOURCE + HOST TESTED, DEVICE LOGIN PROVEN | Uncommitted | Generation-tagged queue, JNI owner-thread drain, HTTPS allowlist, bounded executor, API-23-compatible AES-256-GCM Keystore credential store, mutable-byte credential/JNI paths, RA-only manifest policy, lifecycle detach/reopen, and password dialog are implemented and tested; Thor accepts password login and re-login after logout | Authenticated set/game submission and encrypted outbox replay remain unproven |
| P5 RA panel | SOURCE + HOST TESTED, DEVICE PANEL PROVEN | `3d70eafa` + working correction | Status/account/mode/metadata/achievement/toast bridge and second-screen panel source are implemented; the user-facing mode is static Casual and the Casual chip is not a mode toggle | Full S0-S14 parity and achievement-trigger evidence remain open |
| P6 RA APK | SIDE-BY-SIDE INSTALLED, AUTHENTICATED START PROVEN | `3d70eafa` + working correction | `dev.picori.tmc.ra` and vanilla `dev.picori.tmc` coexist on AYN Thor; RA has `INTERNET`, vanilla does not; both carry arm64-v8a and x86_64; the trigger-enabled RA APK hash matches the device base APK | Casual admission remains fail-closed before game identification until parity is proven |
| P7 Casual state/policy | IMPLEMENTED, ADMISSION BLOCKED | Uncommitted | Versioned CRC state block, fail-closed Casual/Strict predicates, quicksave policy guards and focused tests pass | Casual admission remains blocked until memory validation/parity is proven |
| P8 outbox | HOST + ANDROID TESTED | Uncommitted + `abcae302` | Encrypted Android store, JNI secure-blob bridge, bounded CRC journal, journal-before-HTTP, confirmed-success removal, replay scoping and mixed-scope regression pass | Thor encrypted outbox/replay and authenticated submission are not physically proven |
| P9 address union | TOOLING PASS, EVIDENCE OPEN | Uncommitted | Ordered `tools/ra/tmc_ra_state_manifest.json` covers S0-S14; strict matrix comparator validates identity, phase, generation, requested counters, and fail-closed cumulative union; native post-evaluation capture and shared V1 input-replay contracts pass; the Linux CI leg exercises the C capture-contract test; the Android app-scoped operator trigger is physically proven separately | A current S0 validated-scope capture exists, but its requested bitmap is empty without an authenticated RA set; synchronized S0-S14 native/mGBA captures are absent, so union and full parity remain unverified |
| M0 lifecycle hardening | HOST + DEVICE LIFECYCLE PROVEN | `9f05b58a` + working correction | Identify retries are bounded at two attempts; failed token/password login and logout unload RA state; Casual-only admission is locked while parity is unproven; focused tests pass under ASan/UBSan and TSan; Thor proves login, token restore after relaunch, logout cleanup, and re-login | Invalid-token behavior is host-tested; full memory/parity and submission gates remain open |
| M1 authenticated RA service flow | DEVICE PASS, HISTORICAL | `13a0838d` | On Thor, legal ROM gameplay reaches file select and first overworld; the external display showed authenticated RA account/set state and achievement counters. This historical service proof is retained for connectivity only; it is not Casual admission proof | No submission was attempted; canonical parity remains open |
| M2 credential lifecycle | DEVICE + HOST PASS | Uncommitted | Force-stop/relaunch restores authenticated state; explicit logout changes the panel to `NOT LOGGED IN`, `LOGIN REQUIRED`, `UNKNOWN`, and `NO SET LOADED`; a subsequent password login restores `ONLINE` set state; native rejected-token cleanup tests pass | Physical invalid-token injection and encrypted outbox replay remain untested |
| P10 PR extraction | DRAFT PR OPEN | `13a0838d` + working correction | Ledger/security evidence is published on the user fork; current-tree gitleaks is clean and the three historical candidates are documented false positives | Gameplay matrix, parity, outbox replay, release, and submission gates remain open |

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
| `ANDROID_HOME="$ANDROID_SDK_ROOT" ./gradlew test` | PASS | HTTPS allowlist unit tests pass for debug, RA, and release variants |

Existing baseline warnings:

- `include/region.h` has extraneous comparison parentheses.
- `port/port_offset_USA.h` redefines `offset_gAreaRoomMap_None`.
- Gradle cannot strip `libmain.so` and packages it unchanged.

## Test Evidence

| Test/build | Exact command | Result | Artifact/hash |
|---|---|---|---|
| Debug package | `./gradlew assembleDebug` | PASS | `app-debug.apk`: `eb96eda01189919d0d0c88a347f8db24e0be763927bd0f2f081c3a7fc82fb2ba` |
| Release package | `ANDROID_HOME=/opt/homebrew/share/android-commandlinetools ./gradlew --no-daemon :app:assembleRelease --rerun-tasks` | PASS | Current `app-release.apk`: `4a8d149e54552309bf1dd1475d2a361152e5f3633991475d2650e4039f9e2927`; arm64-v8a and x86_64 |
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
| P3 lifecycle core | Existing `native_ra_tests` under ASan/UBSan and TSan | PASS | This proves the independent core contract only; it is not evidence of game-loop runtime integration |
| P3 host runtime | `xmake f -y --mode=debug --pc_sanitize=y --pc_tsan=n --enable_retroachievements=y --game_version=USA`, then `tmc_ra_runtime_test` and `tmc_pc` | PASS | ASan/UBSan runtime test passes; enabled host game links the owner-thread runtime |
| P3 host runtime | `xmake f -y --mode=debug --pc_sanitize=n --pc_tsan=y --enable_retroachievements=y --game_version=USA`, then `tmc_ra_runtime_test` | PASS | TSan runtime test passes, including a foreign-thread frame no-op |
| P3 Android arm64 | `xmake f -y -p android -a arm64-v8a --ndk=/opt/homebrew/share/android-commandlinetools/ndk/26.3.11579264 --game_version=USA --enable_retroachievements=y --pc_sanitize=n --pc_tsan=n`, then `tmc_ra_runtime_test` and `tmc_pc` | PASS | Android API 21 test binary and `libmain.so` link |
| P3 Android x86_64 | `xmake f -y -p android -a x86_64 --ndk=/opt/homebrew/share/android-commandlinetools/ndk/26.3.11579264 --game_version=USA --enable_retroachievements=y --pc_sanitize=n --pc_tsan=n`, then `tmc_ra_runtime_test` and `tmc_pc` | PASS | Android API 21 test binary and `libmain.so` link |
| P3 runtime lifecycle | `tmc_ra_runtime_test` | PASS | Snapshot publication precedes game identification; foreign-thread frame/idle/reset/shutdown are no-ops; explicit reset/shutdown and default global `atexit` cleanup are exercised under host sanitizers |
| P3 monotonic time | `tmc_ra_runtime_test` | PASS | A 20 ms sleep advances the `CLOCK_MONOTONIC` runtime clock by at least 10 ms |
| P3 checkpoint | Read-only runtime/lifecycle review | PASS | Snapshot ordering, owner-thread guards, monotonic time, transportless default platform, and ledger claims reviewed after corrections |
| Casual-only admission gate | `tmc_ra_runtime_test`, `tmc_ra_adapter_test`, `tmc_ra_ui_bridge_test` | PASS | TMC init requests Casual and destroys the context when parity admission is false; the adapter rejects Spectator and the UI bridge remains unavailable after reset |
| P4 queue | `tmc_ra_android_queue_test` under ASan/UBSan and TSan | PASS | Capacity, copied HTTP body, stale-generation rejection, one password slot, and shutdown admission tests pass |
| P4 Android queue | arm64-v8a and x86_64 `tmc_ra_android_queue_test` | PASS | Both Android API 21 test binaries link |
| P4 host queue current | `xmake f -y -p macosx --game_version=USA --enable_retroachievements=y --pc_sanitize=y --pc_tsan=n && xmake build -r -y tmc_ra_android_queue_test && ./build/pc/tmc_ra_android_queue_test` | PASS | Token login, secure wipe helper, close/reopen, stale-generation rejection, and capacity pass under ASan/UBSan |
| P4 host runtime current | `xmake build -r -y tmc_ra_runtime_test && ./build/pc/tmc_ra_runtime_test` | PASS | Runtime owner-thread, monotonic-time, and lifecycle tests pass under ASan/UBSan |
| P4 Java policy | `ANDROID_HOME=/opt/homebrew/share/android-commandlinetools ./gradlew :app:test` | PASS | Exact host/protocol/port/user-info allowlist tests pass for debug, RA, and release |
| P4 API compatibility | `rg -n 'readNBytes|readAllBytes' android/app/src/main/java/dev/picori/tmc/ra/RACredentialStore.java` | PASS | No API-33-only stream methods remain; parser uses `DataInputStream.readFully` for minSdk 21 |
| P4 Android native arm64 | `xmake f -y -p android -a arm64-v8a --ndk=/opt/homebrew/share/android-commandlinetools/ndk/26.3.11579264 --game_version=USA --enable_retroachievements=y --pc_sanitize=n --pc_tsan=n && xmake build -r -y tmc_pc` | PASS | JNI bridge/queue/RA runtime link for Android API 21 |
| P4 Android native x86_64 | `xmake f -y -p android -a x86_64 --ndk=/opt/homebrew/share/android-commandlinetools/ndk/26.3.11579264 --game_version=USA --enable_retroachievements=y --pc_sanitize=n --pc_tsan=n && xmake build -r -y tmc_pc` | PASS | JNI bridge/queue/RA runtime link for Android API 21 |
| P4 RA APK | `cd android && ANDROID_HOME=/opt/homebrew/share/android-commandlinetools ./gradlew --no-daemon :app:assembleRa --rerun-tasks` | PASS | Current trigger-enabled `dev.picori.tmc.ra`, both ABIs, SHA-256 `18b66dd4c82666d799d8a7b46aa6651898c007e1058a0988b034a0d8816f61ad` |
| P4 vanilla APK | `cd android && ANDROID_HOME=/opt/homebrew/share/android-commandlinetools ./gradlew --no-daemon :app:assembleRelease --rerun-tasks` | PASS | Current `dev.picori.tmc`, both ABIs, SHA-256 `4a8d149e54552309bf1dd1475d2a361152e5f3633991475d2650e4039f9e2927` |
| P4 APK JNI split | `aapt dump badging`, `nm -D` on embedded arm64 `libmain.so`, `apksigner verify` | PASS | RA exports attach/detach/generation; vanilla exports none; both APKs verify and carry arm64/x86_64 |
| P4 manifest split | `:app:assembleRa :app:processReleaseManifest` plus merged-manifest marker check | PASS | `release` has no INTERNET/network/backup declarations; `ra` has the complete policy from `src/ra` |
| P4 Thor install current | `adb install -r` for current RA and vanilla APKs; `adb shell pm list packages` and `dumpsys package` | PASS | `dev.picori.tmc` and `dev.picori.tmc.ra` coexist after current builds, both primary ABI `arm64-v8a` |
| P6 Thor APK hash parity current | `adb -s 6b0af897 shell pm path <package>` followed by `adb -s 6b0af897 shell sha256sum <base.apk>` | PASS | Device RA base APK exactly matches current local `18b66dd4c82666d799d8a7b46aa6651898c007e1058a0988b034a0d8816f61ad`; current vanilla release artifact is `4a8d149e54552309bf1dd1475d2a361152e5f3633991475d2650e4039f9e2927` |
| P4 Thor RA start current | `adb install -r app-ra.apk`; `adb shell monkey -p dev.picori.tmc.ra 1`; filtered logcat | PASS | Current RA APK starts without linker/Java exception; `surface ready 1240x1080`, native library load, and `Prelaunch: ROM detected — waiting for user` are observed |
| P4 Thor vanilla start current | `adb shell am start -n dev.picori.tmc/dev.picori.tmc.TMCLauncherActivity`; filtered logcat | PASS | Current vanilla APK starts without RA dialog, RA JNI initialization, or runtime error |
| P5/P7 native suite | Host release config with `--pc_sanitize=y`, rebuild and run all native RA/memory/adapter/runtime/UI/toast/state/policy targets | PASS | ASan/UBSan green; includes Casual policy/state guards and durable outbox lifecycle |
| P8 journal/integration | `native_ra_outbox_test`, `native_ra_outbox_journal_test`, `native_ra_tests` | PASS | Classifier, CRC journal, journal-before-HTTP, confirmed-success removal, and mixed account/game replay scope regression pass |
| P8 Android store | `ANDROID_HOME=/opt/homebrew/share/android-commandlinetools ./gradlew --no-daemon :app:testDebugUnitTest` | PASS | Keystore AES-GCM outbox tests include missing-vs-unreadable sentinel behavior |
| M0 native lifecycle ASan/UBSan | `xmake f -y -p macosx -a arm64 --pc_tsan=n --pc_sanitize=y --enable_retroachievements=y --game_version=USA`; rebuild/run `native_ra_tests` and `tmc_ra_runtime_test` separately | PASS | Token-failure unload, bounded identify retry, logout cleanup and both pending-load mode directions pass |
| M0 native lifecycle TSan | `xmake f -y -p macosx -a arm64 --pc_tsan=y --pc_sanitize=n --enable_retroachievements=y --game_version=USA`; rebuild/run `native_ra_tests` and `tmc_ra_runtime_test` separately | PASS | `NATIVE RA P3 LIFECYCLE OK`; `tmc_ra_memory_test: ALL PASS`; `tmc_ra_runtime_test: ALL PASS` |
| Current APK signatures | `apksigner verify --verbose --print-certs <apk>` for release and RA | PASS | v1/v2 true; shared local Android Debug signer digest `43566c58c3385d0fad832bffa6a4286fb1b69d4735c819228296fce00a2befcf`; v3/v4/SourceStamp absent |
| P5-P8 native suite current | Host release config with `--pc_sanitize=y`, rebuilding and running all native RA/memory/adapter/runtime/UI/toast/state/policy targets | PASS | ASan/UBSan green after capture-hook integration; outbox classifier/journal and all RA bridge tests pass |
| P7/P8 mode and replay gates current | Host ASan/UBSan rebuild of `native_ra_tests`, `tmc_ra_adapter_test`, and all RA test targets | PASS | Public mode admission is fail-closed through the adapter hook; durable replay dispatches one FIFO record at a time and advances only after confirmed success |
| P9 badge callback race test | `xmake f -y --mode=debug --pc_sanitize=y --enable_retroachievements=y --game_version=USA && xmake build -r -y tmc_ra_badge_gate_test && ./build/pc/tmc_ra_badge_gate_test` | PASS | ASan/UBSan test pauses a callback before cache insertion, runs detach/reset, and confirms the stale badge cannot repopulate the cache |
| Android Java suite current | `cd android && ANDROID_HOME=/opt/homebrew/share/android-commandlinetools ./gradlew --no-daemon :app:testDebugUnitTest --rerun-tasks` | PASS | 21 Gradle tasks; Java credential-store, outbox, policy, and badge-path tests pass |
| Android RA packaging current | Native RA builds for `arm64-v8a` and `x86_64`, then `cd android && ANDROID_HOME=/opt/homebrew/share/android-commandlinetools ./gradlew --no-daemon :app:assembleRa --rerun-tasks` | PASS | `dev.picori.tmc.ra`, INTERNET only in RA manifest, both ABI `libmain.so`; trigger-enabled APK SHA-256 `18b66dd4c82666d799d8a7b46aa6651898c007e1058a0988b034a0d8816f61ad`; package contains arm64 `62123152` bytes and x86_64 `53157232` bytes |
| Android vanilla packaging current | Native vanilla builds for `arm64-v8a` and `x86_64`, then `cd android && ANDROID_HOME=/opt/homebrew/share/android-commandlinetools ./gradlew --no-daemon :app:assembleRelease --rerun-tasks` | PASS | `dev.picori.tmc`, no INTERNET in vanilla manifest, both ABI `libmain.so`; SHA-256 `4a8d149e54552309bf1dd1475d2a361152e5f3633991475d2650e4039f9e2927` |
| P9 native capture hook | RA host build and both Android ABI RA builds with `port/ra/tmc_ra_capture.c` | PASS | One-shot requested-address/snapshot/provenance/validity capture path compiles; no private capture was produced |
| P9 mGBA reference tool | `clang -std=c11 -O2 -Wall -Wextra ... -lmgba ... -o /tmp/tmc-mgba-reference-capture && /tmp/tmc-mgba-reference-capture --self-test` | PASS | mGBA 0.10.5 host tool self-test; no ROM/save input was supplied |
| P9 raw snapshot comparator | `python3 tools/ra/compare_tmc_ra_snapshot.py --self-test && python3 -m unittest discover -s tools/ra/tests -p 'test_*.py' -v` | PASS WITH OPEN DATA | Strict requested-byte/validity/provenance/identity-phase comparator and all 20 Python tests pass; requested-address selection is still empty without an RA set |
| P9 requested-address union | `python3 tools/ra/merge_tmc_ra_requested.py --self-test`; `python3 tools/ra/merge_tmc_ra_requested.py --manifest tools/ra/tmc_ra_state_manifest.json --capture-root artifacts/ra-local --output artifacts/ra-local/20260801-s0-s14-union.bin` | UNVERIFIED, EXPECTED | Self-test and unit tests pass; the real matrix command fails closed on missing `S0/native/metadata.json`, creates no output, and cannot admit a title-only capture or an empty requested bitmap |
| P9 native capture contract | `xmake f -y --mode=debug --pc_sanitize=y --pc_tsan=n --enable_retroachievements=y --game_version=USA && xmake build -r -y tmc_ra_capture_contract_test && ./build/pc/tmc_ra_capture_contract_test` | PASS | ASan/UBSan contract test covers no-env absolute requested/snapshot audits, post-evaluation frame view, canonical S0-S14 checkpoint names, generation, requested counters, `phase`, strict frame parsing, and frame `65536`; capture writes no second memory publication |
| P9 shared input replay | `sh tools/ra/build_mgba_reference_capture.sh && ./tools/ra/mgba_reference_capture --self-test && xmake build -y tmc_pc` | PASS | Shared `TMC_RA_INPUT_REPLAY_V1` parser validates the complete file before consumption; native uses `TMC_RA_INPUT_REPLAY=<path>`, mGBA uses `--input-replay FILE`, both use pressed GBA masks, absent replay preserves live/zero-key behavior, and the self-test covers records at 65535/65536 |
| P9 S0 post-evaluation capture | Host `tmc_pc` with the legal USA ROM/title save and an empty `TMC_RA_INPUT_REPLAY_V1`, plus the mGBA reference tool at frame 60 | VALIDATED-SCOPE PASS, REQUESTED OPEN | Native metadata carries `state=S0`, `checkpoint=boot/title`, `phase=post-evaluation`, frame `60`, generation `60`, and the public ROM identity; the synchronized reference capture is valid. Validated-scope comparison matches `141794/141794` bytes with no unknown, mismatch, or provenance ranges; both captures still have zero requested bytes without an authenticated RA set |
| P9 phase-aware memory scope | Sanitized `tmc_ra_memory_test` and `tmc_ra_adapter_test` after the S0 comparison | PASS | Snapshot offset `0x1000` (`gMain.interruptFlag`) is invalidated as phase-dependent synchronization state; if requested, the adapter remains fail-closed rather than claiming parity |
| P9 Android app-scoped operator capture | Explicit `RA_CAPTURE` component intent on installed `dev.picori.tmc.ra`, authenticated RA state, then app-data bundle inspection | DEVICE PASS | Thor capture `S2` reports `checkpoint=new game intro`, `phase=post-evaluation`, `requested_bytes=56`, `requested_ranges=53`; exactly `metadata.json`, `snapshot.bin`, `provenance.bin`, `validity.bin`, and `requested.bin` were created under the app-scoped `ra-captures/S2/native` directory; pulled only to `/tmp`, not committed |
| P9 title checkpoint parity | `python3 tools/ra/compare_tmc_ra_snapshot.py --native artifacts/ra-local/20260801-title-60-native --reference artifacts/ra-local/20260801-title-60-mgba --validated-scope` | BLOCKED BY CURRENT CONTRACT | The pre-contract title artefact is rejected with `FAIL: native: invalid state`; it remains historical limited-scope context only, not current admissible evidence |
| P9 manifest/parity readiness | `python3 tools/ra/validate_ra_manifest.py --self-test && python3 tools/ra/validate_ra_manifest.py && python3 tools/ra/generate_tmc_memory_map.py --check` | PASS WITH OPEN DATA | Manifest hash `ac01bf3b7e31c38eb0cb38a4cf8deadc587d71a99c394005a631c64fba2dae5b`; 10 symbols remain unresolved |
| P9 S0-S14 matrix gate | `python3 -m unittest discover -s tools/ra/tests -p 'test_*.py' -v`; `python3 tools/ra/compare_tmc_ra_snapshot.py --self-test`; `python3 tools/ra/merge_tmc_ra_requested.py --self-test` | PASS, EVIDENCE OPEN | 20 Python tests plus snapshot/manifest/merge self-tests pass; comparator requires native generation, positive requested ranges, zero invalid-read counters, and exact requested bytes; merge uses validated in-memory bitmaps and all missing/tampered/empty cases remain fail-closed |
| P9 contract suite re-run 2026-08-01 | `python3 tools/ra/compare_tmc_ra_snapshot.py --self-test`; `python3 tools/ra/merge_tmc_ra_requested.py --self-test`; `python3 -m unittest discover -s tools/ra/tests -p 'test_*.py' -v`; sanitized `tmc_ra_capture_contract_test`; empty-matrix CLI check | PASS, EVIDENCE OPEN | Fresh run after the frame-index fix: 20 Python tests, both self-tests, the ASan/UBSan native contract test, mGBA replay-boundary self-test, and the real `tmc_pc` build pass; an empty capture root exits 1 on missing `S0/native/metadata.json` and creates no union output |
| P9 CI parity contract | `git diff --check -- .github/workflows/_build.yaml && ruby -e 'require "yaml"; YAML.load_file(\".github/workflows/_build.yaml\")'` plus workflow static assertions | PASS | CI checks the exact ordered S0-S14 manifest, runs the ROM/private-input-free comparator/union/unit self-tests, and builds/runs `tmc_ra_capture_contract_test` on Linux x86_64; it does not claim real parity |
| Current P9 re-audit | `git fetch origin dual-screen-native && git rev-parse origin/dual-screen-native`; input hashes; ignored-root check | PASS WITH OPEN DATA | Upstream SHA unchanged; public ROM identity matches the local legal input; save fingerprint is intentionally unrecorded; `artifacts/` and `private/` are ignored; parity union remains empty |
| P9 badge image path | Host badge-cache/gate tests, Java unit suite, arm64/x86_64 native RA builds, current RA APK | SOURCE + BUILD PASS | rcheevos badge URLs, HTTPS API/media allowlist, bounded Android decode, generation-gated JNI cache, detach-race regression, and native RGBA rendering are implemented; authenticated badge presentation remains unverified |
| P10 current security scan | `git diff --check && gitleaks detect --source . --no-git --redact=100 --max-archive-depth=1 --no-banner` | PASS | No current-tree leaks; three inherited historical generic-api-key candidates are documented false positives from upstream input-state assignments |

## Device Evidence

- Device serial/model: `6b0af897`, `AYN Thor` (`kalama`), arm64-v8a.
- Install method: `adb install -r` for both APKs; both package IDs coexist.
- Current installed/local APKs: RA `18b66dd4c82666d799d8a7b46aa6651898c007e1058a0988b034a0d8816f61ad`;
  vanilla release artifact `4a8d149e54552309bf1dd1475d2a361152e5f3633991475d2650e4039f9e2927`.
- Current RA package contents: arm64-v8a `libmain.so` is `62123152` bytes and
  x86_64 `libmain.so` is `53157232` bytes; the installed Thor base APK hash
  matches the local artifact exactly.
- Top display result: RA prelaunch screen, file select, new-game flow, and first controllable overworld are physically visible on the primary display.
- Bottom display result: the 1240x1080 second-screen surface renders the live map/HUD and the RA status panel on display 4.
- RA login result: PASS, HISTORICAL SERVICE CHECK. The authenticated panel showed
  account state, `ONLINE`, the identified Minish Cap set, and `1/67` achievements
  with `0/705` points; no submission was attempted. Casual admission was not
  claimed.
- App-scoped capture trigger result: PASS. With the authenticated RA
  panel visible and the primary game in the new-game intro, the explicit
  `RA_CAPTURE` intent for `S2` created the five-file bundle under
  `Android/data/dev.picori.tmc.ra/files/ra-captures/S2/native`. Metadata reports
  `checkpoint=new game intro`, `phase=post-evaluation`, `requested_bytes=56`,
  and `requested_ranges=53`. The bundle was inspected from `/tmp` only; no
  private device capture was added to the repository.
- Historical pre-login recheck 2026-08-01: installing and bringing the
  then-current `dev.picori.tmc.ra` to the foreground produced the empty
  RA login surface; that historical frame-index-fix artifact was
  `800e0547cc6f0c1de973428067c9fe6121789865cea99013ba5ab6d08ee81a6e`.
  It is not the current installed APK; the later M1/M2 evidence below
  supersedes the pre-login state.
- Token restore result: PASS. After force-stop/relaunch and loading the existing slot, the same online RA set state reappears without a password prompt.
- Logout/re-login result: PASS. Logout removes the account/set state from the panel; a subsequent password login restores the online set state.
- Lifecycle matrix result: process stop/start and vanilla/RA package switch had no linker/Java crash; full rotate/recreate matrix remains open.
- Durable outbox result: host/mock persistence, scoped replay and confirmed-success removal pass; no Android encrypted record or authenticated replay was created on Thor.
- Submission mode: no authenticated submission proof; canonical memory parity and historical provenance remain open.

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
- The ignored local legal USA ROM has public identity MD5
  `a104896da0047abe8bee2a6e3f4c7290`, SHA-1
  `b4bd50e4131b027c334547b4524e2dbbd4227130`, and size 16,777,216 bytes.
  The ignored title save is used only locally; its fingerprint is not recorded
  in this ledger.
- The historical title-60 native and mGBA captures previously matched
  `141795/141795` validated bytes with no unknown, value, or provenance
  mismatches, but they predate the identity/phase contract. The current strict
  comparator rejects them with `native: invalid state`; they are not current
  admissible evidence.
- The current S0 post-evaluation capture matches the synchronized mGBA
  reference on all `141794` mutually validated bytes. The VBlank
  `interruptFlag` at snapshot offset `0x1000` is intentionally phase-dependent
  and invalidated; if an authenticated RA set requests that byte, the
  requested-byte gate remains fail-closed.
- The old title capture's requested bitmap was empty because no authenticated RA
  set was loaded. The generated cumulative union is therefore intentionally
  `UNVERIFIED`; no requested-address parity or Casual admission is inferred
  from that historical validated-scope checkpoint.
- The native capture hook now captures the exact post-evaluation frame view once,
  emits canonical S0-S14 state/checkpoint/phase metadata, and is covered by a
  host contract test plus a Linux CI contract leg. Native audit requests remain
  absolute-path-only and are covered when the normal capture env is absent.
- The shared `TMC_RA_INPUT_REPLAY_V1` format now gives native and mGBA the same
  frame-indexed pressed-mask input, with full-file validation before frame 0.
  It makes synchronized traversal automatable but does not create a legal
  gameplay trace or prove the named states.
- The canonical matrix manifest is `tools/ra/tmc_ra_state_manifest.json`.
  `compare_tmc_ra_capture.py` validates every listed native/reference pair and
  returns the exact in-memory requested bitmaps; `merge_tmc_ra_requested.py`
  requires that complete PASS result and atomically writes only a non-empty
  cumulative union. The TOCTOU guard prevents a post-comparison file mutation
  from changing the merged bytes.
- CI runs these structural contracts without ROMs, saves, credentials, or
  authenticated RA data. The current real matrix invocation fails closed on
  missing `S0/native/metadata.json`; therefore S0-S14 evidence, requested-byte
  parity, and Casual admission remain unverified.

## Current Change Set

- Files changed: `.gitignore`, `RA_UPSTREAM_PROGRESS.md`, `.gitmodules`,
  `libs/rcheevos`, `libs/native_ra/**`, `port/ra/**`, `port/port_save.{c,h}`,
  `src/main.c`, `tools/ra/**`, `xmake.lua`, and `android/app/**`.
- Behavior changed: with RA enabled, Android now attaches a Java bridge,
  queues HTTPS completions and login material for the game-owner thread,
  persists only successful token credentials in an AES-256-GCM Android Keystore
  record excluded from backup, shows a Casual-only password dialog, and
  stores replayable submissions in an encrypted bounded outbox. Achievement
  metadata now carries immutable badge URLs; Android fetches only allowlisted
  media with bounded PNG decoding, and the native second-screen cache renders
  bounded RGBA badges. Android queue generations reject late callbacks across
  detach/recreate. Vanilla
  `dev.picori.tmc` remains RA-disabled; the RA build type installs as
  `dev.picori.tmc.ra` and gates the bridge with `RA_ENABLED=true`.
- M0 lifecycle behavior: native identification retries only once after a
  failed load, failed token/password login and logout unload any loaded or
  pending game state, and Casual admission is locked while a game is loaded or
  loading so the wrapper and rcheevos mode cannot diverge.
- Behavior intentionally unchanged: upstream launcher, display, map, dungeon,
  widescreen, and unauthenticated gameplay remain unchanged. Authenticated RA
  game identification is now physically proven; achievement submission,
  canonical parity, and full gameplay policy behavior remain unverified.
- Risks: no legal ROM or credentials are in this repository; the two private
  optional launcher submodules are unavailable but upstream gates them as
  optional. The inherited-history scan also reports three redacted upstream
  candidates: `src/fileScreen.c:378` in `6fcfb53d`,
  `src/chooseFile.c:393` in `3399e6e`, and
  `src/introSetTransition.c:319` in `df80390` are documented false positives
  from upstream input-state assignments. Native outputs share
  `build/android/<abi>` and must be built sequentially for vanilla and RA
  packaging. RA support Java classes are present in both DEX files but
  vanilla has `RA_ENABLED=false` and does not attach the bridge; only the RA
  manifest grants `INTERNET` and backup/network policy. Authenticated game
  identification is physically proven, while submission, canonical parity, and
  replay on device remain unverified.
- Rollback: reset this branch to
  `b03cbe5a690b52fe9ea27534fc43a14970a522af`.

## Known Limitations

- `TMC_RA_UPSTREAM_PORT_PLAN.zip` was not present in the input directory. The
  equivalent top-level plan Markdown files were present and read before code
  changes.
- Release packaging currently uses the local debug signer because no release
  signing material was provided. This is valid for the baseline only.
- Full host gameplay smoke and the S0-S14 state matrix still require the legal
  ROM/save to be driven through the actual required states; the current title
  checkpoint is only a limited packer-parity proof.

## Next Smallest Objective Gate

Use the ignored canonical-USA ROM and isolated save to define a legal
`TMC_RA_INPUT_REPLAY_V1` traversal and collect synchronized native/mGBA S0-S14
checkpoints with the authenticated set loaded before each requested bitmap is
recorded. Then run the strict cumulative union comparator; only after every
requested byte is valid/matching may Casual admission proceed. Until those gates
exist, keep Casual admission fail-closed and update draft PR #16 with focused
commits only.
