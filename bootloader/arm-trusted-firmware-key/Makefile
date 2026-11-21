ifneq ($(UBOOT),)
$(info ========== arm-trusted-firmware-key use $(UBOOT) --------)
include ../$(UBOOT)/.config
include ../../.config
else
$(info -------- arm-trusted-firmware-key use u-boot-2015.04 --------)
include ../u-boot-2015.04/.config
endif

all:
	[ -d ../image/fip ] || mkdir -p ../image/fip
ifeq ($(CONFIG_TARGET_VENUS),y)
	install -m 400 ./files/venus_rotprivk_rsa.pem ../image/fip/rotprivk_rsa.pem
	@echo "xxxxxxx Target: CONFIG_TARGET_VENUS  xxxxxxxxxxx"
else ifeq ($(CONFIG_TARGET_RTK_TAURUS),y)
	install -m 400 ./files/taurus_rotprivk_rsa.pem ../image/fip/rotprivk_rsa.pem
	@echo "xxxxxxx Target: CONFIG_TARGET_RTK_TAURUS  xxxxxxxxxxx"
	install -m 400 ./files/taurus_aes_keys ../image/fip/aes_keys
else ifeq ($(CONFIG_TARGET_RTK_ELNATH),y)
	install -m 400 ./files/elnath_rotprivk_rsa.pem ../image/fip/rotprivk_rsa.pem
	@echo "xxxxxxx Target: CONFIG_TARGET_RTK_ELNATH  xxxxxxxxxxx"
	install -m 400 ./files/elnath_aes_keys ../image/fip/aes_keys
else
	install -m 400 ./files/g3_rotprivk_rsa.pem ../image/fip/rotprivk_rsa.pem
	@echo "------- Target: CONFIG_TARGET_G3 --------------"
endif
