SUBDIRS = strix-aut-cpp

.PHONY: all clean

all:
	for dir in $(SUBDIRS); do $(MAKE) all -C $$dir $@; done

clean:
	rm -f -r Debug
	rm -f -r Release
	rm -f -r x64
	for dir in $(SUBDIRS); do $(MAKE) clean -C $$dir $@; done
