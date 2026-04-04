#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/types.h>

#include "utils.h"

MODULE_AUTHOR("Yangyu Chen <cyy@cyyself.name>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("TSO Enabler for Apple M1");

int m1tso_status[NR_CPUS];

static void m1tso_query(void *info) {
  m1tso_status[smp_processor_id()] = general_read_actlr_eli_bit(1U);
}

static void m1tso_settso(void *info) {
  general_set_actlr_el1_bit(1U);
  m1tso_query(info);
}

static void m1tso_cleartso(void *info) {
  general_clear_actlr_el1_bit(1U);
  m1tso_query(info);
}

static ssize_t m1tso_status_load(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
  on_each_cpu(m1tso_query, NULL, 0);
  ssize_t size = 0;
  for (int i = 0; i < NR_CPUS; i++) if (m1tso_status[i] != -1) {
    size += sprintf(buf + size, "CPU[%d].TSO=%d\n", i, m1tso_status[i]);
  }
  return size;
}

static ssize_t m1tso_status_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t cnt) {
  if (cnt >= 1) {
    if (buf[0] == 's') on_each_cpu(m1tso_settso, NULL, 0);
    else if (buf[0] == 'c') on_each_cpu(m1tso_cleartso, NULL, 0);
  }
  return cnt;
}

static struct kobj_attribute m1tso_status_query = __ATTR(status, 0664, m1tso_status_load, m1tso_status_store);

static struct attribute *m1tso_attrs[] = {
  &m1tso_status_query.attr,
  NULL,
};

static struct attribute_group m1tso_attr_group = {
  .attrs = m1tso_attrs,
};

struct kobject *m1tso_kobj;

static int __init m1tso_init(void) {
  int ret = 0;
  
  for (int i = 0; i < NR_CPUS; i++) m1tso_status[i] = -1;
  on_each_cpu(m1tso_query, NULL, 0);

  if (!(m1tso_kobj =  kobject_create_and_add("m1tso", kernel_kobj)))
    return -ENOMEM; // return immediately, dont overwrite :)
  
  if ((ret = sysfs_create_group(m1tso_kobj, &m1tso_attr_group)))
    kobject_put(m1tso_kobj);
  
  return ret;
}

static void __exit m1tso_exit(void) {
  sysfs_remove_group(m1tso_kobj, &m1tso_attr_group);
  kobject_put(m1tso_kobj);
}

module_init(m1tso_init);
module_exit(m1tso_exit);
