/*
 * Copyright (C) 2012 - 2013 Atmel Corporation
 * Bo Shen <voice.shen@atmel.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <i2c.h>
#include <mmc.h>
#include <asm/io.h>
#include <asm/arch/sama5d3_smc.h>
#include <asm/arch/at91_common.h>
#include <asm/arch/at91_pmc.h>
#include <asm/arch/at91_rstc.h>
#include <asm/arch/gpio.h>
#include <asm/arch/clk.h>
#include <lcd.h>
#include <atmel_hlcdc.h>
#include <atmel_mci.h>
#include <phy.h>
#include <micrel.h>
#include <net.h>
#include <netdev.h>
#include <spl.h>
#include <asm/arch/atmel_mpddrc.h>
#include <asm/arch/at91_wdt.h>

#ifdef CONFIG_USB_GADGET_ATMEL_USBA
#include <asm/arch/atmel_usba_udc.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

/* ------------------------------------------------------------------------- */
/*
 * Miscelaneous platform dependent initialisations
 */

/* One-wire information detect */

/*
 * SN layout
 *   31  30         25         20         15         10         5          0
 *   -----------------------------------------------------------------------
 *   |   | Vendor ID| Board ID | Vendor ID| Board ID | Vendor ID| Board ID |
 *   -----------------------------------------------------------------------
 *       |         EK          |         DM          |         CPU         |
 */
#define GPBR_ONEWIRE_INFO_SN	2	/* SN info is in GPBR2 */
#define		BOARD_ID_SAM9x5_DM	1
#define		BOARD_ID_PDA_DM		7
#define		BOARD_ID_SAMA5D3X_DM	9
#define		BOARD_ID_PDA7_DM	15
char get_dm_board_id(void)
{
	return (readl(ATMEL_BASE_GPBR + 4 * GPBR_ONEWIRE_INFO_SN) >> 10) & 0x1f;
}

/*
 * REV layout
 *   31              24     21     18     15         10         5          0
 *   -----------------------------------------------------------------------
 *   |               |  EK  |  DM  |  CPU |    EK    |    DM    |   CPU    |
 *   -----------------------------------------------------------------------
 *                   |     Revision id    |        Revision Code           |
 */
#define GPBR_ONEWIRE_INFO_REV	3	/* REV info is in GPBR3 */
char get_mb_rev_code(void)
{
	char mb_rev_code = 'a';

	mb_rev_code += 0x1f &
		(readl(ATMEL_BASE_GPBR + 4 * GPBR_ONEWIRE_INFO_REV) >> 10);

	return mb_rev_code;
}

#if defined(CONFIG_AT24MAC)
void at91_sync_env_enetaddr(uint8_t *ethaddr, uint8_t adapter)
{
	uint8_t env_ethaddr[6];
	int ret;

	ret = eth_getenv_enetaddr_by_index("eth", adapter, env_ethaddr);

	if (!ret) {
		/* init MAC address from EEPROM */
                debug("### Setting environment from EEPROM MAC address = "
                        "\"%pM\"\n",
                        env_ethaddr);
		if (adapter == 0) {
			ret = !eth_setenv_enetaddr("ethaddr", ethaddr);
		} else if (adapter == 1) {
			ret = !eth_setenv_enetaddr("eth1addr", ethaddr);
		}
        }

	if (!ret) {
                printf("Failed to set mac address from EEPROM: %d\n", ret);
	}
}

static int at91_read_at24mac_oui(uint8_t addr, uint8_t *buf)
{
	if (i2c_read(addr, 0x9a, 1, (uint8_t*)&buf[0], 6)) {
		printf("Read from EEPROM @ 0x%02x failed\n", addr);
		return 0;
	} else {
		/* Check that MAC address is valid. */
		if (!is_valid_ether_addr(buf)) {
			printf("Found invalid MAC address: %d:%d:%d:%d:%d:%d\r\n", 
				buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
			return 0;
		}
	}
	/* found valid MAC! */
	return 1;
}

#endif

int misc_init_r(void)
{
#if defined(CONFIG_AT24MAC)
	int eeprom_mac_read;
	uint8_t env_ethaddr[6];
	uint8_t ethaddr[6];
	int ethaddr_found;

	ethaddr_found = eth_getenv_enetaddr("ethaddr", env_ethaddr);
	eeprom_mac_read = at91_read_at24mac_oui(CONFIG_AT24MAC_ADDR, ethaddr);

	if (!ethaddr_found) {
		if (eeprom_mac_read) {
			/* set MAC address from AT24MAC*/
			at91_sync_env_enetaddr(ethaddr, 0);
		} 
	} else {
		/* MAC address already in environment,
		 * compare with EEPROM and warn on mismatch
		 */
		if (eeprom_mac_read && memcmp(ethaddr, env_ethaddr, 6)) {
			printf("Warning: ethaddr MAC from AT24MAC doesn't match env\r\n");
			printf("Using MAC from environment\r\n");
		}

	}

#if CONFIG_AT24MAC_CNT == 2
	ethaddr_found = eth_getenv_enetaddr("eth1addr", env_ethaddr);
	eeprom_mac_read = at91_read_at24mac_oui(CONFIG_AT24MAC_ADDR1, ethaddr);

	if (!ethaddr_found) {
		if (eeprom_mac_read) {
			/* set MAC address from AT24MAC*/
			at91_sync_env_enetaddr(ethaddr, 1);
		}
	} else {
		/* MAC address already in environment,
		 * compare with EEPROM and warn on mismatch
		 */
		if (eeprom_mac_read && memcmp(ethaddr, env_ethaddr, 6)) {
			printf("Warning: eth1addr MAC from AT24MAC doesn't match env\r\n");
			printf("Using MAC from environment\r\n");
		}

	}
#endif
#endif
	return 0;
}

#ifdef CONFIG_NAND_ATMEL
void alc3_nand_hw_init(void)
{
	struct at91_smc *smc = (struct at91_smc *)ATMEL_BASE_SMC;

	at91_periph_clk_enable(ATMEL_ID_SMC);

	/* Configure SMC CS3 for NAND/SmartMedia */
	writel(AT91_SMC_SETUP_NWE(2) | AT91_SMC_SETUP_NCS_WR(1) |
	       AT91_SMC_SETUP_NRD(2) | AT91_SMC_SETUP_NCS_RD(1),
	       &smc->cs[3].setup);
	writel(AT91_SMC_PULSE_NWE(3) | AT91_SMC_PULSE_NCS_WR(5) |
	       AT91_SMC_PULSE_NRD(3) | AT91_SMC_PULSE_NCS_RD(5),
	       &smc->cs[3].pulse);
	writel(AT91_SMC_CYCLE_NWE(8) | AT91_SMC_CYCLE_NRD(8),
	       &smc->cs[3].cycle);
	writel(AT91_SMC_TIMINGS_TCLR(3) | AT91_SMC_TIMINGS_TADL(10) |
	       AT91_SMC_TIMINGS_TAR(3)  | AT91_SMC_TIMINGS_TRR(4)   |
	       AT91_SMC_TIMINGS_TWB(5)  | AT91_SMC_TIMINGS_RBNSEL(3)|
	       AT91_SMC_TIMINGS_NFSEL(1), &smc->cs[3].timings);
	writel(AT91_SMC_MODE_RM_NRD | AT91_SMC_MODE_WM_NWE |
	       AT91_SMC_MODE_EXNW_DISABLE |
#ifdef CONFIG_SYS_NAND_DBW_16
	       AT91_SMC_MODE_DBW_16 |
#else /* CONFIG_SYS_NAND_DBW_8 */
	       AT91_SMC_MODE_DBW_8 |
#endif
	       AT91_SMC_MODE_TDF_CYCLE(3),
	       &smc->cs[3].mode);
}
#endif

#ifndef CONFIG_SYS_NO_FLASH
static void alc3_nor_hw_init(void)
{
	struct at91_smc *smc = (struct at91_smc *)ATMEL_BASE_SMC;

	at91_periph_clk_enable(ATMEL_ID_SMC);

	/* Configure SMC CS0 for NOR flash */
	writel(AT91_SMC_SETUP_NWE(1) | AT91_SMC_SETUP_NCS_WR(0) |
	       AT91_SMC_SETUP_NRD(2) | AT91_SMC_SETUP_NCS_RD(0),
	       &smc->cs[0].setup);
	writel(AT91_SMC_PULSE_NWE(10) | AT91_SMC_PULSE_NCS_WR(11) |
	       AT91_SMC_PULSE_NRD(10) | AT91_SMC_PULSE_NCS_RD(11),
	       &smc->cs[0].pulse);
	writel(AT91_SMC_CYCLE_NWE(11) | AT91_SMC_CYCLE_NRD(14),
	       &smc->cs[0].cycle);
	writel(AT91_SMC_TIMINGS_TCLR(0) | AT91_SMC_TIMINGS_TADL(0)  |
	       AT91_SMC_TIMINGS_TAR(0)  | AT91_SMC_TIMINGS_TRR(0)   |
	       AT91_SMC_TIMINGS_TWB(0)  | AT91_SMC_TIMINGS_RBNSEL(0)|
	       AT91_SMC_TIMINGS_NFSEL(0), &smc->cs[0].timings);
	writel(AT91_SMC_MODE_RM_NRD | AT91_SMC_MODE_WM_NWE |
	       AT91_SMC_MODE_EXNW_DISABLE |
	       AT91_SMC_MODE_DBW_16 |
	       AT91_SMC_MODE_TDF_CYCLE(1),
	       &smc->cs[0].mode);

	/* Address pin (A1 ~ A23) configuration */
	at91_set_a_periph(AT91_PIO_PORTE, 1, 0);
	at91_set_a_periph(AT91_PIO_PORTE, 2, 0);
	at91_set_a_periph(AT91_PIO_PORTE, 3, 0);
	at91_set_a_periph(AT91_PIO_PORTE, 4, 0);
	at91_set_a_periph(AT91_PIO_PORTE, 5, 0);
	at91_set_a_periph(AT91_PIO_PORTE, 6, 0);
	at91_set_a_periph(AT91_PIO_PORTE, 7, 0);
	at91_set_a_periph(AT91_PIO_PORTE, 8, 0);
	at91_set_a_periph(AT91_PIO_PORTE, 9, 0);
	at91_set_a_periph(AT91_PIO_PORTE, 10, 0);
	at91_set_a_periph(AT91_PIO_PORTE, 11, 0);
	at91_set_a_periph(AT91_PIO_PORTE, 12, 0);
	at91_set_a_periph(AT91_PIO_PORTE, 13, 0);
	at91_set_a_periph(AT91_PIO_PORTE, 14, 0);
	at91_set_a_periph(AT91_PIO_PORTE, 15, 0);
	at91_set_a_periph(AT91_PIO_PORTE, 16, 0);
	at91_set_a_periph(AT91_PIO_PORTE, 17, 0);
	at91_set_a_periph(AT91_PIO_PORTE, 18, 0);
	at91_set_a_periph(AT91_PIO_PORTE, 19, 0);
	at91_set_a_periph(AT91_PIO_PORTE, 20, 0);
	at91_set_a_periph(AT91_PIO_PORTE, 21, 0);
	at91_set_a_periph(AT91_PIO_PORTE, 22, 0);
	at91_set_a_periph(AT91_PIO_PORTE, 23, 0);
	/* CS0 pin configuration */
	at91_set_a_periph(AT91_PIO_PORTE, 26, 0);
}
#endif

#ifdef CONFIG_CMD_USB
static void alc3_usb_hw_init(void)
{
/*
	at91_set_pio_output(AT91_PIO_PORTD, 25, 0);
	at91_set_pio_output(AT91_PIO_PORTD, 26, 0);
	at91_set_pio_output(AT91_PIO_PORTD, 27, 0);
*/
}
#endif

#ifdef CONFIG_GENERIC_ATMEL_MCI
static void alc3_mci_hw_init(void)
{
	at91_mci_hw_init();
}
#endif
#if 0
#ifdef CONFIG_LCD
vidinfo_t panel_info = {
	.vl_col = 800,
	.vl_row = 480,
	.vl_clk = 24000000,
	.vl_bpix = LCD_BPP,
	.vl_bpox = LCD_OUTPUT_BPP,
	.vl_tft = 1,
	.vl_hsync_len = 128,
	.vl_left_margin = 64,
	.vl_right_margin = 64,
	.vl_vsync_len = 2,
	.vl_upper_margin = 22,
	.vl_lower_margin = 21,
	.mmio = ATMEL_BASE_LCDC,
};

void lcd_enable(void)
{
}

void lcd_disable(void)
{
}

static void sama5d3xek_lcd_hw_init(void)
{
	/* if LCD is PDA7 module, then set output bpp to 18 bit */
	char dm_id = get_dm_board_id();
	if (dm_id == BOARD_ID_PDA7_DM)
		panel_info.vl_bpox = 18;

	gd->fb_base = CONFIG_SAMA5D3_LCD_BASE;

	/* The higher 8 bit of LCD is board related */
	at91_set_c_periph(AT91_PIO_PORTC, 14, 0);	/* LCDD16 */
	at91_set_c_periph(AT91_PIO_PORTC, 13, 0);	/* LCDD17 */
	at91_set_c_periph(AT91_PIO_PORTC, 12, 0);	/* LCDD18 */
	at91_set_c_periph(AT91_PIO_PORTC, 11, 0);	/* LCDD19 */
	at91_set_c_periph(AT91_PIO_PORTC, 10, 0);	/* LCDD20 */
	at91_set_c_periph(AT91_PIO_PORTC, 15, 0);	/* LCDD21 */
	at91_set_c_periph(AT91_PIO_PORTE, 27, 0);	/* LCDD22 */
	at91_set_c_periph(AT91_PIO_PORTE, 28, 0);	/* LCDD23 */

	/* Configure lower 16 bit of LCD and enable clock */
	at91_lcd_hw_init();
}

#ifdef CONFIG_LCD_INFO
#include <nand.h>
#include <version.h>

void lcd_show_board_info(void)
{
	ulong dram_size;
	uint64_t nand_size;
	int i;
	char temp[32];

	lcd_printf("%s\n", U_BOOT_VERSION);
	lcd_printf("(C) 2013 ATMEL Corp\n");
	lcd_printf("at91@atmel.com\n");
	lcd_printf("%s CPU at %s MHz\n", get_cpu_name(),
		   strmhz(temp, get_cpu_clk_rate()));

	dram_size = 0;
	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++)
		dram_size += gd->bd->bi_dram[i].size;

	nand_size = 0;
#ifdef CONFIG_NAND_ATMEL
	for (i = 0; i < CONFIG_SYS_MAX_NAND_DEVICE; i++)
		nand_size += nand_info[i].size;
#endif
	lcd_printf("%ld MB SDRAM, %lld MB NAND\n",
		   dram_size >> 20, nand_size >> 20);
}
#endif /* CONFIG_LCD_INFO */
#endif /* CONFIG_LCD */
#endif
int board_early_init_f(void)
{
	at91_periph_clk_enable(ATMEL_ID_PIOA);
	at91_periph_clk_enable(ATMEL_ID_PIOB);
	at91_periph_clk_enable(ATMEL_ID_PIOC);
	at91_periph_clk_enable(ATMEL_ID_PIOD);
	at91_periph_clk_enable(ATMEL_ID_PIOE);

	at91_seriald_hw_init();

	return 0;
}

int board_init(void)
{
	/* adress of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;

#ifdef CONFIG_NAND_ATMEL
	alc3_nand_hw_init();
#endif
#if 0
#ifndef CONFIG_SYS_NO_FLASH
	alc3_nor_hw_init();
#endif
#endif
#ifdef CONFIG_CMD_USB
	alc3_usb_hw_init();
#endif
#ifdef CONFIG_USB_GADGET_ATMEL_USBA
	at91_udp_hw_init();
#endif
#ifdef CONFIG_GENERIC_ATMEL_MCI
	alc3_mci_hw_init();
#endif
#ifdef CONFIG_ATMEL_SPI
	at91_spi0_hw_init(1 << 0);
	at91_spi1_hw_init(1 << 0);
#endif
#ifdef CONFIG_MACB
	if (has_emac())
		at91_macb_hw_init();
	if (has_gmac())
		at91_gmac_hw_init();
#endif
#if 0
#ifdef CONFIG_LCD
	if (has_lcdc())
		sama5d3xek_lcd_hw_init();
#endif
#endif
	return 0;
}

int dram_init(void)
{
	int ddr_settings;

	ddr_settings = at91_get_pio_value(AT91_PIO_PORTC, 10);
	ddr_settings |= at91_get_pio_value(AT91_PIO_PORTC, 16) << 1;
	if (ddr_settings == 0)
	    gd->ram_size = get_ram_size((void *)CONFIG_SYS_SDRAM_BASE,
				        CONFIG_SYS_SDRAM_64M_SIZE);
	else if (ddr_settings == 1)
	    gd->ram_size = get_ram_size((void *)CONFIG_SYS_SDRAM_BASE,
				        CONFIG_SYS_SDRAM_128M_SIZE);
	else if (ddr_settings == 2)
	    gd->ram_size = get_ram_size((void *)CONFIG_SYS_SDRAM_BASE,
				        CONFIG_SYS_SDRAM_256M_SIZE);		
	/* config not defined: DDR size mem set to 128MB */	
	else if (ddr_settings == 3)
	    gd->ram_size = get_ram_size((void *)CONFIG_SYS_SDRAM_BASE,
				        CONFIG_SYS_SDRAM_128M_SIZE);	
	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
	/* board specific timings for GMAC */
	if (has_gmac()) {
#if 0
		/* rx data delay */
		ksz9021_phy_extended_write(phydev,
					   MII_KSZ9021_EXT_RGMII_RX_DATA_SKEW,
					   0x2222);
		/* tx data delay */
		ksz9021_phy_extended_write(phydev,
					   MII_KSZ9021_EXT_RGMII_TX_DATA_SKEW,
					   0x2222);
#endif
		/* rx/tx clock delay */
		ksz9021_phy_extended_write(phydev,
					   MII_KSZ9021_EXT_RGMII_CLOCK_SKEW,
					   0xf0f0);
//					   0xf2f4);
	}

	/* always run the PHY's config routine */
	if (phydev->drv->config)
		return phydev->drv->config(phydev);

	return 0;
}

int board_eth_init(bd_t *bis)
{
	int rc = 0;

#ifdef CONFIG_MACB
	if (has_emac())
		rc = macb_eth_initialize(0, (void *)ATMEL_BASE_EMAC, 0x00);
	if (has_gmac())
		rc = macb_eth_initialize(0, (void *)ATMEL_BASE_GMAC, 0x01);
#endif
#ifdef CONFIG_USB_GADGET_ATMEL_USBA
	usba_udc_probe(&pdata);
#ifdef CONFIG_USB_ETH_RNDIS
	usb_eth_initialize(bis);
#endif
#endif

	return rc;
}

#ifdef CONFIG_GENERIC_ATMEL_MCI
int board_mmc_init(bd_t *bis)
{
	int rc = 0;

	rc = atmel_mci_init((void *)ATMEL_BASE_MCI0);

	return rc;
}
#endif

/* SPI chip select control */
#ifdef CONFIG_ATMEL_SPI
#include <spi.h>

int spi_cs_is_valid(unsigned int bus, unsigned int cs)
{
	return ((bus == 0 || bus == 1) && cs < 4);
}

void spi_cs_activate(struct spi_slave *slave)
{
	if (slave->bus == 0) {
	 switch (slave->cs) {
		case 0:
			at91_set_pio_output(AT91_PIO_PORTD, 13, 0);
		case 1:
			at91_set_pio_output(AT91_PIO_PORTD, 14, 0);
		case 2:
			at91_set_pio_output(AT91_PIO_PORTD, 15, 0);
		case 3:
			at91_set_pio_output(AT91_PIO_PORTD, 16, 0);
		default:
			break;
	 }
	} else {
		switch (slave->cs) {
		case 0:
			at91_set_pio_output(AT91_PIO_PORTC, 25, 0);
		default:
			break;
		}
	}
}
void spi_cs_deactivate(struct spi_slave *slave)
{
	if (slave->bus == 0) {
		switch (slave->cs) {
		case 0:
			at91_set_pio_output(AT91_PIO_PORTD, 13, 1);
		case 1:
			at91_set_pio_output(AT91_PIO_PORTD, 14, 1);
		case 2:
			at91_set_pio_output(AT91_PIO_PORTD, 15, 1);
		case 3:
			at91_set_pio_output(AT91_PIO_PORTD, 16, 1);
		default:
			break;

		}
	} else {
		switch (slave->cs) {
		case 0:
			at91_set_pio_output(AT91_PIO_PORTC, 25, 1);
		default:
			break;
		}
	}
}
#endif /* CONFIG_ATMEL_SPI */

#ifdef CONFIG_BOARD_LATE_INIT
#include <linux/ctype.h>
int board_late_init(void)
{
#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	const char *SAMA5D3_BOARD_EK_NAME = "ek_name";
	const char *SAMA5D3_MB_REV_ENV_NAME = "mb_rev";
	const char *SAMA5D3_DM_TYPE_ENV_NAME = "dm_type";
	char rev_code[2], dm_id;
	char name[32], *p;

	strcpy(name, get_cpu_name());
	for (p = name; *p != '\0'; *p = tolower(*p), p++);
	strcat(name, "ek");
	setenv(SAMA5D3_BOARD_EK_NAME, name);

	rev_code[0] = get_mb_rev_code();
	rev_code[1] = '\0';
	setenv(SAMA5D3_MB_REV_ENV_NAME, rev_code);

	dm_id = get_dm_board_id();
	switch (dm_id) {
	case BOARD_ID_PDA_DM:
		setenv(SAMA5D3_DM_TYPE_ENV_NAME, "pda4");
		break;
	case BOARD_ID_PDA7_DM:
		setenv(SAMA5D3_DM_TYPE_ENV_NAME, "pda7");
		break;
	case BOARD_ID_SAM9x5_DM:
	case BOARD_ID_SAMA5D3X_DM:
	default:
		setenv(SAMA5D3_DM_TYPE_ENV_NAME, "");
		break;
	}
#endif

	return 0;
}
#endif

/* SPL */
#ifdef CONFIG_SPL_BUILD
void spl_board_init(void)
{
#ifdef CONFIG_SYS_USE_MMC
	alc3_mci_hw_init();
#elif CONFIG_SYS_USE_NANDFLASH
	alc3_nand_hw_init();
#elif CONFIG_SYS_USE_SERIALFLASH
	at91_spi0_hw_init(1 << 0);
#endif
}

static void ddr2_conf(struct atmel_mpddrc_config *ddr2)
{
	ddr2->md = (ATMEL_MPDDRC_MD_DBW_32_BITS | ATMEL_MPDDRC_MD_DDR2_SDRAM);

	ddr2->cr = (ATMEL_MPDDRC_CR_NC_COL_10 |
//		    ATMEL_MPDDRC_CR_NR_ROW_14 |
		    ATMEL_MPDDRC_CR_NR_ROW_13 |
		    ATMEL_MPDDRC_CR_CAS_DDR_CAS3 |
		    ATMEL_MPDDRC_CR_ENRDM_ON |
//		    ATMEL_MPDDRC_CR_NB_8BANKS |
		    ATMEL_MPDDRC_CR_NDQS_DISABLED |
		    ATMEL_MPDDRC_CR_DECOD_INTERLEAVED |
		    ATMEL_MPDDRC_CR_UNAL_SUPPORTED);
	/*
	 * As the DDR2-SDRAm device requires a refresh time is 7.8125us
	 * when DDR run at 133MHz, so it needs (7.8125us * 133MHz / 10^9) clocks
	 */
	ddr2->rtr = 0x411;

	ddr2->tpr0 = (6 << ATMEL_MPDDRC_TPR0_TRAS_OFFSET |
		      2 << ATMEL_MPDDRC_TPR0_TRCD_OFFSET |
		      2 << ATMEL_MPDDRC_TPR0_TWR_OFFSET |
		      8 << ATMEL_MPDDRC_TPR0_TRC_OFFSET |
		      2 << ATMEL_MPDDRC_TPR0_TRP_OFFSET |
		      2 << ATMEL_MPDDRC_TPR0_TRRD_OFFSET |
		      2 << ATMEL_MPDDRC_TPR0_TWTR_OFFSET |
		      2 << ATMEL_MPDDRC_TPR0_TMRD_OFFSET);

	ddr2->tpr1 = (2 << ATMEL_MPDDRC_TPR1_TXP_OFFSET |
		      200 << ATMEL_MPDDRC_TPR1_TXSRD_OFFSET |
		      28 << ATMEL_MPDDRC_TPR1_TXSNR_OFFSET |
		      26 << ATMEL_MPDDRC_TPR1_TRFC_OFFSET);

	ddr2->tpr2 = (7 << ATMEL_MPDDRC_TPR2_TFAW_OFFSET |
		      2 << ATMEL_MPDDRC_TPR2_TRTP_OFFSET |
		      2 << ATMEL_MPDDRC_TPR2_TRPA_OFFSET |
		      7 << ATMEL_MPDDRC_TPR2_TXARDS_OFFSET |
		      8 << ATMEL_MPDDRC_TPR2_TXARD_OFFSET);
}

void mem_init(void)
{
	struct atmel_mpddrc_config ddr2;

	ddr2_conf(&ddr2);

	/* enable MPDDR clock */
	at91_periph_clk_enable(ATMEL_ID_MPDDRC);
	at91_system_clk_enable(AT91_PMC_SYS_CLK_DDRCK);

	/* DDRAM2 Controller initialize */
	ddr2_init(ATMEL_BASE_DDRCS, &ddr2);
}

void at91_pmc_init(void)
{
	struct at91_pmc *pmc = (struct at91_pmc *)ATMEL_BASE_PMC;
	u32 tmp;

	tmp = AT91_PMC_PLLAR_29 |
	      AT91_PMC_PLLXR_PLLCOUNT(0x3f) |
	      AT91_PMC_PLLXR_MUL(43) |
	      AT91_PMC_PLLXR_DIV(1);
	at91_plla_init(tmp);

	writel(0x3 << 8, &pmc->pllicpr);

	tmp = AT91_PMC_MCKR_MDIV_4 |
	      AT91_PMC_MCKR_CSS_PLLA;
	at91_mck_init(tmp);
}
#endif