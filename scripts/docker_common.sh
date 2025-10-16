#!/usr/bin/env bash

if [ "${PROJECT_BUILD_ARCH}" == "aarch64" ] &&
  [ "${PROJECT_ENABLE_MULTIARCH_DOCKER}" == "1" ]; then
  docker run --rm --privileged multiarch/qemu-user-static --reset -p yes
fi

if [ "${DOCKER_NAME}" == "mingw64" ]; then
  APP_VERSION_BUILD_ARGS=${PROJECT_MINGW64_DOCKER_BUILD_ARGS}
  IS_MINGW=1
  IS_UNIX=1
else
  IS_MINGW=0
  IS_UNIX=0
fi

if [ "${DOCKER_CREATE_ONLY}" != "1" ]; then
  docker stop ${DOCKER_CONTAINER}
  docker rm ${DOCKER_CONTAINER}
fi

pushd "${PROJECT_SOURCE_DIR}/support"
cp -f ${PROJECT_SOURCE_DIR}/docker/${PROJECT_BUILD_ARCH}/${DOCKER_NAME} Dockerfile

if [ "${PROJECT_BUILD_ARCH}" == "aarch64" ]; then
  docker build ${APP_VERSION_BUILD_ARGS} \
    --platform linux/arm64 \
    --build-arg NUM_JOBS=${NUM_JOBS} \
    --build-arg UID=$(id -u) \
    --build-arg GID=$(id -g) \
    --build-arg USERNAME=$(id -un) \
    -t ${DOCKER_TAG} . || exit 1
else
  docker build ${APP_VERSION_BUILD_ARGS} \
    --build-arg NUM_JOBS=${NUM_JOBS} \
    --build-arg UID=$(id -u) \
    --build-arg GID=$(id -g) \
    --build-arg USERNAME=$(id -un) \
    -t ${DOCKER_TAG} . || exit 1
fi
rm Dockerfile
popd

if [ "${DOCKER_CREATE_ONLY}" != "1" ]; then
  docker run -td \
    --env DEVELOPER_PRIVATE_KEY=${DEVELOPER_PRIVATE_KEY} \
    --env DEVELOPER_PUBLIC_KEY=${DEVELOPER_PUBLIC_KEY} \
    --env MY_NUM_JOBS=${NUM_JOBS} \
    --name ${DOCKER_CONTAINER} \
    -u $(id -u):$(id -g) \
    -v /.ci/${PROJECT_NAME}:/.ci/${PROJECT_NAME}:ro \
    -v ${PROJECT_SOURCE_DIR}:${PROJECT_SOURCE_DIR}:rw,z \
    -w ${PROJECT_SOURCE_DIR} \
    ${DOCKER_TAG} || exit 1

  if [ "${DOCKER_SHELL}" == "1" ]; then
    docker exec -it ${DOCKER_CONTAINER} /bin/bash
  else
    docker exec \
      ${DOCKER_CONTAINER} \
      /bin/bash -c \
      "${PROJECT_SOURCE_DIR}/scripts/make_common.sh \"${PROJECT_BUILD_ARCH}\" \"${PROJECT_CMAKE_BUILD_TYPE}\" \"${PROJECT_BUILD_CLEAN}\" ${IS_MINGW} ${IS_UNIX}" || exit 1
  fi

  docker stop ${DOCKER_CONTAINER}
  docker rm ${DOCKER_CONTAINER}
fi
