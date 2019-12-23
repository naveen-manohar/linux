// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2019 Intel Corporation

/*
 *  sdw_rt1308 - ASOC Machine driver for Intel SoundWire platforms
 * connected to ALC711 device
 */

#include <linux/acpi.h>
#include <linux/async.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/io.h>
#include <linux/dmi.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <sound/jack.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-acpi.h>
#include "../../codecs/hdac_hdmi.h"
#include "hda_dsp_common.h"

struct mc_private {
	struct list_head hdmi_pcm_list;
	bool common_hdmi_codec_drv;
	struct snd_soc_jack sdw_headset;
};

static const struct snd_soc_dapm_widget widgets[] = {
	SND_SOC_DAPM_SPK("Speaker", NULL),
};

static const struct snd_soc_dapm_route map[] = {
	/* Speakers */
	{ "Speaker", NULL, "rt1308-1 SPOL" },
	{ "Speaker", NULL, "rt1308-1 SPOR" },
	{ "Speaker", NULL, "rt1308-2 SPOL" },
	{ "Speaker", NULL, "rt1308-2 SPOR" },
};

static const struct snd_kcontrol_new controls[] = {
	SOC_DAPM_PIN_SWITCH("Speaker"),
};

SND_SOC_DAILINK_DEF(sdw0_pin2,
		    DAILINK_COMP_ARRAY(COMP_CPU("SDW0 Pin2")));
SND_SOC_DAILINK_DEF(sdw0_codec,
		    DAILINK_COMP_ARRAY(
		    COMP_CODEC("sdw:0:25d:1308:0:0", "rt1308-aif"),
		    COMP_CODEC("sdw:0:25d:1308:0:2", "rt1308-aif")));

SND_SOC_DAILINK_DEF(platform,
		    DAILINK_COMP_ARRAY(COMP_PLATFORM("0000:00:1f.3")));

static struct snd_soc_codec_conf codec_conf[] = {
	{
		.dev_name = "sdw:0:25d:1308:0:0",
		.name_prefix = "rt1308-1",
	},
	{
		.dev_name = "sdw:0:25d:1308:0:2",
		.name_prefix = "rt1308-2",
	},
};

static int rt1308_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai;
	int srate, i, ret = 0;

	srate = params_rate(params);

	for (i = 0; i < rtd->num_codecs; i++) {
		codec_dai = rtd->codec_dais[i];

		/*
		 * Codec TDM is configured as 2 CH over 2 codecs
		 */
		if (!strcmp(codec_dai->component->name, "sdw:0:25d:1308:0:0"))
			codec_dai->tx_mask = 0x1;
		if (!strcmp(codec_dai->component->name, "sdw:0:25d:1308:0:2"))
			codec_dai->tx_mask = 0x2;
	}

	return ret;
}

static const struct snd_soc_ops rt1308_ops = {
	.hw_params = rt1308_hw_params,
};

struct snd_soc_dai_link dailink[] = {
	{
		.name = "SDW0-Playback",
		.id = 0,
		.no_pcm = 1,
		.dpcm_playback = 1,
		.nonatomic = true,
		.ops = &rt1308_ops,
		SND_SOC_DAILINK_REG(sdw0_pin2, sdw0_codec, platform),
	},
};

/* SoC card */
static struct snd_soc_card card_sdw_rt1308 = {
	.name = "sdw-rt1308",
	.dai_link = dailink,
	.num_links = ARRAY_SIZE(dailink),
	.controls = controls,
	.num_controls = ARRAY_SIZE(controls),
	.dapm_widgets = widgets,
	.num_dapm_widgets = ARRAY_SIZE(widgets),
	.dapm_routes = map,
	.num_dapm_routes = ARRAY_SIZE(map),
	.codec_conf = codec_conf,
	.num_configs = ARRAY_SIZE(codec_conf),
};

static int mc_probe(struct platform_device *pdev)
{
	struct mc_private *ctx;
	struct snd_soc_acpi_mach *mach;
	const char *platform_name;
	struct snd_soc_card *card = &card_sdw_rt1308;
	int ret;

	dev_dbg(&pdev->dev, "Entry %s\n", __func__);

	ctx = devm_kzalloc(&pdev->dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

#if IS_ENABLED(CONFIG_SND_SOC_HDAC_HDMI)
	INIT_LIST_HEAD(&ctx->hdmi_pcm_list);
#endif

	card->dev = &pdev->dev;

	/* override platform name, if required */
	mach = (&pdev->dev)->platform_data;
	platform_name = mach->mach_params.platform;

	ret = snd_soc_fixup_dai_links_platform_name(card, platform_name);
	if (ret)
		return ret;

	ctx->common_hdmi_codec_drv = mach->mach_params.common_hdmi_codec_drv;

	snd_soc_card_set_drvdata(card, ctx);

	/* Register the card */
	ret = devm_snd_soc_register_card(&pdev->dev, card);
	if (ret) {
		dev_err(card->dev, "snd_soc_register_card failed %d\n", ret);
		return ret;
	}

	platform_set_drvdata(pdev, card);

	return ret;
}

static struct platform_driver sdw_rt1308_driver = {
	.driver = {
		.name = "sdw_rt1308",
		.pm = &snd_soc_pm_ops,
	},
	.probe = mc_probe,
};

module_platform_driver(sdw_rt1308_driver);

MODULE_DESCRIPTION("ASoC SoundWire rt1308 Machine driver");
MODULE_AUTHOR("Bard Liao <yung-chuan.liao@linux.intel.com>");
MODULE_AUTHOR("Pierre-Louis Bossart <pierre-louis.bossart@linux.intel.com>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:sdw_rt1308");
