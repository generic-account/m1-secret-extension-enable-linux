static uint64_t general_read_actlr_el1(void) {
  uint64_t res;
  asm volatile(
    "mrs %0, ACTLR_EL1;\n"
    : "=r" (res)
    :
  );
  return res;
}

static void general_write_actlr_el1(uint64_t val) {
  asm volatile(
    "msr ACTLR_EL1, %0;\n"
    :
    : "r" (val)
  );
}

// reads bit i of ACTLR_EL1, 0 if disabled, 1 if enabled
static uint8_t general_read_actlr_eli_bit(uint8_t i) {
  return (general_read_actlr_el1() >> i) & 1ULL;
}

static void general_set_actlr_el1_bit(uint8_t i) {
  uint64_t prev = general_read_actlr_el1();
  uint64_t new = prev | (1ULL << i);

  general_write_actlr_el1(new);
  pr_info("on CPU [%d], actlr_el1 bit %d set, actlr_el1 was 0x%llx, now 0x%llx.\n", smp_processor_id(), i, prev, general_read_actlr_el1());
}

static void general_clear_actlr_el1_bit(uint8_t i) {
  uint64_t prev = general_read_actlr_el1();
  uint64_t new = prev & ~(1ULL << i);

  general_write_actlr_el1(new);
  pr_info("on CPU [%d], actlr_el1 bit %d cleared, actlr_el1 was 0x%llx, now 0x%llx.\n", smp_processor_id(), i, prev, general_read_actlr_el1());
}
