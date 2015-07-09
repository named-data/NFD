CONTINUOUS INTEGRATION SCRIPTS
==============================

Environment Variables Used in Build Scripts
-------------------------------------------

- `NODE_LABELS`: the variable defines a list of OS properties.  The set values are used by the
  build scripts to select proper behavior for different OS.

  The list should include at least `[OS_TYPE]`, `[DISTRO_TYPE]`, and `[DISTRO_VERSION]`.

  Possible values for Linux OS:

  * `[OS_TYPE]`: `Linux`
  * `[DISTRO_TYPE]`: `Ubuntu`
  * `[DISTRO_VERSION]`: `Ubuntu-12.04`, `Ubuntu-14.04`, `Ubuntu-15.04`

  Possible values of OSX OS:

  * `[OS_TYPE]`: `OSX`
  * `[DISTRO_TYPE]`: `OSX` (can be absent)
  * `[DISTRO_VERSION]`: `OSX-10.10`, `OSX-10.9`, `OSX-10.8`, `OSX-10.7`

- `JOB_NAME`: optional variable to define type of the job.  Depending on the defined job type,
  the build scripts can perform different tasks.

  Possible values:

  * empty: default build process
  * `code-coverage` (Linux OS is assumed): build process with code coverage analysis
