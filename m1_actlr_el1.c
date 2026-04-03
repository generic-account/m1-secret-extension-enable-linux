#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/types.h>

MODULE_AUTHOR("Gordon Lichtstein <glicht@mit.edu>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ACTLR_EL1 register poker for Apple M1");

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
  pr_info("on CPU [%d], actlr_el1 bit %d set, actlr_el1 was %llx, now %llx.\n", smp_processor_id(), i, prev, general_read_actlr_el1());
}

static void general_clear_actlr_el1_bit(uint8_t i) {
  uint64_t prev = general_read_actlr_el1();
  uint64_t new = prev & ~(1ULL << i);

  general_write_actlr_el1(new);
  pr_info("on CPU [%d], actlr_el1 bit %d cleared, actlr_el1 was %llx, now %llx.\n", smp_processor_id(), i, prev, general_read_actlr_el1());
}

uint64_t actlr_el1_status[NR_CPUS];

static void actlr_el1_query(void* info) {
  // ignoring info argument, even tho bit index is passed sometimes
  actlr_el1_status[smp_processor_id()] = general_read_actlr_el1();
}

static void actlr_el1_set_bit(void* info) {
  uint8_t i = *((uint8_t*)info); // immediately recast info to bit index
  general_set_actlr_el1_bit(i);
  actlr_el1_query(info);
}

static void actlr_el1_clear_bit(void* info) {
  uint8_t i = *((uint8_t*)info); // immediately recast info to bit index
  general_clear_actlr_el1_bit(i);
  actlr_el1_query(info);
}

// For each CPU, prints the value of actlr_el1 register
static ssize_t actlr_el1_status_load(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
  on_each_cpu(actlr_el1_query, NULL, 0);
  ssize_t size = 0;
  for (int i = 0; i < NR_CPUS; i++) if (actlr_el1_status[i] != ~0ULL) {
    size += sprintf(buf+size, "CPU[%d].ACTLR_EL1=%llx\n", i, actlr_el1_status[i]);
  }
  return size;
}

static ssize_t actlr_el1_status_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t cnt) {
  // buf in format "{s,c} bit_index", return -EINVAL if failure

  char action;
  int tmp;
  
  if (sscanf(buf, "%c %d", &action, &tmp) != 2) return -EINVAL;
  if (tmp < 0 || tmp >= 64) return -EINVAL;

  uint8_t i = (uint8_t)tmp;

  if (action == 's') on_each_cpu(actlr_el1_set_bit, &i, 0);
  else if (action == 'c') on_each_cpu(actlr_el1_clear_bit, &i, 0);
  else return -EINVAL;

  return cnt;
}

static struct kobj_attribute actlr_el1_status_query = __ATTR(status, 0664, actlr_el1_status_load, actlr_el1_status_store);

static struct attribute *actlr_el1_attrs[] = {
  &actlr_el1_status_query.attr,
  NULL,
};

static struct attribute_group actlr_el1_attr_group = {
  .attrs = actlr_el1_attrs,
};

struct kobject* actlr_el1_kobj;

static int __init actlr_el1_init(void) {
  int ret = 0;

  for (int i = 0; i < NR_CPUS; i++) actlr_el1_status[i] = ~0ULL;
  on_each_cpu(actlr_el1_query, NULL, 0);

  actlr_el1_kobj = kobject_create_and_add("m1_actlr_el1", kernel_kobj);

  if (!(actlr_el1_kobj)) return -ENOMEM;

  if ((ret = sysfs_create_group(actlr_el1_kobj, &actlr_el1_attr_group)))
    kobject_put(actlr_el1_kobj);

  return ret;
}

static void __exit actlr_el1_exit(void) {
  sysfs_remove_group(actlr_el1_kobj, &actlr_el1_attr_group);
  kobject_put(actlr_el1_kobj);
}

module_init(actlr_el1_init);
module_exit(actlr_el1_exit);
