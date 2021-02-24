# prmon Release Procedure

## Git and GitHub

1. On the `main` branch make the last feature commits and ensure that all
   tests pass.
2. Update the release number in `CMakeLists.txt` and commit.
3. Update the `stable` branch to this commit in `main`.
4. Make a tag, following a semantic versioning scheme `vA.B.C`.
  - Use an *annotated* tag, add brief release notes in the annotation.
5. Push changes to GitHub.
6. Prepare a binary tarball of the static build to add to the release:
  - `cmake -DBUILD_STATIC=ON <PATH_TO_SOURCES>`
  - `make package`
  - Use the `tar.gz` archive.
6. In GitHub go to releases, *Draft a new release*.
7. Select the new tag.
8. Add release notes as needed.
9. Add the binary tarball to the release.

## Zenodo

1. Zenodo should pull the new release automatically and assign it a DOI.
  - The DOI does take a short time to activate.

## DOI

- We reference the *Concept* DOI in the `README.md` file of the project.
- If a release DOI is first reserved then it might be possible to add
  it to the stable branch README a the tag point (we need to check).
