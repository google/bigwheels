# Contributing to BigWheels

## For users: Reporting bugs and requesting features

To report a new bug or request a new feature, please file a GitHub issue. Please
ensure the bug has not already been reported by searching
[issues](https://github.com/google/BigWheels/issues) and
[projects](https://github.com/google/BigWheels/projects). If the bug has
not already been reported open a new one
[here](https://github.com/google/BigWheels/issues/new).

When opening a new issue for a bug, make sure you provide the following:

*   A clear and descriptive title.
    *   We want a title that will make it easy for people to remember what the
        issue is about. Simply using "Segfault in fishtornado" is not helpful
        because there could be (but hopefully aren't) multiple bugs with
        segmentation faults with different causes.
*   A description of the hardware and software combination that the bug occurs:
    *  GPU brand and model.
    *  Driver version.
    *  Target API and API version.
*   A test case that exposes the bug, with the steps and commands to reproduce
    it.
    *   The easier it is for a developer to reproduce the problem, the quicker a
        fix can be found and verified. It will also make it easier for someone
        to possibly realize the bug is related to another issue.

For feature requests, we use
[issues](https://github.com/google/BigWheels/issues) as well. Please
create a new issue, as with bugs. In the issue provide

*   A description of the problem that needs to be solved.
*   Examples that demonstrate the problem.

## For developers: Contributing a patch

Before we can use your code and/or assets, you must sign the
[Google Software Grant and Corporate Contributor License Agreement](https://cla.developers.google.com/about/google-corporate)
(CLA), which you can do online. The CLA is necessary mainly because you own the
copyright to your changes, even after your contribution becomes part of our
codebase, so we need your permission to use and distribute your code. We also
need to be sure of various other things -- for instance that you'll tell us if
you know that your code infringes on other people's patents. You don't have to
sign the CLA until after you've submitted your code for review and a member has
approved it, but you must do it before we can put your code into our codebase.

See [README.md](https://github.com/google/BigWheels/blob/main/README.md)
for instruction on how to get, build, and test the source. Once you have made
your changes:

*   Ensure the code follows the
    [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
    Running `clang-format -style=file -i [modified-files]` can help.
*   Create a pull request (PR) with your patch.
*   1 PR = 1 change. If you want to add 3 features, that's 3 PRs.
*   If your PR brings a non-trivial performance improvement, do provide proof.
*   Make sure the PR description clearly identifies the problem, explains the
    solution, and references the issue if applicable.
*   If your patch completely fixes bug 1234, the commit message should say
    `Fixes https://github.com/google/BigWheels/issues/1234`. When you do
    this, the issue will be closed automatically when the commit goes into
    master.
*   Watch the continuous builds to make sure they pass.
*   Request a code review.

The reviewer can either approve your PR or request changes. If changes are
requested:

*   Please add new commits to your branch, instead of amending your commit.
    Adding new commits makes it easier for the reviewer to see what has changed
    since the last review.
*   Once you are ready for another round of reviews, you can either click
    the "re-request review" button, or add a comment at the bottom, such as
    "Ready for review" or "Please take a look" (or "PTAL"). This
    explicit handoff is useful when responding with multiple small commits.

After the PR has been reviewed it is the job of the reviewer to merge the PR.
Instructions for this are given below.

### Handling PR dependencies

If your PR depends on another, you can mark it as a draft. This way we won't
accidentally merge it.
By default, we won't review draft PRs. But if the PR is ready, and just
marked as draft to manage dependencies, feel free to ping us!

### Life of a PR

We try to review PRs at least once per 24h window.
If your PR remains untouched for more than 24h, please ping us.
Anything will do "PTAL", "up".
Sometimes, we are on holiday, but you cannot guess, so in doubt, ping us 😊.

When a maintainer sees an unassigned PR, they have 2 options:

- If it's a small & trivial change, they will self-assign, and handle
the approval/merging.
- Otherwise, they will choose 2 or more reviewers (can self-assign).

When a change has multiple reviewers, we try to follow the following rules:
- All reviewers must approve.
- If you are the first to approve, you delegate merging to the other(s).
- If you are the last to approve, it's your responsibility to merge.
- If you request changes without approval, others must wait for your approval
before merging (and if you are the last to approve, you have to merge).

Additional cases:
- If a maintainer wants to review, they can add themselves to the reviewer list.
- If a maintainer comments without adding a review, they don't block merging.

## For developers: Contributing assets

We welcome any graphic assets you may have developed for benchmarks and/or
projects. Just like with the code, we request that you only contribute assets
that you have either authored or are able to license for BigWheels to use. The
[Google Software Grant and Corporate Contributor License Agreement](https://cla.developers.google.com/about/google-corporate)
(CLA) applies to assets as well as code.

## For maintainers: Reviewing a PR

The formal code reviews are done on GitHub. Reviewers are to look for all of the
usual things:

*   Coding style follows the
    [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
*   Identify potential functional problems.
*   Identify code duplication.
*   Ensure the unit tests have enough coverage.
*   Ensure continuous integration (CI) bots run on the PR.

## For maintainers: Merging a PR

We intend to maintain a linear history on the GitHub master branch, and the
build and its tests should pass at each commit in that history. A linear
always-working history is easier to understand and to bisect in case we want to
find which commit introduced a bug.

Once you consider the PR to be ready to merge, use the 'Squash and merge' button
on github's UI.
