# Continuous Integration Scripts

## Environment Variables

- `ID`: lower-case string that identifies the operating system, for example: `ID=ubuntu`,
  `ID=centos`. See [os-release(5)] for more information. On macOS, where `os-release` is
  not available, we emulate it by setting `ID=macos`.

- `ID_LIKE`: space-separated list of operating system identifiers that are closely related
  to the running OS. See [os-release(5)] for more information. The listed values are used
  by the CI scripts to select the proper behavior for different platforms and OS flavors.

  Examples:

  - On CentOS, `ID_LIKE="centos rhel fedora linux"`
  - On Ubuntu, `ID_LIKE="ubuntu debian linux"`

- `VERSION_ID`: identifies the operating system version, excluding any release code names.
  See [os-release(5)] for more information. Examples: `VERSION_ID=42`, `VERSION_ID=22.04`.

- `JOB_NAME`: defines the type of the current CI job. Depending on the job type, the CI
  scripts can perform different tasks.

  Supported values:

  - empty: default build task
  - `code-coverage`: debug build with tests and code coverage analysis
  - `limited-build`: only a single debug build with tests

- `CACHE_DIR`: directory containing cached files from previous builds, e.g., a compiled
  version of ndn-cxx. If not set, `/tmp` is used.

- `DISABLE_ASAN`: disable building with AddressSanitizer. This is automatically set for
  the `code-coverage` job type.

[os-release(5)]: https://www.freedesktop.org/software/systemd/man/os-release.html
