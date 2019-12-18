// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2017, Maxim Integrated

#include <linux/acpi.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/soundwire/sdw.h>
#include <linux/soundwire/sdw_type.h>
#include <sound/soc.h>
#include <linux/of.h>

#include "max98373.h"

static struct reg_default max98373_reg[] = {
	{MAX98373_R2000_SW_RESET, 0x00},
	{MAX98373_R2001_INT_RAW1, 0x00},
	{MAX98373_R2002_INT_RAW2, 0x00},
	{MAX98373_R2003_INT_RAW3, 0x00},
	{MAX98373_R2004_INT_STATE1, 0x00},
	{MAX98373_R2005_INT_STATE2, 0x00},
	{MAX98373_R2006_INT_STATE3, 0x00},
	{MAX98373_R2007_INT_FLAG1, 0x00},
	{MAX98373_R2008_INT_FLAG2, 0x00},
	{MAX98373_R2009_INT_FLAG3, 0x00},
	{MAX98373_R200A_INT_EN1, 0x00},
	{MAX98373_R200B_INT_EN2, 0x00},
	{MAX98373_R200C_INT_EN3, 0x00},
	{MAX98373_R200D_INT_FLAG_CLR1, 0x00},
	{MAX98373_R200E_INT_FLAG_CLR2, 0x00},
	{MAX98373_R200F_INT_FLAG_CLR3, 0x00},
	{MAX98373_R2010_IRQ_CTRL, 0x00},
	{MAX98373_R2014_THERM_WARN_THRESH, 0x10},
	{MAX98373_R2015_THERM_SHDN_THRESH, 0x27},
	{MAX98373_R2016_THERM_HYSTERESIS, 0x01},
	{MAX98373_R2017_THERM_FOLDBACK_SET, 0xC0},
	{MAX98373_R2018_THERM_FOLDBACK_EN, 0x00},
	{MAX98373_R201E_PIN_DRIVE_STRENGTH, 0x55},
	{MAX98373_R2020_PCM_TX_HIZ_EN_1, 0xFE},
	{MAX98373_R2021_PCM_TX_HIZ_EN_2, 0xFF},
	{MAX98373_R2022_PCM_TX_SRC_1, 0x00},
	{MAX98373_R2023_PCM_TX_SRC_2, 0x00},
	{MAX98373_R2024_PCM_DATA_FMT_CFG, 0xC0},
	{MAX98373_R2025_AUDIO_IF_MODE, 0x00},
	{MAX98373_R2026_PCM_CLOCK_RATIO, 0x04},
	{MAX98373_R2027_PCM_SR_SETUP_1, 0x08},
	{MAX98373_R2028_PCM_SR_SETUP_2, 0x88},
	{MAX98373_R2029_PCM_TO_SPK_MONO_MIX_1, 0x00},
	{MAX98373_R202A_PCM_TO_SPK_MONO_MIX_2, 0x00},
	{MAX98373_R202B_PCM_RX_EN, 0x00},
	{MAX98373_R202C_PCM_TX_EN, 0x00},
	{MAX98373_R202E_ICC_RX_CH_EN_1, 0x00},
	{MAX98373_R202F_ICC_RX_CH_EN_2, 0x00},
	{MAX98373_R2030_ICC_TX_HIZ_EN_1, 0xFF},
	{MAX98373_R2031_ICC_TX_HIZ_EN_2, 0xFF},
	{MAX98373_R2032_ICC_LINK_EN_CFG, 0x30},
	{MAX98373_R2034_ICC_TX_CNTL, 0x00},
	{MAX98373_R2035_ICC_TX_EN, 0x00},
	{MAX98373_R2036_SOUNDWIRE_CTRL, 0x05},
	{MAX98373_R203D_AMP_DIG_VOL_CTRL, 0x00},
	{MAX98373_R203E_AMP_PATH_GAIN, 0x08},
	{MAX98373_R203F_AMP_DSP_CFG, 0x02},
	{MAX98373_R2040_TONE_GEN_CFG, 0x00},
	{MAX98373_R2041_AMP_CFG, 0x03},
	{MAX98373_R2042_AMP_EDGE_RATE_CFG, 0x00},
	{MAX98373_R2043_AMP_EN, 0x00},
	{MAX98373_R2046_IV_SENSE_ADC_DSP_CFG, 0x04},
	{MAX98373_R2047_IV_SENSE_ADC_EN, 0x00},
	{MAX98373_R2051_MEAS_ADC_SAMPLING_RATE, 0x00},
	{MAX98373_R2052_MEAS_ADC_PVDD_FLT_CFG, 0x00},
	{MAX98373_R2053_MEAS_ADC_THERM_FLT_CFG, 0x00},
	{MAX98373_R2054_MEAS_ADC_PVDD_CH_READBACK, 0x00},
	{MAX98373_R2055_MEAS_ADC_THERM_CH_READBACK, 0x00},
	{MAX98373_R2056_MEAS_ADC_PVDD_CH_EN, 0x00},
	{MAX98373_R2090_BDE_LVL_HOLD, 0x00},
	{MAX98373_R2091_BDE_GAIN_ATK_REL_RATE, 0x00},
	{MAX98373_R2092_BDE_CLIPPER_MODE, 0x00},
	{MAX98373_R2097_BDE_L1_THRESH, 0x00},
	{MAX98373_R2098_BDE_L2_THRESH, 0x00},
	{MAX98373_R2099_BDE_L3_THRESH, 0x00},
	{MAX98373_R209A_BDE_L4_THRESH, 0x00},
	{MAX98373_R209B_BDE_THRESH_HYST, 0x00},
	{MAX98373_R20A8_BDE_L1_CFG_1, 0x00},
	{MAX98373_R20A9_BDE_L1_CFG_2, 0x00},
	{MAX98373_R20AA_BDE_L1_CFG_3, 0x00},
	{MAX98373_R20AB_BDE_L2_CFG_1, 0x00},
	{MAX98373_R20AC_BDE_L2_CFG_2, 0x00},
	{MAX98373_R20AD_BDE_L2_CFG_3, 0x00},
	{MAX98373_R20AE_BDE_L3_CFG_1, 0x00},
	{MAX98373_R20AF_BDE_L3_CFG_2, 0x00},
	{MAX98373_R20B0_BDE_L3_CFG_3, 0x00},
	{MAX98373_R20B1_BDE_L4_CFG_1, 0x00},
	{MAX98373_R20B2_BDE_L4_CFG_2, 0x00},
	{MAX98373_R20B3_BDE_L4_CFG_3, 0x00},
	{MAX98373_R20B4_BDE_INFINITE_HOLD_RELEASE, 0x00},
	{MAX98373_R20B5_BDE_EN, 0x00},
	{MAX98373_R20B6_BDE_CUR_STATE_READBACK, 0x00},
	{MAX98373_R20D1_DHT_CFG, 0x01},
	{MAX98373_R20D2_DHT_ATTACK_CFG, 0x02},
	{MAX98373_R20D3_DHT_RELEASE_CFG, 0x03},
	{MAX98373_R20D4_DHT_EN, 0x00},
	{MAX98373_R20E0_LIMITER_THRESH_CFG, 0x00},
	{MAX98373_R20E1_LIMITER_ATK_REL_RATES, 0x00},
	{MAX98373_R20E2_LIMITER_EN, 0x00},
	{MAX98373_R20FE_DEVICE_AUTO_RESTART_CFG, 0x00},
	{MAX98373_R20FF_GLOBAL_SHDN, 0x00},
	{MAX98373_R21FF_REV_ID, 0x42},
};

static bool max98373_readable_register(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case 0x0040 ... 0x0070:			/* Soundwire Slave Control Port Registers	*/
	case 0x0100 ... 0x0137:			/* Soundwire Data Port 1 Registers	*/
	case 0x0300 ... 0x0337:			/* Soundwire Data Port 3 Registers	*/

	case MAX98373_R2000_SW_RESET:
	case MAX98373_R2001_INT_RAW1 ... MAX98373_R200C_INT_EN3:
	case MAX98373_R2010_IRQ_CTRL:
	case MAX98373_R2014_THERM_WARN_THRESH
		... MAX98373_R2018_THERM_FOLDBACK_EN:
	case MAX98373_R201E_PIN_DRIVE_STRENGTH
		... MAX98373_R2036_SOUNDWIRE_CTRL:
	case MAX98373_R203D_AMP_DIG_VOL_CTRL ... MAX98373_R2043_AMP_EN:
	case MAX98373_R2046_IV_SENSE_ADC_DSP_CFG
		... MAX98373_R2047_IV_SENSE_ADC_EN:
	case MAX98373_R2051_MEAS_ADC_SAMPLING_RATE
		... MAX98373_R2056_MEAS_ADC_PVDD_CH_EN:
	case MAX98373_R2090_BDE_LVL_HOLD ... MAX98373_R2092_BDE_CLIPPER_MODE:
	case MAX98373_R2097_BDE_L1_THRESH
		... MAX98373_R209B_BDE_THRESH_HYST:
	case MAX98373_R20A8_BDE_L1_CFG_1 ... MAX98373_R20B3_BDE_L4_CFG_3:
	case MAX98373_R20B5_BDE_EN ... MAX98373_R20B6_BDE_CUR_STATE_READBACK:
	case MAX98373_R20D1_DHT_CFG ... MAX98373_R20D4_DHT_EN:
	case MAX98373_R20E0_LIMITER_THRESH_CFG ... MAX98373_R20E2_LIMITER_EN:
	case MAX98373_R20FE_DEVICE_AUTO_RESTART_CFG
		... MAX98373_R20FF_GLOBAL_SHDN:
	case MAX98373_R21FF_REV_ID:
		return true;
	default:
		return false;
	}
};

static bool max98373_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {

	case 0x0040 ... 0x0070:			/* Soundwire Slave Control Port Registers	*/
	case 0x0100 ... 0x0137:			/* Soundwire Data Port 1 Registers	*/
	case 0x0300 ... 0x0337:			/* Soundwire Data Port 3 Registers	*/

	case MAX98373_R2000_SW_RESET ... MAX98373_R2009_INT_FLAG3:
	case MAX98373_R203E_AMP_PATH_GAIN:
	case MAX98373_R2054_MEAS_ADC_PVDD_CH_READBACK:
	case MAX98373_R2055_MEAS_ADC_THERM_CH_READBACK:
	case MAX98373_R20B6_BDE_CUR_STATE_READBACK:
	case MAX98373_R21FF_REV_ID:
		return true;
	default:
		return false;
	}
}

static const struct regmap_config max98373_sdw_regmap = {
	.reg_bits = 32,
	.val_bits = 8,

	.max_register = MAX98373_R21FF_REV_ID,

	.reg_defaults  = max98373_reg,
	.num_reg_defaults = ARRAY_SIZE(max98373_reg),

	.readable_reg = max98373_readable_register,
	.volatile_reg = max98373_volatile_reg,

	.cache_type = REGCACHE_RBTREE,

	.use_single_read = true,
	.use_single_write = true,
};


static void max98373_read_config(struct sdw_slave *slave)
{
	int value;
	struct device *dev = &slave->dev;
	struct max98373_priv *max98373 = dev_get_drvdata(dev);

	if (!device_property_read_u32(dev, "maxim,vmon-slot-no", &value))
		max98373->v_slot = value & 0xF;
	else
		max98373->v_slot = 0;

	if (!device_property_read_u32(dev, "maxim,imon-slot-no", &value))
		max98373->i_slot = value & 0xF;
	else
		max98373->i_slot = 1;

	if (!device_property_read_u32(dev, "maxim,spkfb-slot-no", &value))
		max98373->spkfb_slot = value & 0xF;
	else
		max98373->spkfb_slot = 2;

	/* update interleave mode info */
	if (device_property_read_bool(dev, "maxim,interleave_mode"))
		max98373->interleave_mode = true;
	else
		max98373->interleave_mode = false;
}



static void max98373_reset(struct sdw_slave *slave)
{
	int ret, reg, count;
	struct device *dev = &slave->dev;
	struct max98373_priv *max98373 = dev_get_drvdata(dev);

	/* Perform IO operations only if slave is in ATTACHED state */
	if (slave->status != SDW_SLAVE_ATTACHED)
		return;

	/* Software Reset */
	ret = regmap_update_bits(max98373->regmap,
		MAX98373_R2000_SW_RESET,
		MAX98373_SOFT_RESET,
		MAX98373_SOFT_RESET);
	if (ret)
		dev_err(dev, "Reset command failed. (ret:%d)\n", ret);

	count = 0;
	while (count < 3) {
		usleep_range(10000, 11000);
		/* Software Reset Verification */
		ret = regmap_read(max98373->regmap,
			MAX98373_R21FF_REV_ID, &reg);
		if (!ret) {
			dev_info(dev, "Reset completed (retry:%d)\n", count);
			return;
		}
		count++;
	}
	dev_err(dev, "Reset failed. (ret:%d)\n", ret);
}

static int max98373_io_init(struct sdw_slave *slave)
{
	struct device *dev = &slave->dev;
	struct max98373_priv *max98373 = dev_get_drvdata(dev);

	/* Perform IO operations only if slave is in ATTACHED state */
	if (slave->status != SDW_SLAVE_ATTACHED)
		return 0;

	/* Enable Runtime PM */
	pm_runtime_set_autosuspend_delay(dev, 3000);
	pm_runtime_use_autosuspend(dev);
	pm_runtime_enable(dev);

	/* Software Reset */
	max98373_reset(slave);

	/* IV default slot configuration */
	regmap_write(max98373->regmap,		MAX98373_R2020_PCM_TX_HIZ_EN_1,			0xFF);
	regmap_write(max98373->regmap,		MAX98373_R2021_PCM_TX_HIZ_EN_2,			0xFF);
	/* L/R mix configuration */
	regmap_write(max98373->regmap,		MAX98373_R2029_PCM_TO_SPK_MONO_MIX_1,	0x80);
	regmap_write(max98373->regmap,		MAX98373_R202A_PCM_TO_SPK_MONO_MIX_2,	0x01);
	/* Set inital volume (0dB) */
	regmap_write(max98373->regmap,		MAX98373_R203D_AMP_DIG_VOL_CTRL,		0x00);
	regmap_write(max98373->regmap,		MAX98373_R203E_AMP_PATH_GAIN,			0x00);

	/* Enable DC blocker */
	regmap_write(max98373->regmap,		MAX98373_R203F_AMP_DSP_CFG,				0x3);

	/* Enable IMON VMON DC blocker */
	regmap_write(max98373->regmap,		MAX98373_R2046_IV_SENSE_ADC_DSP_CFG,	0x7);

	/* voltage, current slot configuration */
	regmap_write(max98373->regmap,		MAX98373_R2022_PCM_TX_SRC_1,
		(max98373->i_slot << MAX98373_PCM_TX_CH_SRC_A_I_SHIFT |		max98373->v_slot) & 0xFF);

	if (max98373->v_slot < 8)
		regmap_update_bits(max98373->regmap,
			MAX98373_R2020_PCM_TX_HIZ_EN_1,		1 << max98373->v_slot, 0);
	else
		regmap_update_bits(max98373->regmap,
			MAX98373_R2021_PCM_TX_HIZ_EN_2,		1 << (max98373->v_slot - 8), 0);

	if (max98373->i_slot < 8)
		regmap_update_bits(max98373->regmap,
			MAX98373_R2020_PCM_TX_HIZ_EN_1,		1 << max98373->i_slot, 0);
	else
		regmap_update_bits(max98373->regmap,
			MAX98373_R2021_PCM_TX_HIZ_EN_2,		1 << (max98373->i_slot - 8), 0);

	/* speaker feedback slot configuration */
	regmap_write(max98373->regmap,		MAX98373_R2023_PCM_TX_SRC_2,			max98373->spkfb_slot & 0xFF);

	/* Set interleave mode */
	if (max98373->interleave_mode)
		regmap_update_bits(max98373->regmap,
			MAX98373_R2024_PCM_DATA_FMT_CFG,
			MAX98373_PCM_TX_CH_INTERLEAVE_MASK,			MAX98373_PCM_TX_CH_INTERLEAVE_MASK);

	/* Speaker enable */
	regmap_update_bits(max98373->regmap,
		MAX98373_R2043_AMP_EN,
		MAX98373_SPK_EN_MASK, 1);

	pm_runtime_put_sync_autosuspend(dev);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int max98373_suspend(struct device *dev)
{
	struct max98373_priv *max98373 = dev_get_drvdata(dev);

	regcache_cache_only(max98373->regmap, true);
	regcache_mark_dirty(max98373->regmap);
	return 0;
}
static int max98373_resume(struct device *dev)
{
	struct max98373_priv *max98373 = dev_get_drvdata(dev);

	regcache_cache_only(max98373->regmap, false);
	regcache_sync(max98373->regmap);
	return 0;
}
#endif

static const struct dev_pm_ops max98373_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(max98373_suspend, max98373_resume)
};


static int max98373_clock_config(struct sdw_slave *slave, struct sdw_bus_params *params)
{
	struct device *dev = &slave->dev;
	struct max98373_priv *max98373 = dev_get_drvdata(dev);
	unsigned int clk_freq, value;

	clk_freq = (params->curr_dr_freq >> 1);

	/* Perform IO operations only if slave is in ATTACHED state */
	if (slave->status != SDW_SLAVE_ATTACHED)
		return 0;

	/* Before programming the clock - disable the codec */
	value = 0x00; 	regmap_write(max98373->regmap, MAX98373_R20FF_GLOBAL_SHDN, value); 		// Global Disable

	/*
	// Select the proper value for the register based on the requested clock
	// If the value is not in the list, use reasonable default - 12 MHz
	*/
	value = 0x04;
	switch (clk_freq)
	{
		case 12288000:
			value = 0x05; break;
		case 12288000/2:
			value = 0x05 + 0x08; break;
		case 12288000/4:
			value = 0x05 + 0x10; break;
		case 12288000 / 8:
			value = 0x05 + 0x18; break;

		case 12000000:
			value = 0x04; break;
		case 12000000 / 2:
			value = 0x04 + 0x08; break;
		case 12000000 / 4:
			value = 0x04 + 0x10; break;
		case 12000000 / 8:
			value = 0x04 + 0x18; break;

		case 9600000:
			value = 0x02; break;
		case 9600000 / 2:
			value = 0x02 + 0x08; break;
		case 9600000 / 4:
			value = 0x02 + 0x10; break;
		case 9600000 / 8:
			value = 0x02 + 0x18; break;
	}
	regmap_write(max98373->regmap,MAX98373_R2036_SOUNDWIRE_CTRL, value);		// SWCLK

	/* Enable the codec - it can play the audio after 4 ms....	*/
	value = 0x01; regmap_write(max98373->regmap, MAX98373_R20FF_GLOBAL_SHDN, value);		// Global Enable

	return 0;
}

/******************************************************
static const struct snd_soc_component_driver soc_codec_dev_max98373 = {
	.probe			= max98373_probe,
	.controls		= max98373_snd_controls,
	.num_controls		= ARRAY_SIZE(max98373_snd_controls),
	.dapm_widgets		= max98373_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(max98373_dapm_widgets),
	.dapm_routes		= max98373_audio_map,
	.num_dapm_routes	= ARRAY_SIZE(max98373_audio_map),
	.idle_bias_on		= 1,
	.use_pmdown_time	= 1,
	.endianness		= 1,
	.non_legacy_dai_naming	= 1,
};
******************************************************/



static int max98373_init(struct sdw_slave *slave, struct regmap *regmap)
{
	struct max98373_priv *max98373;
	int ret = 0;
	struct device *dev = &slave->dev;

    /*  Allocate and assign private driver data structure  */
	max98373 = devm_kzalloc(dev, sizeof(*max98373), GFP_KERNEL);
	if (!max98373)
		return -ENOMEM;
	dev_set_drvdata(dev, max98373);
	max98373->regmap = regmap;

	/* Read voltage and slot configuration */
	max98373_read_config(slave);

	/* codec registration - TODO    */
/******************************************************
	ret = snd_soc_register_component(dev, &soc_codec_dev_max98373,
		max98373_dai, ARRAY_SIZE(max98373_dai));
	if (ret < 0)
		dev_err(dev, "Failed to register codec: %d\n", ret);
********************************************************/

	/* Perform IO operations only if slave is in ATTACHED state */
	if (slave->status != SDW_SLAVE_ATTACHED)
		return 0;

	/* Enable Runtime PM */
	pm_runtime_set_autosuspend_delay(dev, 3000);
	pm_runtime_use_autosuspend(dev);
	pm_runtime_enable(dev);
	pm_runtime_put_sync_autosuspend(dev);

	dev_info(dev, "%s\n", __func__);
	return ret;
}

static int max98373_update_status(struct sdw_slave *slave,
			       enum sdw_slave_status status)
{
	pr_err("In %s\n", __func__);
	/*
	 * Perform initialization only if slave status is SDW_SLAVE_ATTACHED
	 */
	if (status == SDW_SLAVE_ATTACHED) {
		/* perform I/O transfers required for Slave initialization */
		max98373_io_init(slave);
	}
	return 0;
}

static int max98373_read_prop(struct sdw_slave *slave)
{
	struct sdw_slave_prop *prop = &slave->prop;
	int i;
	u32 bit;
	unsigned long addr;
	struct sdw_dpn_prop *dpn;

	/* set the timeout values */

	prop->source_ports = 0x08;	/* BITMAP: 00001000  Dataport 3 is active */
	prop->sink_ports = 0x02;	/* BITMAP: 00000010  Dataport 1 is active */
	prop->paging_support = true;
	prop->clk_stop_timeout = 20;
	sdw_slave_read_prop(slave);

	dpn = prop->src_dpn_prop;
	i = 0;
	addr = prop->source_ports;
	for_each_set_bit(bit, &addr, 32) {
		dpn[i].num = bit;
		dpn[i].simple_ch_prep_sm = true;
		dpn[i].ch_prep_timeout = 10;
		i++;
	}

	dpn = prop->sink_dpn_prop;
	i = 0;
	addr = prop->sink_ports;
	for_each_set_bit(bit, &addr, 32) {
		dpn[i].num = bit;
		dpn[i].simple_ch_prep_sm = true;
		dpn[i].ch_prep_timeout = 10;
		i++;
	}

	return 0;
}

static int max98373_bus_config(struct sdw_slave *slave,
			    struct sdw_bus_params *params)
{
	int ret;

	ret = max98373_clock_config(slave, params);
	if (ret < 0)
		dev_err(&slave->dev, "Invalid clk config");

	return 0;
}

static int max98373_interrupt_callback(struct sdw_slave *slave,
				    struct sdw_slave_intr_status *status)
{
	pr_debug("%s control_port_stat=%x", __func__, status->control_port);
	return 0;
}

/*
 * slave_ops: callbacks for get_clock_stop_mode, clock_stop and
 * port_prep are not defined for now
 */
static struct sdw_slave_ops max98373_slave_ops = {
	.read_prop = max98373_read_prop,
	.interrupt_callback = max98373_interrupt_callback,
	.update_status = max98373_update_status,
	.bus_config = max98373_bus_config,
};

static int max98373_sdw_probe(struct sdw_slave *slave,
			   const struct sdw_device_id *id)
{
	struct regmap *regmap;
	int ret = 0;

	/* Assign ops */
	slave->ops = &max98373_slave_ops;

	/* Regmap Initialization */
	regmap = devm_regmap_init_sdw(slave, &max98373_sdw_regmap);
	if (!regmap)
		return -EINVAL;

	max98373_init(slave, regmap);

	return ret;
}

static int max98373_sdw_remove(struct sdw_slave *slave)
{
	max98373_reset(slave);
	return 0;
}

/*
//	Tables to identify module thru OpenFirmware or ACPI
//
*/
#if defined(CONFIG_OF)
static const struct of_device_id max98373_of_match[] = {
	{ .compatible = "maxim,max98373", },
	{},
};
MODULE_DEVICE_TABLE(of, max98373_of_match);
#endif

#ifdef CONFIG_ACPI
static const struct acpi_device_id max98373_acpi_match[] = {
	{ "MX98373", 0 },
	{},
};
MODULE_DEVICE_TABLE(acpi, max98373_acpi_match);
#endif

static const struct sdw_device_id max98373_id[] = {
	SDW_SLAVE_ENTRY(0x019F, 0x8373, 0),
	{},
};
MODULE_DEVICE_TABLE(sdw, max98373_id);

/*
//	Structures that define Driver Module
//
*/
static struct sdw_driver max98373_sdw_driver = {
	.driver = {
			.name = "max98373",
			.owner = THIS_MODULE,
			.of_match_table = of_match_ptr(max98373_of_match),
			.acpi_match_table = ACPI_PTR(max98373_acpi_match),
			.pm = &max98373_pm,
		   },
	.probe = max98373_sdw_probe,
	.remove = max98373_sdw_remove,
	.ops = &max98373_slave_ops,
	.id_table = max98373_id,
};

static int __init sdw_slave_init(void)
{
	return sdw_register_slave_driver(&max98373_sdw_driver);
}

static void __exit sdw_slave_exit(void)
{
	sdw_unregister_slave_driver(&max98373_sdw_driver);
}

module_init(sdw_slave_init);
module_exit(sdw_slave_exit);

MODULE_DESCRIPTION("ASoC MAX98373 driver SDW");
MODULE_AUTHOR("Oleg Sherbakov <oleg.sherbakov@maximintegrated.com>");
MODULE_LICENSE("GPL v2");
