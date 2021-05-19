#!/bin/bash

grep -Po "(?<=version: ')[^']+" meson.build
