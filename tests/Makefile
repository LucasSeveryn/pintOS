TESTS = examples threads p1 userprog p2 vm p3 filesys p4

PATH := $(shell pwd)/../src/utils:$(PATH)

all:
	@echo "This Makefile has only \`check' targets."

check:
	$(MAKE) -C .. distclean
	for d in $(TESTS); do $(MAKE) $$d || exit 1; done
	@echo All tests passed.

clean:
	rm -rf $(TESTS)

define mk-sandbox
rm -rf $@ && mkdir $@ && cp -R ../src $@/src
cd $@/src && $(MAKE) -s clean
endef

define run-tests
cd $@/src/$(TASK) && time $(MAKE) check
endef

define grade-tests
cd $@/src/$(TASK) && $(MAKE) grade
cd $@/src/$(TASK) && grep -q PERFECT build/grade
cd $@/src/$(TASK) && grep -qv warning build/grade
endef

define compile
cd $@/src/$(TASK) && $(MAKE)
endef

define test-clean
cd $@/src/$(TASK)/build && $(MAKE) clean
cd $@/src/$(TASK)/build && set `find -type f` > /dev/null && test $$# = 1
endef

define clean
rm -rf $@
endef

define apply-patch
(cd $@ && patch -p0) < ../solutions/$@.patch
endef

TASK = $@
p1: TASK = threads
p2: TASK = userprog
p3: TASK = vm
p4: TASK = filesys

# Tests that just compile the provided code.
threads::
	$(mk-sandbox)
	$(compile)
	$(run-tests) TESTS=tests/threads/alarm-single
	$(test-clean)
	$(clean)
userprog vm filesys::
	$(mk-sandbox)
	$(compile)
	$(test-clean)
	$(clean)
examples::
	$(mk-sandbox)
	$(compile)
	$(clean)

# Tests for the reference solutions.
p1 p2 p3 p4::
	$(mk-sandbox)
	$(apply-patch)
	$(run-tests)
	$(grade-tests)
	$(test-clean)
	$(clean)
