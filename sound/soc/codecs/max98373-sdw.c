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
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <linux/soundwire/sdw.h>
#include <linux/soundwire/sdw_type.h>
#include <sound/soc.h>
#include <sound/tlv.h>
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



/*****************************************************************************/
/* Power management functions and structure                                  */
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

static const struct dev_pm_ops max98373_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(max98373_suspend, max98373_resume)
};
/*                                                                           */
/*****************************************************************************/


/*****************************************************************************/
/*     MAX98373 Reset and initialization.                                    */

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


static int max98373_snd_probe(struct snd_soc_component *component)
{
	struct max98373_priv *max98373 = snd_soc_component_get_drvdata(component);

	/* IV default slot configuration */
	regmap_write(max98373->regmap,
		MAX98373_R2020_PCM_TX_HIZ_EN_1,
		0xFF);
	regmap_write(max98373->regmap,
		MAX98373_R2021_PCM_TX_HIZ_EN_2,
		0xFF);
	/* L/R mix configuration */
	regmap_write(max98373->regmap,
		MAX98373_R2029_PCM_TO_SPK_MONO_MIX_1,
		0x80);
	regmap_write(max98373->regmap,
		MAX98373_R202A_PCM_TO_SPK_MONO_MIX_2,
		0x1);
	/* Set inital volume (0dB) */
	regmap_write(max98373->regmap,
		MAX98373_R203D_AMP_DIG_VOL_CTRL,
		0x00);
	regmap_write(max98373->regmap,
		MAX98373_R203E_AMP_PATH_GAIN,
		0x00);
	/* Enable DC blocker */
	regmap_write(max98373->regmap,
		MAX98373_R203F_AMP_DSP_CFG,
		0x3);
	/* Enable IMON VMON DC blocker */
	regmap_write(max98373->regmap,
		MAX98373_R2046_IV_SENSE_ADC_DSP_CFG,
		0x7);
	/* voltage, current slot configuration */
	regmap_write(max98373->regmap,
		MAX98373_R2022_PCM_TX_SRC_1,
		(max98373->i_slot << MAX98373_PCM_TX_CH_SRC_A_I_SHIFT |
		max98373->v_slot) & 0xFF);
	if (max98373->v_slot < 8)
		regmap_update_bits(max98373->regmap,
			MAX98373_R2020_PCM_TX_HIZ_EN_1,
			1 << max98373->v_slot, 0);
	else
		regmap_update_bits(max98373->regmap,
			MAX98373_R2021_PCM_TX_HIZ_EN_2,
			1 << (max98373->v_slot - 8), 0);

	if (max98373->i_slot < 8)
		regmap_update_bits(max98373->regmap,
			MAX98373_R2020_PCM_TX_HIZ_EN_1,
			1 << max98373->i_slot, 0);
	else
		regmap_update_bits(max98373->regmap,
			MAX98373_R2021_PCM_TX_HIZ_EN_2,
			1 << (max98373->i_slot - 8), 0);

	/* speaker feedback slot configuration */
	regmap_write(max98373->regmap,
		MAX98373_R2023_PCM_TX_SRC_2,
		max98373->spkfb_slot & 0xFF);

	/* Set interleave mode */
	if (max98373->interleave_mode)
		regmap_update_bits(max98373->regmap,
			MAX98373_R2024_PCM_DATA_FMT_CFG,
			MAX98373_PCM_TX_CH_INTERLEAVE_MASK,
			MAX98373_PCM_TX_CH_INTERLEAVE_MASK);

	/* Speaker enable */
	regmap_update_bits(max98373->regmap,
		MAX98373_R2043_AMP_EN,
		MAX98373_SPK_EN_MASK, 1);

	return 0;
}

/*****************************************************************************/

/*****************************************************************************/
/*     DAC Power Up and Power Down modes.                                    */
static int max98373_dac_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct max98373_priv *max98373 = snd_soc_component_get_drvdata(component);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(max98373->regmap,
			MAX98373_R20FF_GLOBAL_SHDN,
			MAX98373_GLOBAL_EN_MASK, 1);
		break;
	case SND_SOC_DAPM_POST_PMD:
		regmap_update_bits(max98373->regmap,
			MAX98373_R20FF_GLOBAL_SHDN,
			MAX98373_GLOBAL_EN_MASK, 0);
		max98373->tdm_mode = false;
		break;
	default:
		return 0;
	}
	return 0;
}
/*      End of DAC Power Up and Down Modes                                   */
/*****************************************************************************/

/*****************************************************************************/
/*          DAI operations                                                   */

#define MAX98373_RATES SNDRV_PCM_RATE_8000_96000

#define MAX98373_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | \
	SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

static int max98373_dai_set_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	struct snd_soc_component *component = codec_dai->component;
	struct max98373_priv *max98373 = snd_soc_component_get_drvdata(component);
	unsigned int format = 0;
	unsigned int invert = 0;

	dev_dbg(component->dev, "%s: fmt 0x%08X\n", __func__, fmt);

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_IB_NF:
		invert = MAX98373_PCM_MODE_CFG_PCM_BCLKEDGE;
		break;
	default:
		dev_err(component->dev, "DAI invert mode unsupported\n");
		return -EINVAL;
	}

	regmap_update_bits(max98373->regmap,
		MAX98373_R2026_PCM_CLOCK_RATIO,
		MAX98373_PCM_MODE_CFG_PCM_BCLKEDGE,
		invert);

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		format = MAX98373_PCM_FORMAT_I2S;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		format = MAX98373_PCM_FORMAT_LJ;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		format = MAX98373_PCM_FORMAT_TDM_MODE1;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		format = MAX98373_PCM_FORMAT_TDM_MODE0;
		break;
	default:
		return -EINVAL;
	}

	regmap_update_bits(max98373->regmap,
		MAX98373_R2024_PCM_DATA_FMT_CFG,
		MAX98373_PCM_MODE_CFG_FORMAT_MASK,
		format << MAX98373_PCM_MODE_CFG_FORMAT_SHIFT);

	return 0;
}

/* BCLKs per LRCLK */
static const int bclk_sel_table[] = {
	32, 48, 64, 96, 128, 192, 256, 384, 512, 320,
};

static int max98373_get_bclk_sel(int bclk)
{
	int i;
	/* match BCLKs per LRCLK */
	for (i = 0; i < ARRAY_SIZE(bclk_sel_table); i++) {
		if (bclk_sel_table[i] == bclk)
			return i + 2;
	}
	return 0;
}

static int max98373_set_clock(struct snd_soc_component *component,
	struct snd_pcm_hw_params *params)
{
	struct max98373_priv *max98373 = snd_soc_component_get_drvdata(component);
	/* BCLK/LRCLK ratio calculation */
	int blr_clk_ratio = params_channels(params) * max98373->ch_size;
	int value;

	if (!max98373->tdm_mode) {
		/* BCLK configuration */
		value = max98373_get_bclk_sel(blr_clk_ratio);
		if (!value) {
			dev_err(component->dev, "format unsupported %d\n",
				params_format(params));
			return -EINVAL;
		}

		regmap_update_bits(max98373->regmap,
			MAX98373_R2026_PCM_CLOCK_RATIO,
			MAX98373_PCM_CLK_SETUP_BSEL_MASK,
			value);
	}
	return 0;
}


static int max98373_dai_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	struct max98373_priv *max98373 = snd_soc_component_get_drvdata(component);
	unsigned int sampling_rate = 0;
	unsigned int chan_sz = 0;

	/* pcm mode configuration */
	switch (snd_pcm_format_width(params_format(params))) {
	case 16:
		chan_sz = MAX98373_PCM_MODE_CFG_CHANSZ_16;
		break;
	case 24:
		chan_sz = MAX98373_PCM_MODE_CFG_CHANSZ_24;
		break;
	case 32:
		chan_sz = MAX98373_PCM_MODE_CFG_CHANSZ_32;
		break;
	default:
		dev_err(component->dev, "format unsupported %d\n",
			params_format(params));
		goto err;
	}

	max98373->ch_size = snd_pcm_format_width(params_format(params));

	regmap_update_bits(max98373->regmap,
		MAX98373_R2024_PCM_DATA_FMT_CFG,
		MAX98373_PCM_MODE_CFG_CHANSZ_MASK, chan_sz);

	dev_dbg(component->dev, "format supported %d",
		params_format(params));

	/* sampling rate configuration */
	switch (params_rate(params)) {
	case 8000:
		sampling_rate = MAX98373_PCM_SR_SET1_SR_8000;
		break;
	case 11025:
		sampling_rate = MAX98373_PCM_SR_SET1_SR_11025;
		break;
	case 12000:
		sampling_rate = MAX98373_PCM_SR_SET1_SR_12000;
		break;
	case 16000:
		sampling_rate = MAX98373_PCM_SR_SET1_SR_16000;
		break;
	case 22050:
		sampling_rate = MAX98373_PCM_SR_SET1_SR_22050;
		break;
	case 24000:
		sampling_rate = MAX98373_PCM_SR_SET1_SR_24000;
		break;
	case 32000:
		sampling_rate = MAX98373_PCM_SR_SET1_SR_32000;
		break;
	case 44100:
		sampling_rate = MAX98373_PCM_SR_SET1_SR_44100;
		break;
	case 48000:
		sampling_rate = MAX98373_PCM_SR_SET1_SR_48000;
		break;
	case 88200:
		sampling_rate = MAX98373_PCM_SR_SET1_SR_88200;
		break;
	case 96000:
		sampling_rate = MAX98373_PCM_SR_SET1_SR_96000;
		break;
	default:
		dev_err(component->dev, "rate %d not supported\n",
			params_rate(params));
		goto err;
	}

	/* set DAI_SR to correct LRCLK frequency */
	regmap_update_bits(max98373->regmap,
		MAX98373_R2027_PCM_SR_SETUP_1,
		MAX98373_PCM_SR_SET1_SR_MASK,
		sampling_rate);
	regmap_update_bits(max98373->regmap,
		MAX98373_R2028_PCM_SR_SETUP_2,
		MAX98373_PCM_SR_SET2_SR_MASK,
		sampling_rate << MAX98373_PCM_SR_SET2_SR_SHIFT);

	/* set sampling rate of IV */
	if (max98373->interleave_mode &&
	    sampling_rate > MAX98373_PCM_SR_SET1_SR_16000)
		regmap_update_bits(max98373->regmap,
			MAX98373_R2028_PCM_SR_SETUP_2,
			MAX98373_PCM_SR_SET2_IVADC_SR_MASK,
			sampling_rate - 3);
	else
		regmap_update_bits(max98373->regmap,
			MAX98373_R2028_PCM_SR_SETUP_2,
			MAX98373_PCM_SR_SET2_IVADC_SR_MASK,
			sampling_rate);

	return max98373_set_clock(component, params);
err:
	return -EINVAL;
}

static int max98373_dai_tdm_slot(struct snd_soc_dai *dai,
	unsigned int tx_mask, unsigned int rx_mask,
	int slots, int slot_width)
{
	struct snd_soc_component *component = dai->component;
	struct max98373_priv *max98373 = snd_soc_component_get_drvdata(component);
	int bsel = 0;
	unsigned int chan_sz = 0;
	unsigned int mask;
	int x, slot_found;

	if (!tx_mask && !rx_mask && !slots && !slot_width)
		max98373->tdm_mode = false;
	else
		max98373->tdm_mode = true;

	/* BCLK configuration */
	bsel = max98373_get_bclk_sel(slots * slot_width);
	if (bsel == 0) {
		dev_err(component->dev, "BCLK %d not supported\n",
			slots * slot_width);
		return -EINVAL;
	}

	regmap_update_bits(max98373->regmap,
		MAX98373_R2026_PCM_CLOCK_RATIO,
		MAX98373_PCM_CLK_SETUP_BSEL_MASK,
		bsel);

	/* Channel size configuration */
	switch (slot_width) {
	case 16:
		chan_sz = MAX98373_PCM_MODE_CFG_CHANSZ_16;
		break;
	case 24:
		chan_sz = MAX98373_PCM_MODE_CFG_CHANSZ_24;
		break;
	case 32:
		chan_sz = MAX98373_PCM_MODE_CFG_CHANSZ_32;
		break;
	default:
		dev_err(component->dev, "format unsupported %d\n",
			slot_width);
		return -EINVAL;
	}

	regmap_update_bits(max98373->regmap,
		MAX98373_R2024_PCM_DATA_FMT_CFG,
		MAX98373_PCM_MODE_CFG_CHANSZ_MASK, chan_sz);

	/* Rx slot configuration */
	slot_found = 0;
	mask = rx_mask;
	for (x = 0 ; x < 16 ; x++, mask >>= 1) {
		if (mask & 0x1) {
			if (slot_found == 0)
				regmap_update_bits(max98373->regmap,
					MAX98373_R2029_PCM_TO_SPK_MONO_MIX_1,
					MAX98373_PCM_TO_SPK_CH0_SRC_MASK, x);
			else
				regmap_write(max98373->regmap,
					MAX98373_R202A_PCM_TO_SPK_MONO_MIX_2,
					x);
			slot_found++;
			if (slot_found > 1)
				break;
		}
	}

	/* Tx slot Hi-Z configuration */
	regmap_write(max98373->regmap,
		MAX98373_R2020_PCM_TX_HIZ_EN_1,
		~tx_mask & 0xFF);
	regmap_write(max98373->regmap,
		MAX98373_R2021_PCM_TX_HIZ_EN_2,
		(~tx_mask & 0xFF00) >> 8);

	return 0;
}

/*      End of DAI operations                                                */
/*****************************************************************************/


static DECLARE_TLV_DB_SCALE(max98373_digital_tlv, -6350, 50, 1);

static const DECLARE_TLV_DB_RANGE(max98373_spk_tlv,
	0, 8, TLV_DB_SCALE_ITEM(0, 50, 0),
	9, 10, TLV_DB_SCALE_ITEM(500, 100, 0),
);

static const DECLARE_TLV_DB_RANGE(max98373_spkgain_max_tlv,
	0, 9, TLV_DB_SCALE_ITEM(800, 100, 0),
);

static const DECLARE_TLV_DB_RANGE(max98373_dht_step_size_tlv,
	0, 1, TLV_DB_SCALE_ITEM(25, 25, 0),
	2, 4, TLV_DB_SCALE_ITEM(100, 100, 0),
);

static const DECLARE_TLV_DB_RANGE(max98373_dht_spkgain_min_tlv,
	0, 9, TLV_DB_SCALE_ITEM(800, 100, 0),
);

static const DECLARE_TLV_DB_RANGE(max98373_dht_rotation_point_tlv,
	0, 1, TLV_DB_SCALE_ITEM(-3000, 500, 0),
	2, 4, TLV_DB_SCALE_ITEM(-2200, 200, 0),
	5, 6, TLV_DB_SCALE_ITEM(-1500, 300, 0),
	7, 9, TLV_DB_SCALE_ITEM(-1000, 200, 0),
	10, 13, TLV_DB_SCALE_ITEM(-500, 100, 0),
	14, 15, TLV_DB_SCALE_ITEM(-100, 50, 0),
);

static const DECLARE_TLV_DB_RANGE(max98373_limiter_thresh_tlv,
	0, 15, TLV_DB_SCALE_ITEM(-1500, 100, 0),
);

static const DECLARE_TLV_DB_RANGE(max98373_bde_gain_tlv,
	0, 60, TLV_DB_SCALE_ITEM(-1500, 25, 0),
);


static const char * const max98373_output_voltage_lvl_text[] = {
	"5.43V", "6.09V", "6.83V", "7.67V", "8.60V",
	"9.65V", "10.83V", "12.15V", "13.63V", "15.29V"
};

static SOC_ENUM_SINGLE_DECL(max98373_out_volt_enum,
			    MAX98373_R203E_AMP_PATH_GAIN, 0,
			    max98373_output_voltage_lvl_text);

static const char * const max98373_dht_attack_rate_text[] = {
	"17.5us", "35us", "70us", "140us",
	"280us", "560us", "1120us", "2240us"
};

static SOC_ENUM_SINGLE_DECL(max98373_dht_attack_rate_enum,
			    MAX98373_R20D2_DHT_ATTACK_CFG, 0,
			    max98373_dht_attack_rate_text);

static const char * const max98373_dht_release_rate_text[] = {
	"45ms", "225ms", "450ms", "1150ms",
	"2250ms", "3100ms", "4500ms", "6750ms"
};

static SOC_ENUM_SINGLE_DECL(max98373_dht_release_rate_enum,
			    MAX98373_R20D3_DHT_RELEASE_CFG, 0,
			    max98373_dht_release_rate_text);

static const char * const max98373_limiter_attack_rate_text[] = {
	"10us", "20us", "40us", "80us",
	"160us", "320us", "640us", "1.28ms",
	"2.56ms", "5.12ms", "10.24ms", "20.48ms",
	"40.96ms", "81.92ms", "16.384ms", "32.768ms"
};

static SOC_ENUM_SINGLE_DECL(max98373_limiter_attack_rate_enum,
			    MAX98373_R20E1_LIMITER_ATK_REL_RATES, 4,
			    max98373_limiter_attack_rate_text);

static const char * const max98373_limiter_release_rate_text[] = {
	"40us", "80us", "160us", "320us",
	"640us", "1.28ms", "2.56ms", "5.120ms",
	"10.24ms", "20.48ms", "40.96ms", "81.92ms",
	"163.84ms", "327.68ms", "655.36ms", "1310.72ms"
};

static SOC_ENUM_SINGLE_DECL(max98373_limiter_release_rate_enum,
			    MAX98373_R20E1_LIMITER_ATK_REL_RATES, 0,
			    max98373_limiter_release_rate_text);

static const char * const max98373_ADC_samplerate_text[] = {
	"333kHz", "192kHz", "64kHz", "48kHz"
};

static SOC_ENUM_SINGLE_DECL(max98373_adc_samplerate_enum,
			    MAX98373_R2051_MEAS_ADC_SAMPLING_RATE, 0,
			    max98373_ADC_samplerate_text);

static const struct snd_kcontrol_new max98373_snd_controls[] = {
SOC_SINGLE("Digital Vol Sel Switch", MAX98373_R203F_AMP_DSP_CFG,
	MAX98373_AMP_VOL_SEL_SHIFT, 1, 0),
SOC_SINGLE("Volume Location Switch", MAX98373_R203F_AMP_DSP_CFG,
	MAX98373_AMP_VOL_SEL_SHIFT, 1, 0),
SOC_SINGLE("Ramp Up Switch", MAX98373_R203F_AMP_DSP_CFG,
	MAX98373_AMP_DSP_CFG_RMP_UP_SHIFT, 1, 0),
SOC_SINGLE("Ramp Down Switch", MAX98373_R203F_AMP_DSP_CFG,
	MAX98373_AMP_DSP_CFG_RMP_DN_SHIFT, 1, 0),
SOC_SINGLE("CLK Monitor Switch", MAX98373_R20FE_DEVICE_AUTO_RESTART_CFG,
	MAX98373_CLOCK_MON_SHIFT, 1, 0),
SOC_SINGLE("Dither Switch", MAX98373_R203F_AMP_DSP_CFG,
	MAX98373_AMP_DSP_CFG_DITH_SHIFT, 1, 0),
SOC_SINGLE("DC Blocker Switch", MAX98373_R203F_AMP_DSP_CFG,
	MAX98373_AMP_DSP_CFG_DCBLK_SHIFT, 1, 0),
SOC_SINGLE_TLV("Digital Volume", MAX98373_R203D_AMP_DIG_VOL_CTRL,
	0, 0x7F, 1, max98373_digital_tlv),
SOC_SINGLE_TLV("Speaker Volume", MAX98373_R203E_AMP_PATH_GAIN,
	MAX98373_SPK_DIGI_GAIN_SHIFT, 10, 0, max98373_spk_tlv),
SOC_SINGLE_TLV("FS Max Volume", MAX98373_R203E_AMP_PATH_GAIN,
	MAX98373_FS_GAIN_MAX_SHIFT, 9, 0, max98373_spkgain_max_tlv),
SOC_ENUM("Output Voltage", max98373_out_volt_enum),
/* Dynamic Headroom Tracking */
SOC_SINGLE("DHT Switch", MAX98373_R20D4_DHT_EN,
	MAX98373_DHT_EN_SHIFT, 1, 0),
SOC_SINGLE_TLV("DHT Min Volume", MAX98373_R20D1_DHT_CFG,
	MAX98373_DHT_SPK_GAIN_MIN_SHIFT, 9, 0, max98373_dht_spkgain_min_tlv),
SOC_SINGLE_TLV("DHT Rot Pnt Volume", MAX98373_R20D1_DHT_CFG,
	MAX98373_DHT_ROT_PNT_SHIFT, 15, 1, max98373_dht_rotation_point_tlv),
SOC_SINGLE_TLV("DHT Attack Step Volume", MAX98373_R20D2_DHT_ATTACK_CFG,
	MAX98373_DHT_ATTACK_STEP_SHIFT, 4, 0, max98373_dht_step_size_tlv),
SOC_SINGLE_TLV("DHT Release Step Volume", MAX98373_R20D3_DHT_RELEASE_CFG,
	MAX98373_DHT_RELEASE_STEP_SHIFT, 4, 0, max98373_dht_step_size_tlv),
SOC_ENUM("DHT Attack Rate", max98373_dht_attack_rate_enum),
SOC_ENUM("DHT Release Rate", max98373_dht_release_rate_enum),
/* ADC configuration */
SOC_SINGLE("ADC PVDD CH Switch", MAX98373_R2056_MEAS_ADC_PVDD_CH_EN, 0, 1, 0),
SOC_SINGLE("ADC PVDD FLT Switch", MAX98373_R2052_MEAS_ADC_PVDD_FLT_CFG,
	MAX98373_FLT_EN_SHIFT, 1, 0),
SOC_SINGLE("ADC TEMP FLT Switch", MAX98373_R2053_MEAS_ADC_THERM_FLT_CFG,
	MAX98373_FLT_EN_SHIFT, 1, 0),
SOC_SINGLE("ADC PVDD", MAX98373_R2054_MEAS_ADC_PVDD_CH_READBACK, 0, 0xFF, 0),
SOC_SINGLE("ADC TEMP", MAX98373_R2055_MEAS_ADC_THERM_CH_READBACK, 0, 0xFF, 0),
SOC_SINGLE("ADC PVDD FLT Coeff", MAX98373_R2052_MEAS_ADC_PVDD_FLT_CFG,
	0, 0x3, 0),
SOC_SINGLE("ADC TEMP FLT Coeff", MAX98373_R2053_MEAS_ADC_THERM_FLT_CFG,
	0, 0x3, 0),
SOC_ENUM("ADC SampleRate", max98373_adc_samplerate_enum),
/* Brownout Detection Engine */
SOC_SINGLE("BDE Switch", MAX98373_R20B5_BDE_EN, MAX98373_BDE_EN_SHIFT, 1, 0),
SOC_SINGLE("BDE LVL4 Mute Switch", MAX98373_R20B2_BDE_L4_CFG_2,
	MAX98373_LVL4_MUTE_EN_SHIFT, 1, 0),
SOC_SINGLE("BDE LVL4 Hold Switch", MAX98373_R20B2_BDE_L4_CFG_2,
	MAX98373_LVL4_HOLD_EN_SHIFT, 1, 0),
SOC_SINGLE("BDE LVL1 Thresh", MAX98373_R2097_BDE_L1_THRESH, 0, 0xFF, 0),
SOC_SINGLE("BDE LVL2 Thresh", MAX98373_R2098_BDE_L2_THRESH, 0, 0xFF, 0),
SOC_SINGLE("BDE LVL3 Thresh", MAX98373_R2099_BDE_L3_THRESH, 0, 0xFF, 0),
SOC_SINGLE("BDE LVL4 Thresh", MAX98373_R209A_BDE_L4_THRESH, 0, 0xFF, 0),
SOC_SINGLE("BDE Active Level", MAX98373_R20B6_BDE_CUR_STATE_READBACK, 0, 8, 0),
SOC_SINGLE("BDE Clip Mode Switch", MAX98373_R2092_BDE_CLIPPER_MODE, 0, 1, 0),
SOC_SINGLE("BDE Thresh Hysteresis", MAX98373_R209B_BDE_THRESH_HYST, 0, 0xFF, 0),
SOC_SINGLE("BDE Hold Time", MAX98373_R2090_BDE_LVL_HOLD, 0, 0xFF, 0),
SOC_SINGLE("BDE Attack Rate", MAX98373_R2091_BDE_GAIN_ATK_REL_RATE, 4, 0xF, 0),
SOC_SINGLE("BDE Release Rate", MAX98373_R2091_BDE_GAIN_ATK_REL_RATE, 0, 0xF, 0),
SOC_SINGLE_TLV("BDE LVL1 Clip Thresh Volume", MAX98373_R20A9_BDE_L1_CFG_2,
	0, 0x3C, 1, max98373_bde_gain_tlv),
SOC_SINGLE_TLV("BDE LVL2 Clip Thresh Volume", MAX98373_R20AC_BDE_L2_CFG_2,
	0, 0x3C, 1, max98373_bde_gain_tlv),
SOC_SINGLE_TLV("BDE LVL3 Clip Thresh Volume", MAX98373_R20AF_BDE_L3_CFG_2,
	0, 0x3C, 1, max98373_bde_gain_tlv),
SOC_SINGLE_TLV("BDE LVL4 Clip Thresh Volume", MAX98373_R20B2_BDE_L4_CFG_2,
	0, 0x3C, 1, max98373_bde_gain_tlv),
SOC_SINGLE_TLV("BDE LVL1 Clip Reduction Volume", MAX98373_R20AA_BDE_L1_CFG_3,
	0, 0x3C, 1, max98373_bde_gain_tlv),
SOC_SINGLE_TLV("BDE LVL2 Clip Reduction Volume", MAX98373_R20AD_BDE_L2_CFG_3,
	0, 0x3C, 1, max98373_bde_gain_tlv),
SOC_SINGLE_TLV("BDE LVL3 Clip Reduction Volume", MAX98373_R20B0_BDE_L3_CFG_3,
	0, 0x3C, 1, max98373_bde_gain_tlv),
SOC_SINGLE_TLV("BDE LVL4 Clip Reduction Volume", MAX98373_R20B3_BDE_L4_CFG_3,
	0, 0x3C, 1, max98373_bde_gain_tlv),
SOC_SINGLE_TLV("BDE LVL1 Limiter Thresh Volume", MAX98373_R20A8_BDE_L1_CFG_1,
	0, 0xF, 1, max98373_limiter_thresh_tlv),
SOC_SINGLE_TLV("BDE LVL2 Limiter Thresh Volume", MAX98373_R20AB_BDE_L2_CFG_1,
	0, 0xF, 1, max98373_limiter_thresh_tlv),
SOC_SINGLE_TLV("BDE LVL3 Limiter Thresh Volume", MAX98373_R20AE_BDE_L3_CFG_1,
	0, 0xF, 1, max98373_limiter_thresh_tlv),
SOC_SINGLE_TLV("BDE LVL4 Limiter Thresh Volume", MAX98373_R20B1_BDE_L4_CFG_1,
	0, 0xF, 1, max98373_limiter_thresh_tlv),
/* Limiter */
SOC_SINGLE("Limiter Switch", MAX98373_R20E2_LIMITER_EN,
	MAX98373_LIMITER_EN_SHIFT, 1, 0),
SOC_SINGLE("Limiter Src Switch", MAX98373_R20E0_LIMITER_THRESH_CFG,
	MAX98373_LIMITER_THRESH_SRC_SHIFT, 1, 0),
SOC_SINGLE_TLV("Limiter Thresh Volume", MAX98373_R20E0_LIMITER_THRESH_CFG,
	MAX98373_LIMITER_THRESH_SHIFT, 15, 0, max98373_limiter_thresh_tlv),
SOC_ENUM("Limiter Attack Rate", max98373_limiter_attack_rate_enum),
SOC_ENUM("Limiter Release Rate", max98373_limiter_release_rate_enum),
};

static const struct snd_soc_dapm_route max98373_audio_map[] = {
	/* Plabyack */
	{"DAI Sel Mux", "Left", "Amp Enable"},
	{"DAI Sel Mux", "Right", "Amp Enable"},
	{"DAI Sel Mux", "LeftRight", "Amp Enable"},
	{"BE_OUT", NULL, "DAI Sel Mux"},
	/* Capture */
	{ "VI Sense", "Switch", "VMON" },
	{ "VI Sense", "Switch", "IMON" },
	{ "SpkFB Sense", "Switch", "FBMON" },
	{ "Voltage Sense", NULL, "VI Sense" },
	{ "Current Sense", NULL, "VI Sense" },
	{ "Speaker FB Sense", NULL, "SpkFB Sense" },
};

static const char * const max98373_switch_text[] = {
	"Left", "Right", "LeftRight"};

static const struct soc_enum dai_sel_enum =
	SOC_ENUM_SINGLE(MAX98373_R2029_PCM_TO_SPK_MONO_MIX_1,
		MAX98373_PCM_TO_SPK_MONOMIX_CFG_SHIFT,
		3, max98373_switch_text);

static const struct snd_kcontrol_new max98373_dai_controls =
	SOC_DAPM_ENUM("DAI Sel", dai_sel_enum);

static const struct snd_kcontrol_new max98373_vi_control =
	SOC_DAPM_SINGLE("Switch", MAX98373_R202C_PCM_TX_EN, 0, 1, 0);

static const struct snd_kcontrol_new max98373_spkfb_control =
	SOC_DAPM_SINGLE("Switch", MAX98373_R2043_AMP_EN, 1, 1, 0);

static const struct snd_soc_dapm_widget max98373_dapm_widgets[] = {
SND_SOC_DAPM_DAC_E("Amp Enable", "HiFi Playback",
	MAX98373_R202B_PCM_RX_EN, 0, 0, max98373_dac_event,
	SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD),
SND_SOC_DAPM_MUX("DAI Sel Mux", SND_SOC_NOPM, 0, 0,
	&max98373_dai_controls),
SND_SOC_DAPM_OUTPUT("BE_OUT"),
SND_SOC_DAPM_AIF_OUT("Voltage Sense", "HiFi Capture", 0,
	MAX98373_R2047_IV_SENSE_ADC_EN, 0, 0),
SND_SOC_DAPM_AIF_OUT("Current Sense", "HiFi Capture", 0,
	MAX98373_R2047_IV_SENSE_ADC_EN, 1, 0),
SND_SOC_DAPM_AIF_OUT("Speaker FB Sense", "HiFi Capture", 0,
	SND_SOC_NOPM, 0, 0),
SND_SOC_DAPM_SWITCH("VI Sense", SND_SOC_NOPM, 0, 0,
	&max98373_vi_control),
SND_SOC_DAPM_SWITCH("SpkFB Sense", SND_SOC_NOPM, 0, 0,
	&max98373_spkfb_control),
SND_SOC_DAPM_SIGGEN("VMON"),
SND_SOC_DAPM_SIGGEN("IMON"),
SND_SOC_DAPM_SIGGEN("FBMON"),
};



static const struct snd_soc_component_driver soc_codec_dev_max98373 = {
	.probe			= max98373_snd_probe,
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

static const struct snd_soc_dai_ops max98373_dai_ops = {
	.set_fmt = max98373_dai_set_fmt,
	.hw_params = max98373_dai_hw_params,
	.set_tdm_slot = max98373_dai_tdm_slot,
};

static struct snd_soc_dai_driver max98373_dai[] = {
	{
		.name = "max98373-aif1",
		.playback = {
			.stream_name = "HiFi Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MAX98373_RATES,
			.formats = MAX98373_FORMATS,
		},
		.capture = {
			.stream_name = "HiFi Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = MAX98373_RATES,
			.formats = MAX98373_FORMATS,
		},
		.ops = &max98373_dai_ops,
	}
};


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

	/* codec registration  */
	ret = snd_soc_register_component(dev, &soc_codec_dev_max98373,
		max98373_dai, ARRAY_SIZE(max98373_dai));
	if (ret < 0)
		dev_err(dev, "Failed to register codec: %d\n", ret);

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
