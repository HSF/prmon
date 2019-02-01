# prmon Release Procedure

## Git and GitHub

1. On the master branch make the last feature commits and ensure that all
   tests pass.
2. Update the release number in `CMakeLists.txt`. Commit.
3. Update the `stable` branch to this commit in `master`.
4. Make a tag, following a semantic versioning scheme `vA.B.C`.
  1. Use an *annotated* tag, add brief release notes in the annotation.
5. Push changes to GitHub.
6. Prepare a binary tarball of the static build to add to the release:
  1. `cmake -DSTATIC_BUILD=ON <PATH_TO_SOURCES>`
  2. `make package`
  3. Use the `tar.gz` archive.
6. In GitHub go to releases, *Draft a new release*.
7. Select the new tag.
8. Add release notes as needed.
9. Add the binary tarball to the release.

## Zenodo

1. Zenodo should pull the new release and assign it a DOI.
  1. The DOI does take a short time to activate.
2. Grab the DOI link and update the `README.md` file on the
`master` branch so that the correct DOI is shown on GitHub.
