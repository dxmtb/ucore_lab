#include <defs.h>
#include <stdio.h>
#include <string.h>
#include <console.h>
#include <kdebug.h>
#include <picirq.h>
#include <trap.h>
#include <clock.h>
#include <intr.h>
#include <pmm.h>
#include <vmm.h>
#include <ide.h>
#include <swap.h>
#include <proc.h>
#include <fs.h>
#include <kmonitor.h>
#include <mp.h>

int kern_init(void) __attribute__((noreturn));
void grade_backtrace(void);
static void lab1_switch_test(void);
extern int      ismp;
extern pde_t *boot_pgdir;
extern volatile struct ioapic *ioapic;


int
kern_init(void) {
    extern char edata[], end[];
    memset(edata, 0, end - edata);


    const char *message = "(THU.CST) os is loading ...";
    cprintf("%s\n\n", message);

    print_kerninfo();

    grade_backtrace();

    pmm_init();                 // init physical memory management
    mp_init();
    if (lapic) {
        pte_t *ptep = get_pte(boot_pgdir, KERNTOP, 1);
        assert(ptep != NULL);
        *ptep = (int32_t)lapic | PTE_P | PTE_W;
        lapic = (void *)KERNTOP;
    }
    lapic_init();
    pic_init();                 // init interrupt controller
    if (ismp) {
#define IOAPIC  0xFEC00000   // Default physical address of IO APIC
        pte_t *ptep = get_pte(boot_pgdir, KERNTOP + PGSIZE, 1);
        assert(ptep != NULL);
        *ptep = IOAPIC | PTE_P | PTE_W;
        ioapic = (void *)(KERNTOP + PGSIZE);
    }
    ioapic_init();              // another interrupt controller
    cons_init();                // init the console
    idt_init();                 // init interrupt descriptor table

    vmm_init();                 // init virtual memory management
    sched_init();               // init scheduler
    proc_init();                // init process table

    ide_init();                 // init ide devices
    swap_init();                // init swap
    fs_init();                  // init fs

    if(!ismp)
        clock_init();           // init clock interrupt
    startothers();   // start other processors

    //LAB1: CAHLLENGE 1 If you try to do it, uncomment lab1_switch_test()
    // user/kernel mode switch test
    //lab1_switch_test();
    mpmain();
}

// Other CPUs jump here from entryother.S.
void
mpenter(void)
{
    pmm_init_ap();
    lapic_init();
    mpmain();
}

// Common CPU setup code.
void
mpmain(void)
{
    assert(cpu->id == cpunum());
    proc_init2();
    xchg(&cpu->started, 1); // tell startothers() we're up
    idt_init();
    if (cpu->id == 0)
        intr_enable();              // enable irq interrupt
    cprintf("CPU%d: started\n", cpu->id);
    cpu_idle();                 // run idle process
}

// Start the non-boot (AP) processors.
void
startothers(void)
{
    cprintf("Try to startothers\n");
    extern uint8_t _binary_obj_entryother_o_start[], _binary_obj_entryother_o_size[];
    uint8_t *code;
    struct cpu *c;
    char *stack;

    // Write entry code to unused memory at 0x7000.
    // The linker has placed the image of entryother.S in
    // _binary_entryother_start.
    code = KADDR(0x7000);
    memmove(code, _binary_obj_entryother_o_start, (uint32_t)_binary_obj_entryother_o_size);

    for(c = cpus; c < cpus+ncpu; c++){
        if(c == cpus+cpunum())  // We've started already.
            continue;

        // Tell entryother.S what stack to use, where to enter, and what
        // pgdir to use. We cannot use kpgdir yet, because the AP processor
        // is running in low  memory, so we use entrypgdir for the APs too.
        stack = kmalloc(KSTACKSIZE);
        *(void**)(code-4) = stack + KSTACKSIZE;
        *(void**)(code-8) = mpenter;
        *(int**)(code-12) = (void *) PADDR(boot_pgdir);

        lapicstartap(c->id, PADDR(code));

        // wait for cpu to finish mpmain()
        while(c->started == 0) ;
    }
    cprintf("All CPU started!\n");
}

void __attribute__((noinline))
grade_backtrace2(int arg0, int arg1, int arg2, int arg3) {
    mon_backtrace(0, NULL, NULL);
}

void __attribute__((noinline))
grade_backtrace1(int arg0, int arg1) {
    grade_backtrace2(arg0, (int)&arg0, arg1, (int)&arg1);
}

void __attribute__((noinline))
grade_backtrace0(int arg0, int arg1, int arg2) {
    grade_backtrace1(arg0, arg2);
}

void
grade_backtrace(void) {
    grade_backtrace0(0, (int)kern_init, 0xffff0000);
}

static void
lab1_print_cur_status(void) {
    static int round = 0;
    uint16_t reg1, reg2, reg3, reg4;
    asm volatile (
            "mov %%cs, %0;"
            "mov %%ds, %1;"
            "mov %%es, %2;"
            "mov %%ss, %3;"
            : "=m"(reg1), "=m"(reg2), "=m"(reg3), "=m"(reg4));
    cprintf("%d: @ring %d\n", round, reg1 & 3);
    cprintf("%d:  cs = %x\n", round, reg1);
    cprintf("%d:  ds = %x\n", round, reg2);
    cprintf("%d:  es = %x\n", round, reg3);
    cprintf("%d:  ss = %x\n", round, reg4);
    round ++;
}

static void
lab1_switch_to_user(void) {
    //LAB1 CHALLENGE 1 : TODO
}

static void
lab1_switch_to_kernel(void) {
    //LAB1 CHALLENGE 1 :  TODO
}

static void
lab1_switch_test(void) {
    lab1_print_cur_status();
    cprintf("+++ switch to  user  mode +++\n");
    lab1_switch_to_user();
    lab1_print_cur_status();
    cprintf("+++ switch to kernel mode +++\n");
    lab1_switch_to_kernel();
    lab1_print_cur_status();
}

