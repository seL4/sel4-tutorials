# Hello World

Too meta af


```c
/*- raw -*/
/*- filter File("src/main.c") -*/
/*- endraw -*/
#include <stdio.h>

int main() {
	printf("Hello world\n");
	return 0;
}
/*- raw -*/
/*- endfilter -*/
/*- endraw -*/
```

```cmake
/*- raw -*/
/*- filter File("CMakeLists.txt") -*/
/*- endraw -*/
add_executable(hello-world src/main.c)

target_link_libraries(hello-world
    sel4
    muslc utils sel4tutorials
    sel4muslcsys sel4platsupport sel4utils sel4debug)

DeclareRootserver(hello-world)

/*- raw -*/
/*- endfilter -*/
/*- endraw -*/
```
