#!/bin/bash

BUILD_TYPE=$1

cd "$(dirname "$0")"

./make_${BUILD_TYPE}.sh || exit 1
