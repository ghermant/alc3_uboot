#
# (C) Copyright 2000-2008
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# (C) Copyright 2013 Atmel Corporation
#		     Bo Shen <voice.shen@atmel.com>
#
# SPDX-License-Identifier:	GPL-2.0+
#

obj-y += clock.o
obj-$(CONFIG_AT91_WANTS_COMMON_PHY) += phy.o
ifneq ($(CONFIG_SPL_BUILD),)
obj-$(CONFIG_AT91SAM9G20) += sdram.o spl_at91.o
obj-$(CONFIG_AT91SAM9M10G45) += mpddrc.o spl_at91.o
obj-$(CONFIG_SAMA5D3) += mpddrc.o spl_atmel.o
obj-$(CONFIG_SAMA5D4) += mpddrc.o spl_atmel.o matrix.o atmel_sfr.o
obj-$(CONFIG_SAMA5D2) += mpddrc.o spl_atmel.o matrix.o atmel_sfr.o
obj-y += spl.o
endif
