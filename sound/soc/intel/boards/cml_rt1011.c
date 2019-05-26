// SPDX-License-Identifier: GPL-2.0
// Copyright(c) 2019 Intel Corporation.

/*
 * Intel Cometlake I2S Machine driver for RT1011 codec
 *
 * Modified from:
 *   Intel I2S Machine driver for RT5682
 *   Intel Geminilake I2S Machine driver for RT5682 + Maxim98357a codec
 */

#define DEBUG 1
#include <linux/input.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <linux/clk.h>
#include <linux/acpi.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <linux/dmi.h>
#include <linux/slab.h>
#include <asm/cpu_device_id.h>
#include <sound/soc-acpi.h>
#include "../skylake/skl.h"
#include "../../codecs/rt1011.h"

/* The platform clock outputs 24Mhz clock to codec as I2S1 MCLK */
#define CML_PLAT_CLK	24000000
#define CML_REALTEK_CODEC_DAI "rt1011-aif"

struct cml_acpi_card {
	char *codec_id;
	int codec_type;
	struct snd_soc_card *soc_card;
};

struct cml_mc_private {
	struct cml_acpi_card *acpi_card;
	char codec_name[SND_ACPI_I2C_ID_LEN];
	struct clk *mclk;
};

static const struct snd_soc_dapm_widget cml_rt1011_widgets[] = {
	SND_SOC_DAPM_SPK("TL Ext Spk", NULL),
	SND_SOC_DAPM_SPK("TR Ext Spk", NULL),
	SND_SOC_DAPM_SPK("WL Ext Spk", NULL),
	SND_SOC_DAPM_SPK("WR Ext Spk", NULL),
};

static const struct snd_soc_dapm_route cml_rt1011_audio_map[] = {
	{"TL Ext Spk", NULL, "TL SPO"},
	{"TR Ext Spk", NULL, "TR SPO"},
	{"WL Ext Spk", NULL, "WL SPO"},
	{"WR Ext Spk", NULL, "WR SPO"},
	{"TL AIF1 Playback", NULL, "ssp1 Tx"},
	{"TR AIF1 Playback", NULL, "ssp1 Tx"},
	{"WL AIF1 Playback", NULL, "ssp1 Tx"},
	{"WR AIF1 Playback", NULL, "ssp1 Tx"},
};

static const struct snd_kcontrol_new cml_controls[] = {
	SOC_DAPM_PIN_SWITCH("TL Ext Spk"),
	SOC_DAPM_PIN_SWITCH("TR Ext Spk"),
	SOC_DAPM_PIN_SWITCH("WL Ext Spk"),
	SOC_DAPM_PIN_SWITCH("WR Ext Spk"),
};

static int cml_aif1_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_card *card = rtd->card;
	int err, srate, i, ret;

	srate = params_rate(params);

	for (i = 0; i < rtd->num_codecs; i++) {
		codec_dai = rtd->codec_dais[i];

		err = snd_soc_dai_set_pll(codec_dai, 0, RT1011_PLL1_S_BCLK,
					64 * srate, 256 * srate);
		if (err < 0) {
			dev_err(card->dev, "codec_dai clock not set\n");
			return err;
		}

		err = snd_soc_dai_set_sysclk(codec_dai, RT1011_FS_SYS_PRE_S_PLL1, 256* srate,
					SND_SOC_CLOCK_IN);
		if (err < 0) {
			dev_err(card->dev, "codec_dai clock not set\n");
			return err;
		}
		/* FIXME */
		/* TDM 4 slots 24 bit, set Rx & Tx bitmask to 4 active slots */
		ret = snd_soc_dai_set_tdm_slot(codec_dai, 0xF, 0xF, 2, 16);
		if (ret < 0) {
			dev_err(rtd->dev, "can't set codec TDM slot %d\n", ret);
			return ret;
		}
	}

	return 0;
}

static int cml_codec_init(struct snd_soc_pcm_runtime *runtime)
{
	struct cml_mc_private *ctx = snd_soc_card_get_drvdata(runtime->card);
	int ret;

	if (!ctx) {
		printk("%s(%d)... ctx null\n", __func__, __LINE__);
		return 0;
	}

	ret = clk_prepare_enable(ctx->mclk);
	if (!ret)
		clk_disable_unprepare(ctx->mclk);

	ret = clk_set_rate(ctx->mclk, CML_PLAT_CLK);

	if (ret)
		dev_err(runtime->dev, "unable to set MCLK rate\n");

	return ret;
}

static const struct snd_soc_ops cml_be_ssp1_ops = {
	.hw_params = cml_aif1_hw_params,
};

static struct snd_soc_dai_link_component rt1011_codec_component[] = {
	{
		.name = "i2c-10EC1011:00",
		.dai_name = "rt1011-aif",
	},
	{
		.name = "i2c-10EC1011:01",
		.dai_name = "rt1011-aif",
	},
	{
		.name = "i2c-10EC1011:02",
		.dai_name = "rt1011-aif",
	},
	{
		.name = "i2c-10EC1011:03",
		.dai_name = "rt1011-aif",
	},
};

/* Cometlake digital audio interface glue - connects codec <--> CPU */
static struct snd_soc_dai_link cml_rt1011_dailink[] = {
	/* Back End DAI links */
	{
		/* SSP1 - Codec */
		.name = "SSP1-Codec",
		.id = 5,
		.cpu_dai_name = "SSP1 Pin",
		.platform_name = "0000:00:1f.3",
		.no_pcm = 1,
		.codecs = rt1011_codec_component,
		.num_codecs = ARRAY_SIZE(rt1011_codec_component),
		.dai_fmt = SND_SOC_DAIFMT_DSP_B |
			SND_SOC_DAIFMT_NB_NF |
			SND_SOC_DAIFMT_CBS_CFS,
		.init = cml_codec_init,
		.nonatomic = true,
		.dpcm_playback = 1,
		.ops = &cml_be_ssp1_ops,
	},
};

static struct snd_soc_codec_conf cml_rt1011_conf[] = {
	{
		.dev_name = "i2c-10EC1011:00",
		.name_prefix = "TL",
	},
	{
		.dev_name = "i2c-10EC1011:01",
		.name_prefix = "TR",
	},
	{
		.dev_name = "i2c-10EC1011:02",
		.name_prefix = "WL",
	},
	{
		.dev_name = "i2c-10EC1011:03",
		.name_prefix = "WR",
	},
};

/* Cometlake audio machine driver for RT1011 */
static struct snd_soc_card snd_soc_card_cml = {
	.name = "cml-audio",
	.dai_link = cml_rt1011_dailink,
	.num_links = ARRAY_SIZE(cml_rt1011_dailink),
	.codec_conf = cml_rt1011_conf,
	.num_configs = ARRAY_SIZE(cml_rt1011_conf),
	.dapm_widgets = cml_rt1011_widgets,
	.num_dapm_widgets = ARRAY_SIZE(cml_rt1011_widgets),
	.dapm_routes = cml_rt1011_audio_map,
	.num_dapm_routes = ARRAY_SIZE(cml_rt1011_audio_map),
	.controls = cml_controls,
	.num_controls = ARRAY_SIZE(cml_controls),
	.fully_routed = true,
};

static struct cml_acpi_card snd_soc_cards[] = {
	{"10EC1011", 0, &snd_soc_card_cml},
};

static int snd_cml_rt1011_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = snd_soc_cards[0].soc_card;
	struct snd_soc_acpi_mach *mach;
	const char *platform_name;
	struct cml_mc_private *drv;
	bool found = false;
	int ret_val = 0;
	int i;

	drv = devm_kzalloc(&pdev->dev, sizeof(*drv), GFP_ATOMIC);
	if (!drv)
		return -ENOMEM;

	mach = (&pdev->dev)->platform_data;

	for (i = 0; i < ARRAY_SIZE(snd_soc_cards); i++) {
		if (acpi_dev_found(snd_soc_cards[i].codec_id) &&
			(!strncmp(snd_soc_cards[i].codec_id, mach->id, 8))) {
			dev_dbg(&pdev->dev,
				"found codec %s\n", snd_soc_cards[i].codec_id);
			card = snd_soc_cards[i].soc_card;
			drv->acpi_card = &snd_soc_cards[i];
			found = true;
			break;
		}
	}

	if (!found) {
		dev_err(&pdev->dev, "No matching HID found in supported list\n");
		return -ENODEV;
	}

	card->dev = &pdev->dev;

	/* override plaform name, if required */
	mach = (&pdev->dev)->platform_data;
	platform_name = mach->mach_params.platform;

	ret_val = snd_soc_fixup_dai_links_platform_name(card,
							platform_name);
	if (ret_val)
		return ret_val;

	snd_soc_card_set_drvdata(card, drv);

	ret_val = devm_snd_soc_register_card(&pdev->dev, &snd_soc_card_cml);

	if (ret_val) {
		dev_err(&pdev->dev,
			"snd_soc_register_card failed %d\n", ret_val);
		return ret_val;
	}

	platform_set_drvdata(pdev, card);

	return ret_val;
}

static struct platform_driver snd_cml_rt1011_driver = {
	.driver = {
		.name = "cml_rt1011",
	},
	.probe = snd_cml_rt1011_probe,
};
module_platform_driver(snd_cml_rt1011_driver);

/* Module information */
MODULE_DESCRIPTION("Cometlake Audio Machine driver-RT1011 in I2S mode");
MODULE_AUTHOR("Naveen Manohar <naveen.m@intel.com>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:cml_rt1011");
