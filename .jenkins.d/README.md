CONTINUOUS INTEGRATION SCRIPTS
==============================

Environment Variables Used in Build Scripts
-------------------------------------------

- `NODE_LABELS`: the variable defines a list of OS properties.  The set values are used by the
  build scripts to select proper behavior for different OS.

  The list should include at least `[OS_TYPE]`, `[DISTRO_TYPE]`, and `[DISTRO_VERSION]`.

  Possible values for Linux:

  * `[OS_TYPE]`: `Linux`
  * `[DISTRO_TYPE]`: `Ubuntu`
  * `[DISTRO_VERSION]`: `Ubuntu-14.04`, `Ubuntu-16.04`

  Possible values for OS X / macOS:

  * `[OS_TYPE]`: `OSX`
  * `[DISTRO_TYPE]`: `OSX` (can be absent)
  * `[DISTRO_VERSION]`: `OSX-10.10`, `OSX-10.11`, `OSX-10.12`

- `JOB_NAME`: optional variable to define type of the job.  Depending on the defined job type,
  the build scripts can perform different tasks.

  Possible values:

  * empty: default build process
  * `code-coverage` (Linux OS is assumed): debug build with tests and code coverage analysis
  * `limited-build`: only a single debug build with tests

- `CACHE_DIR`: the variable defines a path to folder containing cached files from previous builds,
  e.g., a compiled version of ndn-cxx library.  If not set, `/tmp` is used.

- `WAF_JOBS`: number of parallel build jobs used by waf, defaults to 1.
