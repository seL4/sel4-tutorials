<!--
  Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)

  SPDX-License-Identifier: BSD-2-Clause
-->

/*? declare_task_ordering(['hello']) ?*/

# CAmkES Tutorial: Introduction

This tutorial is an introduction to CAmkES. This will involve introducing the CAmkES syntax, bootstrapping a basic
static CAmkES application and describing its components.

## Prerequisites
1. [Set up your machine](https://docs.sel4.systems/HostDependencies#camkes-build-dependencies).
2. [Hello world](https://docs.sel4.systems/Tutorials/hello-world)
2. Familiarize yourself with the [CAmkES manual](https://github.com/seL4/camkes-tool/blob/master/docs/index.md).
Note that it's possible to successfully complete the CAmkES tutorial without having read the manual, however highly
recommended.

## Outcomes

- Understand the structure of a CAmkES application, as a described, well-defined, static system.
- Understand the file-layout of a CAmkES ADL project.
- Become acquainted with the basics of creating a practical CAmkES application.

## Background

The fundamentals of CAmkES are the component, the interface and the connection. Components are logical
groupings of code and resources. They communicate with other component instances via well-defined
interfaces which must be statically defined, over communication channels.

### Component

As briefly described above, we identify a component as a functional grouping of code and resources. We
use the term component in CAmkES to refer to the *type* of our functional grouping (see the Component section in the [manual](https://github.com/seL4/camkes-tool/blob/master/docs/index.md#component)).
An example of this in concrete CAmkES syntax can be seen below:

```c
component foo {
  control;
  uses MyInterface a;
  attribute Int b;
}
```

Disregarding the items defined within the component (we will unpack these in a later tutorial), we defined a component above
whose *type* is `foo`. We later can use our `foo` type to define a component *instance*.
For example, the statement `component foo bar` refers to a component instance `bar` whose type is
`foo`.

### Describing a static system: Assembly, Composition and Configuration

In CAmkES, we will commonly see the use of three hierarchical elements, being `assembly`, `composition` and `configuration`. We use these concepts
to build upon a well-defined static system. We firstly use the term 'assembly' to refer to a complete description of our static system. In the CAmkES ADL (Architecture Description Language),
we employ the term `assembly` as a top-level element that will encapsulate our system definition. Each CAmkES project must contain
at least one `assembly` definition. An example of using the `assembly` term in CAmkES can be seen below:

```c
assembly {
  composition {
    component foo bar;
  }

  configuration {
      bar.b = 0;
  }
}
```

In the above example we can also see the use of the `composition` and `configuration` elements. The `composition` element is
used as a container to encapsulate our component and connector instantiations. Above we declare an instance of
our `foo` component and we appropriately called it `bar`. The `configuration` element is also used to describe settings and attribute
assignments in our given system.

## Creating your first CAmkES application

In this tutorial we will create a simple 'Hello World' example within the CAmkES. This will invole creating a CAmkES component that will
print "Hello CAmkES World" when it starts up.

### Looking at the sources

In the tutorial directory, you will find the following files:
* `CMakeLists.txt` - the file that defines how to build our CAmkES application
* `client.c` - the single source file for our 'Hello World' client component
* `hello.camkes` - Our CAmkES file describing our static system

#### `client.c`

For this tutorial we require our component to simply print "Hello CAmkES World". We define this in
a typical C file `client.c`:

```c
/*- filter File("client.c") --*/
/*
 * CAmkES tutorial part 0: just a component.
 */

#include <stdio.h>

/* generated header for our component */
#include <camkes.h>

/* run the control thread */
int run(void) {
    printf("Hello CAmkES World\n");
    return 0;
}
/*-- endfilter -*/
```

Note above that in the source code of `client.c` instead of typically using `main`, we place
our runtime code in the function `int run(void)`. `run` is the entry point
of a CAmkES component.

#### `hello.camkes`

The `hello.camkes` file is where we form our description of a static CAmkES system. Our `.camkes`
files are written using the CAmkES syntax. Employing the concepts discussed in the background
section, we define the following:

```c
/*- filter File("hello.camkes") --*/
/*
 * CAmkES tutorial part 0: just a component.
 */

component Client {
    control;
}

assembly {
    composition {
        component Client client;
    }
}

/*-- endfilter -*/
```

In the source above we create a minimal static system with a single component instance. We define
our component `Client` and declare an instance of that component in our system.

#### `CMakeLists.txt`

Every CAmkES project requires a `CMakeLists.txt` file to be incorporated in the seL4 build system. Our tutorial
directory should contain the following `CMakeLists.txt` file:

```cmake
/*- set build_file --*/
include(${SEL4_TUTORIALS_DIR}/settings.cmake)
sel4_tutorials_regenerate_tutorial(${CMAKE_CURRENT_SOURCE_DIR})

cmake_minimum_required(VERSION 3.7.2)

project(hello-camkes-0 C ASM)

find_package(camkes-tool REQUIRED)
camkes_tool_setup_camkes_build_environment()

DeclareCAmkESComponent(Client SOURCES client.c)

DeclareCAmkESRootserver(hello.camkes)

GenerateCAmkESRootserver()
/*-- endset -*/
/*? build_file ?*/
```

Our `CMakeLists.txt` file declares our `Client` component, linking it with our `client.c` source
file. In addition it declares the CAmkES Root Server using our `hello.camkes` system description.

### Building your first CAmkES system

At this point all you need to do is build and run the tutorial:

/*? macros.ninja_block() ?*/

If build successfully, we can run our system as follows:

/*? macros.simulate_block() ?*/

and should see the following once the system has booted:

```
/*- filter TaskCompletion("hello", TaskContentType.ALL) --*/
Hello CAmkES World
/*-- endfilter -*/
```

## Done
 Congratulations: be sure to read up on the keywords and
structure of ADL: it's key to understanding CAmkES. And well done on
building and running your first CAmkES application.

/*? macros.help_block() ?*/

/*- filter ExcludeDocs() -*/

```cmake
/*- filter File("CMakeLists.txt") -*/
/*? build_file ?*/

# utility CMake functions for the tutorials (not required in normal, non-tutorial applications)
/*? macros.cmake_check_script(state) ?*/
/*- endfilter -*/
```

/*- endfilter -*/
