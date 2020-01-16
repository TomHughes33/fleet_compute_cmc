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
make -f Makefile_lmc publish
make -f Makefile_lmc add_info_to_dashboard job_result=SUCCESS

# push the tags 
#git push --tags

# update edge service versions and tag
#cd ../../
make -f Makefile_lmc update_service_versions
make -f Makefile_lmc update_version
