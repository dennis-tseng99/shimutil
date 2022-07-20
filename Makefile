default : all

#NAME		= shimutil
VERSION		= 15.6
ifneq ($(origin RELEASE),undefined)
DASHRELEASE	?= -$(RELEASE)
else
DASHRELEASE	?=
endif

ifeq ($(MAKELEVEL),0)
TOPDIR		?= $(shell pwd)
endif
ifeq ($(TOPDIR),)
override TOPDIR := $(shell pwd)
endif
override TOPDIR	:= $(abspath $(TOPDIR))
VPATH		= $(TOPDIR)
export TOPDIR

include $(TOPDIR)/Make.rules
include $(TOPDIR)/Make.defaults

TARGETS	= $(SHIMNAME)

dennis:
	@echo 'TARGET=' $(TARGETS)

OBJS	= shimutil.o 
#KEYS	= shim_cert.h ocsp.* ca.* shim.crt shim.csr shim.p12 shim.pem shim.key shim.cer
ORIG_SOURCES	= shimutil.c

ifeq ($(SOURCE_DATE_EPOCH),)
	UNAME=$(shell uname -s -m -p -i -o)
else
	UNAME=buildhost
endif

SOURCES = $(foreach source,$(ORIG_SOURCES),$(TOPDIR)/$(source))

ifneq ($(origin FALLBACK_VERBOSE), undefined)
	CFLAGS += -DFALLBACK_VERBOSE
endif

ifneq ($(origin FALLBACK_NONINTERACTIVE), undefined)
	CFLAGS += -DFALLBACK_NONINTERACTIVE
endif

ifneq ($(origin FALLBACK_VERBOSE_WAIT), undefined)
	CFLAGS += -DFALLBACK_VERBOSE_WAIT=$(FALLBACK_VERBOSE_WAIT)
endif

all: confcheck $(TARGETS)

confcheck:
ifneq ($(origin EFI_PATH),undefined)
	$(error EFI_PATH is no longer supported, you must build using the supplied copy of gnu-efi)
endif

shimutil.o: $(SOURCES)
ifneq ($(origin ENABLE_SHIM_CERT),undefined)
shim.o: shim_cert.h
endif
shimutil.o: $(wildcard $(TOPDIR)/*.h)

$(SHIMNAME) : $(SHIMSONAME) #post-process-pe

LIBS = 	gnu-efi/$(ARCH_GNUEFI)/lib/libefi.a \
       	gnu-efi/$(ARCH_GNUEFI)/gnuefi/libgnuefi.a lib/lib.a
       
$(SHIMSONAME): $(OBJS) $(LIBS)
	$(LD) -o $@ $(LDFLAGS) $^ -lefi -lgnuefi lib/lib.a

gnu-efi/$(ARCH_GNUEFI)/gnuefi/libgnuefi.a gnu-efi/$(ARCH_GNUEFI)/lib/libefi.a: CFLAGS+=-DGNU_EFI_USE_EXTERNAL_STDARG
gnu-efi/$(ARCH_GNUEFI)/gnuefi/libgnuefi.a gnu-efi/$(ARCH_GNUEFI)/lib/libefi.a:
	mkdir -p gnu-efi/lib gnu-efi/gnuefi
	$(MAKE) -C gnu-efi \
		COMPILER="$(COMPILER)" \
		CCC_CC="$(COMPILER)" \
		CC="$(CC)" \
		ARCH=$(ARCH_GNUEFI) \
		TOPDIR=$(TOPDIR)/gnu-efi \
		-f $(TOPDIR)/gnu-efi/Makefile \
		lib gnuefi inc

lib/lib.a: | $(TOPDIR)/lib/Makefile $(wildcard $(TOPDIR)/include/*.[ch])
	@echo 'SHIMSTEM2 =' $(SHIMSTEM)
	mkdir -p lib
	$(MAKE) VPATH=$(TOPDIR)/lib TOPDIR=$(TOPDIR) -C lib -f $(TOPDIR)/lib/Makefile

#post-process-pe : $(TOPDIR)/post-process-pe.c
#	$(HOSTCC) -std=gnu11 -Og -g3 -Wall -Wextra -Wno-missing-field-initializers -Werror -o $@ $<

%.efi: %.so
ifneq ($(OBJCOPY_GTE224),1)
	$(error objcopy >= 2.24 is required)
endif
	@echo 'SHIMSTEM =' $(SHIMSTEM)
	$(OBJCOPY) -D -j .text -j .sdata -j .data -j .data.ident \
		-j .dynamic -j .rodata -j .rel* \
		-j .rela* -j .dyn -j .reloc -j .eh_frame \
		-j .vendor_cert -j .sbat \
		$(FORMAT) $< $@
#	./post-process-pe -vv $@

ifneq ($(origin ENABLE_SHIM_HASH),undefined)
%.hash : %.efi
	$(PESIGN) -i $< -P -h > $@
endif

ifneq ($(origin ENABLE_SBSIGN),undefined)
%.efi.signed: %.efi shim.key shim.crt
	@$(SBSIGN) \
		--key shim.key \
		--cert shim.crt \
		--output $@ $<
else
%.efi.signed: %.efi certdb/secmod.db
	$(PESIGN) -n certdb -i $< -c "shim" -s -o $@ -f
endif

test test-clean test-coverage test-lto :
	@make -f $(TOPDIR)/include/test.mk \
		COMPILER="$(COMPILER)" \
		CROSS_COMPILE="$(CROSS_COMPILE)" \
		CLANG_WARNINGS="$(CLANG_WARNINGS)" \
		ARCH_DEFINES="$(ARCH_DEFINES)" \
		EFI_INCLUDES="$(EFI_INCLUDES)" \
		test-clean $@

$(patsubst %.c,%,$(wildcard test-*.c)) :
	@make -f $(TOPDIR)/include/test.mk EFI_INCLUDES="$(EFI_INCLUDES)" ARCH_DEFINES="$(ARCH_DEFINES)" $@

.PHONY : $(patsubst %.c,%,$(wildcard test-*.c)) test

#clean-test-objs:
#	@make -f $(TOPDIR)/include/test.mk EFI_INCLUDES="$(EFI_INCLUDES)" ARCH_DEFINES="$(ARCH_DEFINES)" clean

clean-gnu-efi:
	@if [ -d gnu-efi ] ; then \
		$(MAKE) -C gnu-efi \
			CC="$(CC)" \
			HOSTCC="$(HOSTCC)" \
			COMPILER="$(COMPILER)" \
			ARCH=$(ARCH_GNUEFI) \
			TOPDIR=$(TOPDIR)/gnu-efi \
			-f $(TOPDIR)/gnu-efi/Makefile \
			clean ; \
	fi

clean-lib-objs:
	@if [ -d lib ] ; then \
		$(MAKE) -C lib TOPDIR=$(TOPDIR) -f $(TOPDIR)/lib/Makefile clean ; \
	fi

clean-shim-objs:
	@rm -rvf *.o *.efi *.so
#	$(TARGET) $(SHIM_OBJS) $(MOK_OBJS) $(FALLBACK_OBJS) $(KEYS) certdb $(BOOTCSVNAME)
#	@rm -vf *.debug *.so *.efi *.efi.* *.tar.* buildid post-process-pe
#	@rm -vf Cryptlib/*.[oa] Cryptlib/*/*.[oa]
#	@if [ -d .git ] ; then git clean -f -d -e 'Cryptlib/OpenSSL/*'; fi

#clean-openssl-objs:
#	@if [ -d Cryptlib/OpenSSL ] ; then \
#		$(MAKE) -C Cryptlib/OpenSSL -f $(TOPDIR)/Cryptlib/OpenSSL/Makefile clean ; \
#	fi

#clean-cryptlib-objs:
#	@if [ -d Cryptlib ] ; then \
#		$(MAKE) -C Cryptlib -f $(TOPDIR)/Cryptlib/Makefile clean ; \
#	fi

clean: clean-shim-objs clean-lib-objs #clean-test-objs clean-gnu-efi clean-openssl-objs clean-cryptlib-objs 

#GITTAG = $(VERSION)

#test-archive:
#	@./make-archive $(if $(call get-config,shim.origin),--origin "$(call get-config,shim.origin)") --test "$(VERSION)"

#tag:
#	git tag --sign $(GITTAG) refs/heads/main
#	git tag -f latest-release $(GITTAG)

#archive: tag
#	@./make-archive $(if $(call get-config,shim.origin),--origin "$(call get-config,shim.origin)") --release "$(VERSION)" "$(GITTAG)" "shim-$(GITTAG)"

#.PHONY : install-deps shim.key

export ARCH CC CROSS_COMPILE LD OBJCOPY EFI_INCLUDE EFI_INCLUDES OPTIMIZATIONS
export FEATUREFLAGS WARNFLAGS WERRFLAGS
