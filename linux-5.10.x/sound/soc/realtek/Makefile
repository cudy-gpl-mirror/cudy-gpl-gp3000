# RTL819X Platform Support

ifeq ($(CONFIG_SND_RTL8277C_SOC_I2S),m)
	snd-soc-rtl8277c-objs := rtl8277c-pcm.o rtl8277c-i2s.o
else
	snd-soc-rtl8277c-i2s-objs := rtl8277c-i2s.o
	snd-soc-rtl8277c-objs := rtl8277c-pcm.o
endif
#snd-soc-rtl819x-objs := rtl819x-pcm.o
#snd-soc-rtl819x-i2s-objs := rtl819x-i2s.o
snd-soc-i2c-rt5621-objs := i2c_rt5651.o
snd-soc-i2s-rtktest-objs := rtl8277c_test_dai_link.o

obj-$(CONFIG_SND_RTL819X_SOC_ALC5651) += snd-soc-i2c-rt5651.o

ifeq ($(CONFIG_SND_RTL8277C_SOC_I2S),m)
	obj-$(CONFIG_SND_RTL8277C_SOC_I2S) += snd-soc-rtl8277c.o
else
	obj-$(CONFIG_SND_RTL8277C_SOC_I2S) += snd-soc-rtl8277c.o
	obj-$(CONFIG_SND_RTL8277C_SOC_I2S) += snd-soc-rtl8277c-i2s.o
endif
obj-$(CONFIG_SND_RTL819X_SOC_ALC5651) += snd-soc-i2c-rt5651.o
obj-$(CONFIG_SND_RTL8277C_SOC_IIS_TEST) += snd-soc-i2s-rtktest.o

# RTL819X Machine Support
snd-soc-rtl819xD-rt5628-objs := rtl819x_rt5628.o
snd-soc-rtl819xD-rt5642-objs := rtl819x_rt5642.o
snd-soc-rtl819xD-rt5633-objs := rtl819x_rt5633.o
snd-soc-rtl819xD-rt5670-objs := rtl819x_rt5670.o
snd-soc-rtl819xD-rt5651-objs := rtl819x_rt5651.o
snd-soc-rtl819xD-rt5621-objs := rtl819x_rt5621.o

obj-$(CONFIG_SND_RTL819X_SOC_ALC5628) += snd-soc-rtl819x-rt5628.o
obj-$(CONFIG_SND_RTL819X_SOC_ALC5642) += snd-soc-rtl819x-rt5642.o
obj-$(CONFIG_SND_RTL819X_SOC_ALC5633Q) += snd-soc-rtl819x-rt5633.o
obj-$(CONFIG_SND_RTL819X_SOC_ALC5670) += snd-soc-rtl819x-rt5670.o
obj-$(CONFIG_SND_RTL819X_SOC_ALC5651) += snd-soc-rtl819x-rt5651.o
obj-$(CONFIG_SND_RTL819X_SOC_ALC5621) += snd-soc-rtl819x-rt5621.o

# RTL PCM DIAG
snd-soc-rtl8277x-objs := rtl8277x_snd_audio.o
snd-soc-rtl8277x-diag-objs := rtl8277x_snd_audio_diag.o
snd-soc-rtl8277x-pinctrl-objs := rtl8277x_snd_audio_pinctrl.o

obj-$(CONFIG_SND_RTL8277X_SOC_SSP) += snd-soc-rtl8277x.o
obj-$(CONFIG_SND_RTL8277X_SOC_SSP_DIAG) += snd-soc-rtl8277x-diag.o
obj-$(CONFIG_SND_RTL8277X_SOC_SSP_PINCRTL) += snd-soc-rtl8277x-pinctrl.o