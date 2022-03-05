#!/bin/bash

declare -A RELEASES_X86

BUILD_DISTRO=solus
CURRENT_DIR=$(pwd)
DISTRO=$(./detect_linux_build.sh)
RELEASES_X86["64_bit"]=$RELEASE_DISTROS_64_BIT
RELEASE_DISTROS_64_BIT=("centos7 arch")
RELEASE_DISTROS_ARM_64=("debian9 debian10 debian11")
RESULT=0
TAG_BUILDS_ROOT=${CURRENT_DIR}/tag_builds

set_failure() {
  NAME=$1
  if [ -f "${TAG_BUILDS_ROOT}/${NAME}.error" ]; then
    RESULT=1
  fi
}

exit_on_failure() {
  if [ "${RESULT}" != "0" ]; then
    echo "One or more builds failed"
    exit ${RESULT}
  fi
}

function run_build() {
  FILE=$1
  TYPE=$2
  if [ "$TYPE" = "arm64" ]; then
    DISTROPART=$(basename ${FILE})
    DISTRONAME=arm64_${DISTROPART}
    ARM64_DIR="./${DISTRONAME}"
    ARM64_BUILD_SCRIPT="${DISTRONAME}_build.sh"

    if [ "$ENABLE_UPLOAD" != "1" ] || [[ "${RELEASE_DISTROS_ARM_64[@]}" =~ "${DISTROPART}" ]]; then
      if mount -o bind "$(pwd)" "${ARM64_DIR}/mnt"; then
        cat << EOF > "${ARM64_DIR}/${ARM64_BUILD_SCRIPT}"
#!/bin/bash
cd /mnt && bash compile_tag.sh ${TAG} ${TYPE} ${ENABLE_UPLOAD} ${BITBUCKET_AUTH} ${BITBUCKET_TESTING}
EOF
        chmod +x "${ARM64_DIR}/${ARM64_BUILD_SCRIPT}"

        systemd-nspawn -D "${ARM64_DIR}" "./${ARM64_BUILD_SCRIPT}"

        umount "${ARM64_DIR}/mnt"
        rm -f "${ARM64_DIR}/${ARM64_BUILD_SCRIPT}"
        set_failure ${DISTRONAME}
      else
        echo "Mount failed: ${DISTRONAME}" > "${TAG_BUILDS_ROOT}/${DISTRONAME}.error"
        set_failure ${DISTRONAME}
      fi
    fi
  else
    DISTRONAME=$(basename ${FILE})
    CONTAINER_NAME=repertory${NAME_EXTRA}_${DISTRONAME}
    TAG_NAME=repertory${NAME_EXTRA}:${DISTRONAME}

    RELEASE_LOOKUP=${RELEASES_X86[$TYPE]}

    if [ "$ENABLE_UPLOAD" != "1" ] || [[ "${RELEASE_LOOKUP[@]}" =~ "${DISTRONAME}" ]]; then
      docker stop ${CONTAINER_NAME}
      docker rm ${CONTAINER_NAME}
      docker build -t ${TAG_NAME} - < docker/${TYPE}/${DISTRONAME} &&
        docker run -itd --device /dev/fuse --cap-add SYS_ADMIN --name ${CONTAINER_NAME} -v $(pwd):/mnt ${TAG_NAME} &&
        docker exec ${CONTAINER_NAME} bash -c "cd /mnt && bash compile_tag.sh ${TAG} ${TYPE} ${ENABLE_UPLOAD} ${BITBUCKET_AUTH} ${BITBUCKET_TESTING}"
      docker stop ${CONTAINER_NAME}
      docker rm ${CONTAINER_NAME}
      set_failure ${DISTRONAME}${NAME_EXTRA}
    fi
  fi
}

function run_builds() {
  TYPE=$1
  NAME_EXTRA=
  if [ -z "$TYPE" ]; then
    TYPE=64_bit
  fi

  echo Running Builds [${TYPE}]
  if [ "$TYPE" = "arm64" ]; then
    for FILE in $(pwd)/arm64/*; do
      run_build ${FILE} ${TYPE}
    done
  else
    for FILE in $(pwd)/docker/${TYPE}/*; do
      run_build ${FILE} ${TYPE}
    done
  fi
}

if [ "$DISTRO" != "${BUILD_DISTRO}" ]; then
  echo "Builds must be run on '${BUILD_DISTRO} Linux'"
  exit 1
elif [ "$UID" != 0 ]; then
  echo "Builds must be run as root"
  exit 1
else
  SINGLE_BUILD=0
  BUILD_NAME=
  if [ $1 = "-n" ]; then
    SINGLE_BUILD=1
    BUILD_NAME=$2
    TYPE=$3
    shift 3
  fi

  TAG=$1
  ENABLE_UPLOAD=$2
  BITBUCKET_AUTH=$3
  BITBUCKET_TESTING=$4
  if [ -z "$TAG" ]; then
    echo "Branch/tag not supplied"
    exit 1
  fi

  if [ ${SINGLE_BUILD} = 1 ]; then
    if [ "$TYPE" = "arm64" ]; then
      mount binfmt_misc -t binfmt_misc /proc/sys/fs/binfmt_misc
      echo -1 > /proc/sys/fs/binfmt_misc/qemu-aarch64 || true
      echo ':qemu-aarch64:M::\x7fELF\x02\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\xb7\x00:\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff\xfe\xff\xff\xff:/usr/bin/qemu-aarch64-static:OCF' > /proc/sys/fs/binfmt_misc/register

      run_build $(pwd)/arm64/${BUILD_NAME} ${TYPE}
    else
      run_build $(pwd)/docker/${TYPE}/${BUILD_NAME} ${TYPE}
    fi
  else
    RELEASE_LOOKUP=${RELEASES_X86["64_bit"]}
    if [ "$ENABLE_UPLOAD" != "1" ] || [[ "${RELEASE_LOOKUP[@]}" =~ "${BUILD_DISTRO}" ]]; then
      bash ./compile_tag.sh ${TAG} 64_bit ${ENABLE_UPLOAD} ${BITBUCKET_AUTH} ${BITBUCKET_TESTING}
      set_failure ${BUILD_DISTRO}
    fi

    run_builds 64_bit
    run_builds arm64
  fi

  exit_on_failure
fi
