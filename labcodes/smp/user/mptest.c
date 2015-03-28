#include <ulib.h>
#include <stdio.h>

const int max_child = 8;

void
print_info(int n) {
    volatile int i;
    while(1) {
        for (i = 0; i < 10000; i++);
        cprintf("I am child %d on cpu %d\n", n, getcpu());
    }
}

int
main(void) {
    int n, pid;
    for (n = 0; n < max_child; n ++) {
        if ((pid = fork()) == 0) {
            print_info(n);
            exit(0);
        }
        assert(pid > 0);
    }

    if (n > max_child) {
        panic("fork claimed to work %d times!\n", n);
    }

    for (; n > 0; n --) {
        if (wait() != 0) {
            panic("wait stopped early\n");
        }
    }

    if (wait() == 0) {
        panic("wait got too many\n");
    }

    cprintf("forktest pass.\n");
    return 0;
}

