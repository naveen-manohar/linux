// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2020 Intel Corporation

/*
 *  sof_sdw - ASOC Machine driver for Intel SoundWire platforms
 */

#include "sof_sdw_common.h"

void rt715_init(const struct snd_soc_acpi_link_adr *link,
		struct snd_soc_dai_link *dai_links,
		struct codec_info *info,
		bool playback)
{
	/*
	 * DAI ID is fixed at SDW_DMIC_DAI_ID for 715 to
	 * keep sdw DMIC and HDMI setting static in UCM
	 */
	if (sof_sdw_quirk & SOF_RT715_DAI_ID_FIX)
		dai_links->id = SDW_DMIC_DAI_ID;
}

