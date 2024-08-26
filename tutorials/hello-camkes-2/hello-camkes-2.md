<!--
  Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)

  SPDX-License-Identifier: BSD-2-Clause
-->

/*? declare_task_ordering(['hello']) ?*/

# Events in CAmkES
This tutorial shows how to build events in CAmkES.

Learn how to:
- Represent and implement events in CAmkES.
- Use Dataports.

Use this [slide presentation](https://github.com/seL4/sel4-tutorials/blob/master/docs/CAmkESTutorial.pdf) to guide you through the tutorials [0](https://docs.sel4.systems/Tutorials/hello-camkes-0), [1](https://docs.sel4.systems/Tutorials/hello-camkes-1) and [2](https://docs.sel4.systems/Tutorials/hello-camkes-2).

## Prerequisites
1. [Set up your machine](https://docs.sel4.systems/Tutorials/setting-up)
2. [Introduction to CAmkES tutorial](https://docs.sel4.systems/Tutorials/hello-camkes-1)


## CapDL Loader

This tutorial uses the *capDL loader*, a root task which allocates statically
 configured objects and capabilities.

<details markdown='1'>
<summary>Get CapDL</summary>
The capDL loader parses
a static description of the system and the relevant ELF binaries.
It is primarily used in [Camkes](https://docs.sel4.systems/CAmkES/) projects
but we also use it in the tutorials to reduce redundant code.
The program that you construct will end up with its own CSpace and VSpace, which are separate
from the root task, meaning CSlots like `seL4_CapInitThreadVSpace` have no meaning
in applications loaded by the capDL loader.
<br>
More information about CapDL projects can be found [here](https://docs.sel4.systems/CapDL.html).
<br>
For this tutorial clone the [CapDL repo](https://github.com/sel4/capdl). This can be added in a directory that is adjacent to the tutorials-manifest directory.
</details>

## Initialising

/*? macros.tutorial_init("hello-camkes-2") ?*/

<details markdown='1'>
<summary><em>Hint:</em> tutorial solutions</summary>
<br>
All tutorials come with complete solutions. To get solutions run:

/*? macros.tutorial_init_with_solution("hello-camkes-2") ?*/

</details>



### Specify an events interface
Here you're declaring the events that will be bounced
back and forth in this tutorial. An event is a signal is sent over a
Notification connection.

You are strongly advised to read the manual section on Events here:
<https://github.com/seL4/camkes-tool/blob/master/docs/index.md#an-example-of-events>.

  ''Ensure that when declaring the consumes and emits keywords between
  the Client.camkes and Echo.camkes files, you match them up so that
  you're not emitting on both sides of a single interface, or consuming
  on both sides of an interface.''

#### Specify an events interface

```
/*-- filter TaskContent("hello", TaskContentType.BEFORE, subtask="event-task1") -*/
    /* TASK 1: the event interfaces */
    /* hint 1: specify 2 interfaces: one "emits" and one "consumes"
     * hint 2: you can use an arbitrary string as the interface type (it doesn't get used)
     * hint 3: look at https://github.com/seL4/camkes-tool/blob/master/docs/index.md#an-example-of-events
     */
/*-- endfilter -*/
```

<details markdown='1'>
<summary><em>Quick solution</em></summary>

```
/*-- filter TaskContent("hello", TaskContentType.COMPLETED, subtask="event-task1") -*/
    /* TASK 1: the event interfaces */
    emits TheEvent echo;
    consumes TheEvent client;
/*-- endfilter -*/
```

</details>

```
/*-- filter TaskContent("hello", TaskContentType.BEFORE, subtask="event-task3") -*/
    /* TASK 3: the event interfaces */
    /* hint 1: specify 2 interfaces: one "emits" and one "consumes"
     * hint 2: you can use an arbitrary string as the interface type (it doesn't get used)
     * hint 3: look at https://github.com/seL4/camkes-tool/blob/master/docs/index.md#an-example-of-events
     */
/*-- endfilter -*/
```

<details markdown='1'>
<summary><em>Quick solution</em></summary>

```
/*-- filter TaskContent("hello", TaskContentType.COMPLETED, subtask="event-task3") -*/
    /* TASK 3: the event interfaces */
    consumes TheEvent echo;
    emits TheEvent client;
/*-- endfilter -*/
```
</details>

#### Add connections

```
/*-- filter TaskContent("hello", TaskContentType.BEFORE, subtask="event-task5") -*/
    /* TASK 5: Event connections */
    /* hint 1: connect each "emits" interface in a component to the "consumes" interface in the other
     * hint 2: use seL4Notification as the connector
     * hint 3: look at https://github.com/seL4/camkes-tool/blob/master/docs/index.md#an-example-of-events
     */
/*-- endfilter -*/
```
<details markdown='1'>
<summary><em>Quick solution</em></summary>

```
/*-- filter TaskContent("hello", TaskContentType.COMPLETED, subtask="event-task5") -*/
    /* TASK 5: Event connections */
    connection seL4Notification echo_event(from client.echo, to echo.echo);
    connection seL4Notification client_event(from echo.client, to client.client);
/*-- endfilter -*/
```
</details>

### Data
Recall that CAmkES prefixes the name
of the interface instance to the function being called across that
interface? This is the same phenomenon, but for events; in the case of a
connection over which events are sent, there is no API, but rather
CAmkES will generate \_emit() and \_wait() functions to enable the
application to transparently interact with these events.

#### Signal that the data is available

```
/*-- filter TaskContent("hello", TaskContentType.BEFORE, subtask="signal-task10") -*/
    /* TASK 10: emit event to signal that the data is available */
    /* hint 1: use the function <interface_name>_emit
    * hint 2: look at https://github.com/seL4/camkes-tool/blob/master/docs/index.md#an-example-of-events
    */
/*-- endfilter -*/
```

<details markdown='1'>
<summary><em>Quick solution</em></summary>

```c
/*-- filter TaskContent("hello", TaskContentType.COMPLETED, subtask="signal-task10") -*/
    /* TASK 10: emit event to signal that the data is available */
    echo_emit();
/*-- endfilter -*/
```
</details>

#### Wait for data to become available
```
/*-- filter TaskContent("hello", TaskContentType.BEFORE, subtask="wait-task11") -*/
    /* TASK 11: wait to get an event back signalling that the reply data is available */
    /* hint 1: use the function <interface_name>_wait
     * hint 2: look at https://github.com/seL4/camkes-tool/blob/master/docs/index.md#an-example-of-events
     */
/*-- endfilter -*/
```
<details markdown='1'>
<summary><em>Quick solution</em></summary>

```c
/*-- filter TaskContent("hello", TaskContentType.COMPLETED, subtask="wait-task11") -*/
    /* TASK 11: wait to get an event back signalling that the reply data is available */
    client_wait();
/*-- endfilter -*/
```
</details>

#### Signal that data is available

```c
/*-- filter TaskContent("hello", TaskContentType.BEFORE, subtask="emit-task14") -*/
    /* TASK 14: emit event to signal that the data is available */
    /* hint 1: we've already done this before */
/*-- endfilter -*/
```

<details markdown='1'>
<summary><em>Quick solution</em></summary>

```c
/*-- filter TaskContent("hello", TaskContentType.COMPLETED, subtask="emit-task14") -*/
    /* TASK 14: emit event to signal that the data is available */
    echo_emit();
/*-- endfilter -*/
```
</details>

#### Wait for data to be read

```c
/*-- filter TaskContent("hello", TaskContentType.BEFORE, subtask="wait-task15") -*/
    /* TASK 15: wait to get an event back signalling that data has been read */
    /* hint 1: we've already done this before */
/*-- endfilter -*/
```

<details markdown='1'>
<summary><em>Quick solution</em></summary>

```c
/*-- filter TaskContent("hello", TaskContentType.COMPLETED, subtask="wait-task15") -*/
    /* TASK 15: wait to get an event back signalling that data has been read */
    client_wait();
/*-- endfilter -*/
```
</details>

#### Signal that data is available
```c
/*-- filter TaskContent("hello", TaskContentType.BEFORE, subtask="notify-task22") -*/
    /* TASK 22: notify the client that there is new data available for it */
    /* hint 1: use the function <interface_name>_emit
    * hint 2: look at https://github.com/seL4/camkes-tool/blob/master/docs/index.md#an-example-of-events
    */
/*-- endfilter -*/
```

<details markdown='1'>
<summary><em>Quick solution</em></summary>

```c
/*-- filter TaskContent("hello", TaskContentType.COMPLETED, subtask="notify-task22") -*/
    /* TASK 22: notify the client that there is new data available for it */
    client_emit();
/*-- endfilter -*/
```
</details>

#### Signal that data has been read

```c
/*-- filter TaskContent("hello", TaskContentType.BEFORE, subtask="notify-task25") -*/
    /* TASK 25: notify the client that we are done reading the data */
    /* hint 1: use the function <interface_name>_emit
    * hint 2: look at https://github.com/seL4/camkes-tool/blob/master/docs/index.md#an-example-of-events
    */
/*-- endfilter -*/
```

<details markdown='1'>
<summary><em>Quick solution</em></summary>

```c
/*-- filter TaskContent("hello", TaskContentType.COMPLETED, subtask="notify-task25") -*/
    /* TASK 25: notify the client that we are done reading the data */
    client_emit();
/*-- endfilter -*/
```
</details>

### Handle notifications
One way to handle notifications in CAmkES is to
use callbacks when they are raised. CAmkES generates functions that
handle the registration of callbacks for each notification interface
instance. These steps help you to become familiar with this approach.

#### Register a callback handler

```c
/*-- filter TaskContent("hello", TaskContentType.BEFORE, subtask="register-task18") -*/
    /* TASK 18: register the first callback handler for this interface */
    /* hint 1: use the function <interface name>_reg_callback()
     * hint 2: register the function "callback_handler_1"
     * hint 3: pass NULL as the extra argument to the callback
     * hint 4: look at https://github.com/seL4/camkes-tool/blob/master/docs/index.md#an-example-of-events
     */
/*-- endfilter -*/
```

<details markdown='1'>
<summary><em>Quick solution</em></summary>

```c
/*-- filter TaskContent("hello", TaskContentType.COMPLETED, subtask="register-task18") -*/
    /* TASK 18: register the first callback handler for this interface */
    int error = echo_reg_callback(callback_handler_1, NULL);
    ZF_LOGF_IF(error != 0, "Failed to register callback");
/*-- endfilter -*/
```
</details>

#### Register another callback handler

```c
/*-- filter TaskContent("hello", TaskContentType.BEFORE, subtask="register-task21") -*/
  /* TASK 21: register the second callback for this event. */
  /* hint 1: use the function <interface name>_reg_callback()
    * hint 2: register the function "callback_handler_2"
    * hint 3: pass NULL as the extra argument to the callback
    * hint 4: look at https://github.com/seL4/camkes-tool/blob/master/docs/index.md#an-example-of-events
    */
/*-- endfilter -*/
```
<details markdown='1'>
<summary><em>Quick solution</em></summary>

```c
/*-- filter TaskContent("hello", TaskContentType.COMPLETED, subtask="register-task21") -*/
    /* TASK 21: register the second callback for this event. */
    int error = echo_reg_callback(callback_handler_2, NULL);
    ZF_LOGF_IF(error != 0, "Failed to register callback");
/*-- endfilter -*/
```
</details>

#### Register a callback handler

```c
/*-- filter TaskContent("hello", TaskContentType.BEFORE, subtask="register-task24") -*/
  /* TASK 24: register the original callback handler for this event */
    /* hint 1: use the function <interface name>_reg_callback()
     * hint 2: register the function "callback_handler_1"
     * hint 3: pass NULL as the extra argument to the callback
     * hint 4: look at https://github.com/seL4/camkes-tool/blob/master/docs/index.md#an-example-of-events
     */
/*-- endfilter -*/
```

<details markdown='1'>
<summary><em>Quick solution</em></summary>

```c
/*-- filter TaskContent("hello", TaskContentType.COMPLETED, subtask="register-task24") -*/
    /* TASK 24: register the original callback handler for this event */
    int error = echo_reg_callback(callback_handler_1, NULL);
    ZF_LOGF_IF(error != 0, "Failed to register callback");
/*-- endfilter -*/
```
</details>

------------------------------------------------------------------------

### Dataports
Dataports are typed shared memory mappings. In your
CAmkES ADL specification, you state what C data type you'll be using to
access the data in the shared memory -- so you can specify a C struct
type, etc.

The really neat part is more that CAmkES provides access control for
accessing these shared memory mappings: if a shared mem mapping is such
that one party writes and the other reads and never writes, we can tell
CAmkES about this access constraint in ADL.

So in TASKs 2 and 4, you're first being led to create the "Dataport"
interface instances on each of the components that will be participating
in the shared mem communication. We will then link them together using a
"seL4SharedData" connector later on.

#### Specify dataport interfaces
```
/*-- filter TaskContent("hello", TaskContentType.BEFORE, subtask="dataport-task2") -*/
    /* TASK 2: the dataport interfaces */
    /* hint 1: specify 3 interfaces: one of type "Buf", one of type "str_buf_t" and one of type "ptr_buf_t"
     * hint 2: for the definition of these types see "str_buf.h".
     * hint 3: look at https://github.com/seL4/camkes-tool/blob/master/docs/index.md#an-example-of-dataports
     */
 /*-- endfilter -*/
```

<details markdown='1'>
<summary><em>Quick solution</em></summary>

```
/*-- filter TaskContent("hello", TaskContentType.COMPLETED, subtask="dataport-task2") -*/
    /* TASK 2: the dataport interfaces */
    dataport Buf d;
    dataport str_buf_t d_typed;
    dataport ptr_buf_t d_ptrs;
/*-- endfilter -*/
```

</details>

#### Specify dataport interfaces
```
/*-- filter TaskContent("hello", TaskContentType.BEFORE, subtask="dataport-task4") -*/
    /* TASK 4: the dataport interfaces */
    /* hint 1: specify 3 interfaces: one of type "Buf", one of type "str_buf_t" and one of type "ptr_buf_t"
     * hint 3: look at https://github.com/seL4/camkes-tool/blob/master/docs/index.md#an-example-of-dataports
     */
/*-- endfilter -*/
```

<details markdown='1'>
<summary><em>Quick solution</em></summary>

```
/*-- filter TaskContent("hello", TaskContentType.COMPLETED, subtask="dataport-task4") -*/
    /* TASK 4: the dataport interfaces */
    dataport Buf d;
    dataport str_buf_t d_typed;
    dataport ptr_buf_t d_ptrs;
/*-- endfilter -*/
```
</details>

### Dataport connections
And here we are: we're about to specify connections
between the shared memory pages in each client, and tell CAmkES to link
these using shared underlying Frame objects. Fill out this step, and
proceed.

#### Specify dataport connections

```
/*-- filter TaskContent("hello", TaskContentType.BEFORE, subtask="dataport-task6") -*/
    /* TASK 6: Dataport connections */
    /* hint 1: connect the corresponding dataport interfaces of the components to each other
    * hint 2: use seL4SharedData as the connector
    * hint 3: look at https://github.com/seL4/camkes-tool/blob/master/docs/index.md#an-example-of-dataports
    */
/*-- endfilter -*/
```

<details markdown='1'>
<summary><em>Quick solution</em></summary>

```
/*-- filter TaskContent("hello", TaskContentType.COMPLETED, subtask="dataport-task6") -*/
    /* TASK 6: Dataport connections */
    connection seL4SharedData data_conn(from client.d, to echo.d);
    connection seL4SharedData typed_data_conn(from client.d_typed, to echo.d_typed);
    connection seL4SharedData ptr_data_conn(from client.d_ptrs, to echo.d_ptrs);
/*-- endfilter -*/
```
</details>

### Access and manipulate share memory mapping (Dataport) of the client
These steps are asking you to write some C code
to access and manipulate the data in the shared memory mapping
(Dataport) of the client. Follow through to the next step.

#### Copy strings to an untyped dataport
```c
/*-- filter TaskContent("hello", TaskContentType.BEFORE, subtask="copy-task9") -*/
    /* TASK 9: copy strings to an untyped dataport */
    /* hint 1: use the "Buf" dataport as defined in the Client.camkes file
     * hint 2: to access the dataport use the interface name as defined in Client.camkes.
     * For example if you defined it as "dataport Buf d" then you would use "d" to refer to the dataport in C.
     * hint 3: first write the number of strings (NUM_STRINGS) to the dataport
     * hint 4: then copy all the strings from "s_arr" to the dataport.
     * hint 5: look at https://github.com/seL4/camkes-tool/blob/master/docs/index.md#an-example-of-dataports
     */
/*-- endfilter -*/
```

<details markdown='1'>
<summary><em>Quick solution</em></summary>

```c
/*-- filter TaskContent("hello", TaskContentType.COMPLETED, subtask="copy-task9") -*/
    /* TASK 9: copy strings to an untyped dataport */
    int *n = (int*)d;
    *n = NUM_STRINGS;
    char *str = (char*)(n + 1);
    for (int i = 0; i < NUM_STRINGS; i++) {
        strcpy(str, s_arr[i]);
        str += strlen(str) + 1;
    }
/*-- endfilter -*/
```
</details>

#### Read the reply data from a typed dataport
```
/*-- filter TaskContent("hello", TaskContentType.BEFORE, subtask="read-task12") -*/
    /* TASK 12: read the reply data from a typed dataport */
    /* hint 1: use the "str_buf_t" dataport as defined in the Client.camkes file
     * hint 2: to access the dataport use the interface name as defined in Client.camkes.
     * For example if you defined it as "dataport str_buf_t d_typed" then you would use "d_typed" to refer to the dataport in C.
     * hint 3: for the definition of "str_buf_t" see "str_buf.h".
     * hint 4: use the "n" field to determine the number of strings in the dataport
     * hint 5: print out the specified number of strings from the "str" field
     * hint 6: look at https://github.com/seL4/camkes-tool/blob/master/docs/index.md#an-example-of-dataports
     */
/*-- endfilter -*/
```

<details markdown='1'>
<summary><em>Quick solution</em></summary>

```
/*-- filter TaskContent("hello", TaskContentType.COMPLETED, subtask="read-task12") -*/
    /* TASK 12: read the reply data from a typed dataport */
    for (int i = 0; i < d_typed->n; i++) {
        printf("%s: string %d (%p): \"%s\"\n", get_instance_name(), i, d_typed->str[i], d_typed->str[i]);
    }
/*-- endfilter -*/
```
</details>

#### Send data using dataports

```
/*-- filter TaskContent("hello", TaskContentType.BEFORE, subtask="send-task13") -*/
  /* TASK 13: send the data over again, this time using two dataports, one
     * untyped dataport containing the data, and one typed dataport containing
     * dataport pointers pointing to data in the untyped, dataport.
     */
    /* hint 1: for the untyped dataport use the "Buf" dataport as defined in the Client.camkes file
     * hint 2: for the typed dataport use the "ptr_buf_t" dataport as defined in the Client.camkes file
     * hint 3: for the definition of "ptr_buf_t" see "str_buf.h".
     * hint 4: copy all the strings from "s_arr" into the untyped dataport
     * hint 5: use the "n" field of the typed dataport to specify the number of dataport pointers (NUM_STRINGS)
     * hint 6: use the "ptr" field of the typed dataport to store the dataport pointers
     * hint 7: use the function "dataport_wrap_ptr()" to create a dataport pointer from a regular pointer
     * hint 8: the dataport pointers should point into the untyped dataport
     * hint 9: for more information about dataport pointers see: https://github.com/seL4/camkes-tool/blob/master/docs/index.md
     */
/*-- endfilter -*/
```

<details markdown='1'>
<summary><em>Quick solution</em></summary>

```c
/*-- filter TaskContent("hello", TaskContentType.COMPLETED, subtask="send-task13") -*/
    /* TASK 13: send the data over again, this time using two dataports, one
     * untyped dataport containing the data, and one typed dataport containing
     * dataport pointers pointing to data in the untyped, dataport.
     */
    d_ptrs->n = NUM_STRINGS;
    str = (char*)d;
    for (int i = 0; i < NUM_STRINGS; i++) {
        strcpy(str, s_arr[i]);
        d_ptrs->ptr[i] = dataport_wrap_ptr(str);
        str += strlen(str) + 1;
    }
/*-- endfilter -*/
```
</details>

### Access and manipulate share memory mapping (Dataport) of the server
And these steps are asking you to write some C
code to access and manipulate the data in the shared memory mapping
(Dataport) of the server.

#### Read data from an untyped dataport
```c
/*-- filter TaskContent("hello", TaskContentType.BEFORE, subtask="read-task19") -*/
  /* TASK 19: read some data from the untyped dataport */
  /* hint 1: use the "Buf" dataport as defined in the Echo.camkes file
    * hint 2: to access the dataport use the interface name as defined in Echo.camkes.
    * For example if you defined it as "dataport Buf d" then you would use "d" to refer to the dataport in C.
    * hint 3: first read the number of strings from the dataport
    * hint 4: then print each string from the dataport
    * hint 5: look at https://github.com/seL4/camkes-tool/blob/master/docs/index.md#an-example-of-dataports
    */
/*-- endfilter -*/
```

<details markdown='1'>
<summary><em>Quick solution</em></summary>

```c
/*-- filter TaskContent("hello", TaskContentType.COMPLETED, subtask="read-task19") -*/
    /* TASK 19: read some data from the untyped dataport */
    int *n = (int*)d;
    char *str = (char*)(n + 1);
    for (int i = 0; i < *n; i++) {
      printf("%s: saying (%p): \"%s\"\n", get_instance_name(), str, str);
      str += strlen(str) + 1;
    }
/*-- endfilter -*/
```
</details>

#### Put data into a typed dataport

```c
/*-- filter TaskContent("hello", TaskContentType.BEFORE, subtask="put-task20") -*/
  /* TASK 20: put a modified copy of the data from the untyped dataport into the typed dataport */
    /* hint 1: modify each string by making it upper case, use the function "uppercase"
     * hint 2: read from the "Buf" dataport as above
     * hint 3: write to the "str_buf_t" dataport as defined in the Echo.camkes file
     * hint 4: to access the dataport use the interface name as defined in Echo.camkes.
     * For example if you defined it as "dataport str_buf_t d_typed" then you would use "d_typed" to refer to the dataport in C.
     * hint 5: for the definition of "str_buf_t" see "str_buf.h"
     * hint 6: use the "n" field to specify the number of strings in the dataport
     * hint 7: copy the specified number of strings from the "Buf" dataport to the "str" field
     * hint 8: look at https://github.com/seL4/camkes-tool/blob/master/docs/index.md#an-example-of-dataports
     * hint 9: you could combine this TASK with the previous one in a single loop if you want
     */
/*-- endfilter -*/
```

<details markdown='1'>
<summary><em>Quick solution</em></summary>

```c
/*-- filter TaskContent("hello", TaskContentType.COMPLETED, subtask="put-task20") -*/
  /* TASK 20: put a modified copy of the data from the untyped dataport into the typed dataport */
    n = (int*)d;
    str = (char*)(n + 1);
    for (int i = 0, j = *n - 1; i < *n; i++, j--) {
        strncpy(d_typed->str[j], str, STR_LEN);
        uppercase(d_typed->str[j]);
        str += strlen(str) + 1;
    }
    d_typed->n = *n;
/*-- endfilter -*/
```
</details>

#### Read data from a typed dataport

```c
/*-- filter TaskContent("hello", TaskContentType.BEFORE, subtask="read-task23") -*/
  /* TASK 23: read some data from the dataports. specifically:
    * read a dataport pointer from one of the typed dataports, then use
    * that pointer to access data in the untyped dataport.
    */
  /* hint 1: for the untyped dataport use the "Buf" dataport as defined in the Echo.camkes file
    * hint 2: for the typed dataport use the "ptr_buf_t" dataport as defined in the Echo.camkes file
    * hint 3: for the definition of "ptr_buf_t" see "str_buf.h".
    * hint 4: the "n" field of the typed dataport specifies the number of dataport pointers
    * hint 5: the "ptr" field of the typed dataport contains the dataport pointers
    * hint 6: use the function "dataport_unwrap_ptr()" to create a regular pointer from a dataport pointer
    * hint 7: for more information about dataport pointers see: https://github.com/seL4/camkes-tool/blob/master/docs/index.md
    * hint 8: print out the string pointed to by each dataport pointer
    */
/*-- endfilter -*/
```
<details markdown='1'>
<summary><em>Quick solution</em></summary>

```c
/*-- filter TaskContent("hello", TaskContentType.COMPLETED, subtask="read-task23") -*/
  /* TASK 23: read some data from the dataports. specifically:
    * read a dataport pointer from one of the typed dataports, then use
    * that pointer to access data in the untyped dataport.
    */
  char *str;
  for (int i = 0; i < d_ptrs->n; i++) {
      str = dataport_unwrap_ptr(d_ptrs->ptr[i]);
      printf("%s: dptr saying (%p): \"%s\"\n", get_instance_name(), str, str);
  }
/*-- endfilter -*/
```
</details>

### CAmkES attributes
This is an introduction to CAmkES attributes: you're
being asked to set the priority of the components.

#### Set component priorities

```
/*-- filter TaskContent("hello", TaskContentType.BEFORE, subtask="set-task7") -*/
    /* TASK 7: set component priorities */
    /* hint 1: component priority is specified as an attribute with the name <component name>.priority
    * hint 2: the highest priority is represented by 255, the lowest by 0
    */
/*-- endfilter -*/
```

<details markdown='1'>
<summary><em>Quick solution</em></summary>

```
/*-- filter TaskContent("hello", TaskContentType.COMPLETED, subtask="set-task7") -*/
    /* TASK 7: set component priorities */
    client.priority = 255;
    echo.priority = 254;
/*-- endfilter -*/
```
</details>

### Specify the data access constraints for Dataports
This is where we specify the data access constraints
for the Dataports in a shared memory connection. We then go about
attempting to violate those constraints to see how CAmkES has truly met
our constraints when mapping those Dataports.

#### Restrict access to dataports

```
/*-- filter TaskContent("hello", TaskContentType.BEFORE, subtask="restrict-task8") -*/
    /* TASK 8: restrict access to dataports */
    /* hint 1: use attribute <component>.<interface_name>_access for each component and interface
    * hint 2: appropriate values for the to_access and from_access attributes are: "R" or "W"
    * hint 4: make the "Buf" dataport read only for the Echo component
    * hint 3: make the "str_buf_t" dataport read only for the Client component
    */
/*-- endfilter -*/
```

<details markdown='1'>
<summary><em>Quick solution</em></summary>

```
/*-- filter TaskContent("hello", TaskContentType.COMPLETED, subtask="restrict-task8") -*/
    /* TASK 8: restrict access to dataports */
    echo.d_access = "R";
    client.d_access = "W";
    echo.d_typed_access = "W";
    client.d_typed_access = "R";
/*-- endfilter -*/
```
</details>

#### Test the read and write permissions on the dataport
```
/*-- filter TaskContent("hello", TaskContentType.BEFORE, subtask="test-task16") -*/
    /* TASK 16: test the read and write permissions on the dataport.
    * When we try to write to a read-only dataport, we will get a VM fault.
    */
    /* hint 1: try to assign a value to a field of the "str_buf_t" dataport */
/*-- endfilter -*/
```

<details markdown='1'>
<summary><em>Quick solution</em></summary>

```
/*-- filter TaskContent("hello", TaskContentType.COMPLETED, subtask="test-task16") -*/
    /* TASK 16: test the read and write permissions on the dataport.
    * When we try to write to a read-only dataport, we will get a VM fault.
    */
    d_typed->n = 0;
/*-- endfilter -*/
```
</details>

## Done!
 Congratulations: be sure to read up on the keywords and
structure of ADL: it's key to understanding CAmkES. And well done on
writing your first CAmkES application.




/*-- filter ExcludeDocs() -*/
```c
/*- filter File("components/Client/Client.camkes") --*/

component Client {
    /* include definitions of typedefs for the dataports */
    include "str_buf.h";

    /* this component has a control thread */
    control;

/*? include_task_type_append([("hello","event-task1")]) ?*/

/*? include_task_type_append([("hello","dataport-task2")]) ?*/

}
/*-- endfilter -*/
```

```c
/*- filter File("components/Echo/Echo.camkes") --*/

component Echo {
    /* include definitions of typedefs for the dataports */
    include "str_buf.h";

/*? include_task_type_append([("hello","event-task3")]) ?*/

/*? include_task_type_append([("hello","dataport-task4")]) ?*/

}


/*-- endfilter -*/
```
```c
/*- filter File("hello-2.camkes") --*/
/*
 * CAmkES tutorial part 2: events and dataports
 */

import <std_connector.camkes>;

/* import component defintions */
import "components/Client/Client.camkes";
import "components/Echo/Echo.camkes";

assembly {
    composition {
        /* component instances */
        component Client client;
        component Echo echo;

/*? include_task_type_append([("hello","event-task5")]) ?*/

/*? include_task_type_append([("hello","dataport-task6")]) ?*/

    }
    configuration {
/*? include_task_type_append([("hello","set-task7")]) ?*/

/*? include_task_type_append([("hello","restrict-task8")]) ?*/

    }
}


/*-- endfilter -*/
```

```c
/*- filter File("include/str_buf.h") --*/
/*
 * CAmkES tutorial part 2: events and dataports
 */

#ifndef __STR_BUF_H__
#define __STR_BUF_H__

#include <camkes/dataport.h>

#define NUM_STRINGS 5
#define STR_LEN 256

/* for a typed dataport containing strings */
typedef struct {
    int n;
    char str[NUM_STRINGS][STR_LEN];
} str_buf_t;

#define MAX_PTRS 20

/* for a typed dataport containing dataport pointers */
typedef struct {
    int n;
    dataport_ptr_t ptr[MAX_PTRS];
} ptr_buf_t;

#endif

/*-- endfilter -*/
```
```c
/*- filter File("components/Client/src/client.c") --*/
/*
 * CAmkES tutorial part 2: events and dataports
 */

#include <stdio.h>
#include <string.h>

#include <camkes/dataport.h>

/* generated header for our component */
#include <camkes.h>

/* strings to send across to the other component */
char *s_arr[NUM_STRINGS] = { "hello", "world", "how", "are", "you?" };

/* run the control thread */
int run(void) {
    printf("%s: Starting the client\n", get_instance_name());

/*? include_task_type_append([("hello","copy-task9")]) ?*/

/*? include_task_type_append([("hello","signal-task10")]) ?*/

/*? include_task_type_append([("hello","wait-task11")]) ?*/

/*? include_task_type_append([("hello","read-task12")]) ?*/

/*? include_task_type_append([("hello","send-task13")]) ?*/

/*? include_task_type_append([("hello","emit-task14")]) ?*/

/*? include_task_type_append([("hello","wait-task15")]) ?*/

    printf("%s: the next instruction will cause a vm fault due to permissions\n", get_instance_name());

/*? include_task_type_append([("hello","test-task16")]) ?*/

    return 0;
}

/*-- endfilter -*/
```

```c
/*- filter File("components/Echo/src/echo.c") --*/
/*
 * CAmkES tutorial part 2: events and dataports
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <camkes/dataport.h>
#include <sel4utils/sel4_zf_logif.h>

/* generated header for our component */
#include <camkes.h>

/* hepler function to modify a string to make it all uppercase */
void uppercase(char *str) {
    while (*str != '\0') {
        *str = toupper(*str);
        str++;
    }
}

void callback_handler_2(void *a);

/* this callback handler is meant to be invoked when the first event
 * arrives on the "consumes" event interface.
 * Note: the callback handler must be explicitly registered before the
 * callback will be invoked.
 * Also the registration is one-shot only, if it wants to be invoked
 * when a new event arrives then it must re-register itself.  Or it can
 * also register a different handler.
 */
void callback_handler_1(void *a) {

/*? include_task_type_append([("hello","read-task19")]) ?*/

/*? include_task_type_append([("hello","put-task20")]) ?*/

/*? include_task_type_append([("hello","register-task21")]) ?*/

/*? include_task_type_append([("hello","notify-task22")]) ?*/

}

/* this callback handler is meant to be invoked the second time an event
 * arrives on the "consumes" event interface.
 * Note: the callback handler must be explicitly registered before the
 * callback will be invoked.
 * Also the registration is one-shot only, if it wants to be invoked
 * when a new event arrives then it must re-register itself.  Or it can
 * also register a different handler.
 */
void callback_handler_2(void *a) {

/*? include_task_type_append([("hello","read-task23")]) ?*/

/*? include_task_type_append([("hello","register-task24")]) ?*/

/*? include_task_type_append([("hello","notify-task25")]) ?*/

}

/* this function is invoked to initialise the echo event interface before it
 * becomes active. */
/* TASK 17: replace "echo" with the actual name of the "consumes" event interface */
/* hint 1: use the interface name as defined in Echo.camkes.
 * For example if you defined it as "consumes TheEvent c_event" then you would use "c_event".
 */
void echo__init(void) {
/*? include_task_type_append([("hello","register-task18")]) ?*/
}


/*-- endfilter -*/
```


```
/*- filter TaskCompletion("hello", TaskContentType.BEFORE) --*/
client: the next instruction will cause a vm fault due to permissions
/*-- endfilter -*/
/*- filter TaskCompletion("hello", TaskContentType.ALL) --*/
FAULT HANDLER: data fault from client.client_0_control
/*-- endfilter -*/
```


```cmake
/*- filter File("CMakeLists.txt") --*/
include(${SEL4_TUTORIALS_DIR}/settings.cmake)
sel4_tutorials_regenerate_tutorial(${CMAKE_CURRENT_SOURCE_DIR})

cmake_minimum_required(VERSION 3.7.2)

project(hello-camkes-2 C ASM)

find_package(camkes-tool REQUIRED)
camkes_tool_setup_camkes_build_environment()

DeclareCAmkESComponent(Client INCLUDES include SOURCES components/Client/src/client.c)
DeclareCAmkESComponent(Echo INCLUDES include SOURCES components/Echo/src/echo.c)

DeclareCAmkESRootserver(hello-2.camkes)

GenerateCAmkESRootserver()

# utility CMake functions for the tutorials (not required in normal, non-tutorial applications)
/*? macros.cmake_check_script(state) ?*/
/*-- endfilter -*/
```
/*-- endfilter -*/