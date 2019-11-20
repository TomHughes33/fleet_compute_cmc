export DOCKER_REGISTRY_URL ?= 172.16.141.201:5000

service    := virtlyst
registry   ?= $(DOCKER_REGISTRY_URL)
image_name := inmarsat/fc/$(service)
ver_major  := $(shell git describe | sed -e 's/\([0-9]\+\).*/\1/')
ver_minor  := $(shell git describe | sed -e 's/[0-9]\+\.\([0-9]\+\).*/\1/')
ver_patch  := $(shell git describe | sed -e 's/[0-9]\+\.[0-9]\+\.\([0-9]\+\).*/\1/')
ver_commit := $(shell git describe | sed -e '/.*-.*/! d ; /.*-.*/ s/.*-\([0-9]\+\).*/-\1/' )
image_ver  ?= $(ver_major).$(ver_minor).$(ver_patch)$(ver_commit)
image      := $(image_name):$(image_ver)
PROXY      ?=


DOCKER_RUN_FLAGS := \
  --rm -t \
  -v $(WORKSPACE):/work \
  -w /work

ifdef PROXY
export all_proxy := $(PROXY)
DOCKER_RUN_FLAGS += --env all_proxy=$(PROXY)
endif

all: publish

version:
	@echo $(image_ver)

versions:
	@echo "$(service)=$(image_ver)"

.PHONY: version versions

image:
	docker build -t $(image) --build-arg image_ver=$(image_ver) -f Virtlyst/Dockerfile .

publish: image
	docker tag $(image) $(registry)/$(image)
	docker tag $(image) $(registry)/$(image_name):latest
	docker push $(registry)/$(image)
	docker push $(registry)/$(image_name):latest
	docker rmi $(registry)/$(image)
	docker rmi $(registry)/$(image_name):latest

clean:
	-docker rmi $(image) 2>/dev/null

.PHONY: image publish clean
