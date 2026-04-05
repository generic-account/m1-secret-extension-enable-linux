#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>

typedef struct {
  const char *name;
  uint32_t a;
  uint32_t b;
  int is_sub; // 0 if sub, 1 if add
} test_case_t;

typedef struct {
  uint32_t result;
  uint64_t nzcv;
  uint8_t pf; // NZCV bit 26
  uint8_t af; // NZCV bit 27
} hw_flags_t;

static inline hw_flags_t do_adds32(uint32_t a, uint32_t b) {
  hw_flags_t out;
  uint64_t nzcv;

  asm volatile(
    "adds %w[result], %w[a], %w[b]\n\t"
    "mrs %x[nzcv], nzcv\n\t"
    : [result] "=&r" (out.result),
      [nzcv] "=r" (nzcv)
    : [a] "r" (a),
      [b] "r" (b)
    : "cc"
  );

  out.nzcv = nzcv;
  out.pf = (uint8_t)((nzcv >> 26) & 1u);
  out.af = (uint8_t)((nzcv >> 27) & 1u);
  return out;
}

static inline hw_flags_t do_subs32(uint32_t a, uint32_t b) {
  hw_flags_t out;
  uint64_t nzcv;

  asm volatile(
    "subs %w[result], %w[a], %w[b]\n\t"
    "mrs %x[nzcv], nzcv\n\t"
    : [result] "=&r" (out.result),
      [nzcv] "=r" (nzcv)
    : [a] "r" (a),
      [b] "r" (b)
    : "cc"
  );

  out.nzcv = nzcv;
  out.pf = (uint8_t)((nzcv >> 26) & 1u);
  out.af = (uint8_t)((nzcv >> 27) & 1u);
  return out;
}

// PF = 1 when low byte has an even number of set bits
static uint8_t expected_pf(uint32_t result) {
  uint8_t x = (uint8_t)result;
  x ^= x >> 4;
  x ^= x >> 2;
  x ^= x >> 1;
  return (uint8_t)(~x & 1U);
}

// AF = carry/borrow across bit 3 (lower and upper nibbles (I love the word nibbles))
static uint8_t expected_af(uint32_t a, uint32_t b, uint32_t result) {
  return (uint8_t)((((a ^ b ^ result) & 0x10u) != 0u));
}

static void run_one(const test_case_t *tc) {
  hw_flags_t hw = tc->is_sub ? do_subs32(tc->a, tc->b) : do_adds32(tc->a, tc->b);
  uint8_t exp_pf = expected_pf(hw.result);
  uint8_t exp_af = expected_af(tc->a, tc->b, hw.result);

  uint8_t ok_pf = hw.pf == exp_pf;
  uint8_t ok_af = hw.af == exp_af;

  printf("%-12s\t PF %u/%u\t AF %u/%u\t %s\n",
        tc->name, hw.pf, exp_pf, hw.af, exp_af, (ok_pf && ok_af) ? "OK" : "BAD");
}

int main(void) {
  static const test_case_t tests[] = {
    // ADDS
    { "add_af1_pf0", 0x0000000f, 0x00000001, 0 }, // 0x10: AF=1, PF=0
    { "add_af0_pf0", 0x00000000, 0x00000001, 0 }, // 0x01: AF=0, PF=0
    { "add_af0_pf1", 0x00000001, 0x00000002, 0 }, // 0x03: AF=0, PF=1
    { "add_af1_pf1", 0x000000ff, 0x00000001, 0 }, // 0x100: AF=1, PF=1

    // SUBS
    { "sub_af1_pf1", 0x00000010, 0x00000001, 1 }, // 0x0f: AF=1, PF=1
    { "sub_af0_pf1", 0x00000003, 0x00000000, 1 }, // 0x03: AF=0, PF=1
    { "sub_af0_pf0", 0x00000001, 0x00000000, 1 }, // 0x01: AF=0, PF=0
    { "sub_af1_pf0", 0x00000008, 0x00000001, 1 }, // 0x07: AF=1, PF=0
  };

  printf("test name\t PF/AF hw/exp\t OK/BAD\n");

  size_t i;
  for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
    run_one(&tests[i]);
  }

  return 0;
}
