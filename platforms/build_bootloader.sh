#!/bin/bash

if [ -n "$1" ]
then
  platform="$1"
fi

if [ -z "$platform" ]
then
  echo "Platform neither specified via command line nor environment variable."
  echo "Environment: "
  set
  exit -1
fi

if [ ! -d "$platform" ]
then
  echo "No such platform $platform."
  exit -1
fi


if [ ! -d "$platform/bootloader/makefiles" ]
then
  echo "No makefile for platform $platform."
  exit -1
fi

cd "$platform/bootloader/makefiles"
make -s clean &&
make -s &&
make -s targz

