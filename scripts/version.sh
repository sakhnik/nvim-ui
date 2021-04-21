#!/bin/bash

grep -Po '(?<=set\(VERSION )[^) ]+' CMakeLists.txt
