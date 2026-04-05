# ACTLR_EL1 Register Poker for Linux

A Linux Kernel Module that lets you poke the ACTLR_EL1 register, enabling/disabling some undocumented hardware extensions.
Right now I've focused on poking bit 4 (APFLG) to enable the AF/PF flag extension, as described in [Why is Rosetta 2 fast](https://dougallj.wordpress.com/2022/11/09/why-is-rosetta-2-fast/). I also updated the original m1tso enabler kernel module to use my helpers, and fix a bug or two.

It was tested on `6.18.15-400.asahi.fc43.aarch64+16k` with Apple Macbook Pro (M1, 2020). Make sure to recompile kernel with flags unset before using.

## How to Use?

### Load the kernel module

```bash
make
sudo insmod build/m1_acltr_el1.ko
```

### Query status

```bash
sudo cat /sys/kernel/m1_acltr_el1/status
```

You'll get the value of ACLTR_EL1 register on each CPU (it will look something like this if all possible bits are set):
```
CPU[0].ACTLR_EL1=0xe7b
CPU[1].ACTLR_EL1=0xe7b
CPU[2].ACTLR_EL1=0xe7b
CPU[3].ACTLR_EL1=0xe7b
CPU[4].ACTLR_EL1=0xe7b
CPU[5].ACTLR_EL1=0xe7b
CPU[6].ACTLR_EL1=0xe7b
CPU[7].ACTLR_EL1=0xe7b
```

### Set bit i

```bash
echo s {i} | sudo tee /sys/kernel/m1_acltr_el1/status
```

Ex (turning on APFLG bit 4, the AF/PF extension):

```bash
echo s 4 | sudo tee /sys/kernel/m1_acltr_el1/status
```

### Clear bit i

```bash
echo c {i} | sudo tee /sys/kernel/m1_acltr_el1/status
```

### ACTLR_EL1 Bit Table

from [Asahi system register dump](https://asahilinux.org/docs/hw/cpu/system-registers/). The bit is set in the following binary number if it is settable / saved: 0b111001111011. Note that this includes all of the Asahi-identified bits except bit 12 for some reason, but also bits 0, 9, 10, and 11. Maybe bit 12 is settable if some of the other bits are unset?

ACTLR_EL1 (ARM standard-not-standard):
- [1] Enable TSO
- [3] Disable HWP
- [4] Enable APFLG <- this controls AF/PF
- [5] Enable Apple FP extensions. This makes FPCR.FZ a don't care, and replaces it with AFPCR.DAZ and AFPCR.FTZ.
- [6] Enable PRSV
- [12] IC IVAU Enable ASID

## How to Test?

For TSO, see [testtso](https://github.com/saagarjha/TSOEnabler/blob/master/testtso/main.c) in [saagarjha/TSOEnabler](https://github.com/saagarjha/TSOEnabler).
I'll implement a test for TSO in a bit.

For PF/AF extension:
```bash
./build/pf_af_test
```

## Note for AsahiLinux Kernel > v6.3

Please recompile the kernel with `CONFIG_ARM64_ACTLR_STATE is not set` and `CONFIG_ARM64_MEMORY_MODEL_CONTROL is not set` in your kernel `.config`.

> ⚠️ If you don't do this, every process without prctl set to TSO mode will **return to non-TSO mode** after context switch, and the value of ACTLR_EL1 will be reset.

Related Issues: [https://github.com/AsahiLinux/linux/issues/189](https://github.com/AsahiLinux/linux/issues/189) [https://github.com/cyyself/m1tso-linux/issues/2](https://github.com/cyyself/m1tso-linux/issues/2)

## Related Blogposts (from original author of the m1tso kernel module)

[Is TSO a historical burden for modern CPU? Case study on Apple M1](https://blog.cyyself.name/tso-apple-m1/)
