#!/bin/bash

abort() {
  echo "ERROR: $1!"
  exit 1
}

#
# This script must be run within darling.
#
if [[ "$OSTYPE" != "darwin"* ]]; then
    abort "This script must be run on macOS/darling"
fi

DARLING_CLT_DIR="/Library/Developer/DarlingCLT"

#
# Install required Xcode 3.2.6 packages.
#
installer -pkg DeveloperToolsCLI.pkg -target /Library/Developer/DarlingCLT
installer -pkg gcc4.2.pkg -target /Library/Developer/DarlingCLT/

#
# Move ld and install map_fd.
#
mkdir -p "${DARLING_CLT_DIR}/usr/lib"
tar -xf map_fd-1.0.tbz -C "${DARLING_CLT_DIR}/usr/lib"
mv "${DARLING_CLT_DIR}/usr/bin/ld_classic" "${DARLING_CLT_DIR}/usr/bin/ld_classic-bin"
cp ld_classic "${DARLING_CLT_DIR}/usr/bin/ld_classic"
