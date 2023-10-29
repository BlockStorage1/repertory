#!/bin/bash

pushd "$(dirname "$0")/.."
scripts/make_unix_docker.sh ${1} "${2}" 1
popd
