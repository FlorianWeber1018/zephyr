# Copyright 2019-2020 Peter Bigot Consulting
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(timeutil)

target_sources(testbinary
  PRIVATE
    main.c
    test_gmtime.c
    test_s32.c
    test_s64.c
    test_sync.c
)
