#!/bin/bash

#
# Download darling
#
wget -O /tmp/darling-debs.zip https://github.com/darlinghq/darling/releases/download/v0.1.20260608/debs_20260608.zip
rm -rf /tmp/darling-debs
unzip -j /tmp/darling-debs.zip -d /tmp/darling-debs

#
# Install darling packages.
#
sudo apt update
sudo apt -y install /tmp/darling-debs/*.deb

rm -rf /tmp/darling-debs
rm -rf /tmp/darling-debs.zip
