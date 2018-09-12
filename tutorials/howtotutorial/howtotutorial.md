# How to write tutorials: A tutorial

This tutorial aims to teach you how to write tutorials targeting seL4. It has the following goals:
- Introduce the abstractions that the tutorial tooling provide
- Give examples for how to put a tutorial together
- Demonstrate that the tutorial tools are powerful enough to write a self-referential tutorial

## Who cares about seL4 tutorials

The tutorials should benefit/enable the following roles:
- Participants (Consumers): They use the tutorials
- Maintainers (Producers): They create and maintain the tutorials
- Teachers: Teach using the tutorial material
- Continuous integration: Tests that the tutorial material is still valid.

## Principles

### The shorter a tutorial is, the less it can break

Everything bit-rots. Tutorials seem to bit-rot the most as the people responsible for making breaking changes
are the least likely to be doing the tutorials and therefore least likely to notice.  When someone does
notice that a tutorial has broken and reports it, the people fixing it are less likely to consider how
their change affects the rest of the tutorial. The larger a tutorial is, the more likely it is to get broken
and the less likely someone will read through the entire thing when making a fix. Conversely, the shorter a
tutorial is, the less likely it will break, and the more obvious fixing it will be.  Additionally, short
tutorials are likely more helpful to participants as they can pick and choose what they want to learn on a 
more granular level.

### Don't Repeat yourself

Writing tutorials can be pretty boring.  But not as boring as maintaining tutorials.  Also, having to redo the
same tasks from one tutorial to the next can get annoying for the participant.  Any repetition for either 
the participant or developer/maintainer should be abstracted away or removed.  This means putting common code
into libraries, using tools such as capDL to set a system up to the exact state needed to start a tutorial, and
also structuring tutorial material to not be repeats of the same task/idea.

### Try not to introduce special ways of doing things

The tutorials should be a representation of what developing on seL4 is like. This means that creating tooling
to hide pain points only kicks the can further down the road.  It is understandable to try and not overwhelm
someone using seL4 for the first time, but adding new functionality for doing something that will only be done
in the tutorials will force people to learn tools that won't help anywhere else, and will also force tutorial
maintainers to maintain these extra set of tools.  If something is so bad to use that it can't be put into a tutorial
in its current form then maybe it should be improved.

### People learn in different ways

It should be possible for people to learn from the tutorials in many different ways. While some people want to
check out some code and modify it, others will only want to be able to read through it on a webpage without installing
a development environment. Someone may just be referring back to refresh themselves on how to do something.  It
is important that the tutorials are useful in all of these ways:
- As pages on a website
- As guided walk-throughs on how to use and apply different seL4 concepts
- As working code examples with reproducible output.

### The REPL cycle aspiration

Read-Eval-Print-Loop is a very popular tool for learning and testing a new language. This is because:
- Quick feedback cycle,
- Works on expressions rather than a compilation unit, and
- doesn't require complicated compilation/installation steps before you can test something.

It isn't realistic at this stage to implement a REPL on seL4 or in Camkes, but it is worth noting that it
is important to have a very quick edit-compile-execute loop and a reasonably fast first compilation time.
When someone wants to test a new concept---checking out a particular tutorial, generating, building and running
it should be super quick.

### Modern development tools are all about _empowerment_

_It wasn't always so clear, but the Rust programming language is fundamentally about empowerment: 
no matter what kind of code you are writing now, Rust empowers you to reach farther, to program with 
confidence in a wider variety of domains than you did before._
> See: https://doc.rust-lang.org/book/second-edition/foreword.html#foreword

This means not forcing particular solutions to each problem, but allowing the tutorial participant
the freedom to choose and implement their own solution. Enabling them to utilise the skills that they are
trying to learn to their own advantage or detriment, while providing tools and help when they get themselves stuck.


## Concepts

### Literate Programming

An paradigm in which the explanation of the program is closely tied to the program itself.  Explanation
interspersed with snippets of source code.  Documentation and source code can then both be generated from
this common source.  This is supposed to make it easier to communicate the purpose and ideas of a program.
As this is a format that encourages much more description than comments inside a program it will hopefully
lead to more useful/maintainable tutorials where the example snippets used in the tutorial materials
actually end up generating the tutorial programs.


### Breaking a problem (program) up into a series of steps

A tutorial is about taking a goal, breaking it up into a series of small steps, and guiding the participant
through these steps, enabling them to achieve the goal and learn new technologies/techniques in the process.
These steps should be understood by the tutorial tooling, tested that each step still works. One of the problems
with our tutorials currently is that something will break somewhere around step 7 of 14 and no one wants to work
through the first 6 steps to diagnose the problem.


## Tools



### Python scripts
- init.py
- test.py
- template.py


### CMake scripts
- GenerateTutorial
- ExecuteGenerationProcess
- UpdatGeneratedFiles
- 


### The capDL-loader as the perfect sandbox

### capdl-utils
- capdl-pp
- capdl-ld
- capdl-objdump
- capdl-ar

### Future script functionality
- git diff source control of tutorial checkouts
- better front end tool


## Writing a tutorial

### Initialising a tutorial

```sh
./init

```



### Keeping different languages separate within the same file

Combining code snippets and text was a good idea they called literate programming.  But further preprocessing
literate programming files was somehow considered a bad idea?


## Tutorial Life-cycle

1. A new tutorial is born.

2. The tutorial is developed

3. The tutorial is distributed to someone else

4. The person learns the tutorial


5. The tutorial is put under regression
6. Breakages are detected and fixed
7. Someone wants to update the tutorial with new content/change content.
8. The tutorial is taken as a base program and built upon by someone.

/*- filter ExcludeDocs() -*/


This is the main source file of the tutorial
/*? ExternalFile("HelloWorld.md") ?*/

This is a pretty gross CMakeLists.txt which builds a tutorial from within a tutorial
/*? ExternalFile("CMakeLists.txt") ?*/

/*- endfilter -*/
