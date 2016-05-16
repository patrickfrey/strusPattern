#!/bin/sh

# DEBIAN
PACKAGEID="strusstream-0.7"

cd pkg/$PACKAGEID
dpkg-buildpackage

