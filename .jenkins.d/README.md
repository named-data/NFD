# CONTINUOUS INTEGRATION SCRIPTS

## Environment Variables Used in Build Scripts

- `NODE_LABELS`: space-separated list of platform properties. The included values are used by
  the build scripts to select the proper behavior for different operating systems and versions.

  The list should normally contain `[OS_TYPE]`, `[DISTRO_TYPE]`, and `[DISTRO_VERSION]`.

  Example values:

  - `[OS_TYPE]`: `Linux`, `OSX`
  - `[DISTRO_TYPE]`: `Ubuntu`, `CentOS`
  - `[DISTRO_VERSION]`: `Ubuntu-16.04`, `Ubuntu-18.04`, `CentOS-8`, `OSX-10.14`, `OSX-10.15`

- `JOB_NAME`: optional variable that defines the type of build job. Depending on the job type,
  the build scripts can perform different tasks.

  Possible values:

  - empty: default build task
  - `code-coverage`: debug build with tests and code coverage analysis (Ubuntu Linux is assumed)
  - `limited-build`: only a single debug build with tests

- `CACHE_DIR`: directory containing cached files from previous builds, e.g., a compiled version
  of ndn-cxx. If not set, `/tmp` is used.

- `WAF_JOBS`: number of parallel build threads used by waf, defaults to 1.
