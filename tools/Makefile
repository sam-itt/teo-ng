TOOLS = cc90hfe k7tools sap

.PHONY:	all $(TOOLS)

all:	$(TOOLS)

$(TOOLS):	$(addsuffix /makefile, $(TOOLS))
	$(MAKE) -C $@

%/makefile:
	cd $@ && ./fixunix.sh
