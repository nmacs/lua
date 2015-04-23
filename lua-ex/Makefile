.EXPORT_ALL_VARIABLES:

ifdef FOR_WINDOWS
all:
	$(MAKE) -C src/w32api libex.a
	cp src/w32api/libex.a libex.a
else
all:
	$(MAKE) -C src/posix libex.a
	cp src/posix/libex.a libex.a
endif

clean:
	-rm -f libex.a
	-$(MAKE) -C src/w32api clean
	-$(MAKE) -C src/posix clean
