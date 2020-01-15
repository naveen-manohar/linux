// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2019 Intel Corporation

/*
 *  sdw_tgl_max98373 - ASOC Machine driver for Intel SoundWire platforms
 * connected to 2 max98373 codec devices
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
#include <linux/soundwire/sdw.h>
#include <linux/soundwire/sdw_type.h>
#include <sound/jack.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-acpi.h>
#include "../../codecs/hdac_hdmi.h"

struct mc_private {
	struct list_head hdmi_pcm_list;
};

#if IS_ENABLED(CONFIG_SND_SOC_HDAC_HDMI)

struct hdmi_pcm {
	struct list_head head;
	struct snd_soc_dai *codec_dai;
	int device;
};

#define NAME_SIZE	32
static int card_late_probe(struct snd_soc_card *card)
{
	return 0;
}
#endif

static const struct snd_soc_dapm_widget widgets[] = {
	SND_SOC_DAPM_SPK("Speaker", NULL),
//	SND_SOC_DAPM_SPK("Right Spk", NULL),
};

static const struct snd_soc_dapm_route map[] = {
	/* Speakers */
//	{ "Left Spk", NULL, "BE_OUT" },
	{ "Speaker", NULL, "BE_OUT" },
};

static const struct snd_soc_dapm_route second_speaker_map[] = {
	{ "Speaker", NULL, "BE_OUT" },
//	{ "Speaker", NULL, "BE_OUT" },
};

static const struct snd_kcontrol_new controls[] = {
	SOC_DAPM_PIN_SWITCH("Speaker"),
//	SOC_DAPM_PIN_SWITCH("Speaker"),
};

static int second_spk_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_card *card = rtd->card;
	int ret;
	printk("naveen %s %d\n", __func__, __LINE__);
	ret = snd_soc_dapm_add_routes(&card->dapm, second_speaker_map,
				      ARRAY_SIZE(second_speaker_map));

	if (ret)
		dev_err(rtd->dev, "second Speaker map addition failed: %d\n",
			ret);
	return ret;
}

//SND_SOC_DAILINK_DEF(sdw0_pin2,
//	DAILINK_COMP_ARRAY(COMP_CPU("SDW0 Pin2")));
//SND_SOC_DAILINK_DEF(sdw0_pin3,
//	DAILINK_COMP_ARRAY(COMP_CPU("SDW0 Pin3")));

SND_SOC_DAILINK_DEF(sdw1_pin2,
	DAILINK_COMP_ARRAY(COMP_CPU("SDW1 Pin2")));
SND_SOC_DAILINK_DEF(sdw1_codec,
	DAILINK_COMP_ARRAY(COMP_CODEC("sdw:1:19f:8373:0:3", "max98373-aif1"),
			COMP_CODEC("sdw:1:19f:8373:0:7", "max98373-aif1")));

SND_SOC_DAILINK_DEF(platform,
		DAILINK_COMP_ARRAY(COMP_PLATFORM("0000:00:1f.3")));

static struct snd_soc_codec_conf codec_conf[] = {
	{
		.dev_name = "sdw:1:19f:8373:0:3",
		.name_prefix = "Right",
	},
	{
		.dev_name = "sdw:1:19f:8373:0:7",
		.name_prefix = "Left",
	},

};

struct snd_soc_dai_link dailink[] = {
	{
		.name = "SDW1-Playback",
		.id = 2,
		.no_pcm = 1,
		.dpcm_playback = 1,
		.nonatomic = true,
		SND_SOC_DAILINK_REG(sdw1_pin2, sdw1_codec, platform),
	}
};

/* SoC card */
static struct snd_soc_card card_mx8373 = {
	.name = "tgl-sdw-mx8373",
	.dai_link = dailink,
	.num_links = ARRAY_SIZE(dailink),
	.controls = controls,
	.num_controls = ARRAY_SIZE(controls),
	.dapm_widgets = widgets,
	.num_dapm_widgets = ARRAY_SIZE(widgets),
	.dapm_routes = map,
	.num_dapm_routes = ARRAY_SIZE(map),
	.late_probe = card_late_probe,
	.codec_conf = codec_conf,
	.num_configs = ARRAY_SIZE(codec_conf),
};

static int mc_probe(struct platform_device *pdev)
{
	struct mc_private *ctx;
	struct snd_soc_acpi_mach *mach;
	const char *platform_name;
	struct snd_soc_card *card = &card_mx8373;
	int ret;
	printk("naveen %s %d\n", __func__, __LINE__);
	dev_dbg(&pdev->dev, "Entry %s\n", __func__);

	ctx = devm_kzalloc(&pdev->dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	card->dev = &pdev->dev;

	/* override platform name, if required */
	mach = (&pdev->dev)->platform_data;
	platform_name = mach->mach_params.platform;

	ret = snd_soc_fixup_dai_links_platform_name(card, platform_name);
	if (ret)
		return ret;

	snd_soc_card_set_drvdata(card, ctx);
//	card->num_links = ARRAY_SIZE(dailink) - 1 ;
//	card->num_configs = ARRAY_SIZE(codec_conf) - 1;

	/* Register the card */
	ret = devm_snd_soc_register_card(&pdev->dev, card);
	if (ret) {
		dev_err(card->dev, "snd_soc_register_card failed %d\n", ret);
		return ret;
	}

	platform_set_drvdata(pdev, card);

	return ret;
}

static struct platform_driver sdw_mx8373_driver = {
	.driver = {
		.name = "tgl_sdw_mx8373",
		.pm = &snd_soc_pm_ops,
	},
	.probe = mc_probe,
};

module_platform_driver(sdw_mx8373_driver);

MODULE_DESCRIPTION("TGL ASoC SoundWire MAX98373 Machine driver");

MODULE_AUTHOR("Naveen Manohar <naveen.m@intel.com>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:tgl_sdw_mx8373");
