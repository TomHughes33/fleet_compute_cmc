#!/bin/sh

set -e

service=cmc
changed=$(cicd version changed-components --component ${service}=.)

test -z "${changed}" && exit 0

cur_tag=$(cicd version latest-tag --component ${service})
new_tag=$(cicd version create-tag --component ${service})

echo "Created new release tag ${new_tag}"

# build and publish #
echo "Making a cmc release"
make publish service=${service}
make add_info_to_dashboard service=${service} job_result=SUCCESS

# push the tags
git push --tags

# update shore service versions and tag
echo "Making a shore release"
git clone git@github.com:Inmarsat/fleet_compute.git
cd fleet_compute
sed -i "/version_cmc/c\version_${service}: $(new_tag)" shore/provisioning/service_versions.yaml
if ! git diff --exit-code -- shore/provisioning/service_versions.yaml; then
    git add shore/provisioning/service_versions.yaml;
    git commit -m "Auto update Shore service version file";
    i=0;
    while [ $$i -lt 5 ] && ! git push origin master;
    do
        i=$$((i+1));
        git pull origin master;
    done;
fi
cicd version create-tag --only-if-necessary
git push --tags
