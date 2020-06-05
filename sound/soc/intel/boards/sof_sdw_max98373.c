// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2020 Intel Corporation
//
// sof_sdw_max98373 - Helpers to handle 2x MAX98373
// codec devices from generic machine driver

#include <linux/device.h>
#include <linux/errno.h>
#include <sound/soc.h>
#include <sound/soc-acpi.h>
#include "sof_sdw_common.h"

static const struct snd_soc_dapm_widget mx8373_widgets[] = {
	SND_SOC_DAPM_SPK("Left Spk", NULL),
	SND_SOC_DAPM_SPK("Right Spk", NULL),
};

static const struct snd_soc_dapm_route mx8373_map[] = {
	/* Speakers */
	{ "Left Spk", NULL, "mx8373-1 BE_OUT" },
	{ "Right Spk", NULL, "mx8373-2 BE_OUT" },
};

static const struct snd_kcontrol_new mx8373_controls[] = {
	SOC_DAPM_PIN_SWITCH("Left Spk"),
	SOC_DAPM_PIN_SWITCH("Right Spk"),
};

static int spk_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_dai *codec_dai;
	int i, ret;

	card->components = devm_kasprintf(card->dev, GFP_KERNEL,
					  "%s spk:mx8373",
					  card->components);
	if (!card->components)
		return -ENOMEM;

	ret = snd_soc_add_card_controls(card, mx8373_controls,
					ARRAY_SIZE(mx8373_controls));
	if (ret) {
		dev_err(card->dev, "mx8373 ctrls addition failed: %d\n", ret);
		return ret;
	}

	ret = snd_soc_dapm_new_controls(&card->dapm, mx8373_widgets,
					ARRAY_SIZE(mx8373_widgets));
	if (ret) {
		dev_err(card->dev, "mx8373 widgets addition failed: %d\n", ret);
		return ret;
	}

	ret = snd_soc_dapm_add_routes(&card->dapm, mx8373_map, 2);
	if (ret)
		dev_err(rtd->dev, "failed to add first SPK map: %d\n", ret);

	for_each_rtd_codec_dais(rtd, i, codec_dai) {
		if (!strcmp(codec_dai->component->name, "sdw:1:19f:8373:0:3")) {
			/* DEV0 tdm slot configuration */
			snd_soc_dai_set_tdm_slot(codec_dai, 3, 3, 2, 32);
		}
		if (!strcmp(codec_dai->component->name, "sdw:1:19f:8373:0:7")) {
			/* DEV1 tdm slot configuration */
			snd_soc_dai_set_tdm_slot(codec_dai, 3, 3, 2, 32);
		}
	}

	return ret;
}

int sof_sdw_mx8373_init(const struct snd_soc_acpi_link_adr *link,
			struct snd_soc_dai_link *dai_links,
			struct sof_sdw_codec_info *info,
			bool playback)
{
	info->amp_num++;
	if (info->amp_num == 2)
		dai_links->init = spk_init;

	return 0;
}
