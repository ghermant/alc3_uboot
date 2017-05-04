/*
 * Configuation settings for the ALC3 board.
 *
 * Copyright (C) 2016 Calao-systems
 *
 * based on at91sam9m10g45ek.h by:
 * Stelian Pop <stelian@popies.net>
 * Lead Tech Design <www.leadtechdesign.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/*
 * If has No NOR flash, please put the definition: CONFIG_SYS_NO_FLASH
 * before the common header.
 */
#define CONFIG_SYS_NO_FLASH
#include "at91-sama5_common.h"

/*
#define CONFIG_BOARD_LATE_INIT 
#define CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
*/
#define CONFIG_FIT

/* serial console */
#define CONFIG_ATMEL_USART
#define CONFIG_USART_BASE		ATMEL_BASE_DBGU
#define	CONFIG_USART_ID			ATMEL_ID_DBGU

/* SDRAM 128MB */
#define CONFIG_NR_DRAM_BANKS		1
#define CONFIG_SYS_SDRAM_BASE           ATMEL_BASE_DDRCS
#define CONFIG_SYS_SDRAM_64M_SIZE	0x4000000
#define CONFIG_SYS_SDRAM_128M_SIZE	0x8000000
#define CONFIG_SYS_SDRAM_256M_SIZE	0x10000000	

#ifdef CONFIG_SPL_BUILD
#define CONFIG_SYS_INIT_SP_ADDR		0x310000
#else
#define CONFIG_SYS_INIT_SP_ADDR \
	(CONFIG_SYS_SDRAM_BASE + 4 * 1024 - GENERATED_GBL_DATA_SIZE)
#endif

/* SerialFlash */
#define CONFIG_CMD_SF

#ifdef CONFIG_CMD_SF
#define CONFIG_ATMEL_SPI
#define CONFIG_SPI_FLASH
#define CONFIG_SPI_FLASH_ATMEL
#define CONFIG_SF_DEFAULT_SPEED		30000000
#endif

/* NAND flash */
#define CONFIG_CMD_NAND

#ifdef CONFIG_CMD_NAND
#define CONFIG_NAND_ATMEL
#define CONFIG_SYS_MAX_NAND_DEVICE	1
#define CONFIG_SYS_NAND_BASE		ATMEL_BASE_CS3
/* our ALE is AD21 */
#define CONFIG_SYS_NAND_MASK_ALE	(1 << 21)
/* our CLE is AD22 */
#define CONFIG_SYS_NAND_MASK_CLE	(1 << 22)
#define CONFIG_SYS_NAND_ONFI_DETECTION
/* PMECC & PMERRLOC */
#define CONFIG_ATMEL_NAND_HWECC
#define CONFIG_ATMEL_NAND_HW_PMECC
#define CONFIG_PMECC_CAP		4
#define CONFIG_PMECC_SECTOR_SIZE	512
#define CONFIG_CMD_NAND_TRIMFFS
#endif

/* Ethernet Hardware */
#define CONFIG_MACB
#define CONFIG_RMII
#define CONFIG_NET_RETRY_COUNT		20
#define CONFIG_MACB_SEARCH_PHY
#define CONFIG_RGMII
#define CONFIG_CMD_MII
#define CONFIG_PHYLIB
#define CONFIG_PHY_MICREL
#define CONFIG_PHY_MICREL_KSZ9021

/* AT24MAC MAC Address EEPROM */
#define CONFIG_MISC_INIT_R
#define CONFIG_CMD_I2C
#define CONFIG_SYS_I2C
#define CONFIG_SYS_I2C_SOFT
#define CONFIG_SYS_I2C_SOFT_SPEED       50000
#define CONFIG_SOFT_I2C_GPIO_SCL        GPIO_PIN_PC(27)
#define CONFIG_SOFT_I2C_GPIO_SDA        GPIO_PIN_PC(26)
#define CONFIG_AT24MAC
#define CONFIG_AT24MAC_CNT		2
#define CONFIG_AT24MAC_ADDR		0x58
#define CONFIG_AT24MAC_ADDR1		0x59

/* MMC */
#define CONFIG_CMD_MMC

#ifdef CONFIG_CMD_MMC
#define CONFIG_MMC
#define CONFIG_GENERIC_MMC
#define CONFIG_GENERIC_ATMEL_MCI
#define ATMEL_BASE_MMCI			ATMEL_BASE_MCI0
#define CONFIG_ATMEL_MCI_8BIT
#endif

/* USB */
#ifdef CONFIG_CMD_USB
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_ATMEL
#define CONFIG_SYS_USB_EHCI_MAX_ROOT_PORTS	3
#define CONFIG_DOS_PARTITION
#define CONFIG_USB_STORAGE
#endif

/* USB device */
#undef CONFIG_USB_GADGET

#ifdef CONFIG_USB_GADGET
#define CONFIG_USB_GADGET_DUALSPEED
#define CONFIG_USB_GADGET_ATMEL_USBA
#define CONFIG_USB_ETHER
#define CONFIG_USB_ETH_RNDIS
#define CONFIG_USBNET_MANUFACTURER      "CALAO ALC3"
#endif

#if defined(CONFIG_CMD_USB) || defined(CONFIG_CMD_MMC)
#define CONFIG_CMD_FAT
#define CONFIG_FAT_WRITE
#endif

#define CONFIG_SYS_LOAD_ADDR			0x22000000 /* load address */

#define CONFIG_EXTRA_ENV_SETTINGS \
	"set_mb_name="						\
		"if test $mb_rev = 'c'; then "			\
			"setenv mb_name $ek_name'_rev'$mb_rev; "\
		"else "						\
			"setenv mb_name $ek_name; "		\
		"fi; \0"					\
	"set_video_mode="					\
		"if test $dm_type = undefined; then "		\
			"setenv video_mode video=LVDS-1:800x480-16; "\
		"else "						\
			"if test $dm_type = pda4; then "	\
				"setenv video_mode video=LVDS-1:480x272-16; "\
			"fi; "					\
		"fi; "						\
		"if test $ek_name = sama5d35ek; then "		\
			"setenv video_mode; "			\
		"fi; \0"					\
	"findfdt="						\
		"run set_mb_name; "				\
		"run set_video_mode; "				\
		"if test $dm_type = undefined; then "		\
			"setenv conf_name $mb_name; "		\
		"else "						\
			"if test $ek_name = sama5d35ek; then "	\
				"setenv conf_name $mb_name; "	\
			"else "					\
				"setenv conf_name $mb_name'_'$dm_type; "\
			"fi; "					\
		"fi; "						\
		"setenv fdtfile $conf_name'.dtb'; \0"

#ifdef CONFIG_SYS_USE_SERIALFLASH
/* override the bootcmd, bootargs and other configuration for spi flash env*/
/* bootstrap + u-boot + env + linux in serial flash */
#undef	CONFIG_BOOTCOMMAND
#define CONFIG_BOOTCOMMAND	"run findfdt; " \
				"nand read 0x21000000 0x180000 0x80000; " \
				"nand read 0x22000000 0x200000 0x600000; " \
				"bootz 0x22000000 - 0x21000000"
#elif CONFIG_SYS_USE_NANDFLASH
/* override the bootcmd, bootargs and other configuration nandflash env */
/* bootstrap + u-boot + env in nandflash */
#undef	CONFIG_BOOTCOMMAND
#define CONFIG_BOOTCOMMAND	"run findfdt; " \
				"nand read 0x22000000 0x200000 0x600000; " \
				"setenv bootargs $bootargs $video_mode;" \
				"bootm 0x22000000#conf@$conf_name"
#elif CONFIG_SYS_USE_MMC
/* override the bootcmd, bootargs and other configuration for sd/mmc env */

/* bootstrap + u-boot + env in sd card */
#undef	CONFIG_BOOTCOMMAND
#define CONFIG_BOOTCOMMAND	"run findfdt; " \
				"fatload mmc 0:1 0x22000000 sama5d3xek.itb; " \
				"setenv bootargs $bootargs $video_mode;" \
				"bootm 0x22000000#conf@$conf_name"
#else
#define CONFIG_ENV_IS_NOWHERE
#endif

/* SPL */
#define CONFIG_SPL_FRAMEWORK
#define CONFIG_SPL_TEXT_BASE		0x300000
#define CONFIG_SPL_MAX_SIZE		0x10000
#define CONFIG_SPL_BSS_START_ADDR	0x20000000
#define CONFIG_SPL_BSS_MAX_SIZE		0x80000
#define CONFIG_SYS_SPL_MALLOC_START	0x20080000
#define CONFIG_SYS_SPL_MALLOC_SIZE	0x80000

#define CONFIG_SPL_LIBCOMMON_SUPPORT
#define CONFIG_SPL_LIBGENERIC_SUPPORT
#define CONFIG_SPL_GPIO_SUPPORT
#define CONFIG_SPL_SERIAL_SUPPORT

#define CONFIG_SPL_BOARD_INIT
#define CONFIG_SYS_MONITOR_LEN		(512 << 10)

#ifdef CONFIG_SYS_USE_MMC
#define CONFIG_SPL_LDSCRIPT		arch/arm/cpu/at91-common/u-boot-spl.lds
#define CONFIG_SPL_MMC_SUPPORT
#define CONFIG_SYS_U_BOOT_MAX_SIZE_SECTORS	0x400
#define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR 0x200
#define CONFIG_SYS_MMCSD_FS_BOOT_PARTITION	1
#define CONFIG_SPL_FS_LOAD_PAYLOAD_NAME	"u-boot.img"
#define CONFIG_SPL_FAT_SUPPORT
#define CONFIG_SPL_LIBDISK_SUPPORT

#elif CONFIG_SYS_USE_NANDFLASH
#define CONFIG_SPL_NAND_SUPPORT
#define CONFIG_SPL_NAND_DRIVERS
#define CONFIG_SPL_NAND_BASE
#define CONFIG_SYS_NAND_U_BOOT_OFFS	0x40000
#define CONFIG_SYS_NAND_5_ADDR_CYCLE
#define CONFIG_SYS_NAND_PAGE_SIZE	0x800
#define CONFIG_SYS_NAND_PAGE_COUNT	64
#define CONFIG_SYS_NAND_OOBSIZE		64
#define CONFIG_SYS_NAND_BLOCK_SIZE	0x20000
#define CONFIG_SYS_NAND_BAD_BLOCK_POS	0x0
#define CONFIG_SPL_GENERATE_ATMEL_PMECC_HEADER

#elif CONFIG_SYS_USE_SERIALFLASH
#define CONFIG_SPL_SPI_SUPPORT
#define CONFIG_SPL_SPI_FLASH_SUPPORT
#define CONFIG_SPL_SPI_LOAD
#define CONFIG_SYS_SPI_U_BOOT_OFFS	0x8000
#define CONFIG_SPI_FLASH_SST

#endif

#endif
