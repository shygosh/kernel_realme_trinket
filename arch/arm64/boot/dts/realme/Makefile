ifeq ($(CONFIG_BUILD_ARM64_DT_OVERLAY),y)

dtbo-$(CONFIG_VENDOR_EDIT) += \
	RMX1911-trinket-overlay.dtbo \
	RMX1927-trinket-overlay.dtbo \
	RMX2030-trinket-overlay.dtbo

RMX1911-trinket-overlay.dtbo-base := realme-trinket.dtb
RMX1927-trinket-overlay.dtbo-base := realme-trinket.dtb
RMX2030-trinket-overlay.dtbo-base := realme-trinket.dtb
endif
always		:= $(dtb-y)
subdir-y	:= $(dts-dirs)

$(obj)/%.dtbo:$(src)/../../../../arm64/boot/dts/realme/%.dts FORCE
	$(call if_changed_dep,dtc)
	$(call if_changed,dtbo_verify)

dtbs: $(addprefix $(obj)/,$(dtb-y)) $(addprefix $(obj)/,$(dtbo-y))
clean-files	:= *.dtb *.dtbo
