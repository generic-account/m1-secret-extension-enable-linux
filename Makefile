obj-m += m1tso.o
obj-m += m1_actlr_el1.o

KDIR := /lib/modules/$(shell uname -r)/build
BUILD_DIR := $(CURDIR)/build

all: modules user

modules: $(BUILD_DIR)
	$(MAKE) -C $(KDIR) M=$(CURDIR) MO=$(BUILD_DIR) modules

user: $(BUILD_DIR)/pf_af_test

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/pf_af_test: pf_af_test.c | $(BUILD_DIR)
	$(CC) -O2 -Wall -Wextra -std=gnu11 $< -o $@

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
	rm -rf $(BUILD_DIR)
