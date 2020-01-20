
service ?= cmc

all $(MAKECMDGOALS):
	make -f service.mk $@ service=$(service)
