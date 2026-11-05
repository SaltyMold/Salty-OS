KERNEL_ORIGIN = KERNEL_VIRTUAL_ORIGIN + EPSILON_OFFSET;

MEMORY {
  KERNEL (rx) : ORIGIN = KERNEL_ORIGIN, LENGTH = KERNEL_IMAGE_LENGTH
  SRAM_PROCESS_STACK (rw) : ORIGIN = KERNEL_SRAM_ORIGIN - USERLAND_STACK_LENGTH, LENGTH = USERLAND_STACK_LENGTH
  SRAM_STACK (rw) : ORIGIN = KERNEL_SRAM_ORIGIN, LENGTH = KERNEL_STACK_LENGTH
  SRAM_DATA_BSS (rw) : ORIGIN = KERNEL_SRAM_ORIGIN + KERNEL_STACK_LENGTH, LENGTH = KERNEL_SRAM_DATA_BSS_LENGTH
}

SECTIONS {
  INCLUDE flash/prologue.ld

  .salty_kernel_header ORIGIN(KERNEL) + SIZEOF(.signed_payload_prefix) + SIZEOF(.kernel_header) + SIZEOF(.isr_vector_table): {
    FILL(0xFF);
    . = ORIGIN(KERNEL) + SALTY_KERNEL_HEADER_OFFSET;
    KEEP(*(.salty_kernel_header))
  } >KERNEL

  ASSERT(ADDR(.salty_kernel_header) <= ORIGIN(KERNEL) + SALTY_KERNEL_HEADER_OFFSET, "The vector table overflows the salty kernel header offset")

  .text ADDR(.salty_kernel_header) + SIZEOF(.salty_kernel_header): {
    . = ALIGN(4);
    *(.text)
    *(.text.*)
  } >KERNEL

  .init_array : {
    . = ALIGN(4);
    _init_array_start = .;
    KEEP (*(.init_array*))
    _init_array_end = .;
  } >KERNEL

  .rodata : {
    *(.rodata)
    *(.rodata.*)
  } >KERNEL

  .data : {
    /* The data section is written to Flash but linked as if it were in RAM.
     *
     * This is required because its initial value matters (so it has to be in
     * persistant memory in the first place), but it is a R/W area of memory
     * so it will have to live in RAM upon execution (in linker lingo, that
     * translates to the data section having a LMA in Flash and a VMA in RAM).
     *
     * This means we'll have to copy it from Flash to RAM on initialization.
     * To do this, we'll need to know the source location of the data section
     * (in Flash), the target location (in RAM), and the size of the section.
     * That's why we're defining three symbols that we'll use in the initial-
     * -ization routine. */
    . = ALIGN(4);
    _data_section_start_flash = LOADADDR(.data);
    _data_section_start_ram = .;
    *(.data)
    *(.data.*)
    _data_section_end_ram = .;
  } >SRAM_DATA_BSS AT> KERNEL

  /* The kernel image is padded up to KERNEL_IMAGE_LENGTH (64 kB). The second
   * 64 kB of the kernel region hold the persisting bytes and are NOT part of the
   * image: flashing the kernel never erases them. */
  .fill :
  {
    FILL(0xFF);
    . = ORIGIN(KERNEL) + LENGTH(KERNEL) - 1;
    BYTE(0xFF)
  } > KERNEL

  .bss : {
    /* The bss section contains data for all uninitialized variables
     * So like the .data section, it will go in RAM, but unlike the data section
     * we don't care at all about an initial value.
     *
     * Before execution, crt0 will erase that section of memory though, so we'll
     * need pointers to the beginning and end of this section. */
    . = ALIGN(4);
    _bss_section_start_ram = .;
    *(.bss)
    *(.bss.*)
    /* The compiler may choose to allocate uninitialized global variables as
     * COMMON blocks. This can be disabled with -fno-common if needed. */
    *(COMMON)
    _bss_section_end_ram = .;
  } >SRAM_DATA_BSS

  .canary (NOLOAD): {
    INCLUDE flash/canary.ld
  } >SRAM_DATA_BSS

  .process_stack : {
    . = (ORIGIN(SRAM_PROCESS_STACK));
    . = ALIGN(8);
    _process_stack_end = .;
    . += (LENGTH(SRAM_PROCESS_STACK) - 8);
    . = ALIGN(8);
    _process_stack_start = .;
  } >SRAM_PROCESS_STACK

  .main_stack : {
    . = (ORIGIN(SRAM_STACK));
    . = ALIGN(8);
    _main_stack_end = .;
    . += (LENGTH(SRAM_STACK) - 8);
    . = ALIGN(8);
    _main_stack_start = .;
  } >SRAM_STACK

  /DISCARD/ : {
    /* exidx and extab are needed for unwinding, which we don't use */
    *(.ARM.exidx*)
    *(.ARM.extab*)
  }
}

