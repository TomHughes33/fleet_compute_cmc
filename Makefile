export DOCKER_REGISTRY_URL ?= 172.16.141.201:5000

service    := cmc
registry   := $(DOCKER_REGISTRY_URL)
image_name := inmarsat/fc/$(service)
ver_major  := $(shell git describe | sed -e 's/\([0-9]\+\).*/\1/')
ver_minor  := $(shell git describe | sed -e 's/[0-9]\+\.\([0-9]\+\).*/\1/')
ver_patch  := $(shell git describe | sed -e 's/[0-9]\+\.[0-9]\+\.\([0-9]\+\).*/\1/')
ver_commit := $(shell git describe | sed -e '/.*-.*/! d ; /.*-.*/ s/.*-\([0-9]\+\).*/-\1/' )
image_ver  ?= $(ver_major).$(ver_minor).$(ver_patch)$(ver_commit)
image      := $(image_name):$(image_ver)
user       := $(shell id -u)
PROXY      ?=

DOCKER_RUN_FLAGS := \
  -d \
  -t \
  --rm \
  --privileged \
  --net=host \
  -v `pwd`/Virtlyst:/Virtlyst \
  --name=cmc_build

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

build_env:
	-docker rm -f cmc cmc_build
	docker build -t cmd_build_env --build-arg uid=$(user) -f Virtlyst/Dockerfile.build .
	docker run $(DOCKER_RUN_FLAGS) cmd_build_env bash

build: build_env
	docker exec -t -u builder cmc_build bash -c "mkdir -p build && cd build && cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local && make"
	docker exec -t -u root cmc_build bash -c "cd build && make install"
	docker commit cmc_build cmc_build_env

image: build
	docker build -t $(image) -f Virtlyst/Dockerfile .

run: image
	docker run -d -t --rm --name=cmc -v /data:/usr/local/Virtlyst/data -v /var/run/libvirt/libvirt-sock:/var/run/libvirt/libvirt-sock -p 3000:3000 $(image)

stop:
	-docker rm -f cmc

publish: image
	docker tag $(image) $(registry)/$(image)
	docker tag $(image) $(registry)/$(image_name):latest
	docker push $(registry)/$(image)
	docker push $(registry)/$(image_name):latest
	docker rmi $(registry)/$(image)
	docker rmi $(registry)/$(image_name):latest

clean:
	-docker rm -f cmc cmc_build 2>/dev/null
	-docker rmi cmc_build_env 2>/dev/null
	-docker rmi $(image) 2>/dev/null

.PHONY: image publish clean
