export OSX_VERSION	:=	jaguar

include common/kext_info.mk

BUILD_PKG	:=	build_pkg_$(OSX_VERSION)
KEXTS		:=	XenonPlatform XenonPCI XenonCoreGraphics XenonAudio XenonEthernet XenonSATA XenonSMC
MKEXT_NAME	:=	Xbox360.mkext
ARCHIVE_ZIP	:= 	Xeintosh-osx-drivers-$(KEXT_VERSION).zip

.PHONY: all clean

all:
	@echo "OSX target version: $(OSX_VERSION)"
	$(foreach kext,$(KEXTS),$(MAKE) -C $(kext) all &&) true

	rm -rf $(BUILD_PKG)
	$(foreach kext,$(KEXTS),mkdir -p $(BUILD_PKG)/Kexts/$(kext).kext &&) true
	$(foreach kext,$(KEXTS),cp -r $(kext)/build_kext_$(OSX_VERSION)/$(kext).kext $(BUILD_PKG)/Kexts &&) true

package:
	@echo "OSX target version: $(OSX_VERSION)"
	python3 ./tools/make-mkext.py $(BUILD_PKG)/Kexts $(BUILD_PKG)/$(MKEXT_NAME)
	cd $(BUILD_PKG); zip -qry -FS ../$(ARCHIVE_ZIP) *

clean:
	rm -rf $(BUILD_PKG)*
	rm -rf *.mkext
	rm -rf *.zip
	$(foreach kext,$(KEXTS),$(MAKE) -C $(kext) clean &&) true
