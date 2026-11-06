# DEVELOPMENT CONTAINER

prmon is Linux only, as it reads from `/proc` and `/sys`. To make it easy
to build prmon and run all of its tests on any machine (including macOS or
Windows hosts), a container build file is provided in
[containers/Dockerfile](../containers/Dockerfile).

The image is based on `almalinux:latest` and contains everything needed to
build prmon and run the full test suite, including the gtest unit tests:
`gcc-c++`, `clang`, `cmake`, `make`, `boost`, `curl-devel`,
`clang-tools-extra`, `util-linux`, `python3` and `git`.

The container does not run as `root`. The default user is `hsf` (password
`hsf`), who has `sudo` rights, so extra packages can be installed when
needed, e.g., `sudo dnf install -y gdb`.

The examples below use `podman`, but `docker` takes the same arguments.

## Building the image

From the top level of a prmon clone:

```sh
podman build -t prmon-dev -f containers/Dockerfile containers
```

This gives you a local image called `prmon-dev`. Rebuild it from time to time
to pick up updates to the AlmaLinux base image.

## Building and testing a fresh clone

To build and test prmon from the upstream repository, start the container
and clone inside it:

```sh
podman run -it --rm prmon-dev
```

Then, inside the container:

```sh
git clone --recurse-submodules https://github.com/HSF/prmon.git
cmake -S prmon -B build -DBUILD_GTESTS=ON
cmake --build build
cd build && ctest --output-on-failure
```

Because of `--rm`, everything is lost when the container exits.

The googletest sources are downloaded when cmake first configures the
project, so the container needs network access at that point.

## Local development

For day-to-day development, keep editing the sources on the host, using your
usual editor and git tools, and use the container only to build and test.
To do this, mount your git clone into the container at `/mnt`. From the top
level of your clone:

```sh
podman run -it --rm -v $(pwd):/mnt prmon-dev
```

Your clone needs its submodules (`nlohmann_json` and `spdlog`), so if you
did not clone with `--recurse-submodules`, first run this on the host:

```sh
git submodule update --init
```

Inside the container the sources are in `/mnt`, and changes made on the host
show up there straight away. Build in a directory outside the source tree,
e.g., in the `hsf` home directory:

```sh
cmake -S /mnt -B build -DBUILD_GTESTS=ON
cmake --build build
cd build && ctest --output-on-failure
```

Leave the container running while you work. After editing on the host,
rebuild and rerun the tests:

```sh
cmake --build ~/build && (cd ~/build && ctest --output-on-failure)
```

Or run a single test with, e.g., `ctest -R testCPUsingle --output-on-failure`.

Since `~/build` is inside the container, it disappears when the container
exits. To keep the build between sessions, put the build directory in the
mounted clone instead (e.g., `cmake -S /mnt -B /mnt/build-container ...`).
Add it to your local `.git/info/exclude` so git ignores it.

### Notes

- **File ownership on Linux hosts**: the `hsf` user in the container has
  uid 1000. With `docker`, files in `/mnt` are only writable by `hsf` if
  your host uid is also 1000. With rootless `podman`, your host user maps to
  `root` in the container, so add `--userns=keep-id` to map it to `hsf`
  instead:

  ```sh
  podman run -it --rm --userns=keep-id -v $(pwd):/mnt prmon-dev
  ```

  If you build outside `/mnt`, as above, the container only needs to read
  the sources and this does not matter. On macOS, podman machine and Docker
  Desktop handle the mapping for you.
- **SELinux**: on hosts with SELinux enforcing (e.g., Fedora, RHEL,
  AlmaLinux), add `:Z` to the volume option, `-v $(pwd):/mnt:Z`, so the
  container is allowed to read the mount.
- **Git worktrees**: in a `git worktree` checkout, `.git` points to a path in
  the main repository that is not mounted, so git commands fail inside the
  container. Building and testing still work; run git on the host instead.
- **Memory**: compiling `prmon.cpp` takes a lot of memory. If a parallel build
  (`cmake --build build -j N`) dies with `Killed signal terminated program
  cc1plus`, give the podman machine / Docker VM more memory or build with
  fewer jobs.
- **Formatting**: `clang-format` is available in the image, so
  `cmake --build build --target clang-format` reformats the C++ sources in
  `/mnt` (this needs `/mnt` to be writable, see above).
