NFD version 0.7.1
-----------------

Release date: October 8, 2020

The build requirements have been increased to require Clang >= 4.0, Xcode >= 9.0, and Python 3.
Meanwhile, it is *recommended* to use GCC >= 7.4.0 and Boost >= 1.65.1.
This effectively drops official support for Ubuntu 16.04 when using distribution-provided Boost
packages -- NFD may still work on this platform, but we provide no official support for it.
Additionally, this release drops support for CentOS 7.

New features
^^^^^^^^^^^^

- Allow Ethernet face MTU to adjust to changes in underlying interface MTU (:issue:`3257`)

- Allow face MTU to be overriden via management (:issue:`5056`)

- Deduplicate received link-layer packets when link-layer reliability is enabled (:issue:`5079`)

- Add support for the ``HopLimit`` Interest element (:issue:`4806`)

- Add counter for unsolicited data

Improvements and bug fixes
^^^^^^^^^^^^^^^^^^^^^^^^^^

- Fix PIT entry rejection in ``MulticastStrategy`` upon receiving the same Interest (:issue:`5123`)

- If no face exists when a route is added via nfdc, nfdc will attempt to create it (:issue:`4332`)

- Attach PIT tokens after CS hit (:issue:`5090`)

- ``nfdc cs erase`` will now erase up to the specified count (:issue:`4744`)

- Improve ``LpReliability`` logging (:issue:`5080`)

- Use standard path for Unix stream socket on Linux (:issue:`5039`)

- Adjust timeout sensitivity and avoid ignoring additional probe responses in ASF strategy
  (:issue:`4983`, :issue:`4193`)

- Bug fixes in config file processing (:issue:`4489`)

- Fix incompatibility with version header in C++20

- Fix integer overflow in PIT ``FaceRecord`` (:issue:`4979`)

- Various improvements to documentation, test suites, and authors list

Removals
^^^^^^^^

- Delete deprecated uses of default SigningInfo (:issue:`4804`)

- Remove Endpoint IDs from egress APIs (:issue:`4843`, :issue:`4849`, :issue:`4973`)
