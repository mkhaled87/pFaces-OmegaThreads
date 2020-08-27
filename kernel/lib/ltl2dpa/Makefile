.PHONY: all owl clean

all: owl
	rm -rf build
	mkdir build; cd build; cmake ..; make

owl:
ifneq ($(wildcard ./owl/.*),)
	$(info OWL folder is found and we assume it is installed.)
else
	$(error OWL folder is not found and you should install it first (run the install script install-owl.sh).)
endif

clean:
	rm -rf ./build/
	rm -rf ./tests/strix/build/
	rm -rf ./tests/symmodel_solver/build/
	rm -f -r Debug
	rm -f -r Release
	rm -f -r x64
