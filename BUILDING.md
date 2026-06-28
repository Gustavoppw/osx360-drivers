# Building osx360-drivers

## Requirements
* Linux with [Darling](https://github.com/darlinghq/darling) installed
  * [install-darling.sh](tools/install-darling.sh) will install Darling on Ubuntu 24.04 LTS, currently used by GitHub Actions
  * Newer Darling versions and other Linux distributions should work but have not been tested.
* [Xcode Tools 3.2.6](https://developer.apple.com/services-account/download?path=/Developer_Tools/xcode_3.2.6_and_ios_sdk_4.3__final/xcode_3.2.6_and_ios_sdk_4.3.dmg) (requires an Apple login)

## Xcode tools installation
  From the Xcode Tools DMG, you'll need to extract two packages to the [tools](tools) directory:
    * DeveloperToolsCLI.pkg
    * gcc4.2.pkg

  Grab the `map_fd-1.0.tbz` from the latest release of [map_fd](https://github.com/bfleischer/map_fd). This will enable ld_classic to function under Darling.

  Enter a darling shell with `sudo darling shell` and run the `install-xcode-tools.sh` script.

## Building
  Once all pre-requisites have been installed, you should be able to build the project by running `make && make package` in the root of the project repo. This will build all kexts and generate the mkext archive.
