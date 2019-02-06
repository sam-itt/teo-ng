# Makefile for GNU Make

TOOLS = cc90hfe k7tools sap

.PHONY:	all clean $(TOOLS)

all:	$(TOOLS)

clean:
	@for d in $(TOOLS) ; do \
	  if [ -f $$d/makefile ] ; then \
	    $(MAKE) -C $$d $@ ; \
	  fi ; \
	done

$(TOOLS):	$(addsuffix /makefile, $(TOOLS))
	$(MAKE) -C $@

%/makefile:
	cd $@ && ./fixunix.sh
