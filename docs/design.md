<!--
     Copyright 2020, Data61, CSIRO (ABN 41 687 119 230).

     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# seL4-tutorials Design

The seL4-tutorials project is a set tutorials for learning how to use seL4 as
well as some tools for developing and keeping the tutorials up-to-date.

## Motivation

The seL4-tutorials aim to educate people about using seL4 and reduce its steep
learning curve.
seL4 is a verified microkernel. This means that its APIs are both low-level,
due to the microkernel philosophy, and not easy to use, due to the higher priority
of other design goals such as verification and security. seL4 seeks wide adoption
but having a very steep learning curve for people starting out is a significant
friction.

Some of the problems the tutorials try and solve:
- Introduce and explain unfamiliar system primitives: Some of seL4's primitives
  are unfamiliar or counter-intuitive for people that haven't already worked on
  similar projects in the past.
- Lack of people available to ask questions: There aren't enough people with seL4
  knowledge to be able to easily find someone who can answer specific questions.
- Support different kernel configurations/platforms and versions: Due to the low-level
  API, each platform or kernel configuration can have different APIs and behavior.
  Additionally, on-going research into new APIs and mechanisms can lead to different
  kernel implementations with different behavior.
- Establish a baseline of common assumed knowledge: When training someone to use
  seL4, it is helpful to be able to assume a baseline of understanding. Having worked
  through a standard set of training material can help establish this.
- Being able to deep-dive on a particular topic and step through a worked-example
  can make it easy to seek out knowledge more precisely instead of having to commit
  to more upfront learning.
- Keeping education material up-to-date with moving-target of kernel development.

## Goals

The tooling and tutorials in sel4-tutorials should have the following properties:
- Tutorials should be specific to seL4 or related concepts.
- A tutorial should have a stated purpose and outcomes about what is being conveyed.
- A tutorial should have a stated assumed set of starting knowledge, such as assuming
  completion of certain other tutorials, and be possible to complete without an
  unreasonable amount of additional knowledge.
- A tutorial should be able to be completed in a reasonably short amount of time.
  It should be possible to work through a couple tutorials within a couple hours.
- A tutorial should have useful outcomes that allow someone to accomplish something
  with their new knowledge.
- It should be possible to machine-check that code examples in tutorials successfully
  build and run. This includes code-examples in intermediate states during the tutorial.
- It should be possible for tutorials to support different kernel versions, configurations
  and platforms and also have tutorials that are specific to certain platforms, versions
  and configurations.
- It should be possible to work through tutorials independently to enable self-learning.
- As much as possible, tutorials should be workable with a minimum set of hardware
  dependencies. So apart from tutorials that focus on specialised hardware features, it
  should be possible to use a hardware simulator (Qemu) for a test platform.
- Tutorials need to have balance of expressive (english) and precise (code) communication.
- Tutorials can target different levels of assumed knowledge and should be used to
  demonstrate complex concepts also.

## Features

This project has the following components/features:

### Tutorials

There are a collection of tutorials that are used to learn about seL4 conecpts.
Tutorials can be worked through sequentially with access to a machine and an installation
of the required seL4 dependencies. Each new tutorial started creates a new source folder
and build configuration that can be used to modify and build the sources. This makes it
easy to take what is produced in a tutorial and create a new project repository around it
and continue developing it into a longer term project.

It is also possible to read through a tutorial
from start to end without needing to setup a machine and perform the tasks. The docsite
hosts this second version of the tutorials in a web-rendered format. 

A tutorial's source format is a markdown file that gets interpreted by the tutorial tooling
to produce a workable tutorial.

### Tooling

There is tooling that processes a tutorial source file and can produce a static file
for reading only the tutorial documentation, or it can be used to generate a tutorial
source and build directory in some completion state.  The tooling can also be used
to step-forward or step-back a tutorial to be able to start a tutorial in an intermediate
state and also machine-check that all steps in a tutorial work consistently.

#### Literate programming

The source format of the tutorials follow a literate programming paradigm where the main
format is markdown and code examples are embedded within the markdown source. A tutorial
file is processed from top to bottom and produces an internal representation of the
tutorial as a sequence of tasks that have a start and finish state. The tooling can then
render a source directory of the tutorial at a particular state and also a version of the
tutorial document with examples relevant to that state.  This approach tries to strike
a balance between representing a tutorial as a sequence of distinct steps towards
achieving a goal while still maintaining a big-picture representation.

#### Multiple tasks

A tutorial tries to teach the process of how an example is developed. To be able to
easily maintain that the process component of a tutorial stays consistently up-to-date,
the tooling understands that a tutorial has multiple steps. This enables testing to
check that a tutorial's code components are still correct throughout the tutorial with
minimal duplication and also an additional debugging feature during self-study where
a step can be skipped forwards and back to check how a task is supposed to be approached.

#### Revision control

Storing the tutorials in revision control alongside the dependencies that are used
to build and run them ensures that it will be possible to obtain a working version
of the tutorials for any previous version of the kernel and libraries.
