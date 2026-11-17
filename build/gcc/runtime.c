/* The vendor reset handler calls __libc_init_array. With -nostartfiles,
 * provide the init/fini hooks normally supplied by the C runtime startup.
 * Standard-library system call stubs come from the selected nosys.specs. */
void _init(void) { }
void _fini(void) { }
