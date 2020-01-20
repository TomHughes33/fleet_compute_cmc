export DOCKER_REGISTRY_URL ?= 172.16.141.201:5000

service    ?= cmc
ecr        ?= ecr.isatedge-shore.com
registry   := $(DOCKER_REGISTRY_URL)
image_name := inmarsat/fc/$(service)
ver_base   := $(shell cicd version latest-tag --component $(service) --version-only)
ver_branch ?= $(shell git rev-parse --abbrev-ref HEAD)

image_ver  ?= $(ver_base).0-$(ver_branch)
image      := $(image_name):$(image_ver)
user       := $(shell id -u)
PROXY      ?=

CUTELYST_VERSION := v2.7.0

DOCKER_RUN_FLAGS := \
  -d \
  -t \
  --rm \
  --privileged \
  --net=host \
  -v `pwd`/Virtlyst:/Virtlyst \
  --name=virtlyst_build

DOCKER_RUN_INFO_FLAGS := \
  --rm -t \
  --env JOB_NAME \
  --env BUILD_NUMBER \
  --env BUILD_URL \
  --env GIT_COMMIT \
  --env GIT_PREVIOUS_COMMIT \
  --env GIT_BRANCH \
  --env GIT_URL

ifdef PROXY
export all_proxy := $(PROXY)
DOCKER_RUN_FLAGS += --env all_proxy=$(PROXY)
endif

GET_SERVICE_VERSIONS := docker run  $(DOCKER_RUN_INFO_FLAGS) \
  ecr.isatedge-shore.com/inmarsat/pandora-pipeline/test-environment \
  /opt/pandora_python_libs/pandora/get_artifact_info.py

RECORD_VER_INFO := docker run $(DOCKER_RUN_INFO_FLAGS) \
  $(ecr)/inmarsat/pandora-pipeline/test-environment \
  /opt/pandora_python_libs/pandora/record_version_information.py

all: publish

version:
	@echo $(image_ver)

versions:
	@echo "$(service)=$(image_ver)"

.PHONY: version versions

virtlyst_build_env:
	-docker rm -f virtlyst_build
	docker build -t $@ --build-arg uid=$(user) -f Virtlyst/Dockerfile.build .
	docker run $(DOCKER_RUN_FLAGS) $@ bash

build: virtlyst_build_env
	docker exec -t -u builder virtlyst_build bash -c "mkdir -p build && cd build && cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local && make"
	docker exec -t -u root virtlyst_build bash -c "cd build && make install && make package"

image: build
	docker build -t $(image) -f Virtlyst/Dockerfile .

run: image
	docker run -d -t --rm --name=$(service) -v /data:/usr/local/Virtlyst/data -v /var/run/libvirt/libvirt-sock:/var/run/libvirt/libvirt-sock -p 3000:3000 $(image)

stop:
	-docker rm -f $(service)

publish: image
	docker tag $(image) $(registry)/$(image)
	docker tag $(image) $(registry)/$(image_name):latest
	docker push $(registry)/$(image)
	docker push $(registry)/$(image_name):latest
	docker rmi $(registry)/$(image)
	docker rmi $(registry)/$(image_name):latest

add_info_to_dashboard:
	$(RECORD_VER_INFO) \
                --component $(service) \
                --version '$(image_ver)' \
                --job-result '$(job_result)'

clean:
	-docker rm -f $(service) virtlyst_build 2>/dev/null
	-docker rmi $(image) 2>/dev/null

distclean: clean
	-docker rmi virtlyst_build_env 2>/dev/null

.PHONY: image publish add_info_to_dashboard distclean clean
