# Image release packages

This workflow builds the updater once for downstream immutable images. It is
separate from the desktop installer assets and never replaces existing releases.

- `source-commit.txt` pins the application source. This can remain unchanged for
  a packaging-only revision, preserving the desktop launcher's source handoff.
- `builder-image.txt` pins the Bazzite builder by digest. `fedora-version.txt` pins the matching
  Fedora target (currently 45); x86_64 is the only current architecture. Development RPMs still come from configured repositories;
  the exact installed RPM list is recorded, not claimed to be fully reproducible.
- `revision.txt` is a positive package revision. Increment it for another package
  of the same application release. Never move a published tag or replace assets.

Push a new `v<FORK_VERSION>-image.<REVISION>` tag on a reviewed packaging commit.
The workflow checks out the pinned app and its pinned Controllable submodule,
executes source contracts and isolated native UI tests, builds production in a
fresh tree with the UI probe OFF, and checks dynamic linking and safe offscreen
startup. Only then does it publish a prerelease with the image archive, complete
corresponding application source (including Controllable), BUILD-INFO.json, and
SHA256SUMS. The metadata separately records source_commit and packaging_commit.
Manual workflow runs and pull requests build artifacts but do not publish.

The archive contains only `usr/lib/bazzite-updater-kde/`, including its executable,
private libraries and QML plugin, configuration staged beneath that private root,
licenses, source-commit, fork-version, and BUILD-INFO.json. It never writes host
`/etc` configuration or contains a runtime-verified marker. Consumers must retain
their trusted configuration, existing command/desktop IDs and recovery behavior.

The downstream image pins the release tag, exact asset name, source and packaging
SHAs, architecture, builder digest and archive SHA256. It verifies before
extraction, then performs dynamic-link and QML startup checks against its own
final runtime. A successful package build is not proof of compatibility with a
future Bazzite image. Generate runtime-verified only after those downstream checks.
No real update, rollback, rebase or reboot is part of either smoke test.

For local checks: `python3 tests/test_image_package.py -v` and
`bash -n packaging/image/build.sh`. Full builds run in the pinned container using
the same command as `.github/workflows/release-image.yml`; do not install compiler
or development dependencies on the live immutable desktop.
