# Contributing

Open an issue or pull request in this repository on GitHub. Describe the problem,
proposed behavior and validation; keep unrelated changes separate. Preserve public
APIs and native host/thread ownership, and add focused tests for behavior changes.

Use a C++23 module-capable compiler and CMake 3.28 or newer. Configure a separate
build directory with BUILD_TESTING=ON, build Debug and Release, and run CTest.
Follow README for the relevant library/example dependency graph. Report platform
coverage and distinguish compilation, CPU tests and interactive runtime evidence.
Never include credentials, generated caches or unrelated local files.

Unless explicitly stated otherwise, contributions intentionally submitted for
inclusion are offered under the project's MIT choice (inbound equals
outbound). Submit only work you have authority to license; preserve third-party
notices and identify borrowed material. Maintainers review changes and tests
before merging. No separate CLA or sign-off requirement is imposed here.
