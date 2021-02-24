# Contributing to prmon

Thanks for taking the time to contribute to the `prmon` project. We are very
happy to get contributions for

- Bug reports, for things that don't work properly
- Feature requests, for things that prmon could do in the future
- Documentation improvements, for things that could be better explained

That does not cover all possible contributions, so if you believe something can
be useful to the project, please raise the issue.

## Code of Conduct

All of our interactions in the project are governed by the [code of
conduct](../CODE_OF_CONDUCT.md). Thanks for respecting it!

## Issues and Bug Reports

Raise bug reports and general issues using the [*issues*](https://github.com/HSF/prmon/issues)
feature. For bug reports make sure that you:

- are using the latest release of prmon
- explain how are are running the code
- state what the problem is clearly
- say what you would expect the behaviour to be

It is really ideal if you can give instructions using a Docker container so that
the project maintainers can verify the problem and validate a fix.

Do bear in mind the scope of prmon is Linux systems (the availability of `/proc`
is critical), so if the issue is that something does not work on another
platform, that is out of scope for us.

## Feature Requests

If you would like to contribute a new feature to prmon, please *fork* the
project and develop your feature. Then make a *pull request* back to the main
repository.

- the feature should be supported by an integration test
- the code should be commented as needed so that others can understand it
- C++ code should be formatted using the *Google* style of `clang-format`
  - i.e. `clang-format --style=Google ...`
  - There is a `clang-format` CMake target that will do this automatically so we
    recommend that is run before making the pull request
- Python code should be formatted with `black` (following the Scikit-HEP
  recommendation) and should also be clean with the `flake8` linter
  - See the `.flake8` file for the configuration to be compatible with
    `black`
  - There are `black` and `flake8` build targets that will run the
    formatter and linter respectively

We will review the code before accepting the pull request.

Please bear in mind the scope of prmon, which is to do *lightweight* monitoring
of vast numbers of jobs. Thus features which are heavy or not of general
interest across millions of jobs are out of scope for the project. In particular
prmon focuses on resource consumption, it is not a profiling and optimisation
tool.

If you are not sure if the feature makes sense for the project, use a GitHub
issue to first discuss with the maintainers.

If you want to add a new *monitoring component* please take a look at [this
guide](ADDING_MONITORS.md).

## Documentation Improvements

It is almost a truism that documentation can always be improved! If you are not
sure how to improve an area of documentation you can just raise an issue and we
will discuss how to do it. If you have concrete contribution then please submit
it as a feature request. Documentation includes code comments as well as more
specific files (like this one) and all improvements are welcome.

## Authorship and Copyright

As members of the High-Energy Physics community, our host laboratory,
[CERN](https://home.cern/), holds copyright on this project.

All contributions need to agree to pass copyright to CERN.

Every significant author of prmon will be acknowledged in our
[AUTHORS](../AUTHORS.md) file.
