#!/bin/bash

CENTOS_VERSIONS=("7 8")
DEBIAN_VERSIONS=("9 10 11")
FEDORA_VERSIONS=("35 34 33 32 31 30 29 28")
OPENSUSE_VERSIONS=("15.0 15.1 15.2 15.3")
UBUNTU_VERSIONS=("18.04 19.10 20.04 20.10 21.04 21.10")

DISTNAME=unknown
DISTVER=

resetDistVer() {
  DISTNAME=unknown
  DISTVER=
}

if [ -f /etc/centos-release ]; then
  . /etc/os-release
  if [[ "${CENTOS_VERSIONS[@]}" =~ "${VERSION_ID}" ]]; then
    DISTNAME=centos
    DISTVER=$VERSION_ID
  else
    resetDistVer
  fi
elif [ -f /etc/fedora-release ]; then
  . /etc/os-release
  if [[ "${FEDORA_VERSIONS[@]}" =~ "${VERSION_ID}" ]]; then
    DISTNAME=fedora
    DISTVER=$VERSION_ID
  else
    resetDistVer
  fi
elif [ -f /etc/solus-release ]; then
  DISTNAME=solus
elif [ -f /etc/lsb-release ]; then
  . /etc/lsb-release
  DISTNAME=$(echo ${DISTRIB_ID} | awk '{print tolower($0)}')
  DISTVER=${DISTRIB_RELEASE}
  if [ "$DISTNAME" != "ubuntu" ]; then
    resetDistVer
  elif [[ ! "${UBUNTU_VERSIONS[@]}" =~ "${DISTVER}" ]]; then
    resetDistVer
  fi
fi

if [ "$DISTNAME" = "unknown" ] && [ -f /etc/debian_version ]; then
  DISTNAME=debian
  DISTVER=$(head -1 /etc/debian_version | awk -F. '{print $1}')
  if [[ ! "${DEBIAN_VERSIONS[@]}" =~ "${DISTVER}" ]]; then
    resetDistVer
  fi
fi

if [ "$DISTNAME" = "unknown" ]; then
  if [ -f /etc/os-release ]; then
    . /etc/os-release
    if [ "$ID" = "arch" ] || [ "$ID" = "manjaro" ]; then
      DISTNAME=arch
    elif [ "$ID" = "opensuse-leap" ]; then
      if [[ "${OPENSUSE_VERSIONS[@]}" =~ "${VERSION_ID}" ]]; then
        DISTNAME=opensuse
        if [ "$VERSION_ID" = "15.0" ]; then
          DISTVER=15
        else
          DISTVER=$VERSION_ID
        fi
      else
        resetDistVer
      fi
    elif [ "$ID" = "opensuse-tumbleweed" ]; then
      DISTNAME=tumbleweed
      DISTVER=
    else
      resetDistVer
    fi
  else
    resetDistVer
  fi
fi

echo ${DISTNAME}${DISTVER}
