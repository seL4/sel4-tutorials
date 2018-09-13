# Introduction to seL4
/*? declare_task_ordering(['task-1']) ?*/

Dfsdfsdf jdf l

## Client



```c
/*- filter ELF("client_adder") -*/
/*- filter TaskContent("task-1", TaskContentType.ALL, subtask="client") -*/

#include <stdio.h>

#include <sel4/sel4.h>

/*? RecordObject(seL4_EndpointObject, "reverse_ep", cap_symbol="endpoint", read=True, write=True, grant=True) ?*/

int main(int c, char *argv[]) {
    printf("I am the client1\n");
    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 1);

    seL4_SetMR(0, 24);

    seL4_Call(endpoint, info);
    return 0;
}
/*- endfilter -*/
/*- endfilter -*/
```


## Server

```c
/*- filter ELF("server_adder") -*/
/*- filter TaskContent("task-1", TaskContentType.ALL, subtask="server") -*/

#include <sel4/sel4.h>
#include <stdio.h>

/*? RecordObject(seL4_EndpointObject, "reverse_ep", cap_symbol="endpoint", read=True, write=True, grant=True) ?*/

int main(int c, char *argv[]) {
	seL4_Word sender;
    printf("I am the server\n");
    seL4_Recv(endpoint, &sender);
    int val = seL4_GetMR(0);
    printf("Received a message %d\n", val);

    return 0;
}
/*- endfilter -*/
/*- endfilter -*/
```

```
/*- filter TaskCompletion("task-1", TaskContentType.ALL) -*/
Received a message 24
/*- endfilter -*/
```


/*- filter ExcludeDocs() -*/

/*? ExternalFile("CMakeLists.txt") ?*/
/*- endfilter -*/
