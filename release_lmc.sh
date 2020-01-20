#!/bin/sh

set -e

service=lmc
changed=$(cicd version changed-components --component ${service}=.)

test -z "${changed}" && exit 0

cur_tag=$(cicd version latest-tag --component ${service})
new_tag=$(cicd version create-tag --component ${service})

echo "Created new release tag ${new_tag}"

# build and publish #
echo "Making release"
make publish service=$(service)
make add_info_to_dashboard service=$(service) job_result=SUCCESS

# push the tags
git push --tags

# update edge service versions and tag
git clone git@github.com:Inmarsat/fleet_compute.git
cd fleet_compute/edge
make update_service_versions
make update_version
