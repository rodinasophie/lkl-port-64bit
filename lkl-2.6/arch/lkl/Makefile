#
# arch/v850/Makefile
#
#  Copyright (C) 2001,02,03,05  NEC Corporation
#  Copyright (C) 2001,02,03,05  Miles Bader <miles@gnu.org>
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies. Remember to do have actions
# for "archclean" and "archdep" for cleaning up and making dependencies for
# this architecture
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#

LDFLAGS_vmlinux := -r

arch_dir = arch/lkl

KBUILD_CFLAGS += -fno-builtin -g
def_arch = -D__arch_lkl__
KBUILD_CPPFLAGS +=  $(def_arch)
CHECKFLAGS     += $(def_arch)
LINUXINCLUDE += -I$(srctree)/arch

KALLSYMS += --symbol-prefix=$(shell echo | $(CROSS_COMPILE)cpp -dM | grep USER_LABEL_PREFIX | cut -f3 -d ' ')

KBUILD_DEFCONFIG := defconfig

core-y += $(arch_dir)/kernel/ 
libs-y  += $(arch_dir)/lib/
drivers-y += $(arch_dir)/drivers/

lkl.a: vmlinux
	$(MAKE) $(build)=$(arch_dir)/envs/ lkl.a
	$(OBJCOPY) -w -G_lkl* -Glkl* -Gget_filesystem_list vmlinux
	$(AR) -q $@ $^

lkl.o: vmlinux
	$(MAKE) $(build)=$(arch_dir)/envs/ lkl.o
	$(OBJCOPY) -w -G_lkl* -Glkl*  -Gget_filesystem_list -G_get_filesystem_list lkl.o





