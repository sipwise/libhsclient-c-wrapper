export LIBNAME=libhsclient-c-wrapper
export VERSION=0
export LIBSO=$(LIBNAME).so
export LIBSOVER=$(LIBNAME).so.$(VERSION)

LIBDIR=lib
HDR=include/hsclient-c-wrapper

DESTDIR?=/usr/local

all:
	$(MAKE) -C src
	$(MAKE) -C tests

clean:
	$(MAKE) -C src clean
	$(MAKE) -C tests clean
	rm -rf project.tgz cov-int

install: all
	mkdir -p $(DESTDIR)/$(HDR)
	cp include/*.h $(DESTDIR)/$(HDR)
	mkdir -p $(DESTDIR)/$(LIBDIR)
	cp src/$(LIBSOVER) $(DESTDIR)/$(LIBDIR)/$(LIBSOVER)
	ln -s $(LIBSOVER) $(DESTDIR)/$(LIBDIR)/$(LIBSO)

coverity:
	cov-build --dir cov-int $(MAKE)
	tar -czf project.tgz cov-int
	curl --form token=$(COVERITY_TOKEN) \
		--form email=$(DEBEMAIL) \
		--form file=@project.tgz \
		--form version="$(COVERITY_VERSION)" \
		--form description="automatic upload" \
		https://scan.coverity.com/builds?project=$(COVERITY_PROJECT)

.PHONY: all clean install coverity
