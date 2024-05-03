# prmon Release Procedure

## Git and GitHub

1. On the `main` branch make the last feature commits and ensure that all
   tests pass.
2. Update the release number in `CMakeLists.txt` and commit.
  - To commit directly, use the command line
3. Update the `stable` branch to this commit in `main`.
  - e.g., `git branch -f stable main`
4. Make a tag, following a semantic versioning scheme `vA.B.C`.
  - e.g., `git tag -a v3.1.0`
  - This is an *annotated* tag - add brief release notes in the annotation.
5. Push changes to GitHub.
  - `git push origin tag v3.1.0`
  - Again, from command line to bypass normal branch protection
6. Prepare a binary tarball of the static build to add to the release:
  - On AlamaLinux install the packages `glibc-static` and `libstdc++-static`
  - `cmake -DBUILD_STATIC=ON <PATH_TO_SOURCES>`
  - By hand, strip the `prmon` binary to reduce its size
  - `make package`
  - Use the `tar.gz` archive.
6. In GitHub go to releases, *Draft a new release*.
7. Select the new tag.
8. Add release notes as needed.
  - Github does a good job of generating changelogs now, so just add major
  items at the top of the notes
9. Add the binary tarball to the release.

## Zenodo

1. Zenodo should pull the new release automatically and assign it a DOI.
  - The DOI does take a short time to activate.

## DOI

- We reference the *Concept* DOI in the `README.md` file of the project.
- If a release DOI is first reserved then it might be possible to add
  it to the stable branch README a the tag point (we need to check).
