<!--
     Copyright 2017, Data61
     Commonwealth Scientific and Industrial Research Organisation (CSIRO)
     ABN 41 687 119 230.

     This software may be distributed and modified according to the terms of
     the BSD 2-Clause license. Note that NO WARRANTY is provided.
     See "LICENSE_BSD2.txt" for details.

     @TAG(DATA61_BSD)
-->
Tutorial Development
====================

# Environment Management

This repo contains multiple tutorials, and for each tutorial there is a solution
and exercise. At any point the build system can only understand a single element
of {{seL4-tutorials, CAmkES-tutorials} x {solutions, exercises}}. To help manage
the various combinations that are possible, a script "manage.py" is provided. It
facilitates switching between seL4 and CAmkES tutorial environments, and between
exercises and solutions. Additionally, it can tell you which combination you are
currently working on.

Here's an example of typical workflow:

```bash
# starting from a fresh checkout

# query your environment
$ ./manage.py status
Environment not set up. Run `./manage.py env {sel4|camkes}`

# switch to camkes environment
$ ./manage.py env camkes

# query again
$ ./manage.py status
camkes exercise

# apps directory contains camkes exercise apps
$ ls apps
capdl-loader-experimental  hello-camkes-0  hello-camkes-1  hello-camkes-2  hello-camkes-timer

# switch to sel4 environment
$ ./manage.py env sel4

# apps directory now contains sel4 exercise apps
$ ls apps
hello-1  hello-2  hello-2-nolibs  hello-3  hello-3-nolibs  hello-4  hello-4-app  hello-timer  hello-timer-client

# switch to solution
$ ./manage.py solution

$ ./manage.py status
sel4 solution

# switch back to exercises
$ ./manage.py exercise

$ ./manage.py status
sel4 exercise
```

# Building using CMake:

There is support for building using CMake. To use this support, please see the
README-tutorials.md file.

# Running Tutorials

The "manage.py" script can be used to run tutorials in the current environment.
Use the command `./manage.py run ARCH NAME` where `ARCH` is the name of an
architecture (currently either `ia32` or `arm`) and `NAME` is the name of an app
in the current `apps` directory.

# Publishing Tutorials

Tutorial files contain jinja template comments that allow the solution-specific
parts of files to be omitted from exercises. This prevents us from having to
maintain a solution and exercise version of each file. To simplify the tutorials
that people consume, we will present a version of the tutorials with separate
files for solutions and exercises. This version of the tutorials is generated
using the "manage.py" script.

After making changes to the tutorial templates (but not necessarily commiting
them), run `./manage.py publish REMOTE [BRANCH]`, specifying the remote and
branch of a static tutorial repository (a repository where tutorials have
separate exercise and solution files). The repository will be cloned in a
temporary directory, and the current changes to the templates will be applied to
the exercises and solutions in the newly-cloned repository. You will be dropped
into a shell in the newly-cloned repository, from which you can stage, commit
and push.
