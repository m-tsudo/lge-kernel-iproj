/* lge/lge_board/i_skt/board_i_skt_snd.c
 *
 * Copyright (C) 2010 LGE, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#define CONFIG_LGE_AUDIO_NO_NCP_MODE
#define CONFIG_LGE_AUDIO_USE_AUXMIC_FOR_SPEAKER_MODE

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/mfd/pmic8058.h>
#include <linux/pmic8058-othc.h>
#include <linux/mfd/pmic8901.h>
#include <linux/mfd/msm-adie-codec.h>
#include <linux/regulator/pmic8058-regulator.h>
#include <linux/regulator/pmic8901-regulator.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/machine.h>

#include <mach/qdsp6v2/audio_dev_ctl.h>
#include <mach/mpp.h>
#include <sound/apr_audio.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>

#include "snddev_icodec.h"
#include "snddev_ecodec.h"
#include "timpani_profile_8x60_i_vzw.h"
#include "snddev_hdmi.h"
#include "snddev_mi2s.h"
#include "snddev_virtual.h"

#ifdef CONFIG_LGE_HEADSET_DETECTION_FSA8008
#include "fsa8008.h"
#endif

#ifdef CONFIG_LGE_AUDIO_AMP_WM9093
#include <linux/i2c.h>
#include "devices_i_vzw.h"
#include "wm9093.h"

//#define LGE_AUDIO_COMMENT_OUT_FOR_REFERENCE

static wmCodecCmd seq_lin_to_spkout[] = 
{
    {0x39, 0x000D},
    {0x01, 0x000B},
    {0x02, 0x6030},
    {0x17, 0x0001}, // IN2_CLAMP 0bit 0 : clamp de-activate 1: clamp activated  
    {0x1A, 0x0101}, // IN2A -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)  
    {0x1B, 0x0101}, // IN2B -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)
    {0x36, 0x0005},
    {0x03, 0x0008},
    {0x22, 0x0000}, // 0dB
    {0x03, 0x0108},  
    {0x25, 0x0170},//{0x25, 0x0140}, // 12dB(178}, 9dB(170}, 7.5dB(168}, 6dB(160}, 4.5dB(158}, 3dB(150}, 1.5dB(148}, 0dB(140)
    {0x26, 0x013F}, // refer page 26  
    {0x17, 0x0000},  
    {0x01, 0x100B}
};

/* QTR Line ==> WM9093 IN2 ==> SPK */
wmCodecCmd seq_lin_to_spkout_playback[] = 
{
    {0x39, 0x000D},
    {0x01, 0x000B},
    {0x02, 0x6030},
    {0x17, 0x0001}, // IN2_CLAMP 0bit 0 : clamp de-activate 1: clamp activated  
    {0x1A, 0x0101}, // IN2A -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)  
    {0x1B, 0x0101}, // IN2B -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)
    {0x36, 0x0005},
    {0x03, 0x0008},
    {0x22, 0x0000}, // 0dB
    {0x03, 0x0108},  
    {0x25, 0x0170}, // 12dB(178}, 9dB(170}, 7.5dB(168}, 6dB(160}, 4.5dB(158}, 3dB(150}, 1.5dB(148}, 0dB(140)
    {0x26, 0x013F}, // refer page 26  
    {0x17, 0x0000},  
    {0x01, 0x100B}
};

wmCodecCmd seq_lin_to_spkout_vzwnavi[] = 
{
    {0x39, 0x000D},
    {0x01, 0x000B},
    {0x02, 0x6030},
    {0x17, 0x0001}, // IN2_CLAMP 0bit 0 : clamp de-activate 1: clamp activated  
    {0x1A, 0x0102}, // IN2A -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)  
    {0x1B, 0x0102}, // IN2B -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)
    {0x36, 0x0005},
    {0x03, 0x0008},
    {0x22, 0x0000}, // 0dB
    {0x03, 0x0108},  
    {0x25, 0x0170}, // 12dB(178}, 9dB(170}, 7.5dB(168}, 6dB(160}, 4.5dB(158}, 3dB(150}, 1.5dB(148}, 0dB(140)
    {0x26, 0x013F}, // refer page 26  
    {0x17, 0x0000},  
    {0x01, 0x100B}
};

/* QTR HPH ==> WM9093 IN1 ==> EAR */
wmCodecCmd seq_lin_to_headset[] = 
{
    {0x39, 0x000D},
    {0x01, 0x000B},
    {0x02, 0x60C0},
    {0x16, 0x0001}, // IN1_CLAMP 0bit 0 : clamp de-activate 1: clamp activated
    {0x18, 0x0100}, // IN1A -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)  
    {0x19, 0x0100}, // IN1B -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)  
    {0x2D, 0x0040},
    {0x2E, 0x0010},
    {0x03, 0x0030},
    {0x2F, 0x0000},
    {0x30, 0x0000},
    {0x1C, 0x0139}, //refer to page 26
    {0x1D, 0x0139}, //refer to page 26  
    {0x16, 0x0000},
    {0x46, 0x0100},
    {0x49, 0x0100}
};

/* QTR HPH ==> WM9093 IN1 ==> EAR */
wmCodecCmd seq_lin_to_headset_playback[] = 
{
    {0x39, 0x000D},
    {0x01, 0x000B},
    {0x02, 0x60C0},
    {0x16, 0x0001}, // IN1_CLAMP 0bit 0 : clamp de-activate 1: clamp activated
    {0x18, 0x0100}, // IN1A -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)  
    {0x19, 0x0100}, // IN1B -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)  
    {0x2D, 0x0040},
    {0x2E, 0x0010},
    {0x03, 0x0030},
    {0x2F, 0x0000},
    {0x30, 0x0000},
    {0x1C, 0x0131},//{0x1C, 0x0137}, //refer to page 26 2011.7.7 dolby tuning
    {0x1D, 0x0131},//{0x1D, 0x0137}, //refer to page 26 2011.7.7 dolby tuning 
    {0x16, 0x0000},
    {0x46, 0x0100},
    {0x49, 0x0100}
};

//LGE_UPDATE_S, jeremy.pi@lge.com, 2011-04-09, at&t simultaneously Ringing Headset and SPK
/* QTR HPH ==> WM9093 IN1 ==> EAR and SPK */
wmCodecCmd seq_lin_to_headset_spkout[] = 
{
    {0x39, 0x000D},
    {0x01, 0x130B}, // Enable Speaker output, Headphone output
    {0x02, 0x60C0},	
    {0x16, 0x0001},	
    {0x18, 0x0100}, // IN1A -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)  
    {0x19, 0x0100}, // IN1B -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)  
    {0x36, 0x0050},
    {0x22, 0x0000}, // 0dB
    {0x25, 0x0170}, // 12dB(178}, 9dB(170}, 7.5dB(168}, 6dB(160}, 4.5dB(158}, 3dB(150}, 1.5dB(148}, 0dB(140)
	{0x26, 0x0139}, //{0x26, 0x0137},//	 {0x26, 0x013F}, // refer to page 26
    {0x2D, 0x0040},
    {0x2E, 0x0010},	
    {0x03, 0x0138},
    {0x2F, 0x0000},	
    {0x30, 0x0000},	
    {0x1C, 0x011D}, //{0x1C, 0x0117}, //refer to page 26
    {0x1D, 0x011D}, //{0x1D, 0x0117}, //refer to page 26    
    {0x16, 0x0000},	
    {0x46, 0x0100},	
    {0x49, 0x0100}
};
//LGE_UPDATE_E, jeremy.pi@lge.com, 2011-04-09, at&t simultaneously Ringing Headset and SPK

//LGE_UPDATE_S, jeremy.pi@lge.com, 2011-04-09, at&t TTY
/* QTR HPH ==> WM9093 IN1 ==> EAR */
wmCodecCmd seq_lin_to_headset_tty[] = 
{
    {0x39, 0x000D},
    {0x01, 0x000B},
    {0x02, 0x60C0},
    {0x16, 0x0001}, // IN1_CLAMP 0bit 0 : clamp de-activate 1: clamp activated
    {0x18, 0x0102}, // IN1A -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)  
    {0x19, 0x0102}, // IN1B -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)  
    {0x2D, 0x0040},
    {0x2E, 0x0010},
    {0x03, 0x0030},
    {0x2F, 0x0000},
    {0x30, 0x0000},
    {0x1C, 0x012F}, //refer to page 26
    {0x1D, 0x012F}, //refer to page 26  
    {0x16, 0x0000},
    {0x46, 0x0100},
    {0x49, 0x0100}
};
//LGE_UPDATE_E, jeremy.pi@lge.com, 2011-04-09, at&t TTY

static wmCodecCmd seq_power_down[] = 
{
	{0x46, 0x0100},
	{0x49, 0x0110},
	{0x00, 0x0000},
	{0x02, 0x6000},
	{0x39, 0x0000}
};

/* QTR Line ==> WM9093 IN2 ==> SPK */
wmCodecCmd seq_tuning_lin_to_spkout[] = 
{
    {0x39, 0x000D},
    {0x01, 0x000B},
    {0x02, 0x6030},
    {0x17, 0x0001}, // IN2_CLAMP 0bit 0 : clamp de-activate 1: clamp activated  
    {0x1A, 0x0101}, // IN2A -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)  
    {0x1B, 0x0101}, // IN2B -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)
    {0x36, 0x0005},
    {0x03, 0x0008},
    {0x22, 0x0000}, // 0dB
    {0x03, 0x0108},  
    {0x25, 0x0170},//{0x25, 0x0140}, // 12dB(178}, 9dB(170}, 7.5dB(168}, 6dB(160}, 4.5dB(158}, 3dB(150}, 1.5dB(148}, 0dB(140)
    {0x26, 0x013F}, // refer page 26  
    {0x17, 0x0000},  
    {0x01, 0x100B}
};

/* QTR Line ==> WM9093 IN2 ==> SPK */
wmCodecCmd seq_tuning_lin_to_spkout_playback[] = 
{
    {0x39, 0x000D},
    {0x01, 0x000B},
    {0x02, 0x6030},
    {0x17, 0x0001}, // IN2_CLAMP 0bit 0 : clamp de-activate 1: clamp activated  
    {0x1A, 0x0101}, // IN2A -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)  
    {0x1B, 0x0101}, // IN2B -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)
    {0x36, 0x0005},
    {0x03, 0x0008},
    {0x22, 0x0000}, // 0dB
    {0x03, 0x0108},  
    {0x25, 0x0170}, // 12dB(178}, 9dB(170}, 7.5dB(168}, 6dB(160}, 4.5dB(158}, 3dB(150}, 1.5dB(148}, 0dB(140)
    {0x26, 0x013F}, // refer page 26  
    {0x17, 0x0000},  
    {0x01, 0x100B}
};


/* QTR HPH ==> WM9093 IN1 ==> EAR */
wmCodecCmd seq_tuning_lin_to_headset[] = 
{
    {0x39, 0x000D},
    {0x01, 0x000B},
    {0x02, 0x60C0},
    {0x16, 0x0001}, // IN1_CLAMP 0bit 0 : clamp de-activate 1: clamp activated
    {0x18, 0x0100}, // IN1A -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)  
    {0x19, 0x0100}, // IN1B -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)  
    {0x2D, 0x0040},
    {0x2E, 0x0010},
    {0x03, 0x0030},
    {0x2F, 0x0000},
    {0x30, 0x0000},
    {0x1C, 0x0139}, //refer to page 26
    {0x1D, 0x0139}, //refer to page 26  
    {0x16, 0x0000},
    {0x46, 0x0100},
    {0x49, 0x0100}
};

/* QTR HPH ==> WM9093 IN1 ==> EAR */
wmCodecCmd seq_tuning_lin_to_headset_playback[] = 
{
    {0x39, 0x000D},
    {0x01, 0x000B},
    {0x02, 0x60C0},
    {0x16, 0x0001}, // IN1_CLAMP 0bit 0 : clamp de-activate 1: clamp activated
    {0x18, 0x0100}, // IN1A -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)  
    {0x19, 0x0100}, // IN1B -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)  
    {0x2D, 0x0040},
    {0x2E, 0x0010},
    {0x03, 0x0030},
    {0x2F, 0x0000},
    {0x30, 0x0000},
    {0x1C, 0x0131},//{0x1C, 0x0137}, //refer to page 26 2011.7.7 dolby tuning
    {0x1D, 0x0131},//{0x1D, 0x0137}, //refer to page 26 2011.7.7 dolby tuning 
    {0x16, 0x0000},
    {0x46, 0x0100},
    {0x49, 0x0100}
};

//LGE_UPDATE_S, jeremy.pi@lge.com, 2011-04-09, at&t simultaneously Ringing Headset and SPK
/* QTR HPH ==> WM9093 IN1 ==> EAR and SPK */
wmCodecCmd seq_tuning_lin_to_headset_spkout[] = 
{
    {0x39, 0x000D},
    {0x01, 0x130B}, // Enable Speaker output, Headphone output
    {0x02, 0x60C0},	
    {0x16, 0x0001},	
    {0x18, 0x0100}, // IN1A -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)  
    {0x19, 0x0100}, // IN1B -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)  
    {0x36, 0x0050},
    {0x22, 0x0000}, // 0dB
    {0x25, 0x0140}, // 12dB(178}, 9dB(170}, 7.5dB(168}, 6dB(160}, 4.5dB(158}, 3dB(150}, 1.5dB(148}, 0dB(140)
	{0x26, 0x0139}, //{0x26, 0x0137},//	 {0x26, 0x013F}, // refer to page 26
    {0x2D, 0x0040},
    {0x2E, 0x0010},	
    {0x03, 0x0138},
    {0x2F, 0x0000},	
    {0x30, 0x0000},	
    {0x1C, 0x011D}, //{0x1C, 0x0117}, //refer to page 26
    {0x1D, 0x011D}, //{0x1D, 0x0117}, //refer to page 26    
    {0x16, 0x0000},	
    {0x46, 0x0100},	
    {0x49, 0x0100}
};
//LGE_UPDATE_E, jeremy.pi@lge.com, 2011-04-09, at&t simultaneously Ringing Headset and SPK

//LGE_UPDATE_S, jeremy.pi@lge.com, 2011-04-09, at&t TTY
/* QTR HPH ==> WM9093 IN1 ==> EAR */
wmCodecCmd seq_tuning_lin_to_headset_tty[] = 
{
    {0x39, 0x000D},
    {0x01, 0x000B},
    {0x02, 0x60C0},
    {0x16, 0x0001}, // IN1_CLAMP 0bit 0 : clamp de-activate 1: clamp activated
    {0x18, 0x0102}, // IN1A -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)  
    {0x19, 0x0102}, // IN1B -6dB(00}, -3.5dB(01}, 0dB(02}, 3.5dB(03}, 6dB(04}, 12dB(05}, 18dB(06}, 18dB(07)  
    {0x2D, 0x0040},
    {0x2E, 0x0010},
    {0x03, 0x0030},
    {0x2F, 0x0000},
    {0x30, 0x0000},
    {0x1C, 0x012F}, //refer to page 26
    {0x1D, 0x012F}, //refer to page 26  
    {0x16, 0x0000},
    {0x46, 0x0100},
    {0x49, 0x0100}
};

struct wm9093_platform_data lge_audio_wm9093_platform = {
	.speaker_on = {
		.amp_function = seq_lin_to_spkout,
		.amp_tuning_function = seq_tuning_lin_to_spkout,	
		.amp_function_size = ARRAY_SIZE(seq_lin_to_spkout),
	},
		
	.hph_on = {
		.amp_function = seq_lin_to_headset,
		.amp_tuning_function = seq_tuning_lin_to_headset,				
		.amp_function_size = ARRAY_SIZE(seq_lin_to_headset),
	},

//LGE_UPDATE_S, jeremy.pi@lge.com, 2011-04-09, at&t
    .hph_spk_on = { // at&t simultaneously Ringing Headset and SPK
        .amp_function = seq_lin_to_headset_spkout,
		.amp_tuning_function = seq_tuning_lin_to_headset_spkout,			
        .amp_function_size = ARRAY_SIZE(seq_lin_to_headset_spkout),
    },

    .tty_on = { // TTY
        .amp_function = seq_lin_to_headset_tty,
		.amp_tuning_function = seq_tuning_lin_to_headset_tty,			
        .amp_function_size = ARRAY_SIZE(seq_lin_to_headset_tty),
    },

    .speaker_playback_on = { // playback
        .amp_function = seq_lin_to_spkout_playback,
		.amp_tuning_function = seq_tuning_lin_to_spkout_playback,			
        .amp_function_size = ARRAY_SIZE(seq_lin_to_spkout_playback),
    },

    .hph_playback_on = { // playback
        .amp_function = seq_lin_to_headset_playback,
		.amp_tuning_function = seq_tuning_lin_to_headset_playback,			
        .amp_function_size = ARRAY_SIZE(seq_lin_to_headset_playback),
    },
//LGE_UPDATE_E, jeremy.pi@lge.com, 2011-04-09, at&t

	.speaker_vzwnavi_on = { // playback
		.amp_function = seq_lin_to_spkout_vzwnavi,
		.amp_tuning_function = seq_tuning_lin_to_spkout_playback,			
		.amp_function_size = ARRAY_SIZE(seq_lin_to_spkout_vzwnavi),
	},

	.power_down = {
		.amp_function = seq_power_down,
		.amp_tuning_function = seq_power_down,	
		.amp_function_size = ARRAY_SIZE(seq_power_down),
	},
	.set_amp_path = NULL,
	.wm9093_cmd_register = NULL,
	.wm9093_reg_dump = NULL
	
};

#define WM9093_I2C_SLAVE_ADDR   0xDC

static struct i2c_board_info lge_i2c_wm9093_info[] = {
        {
                I2C_BOARD_INFO("wm9093", WM9093_I2C_SLAVE_ADDR>>1),
				.platform_data = &lge_audio_wm9093_platform
        },
};
#endif

#ifdef CONFIG_DEBUG_FS
static struct dentry *debugfs_hsed_config;
static void snddev_hsed_config_modify_setting(int type);
static void snddev_hsed_config_restore_setting(void);
#endif

#ifdef LGE_AUDIO_COMMENT_OUT_FOR_REFERENCE
/* GPIO_CLASS_D0_EN */
#define SNDDEV_GPIO_CLASS_D0_EN 227

/* GPIO_CLASS_D1_EN */
#define SNDDEV_GPIO_CLASS_D1_EN 229

#define SNDDEV_GPIO_MIC2_ANCR_SEL 294
#define SNDDEV_GPIO_MIC1_ANCL_SEL 295
#define SNDDEV_GPIO_HS_MIC4_SEL 296
#endif

static struct resource msm_cdcclk_ctl_resources[] = {
	{
		.name   = "msm_snddev_tx_mclk",
		.start  = 108,
		.end    = 108,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "msm_snddev_rx_mclk",
		.start  = 109,
		.end    = 109,
		.flags  = IORESOURCE_IO,
	},
};

static struct platform_device msm_cdcclk_ctl_device = {
	.name   = "msm_cdcclk_ctl",
	.num_resources  = ARRAY_SIZE(msm_cdcclk_ctl_resources),
	.resource       = msm_cdcclk_ctl_resources,
};

static struct resource msm_aux_pcm_resources[] = {

	{
		.name   = "aux_pcm_dout",
		.start  = 111,
		.end    = 111,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_din",
		.start  = 112,
		.end    = 112,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_syncout",
		.start  = 113,
		.end    = 113,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_clkin_a",
		.start  = 114,
		.end    = 114,
		.flags  = IORESOURCE_IO,
	},
};

static struct platform_device msm_aux_pcm_device = {
	.name   = "msm_aux_pcm",
	.num_resources  = ARRAY_SIZE(msm_aux_pcm_resources),
	.resource       = msm_aux_pcm_resources,
};

static struct resource msm_mi2s_gpio_resources[] = {

	{
		.name   = "mi2s_ws",
		.start  = 101,
		.end    = 101,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "mi2s_sclk",
		.start  = 102,
		.end    = 102,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "mi2s_mclk",
		.start  = 103,
		.end    = 103,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "fm_mi2s_sd",
		.start  = 107,
		.end    = 107,
		.flags  = IORESOURCE_IO,
	},
};

static struct platform_device msm_mi2s_device = {
	.name		= "msm_mi2s",
	.num_resources	= ARRAY_SIZE(msm_mi2s_gpio_resources),
	.resource	= msm_mi2s_gpio_resources,
};

#ifdef LGE_AUDIO_COMMENT_OUT_FOR_REFERENCE
/* Must be same size as msm_icodec_gpio_resources */
static int msm_icodec_gpio_defaults[] = {
	0,
	0,
};

static struct resource msm_icodec_gpio_resources[] = {
	{
		.name   = "msm_icodec_speaker_left",
		.start  = SNDDEV_GPIO_CLASS_D0_EN,
		.end    = SNDDEV_GPIO_CLASS_D0_EN,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "msm_icodec_speaker_right",
		.start  = SNDDEV_GPIO_CLASS_D1_EN,
		.end    = SNDDEV_GPIO_CLASS_D1_EN,
		.flags  = IORESOURCE_IO,
	},
};

static struct platform_device msm_icodec_gpio_device = {
	.name   = "msm_icodec_gpio",
	.num_resources  = ARRAY_SIZE(msm_icodec_gpio_resources),
	.resource       = msm_icodec_gpio_resources,
	.dev = { .platform_data = &msm_icodec_gpio_defaults },
};

static struct regulator *s3;
static struct regulator *mvs;

static int msm_snddev_enable_dmic_power(void)
{
	int ret;

	s3 = regulator_get(NULL, "8058_s3");
	if (IS_ERR(s3)) {
		ret = -EBUSY;
		goto fail_get_s3;
	}

	ret = regulator_set_voltage(s3, 1800000, 1800000);
	if (ret) {
		pr_err("%s: error setting voltage\n", __func__);
		goto fail_s3;
	}

	ret = regulator_enable(s3);
	if (ret) {
		pr_err("%s: error enabling regulator\n", __func__);
		goto fail_s3;
	}

	mvs = regulator_get(NULL, "8901_mvs0");
	if (IS_ERR(mvs))
		goto fail_mvs0_get;

	ret = regulator_enable(mvs);
	if (ret) {
		pr_err("%s: error setting regulator\n", __func__);
		goto fail_mvs0_enable;
	}
	return ret;

fail_mvs0_enable:
	regulator_put(mvs);
	mvs = NULL;
fail_mvs0_get:
	regulator_disable(s3);
fail_s3:
	regulator_put(s3);
	s3 = NULL;
fail_get_s3:
	return ret;
}

static void msm_snddev_disable_dmic_power(void)
{
	int ret;

	if (mvs) {
		ret = regulator_disable(mvs);
		if (ret < 0)
			pr_err("%s: error disabling vreg mvs\n", __func__);
		regulator_put(mvs);
		mvs = NULL;
	}

	if (s3) {
		ret = regulator_disable(s3);
		if (ret < 0)
			pr_err("%s: error disabling regulator s3\n", __func__);
		regulator_put(s3);
		s3 = NULL;
	}
}

#define PM8901_MPP_3 (2) /* PM8901 MPP starts from 0 */

static int config_class_d0_gpio(int enable)
{
	int rc;

	if (enable) {
		rc = pm8901_mpp_config_digital_out(PM8901_MPP_3,
			PM8901_MPP_DIG_LEVEL_MSMIO, 1);

		if (rc) {
			pr_err("%s: CLASS_D0_EN failed\n", __func__);
			return rc;
		}

		rc = gpio_request(SNDDEV_GPIO_CLASS_D0_EN, "CLASSD0_EN");

		if (rc) {
			pr_err("%s: spkr pamp gpio pm8901 mpp3 request"
			"failed\n", __func__);
			pm8901_mpp_config_digital_out(PM8901_MPP_3,
			PM8901_MPP_DIG_LEVEL_MSMIO, 0);
			return rc;
		}

		gpio_direction_output(SNDDEV_GPIO_CLASS_D0_EN, 1);
		gpio_set_value(SNDDEV_GPIO_CLASS_D0_EN, 1);

	} else {
		pm8901_mpp_config_digital_out(PM8901_MPP_3,
		PM8901_MPP_DIG_LEVEL_MSMIO, 0);
		gpio_set_value(SNDDEV_GPIO_CLASS_D0_EN, 0);
		gpio_free(SNDDEV_GPIO_CLASS_D0_EN);
	}
	return 0;
}

static atomic_t pamp_ref_cnt;

static int msm_snddev_poweramp_on(void)
{
	int rc;

	if (atomic_inc_return(&pamp_ref_cnt) > 1)
		return 0;

	pr_debug("%s: enable stereo spkr amp\n", __func__);
	rc = config_class_d0_gpio(1);
	if (rc) {
		pr_err("%s: d0 gpio configuration failed\n", __func__);
		goto config_gpio_fail;
	}
config_gpio_fail:
	return rc;
}

static void msm_snddev_poweramp_off(void)
{
	if (atomic_dec_return(&pamp_ref_cnt) == 0) {
		pr_debug("%s: disable stereo spkr amp\n", __func__);
		config_class_d0_gpio(0);
		msleep(30);
	}
}
#endif

static int msm_snddev_poweramp_on_spk(void)
{
	pr_debug("%s\n", __func__);

#ifdef CONFIG_LGE_AUDIO_AMP_WM9093
	if (lge_audio_wm9093_platform.set_amp_path)
		lge_audio_wm9093_platform.set_amp_path(ICODEC_SPEAKER_RX);
#endif

	return 0;
}

static void msm_snddev_poweramp_off_spk(void)
{
#ifdef CONFIG_LGE_AUDIO_AMP_WM9093
	if (lge_audio_wm9093_platform.set_amp_path)
		lge_audio_wm9093_platform.set_amp_path(ICODEC_AMP_OFF);
#endif

	pr_debug("%s\n", __func__);
 }

static int msm_snddev_poweramp_on_spk_playback(void)
{
	pr_debug("%s\n", __func__);

#ifdef CONFIG_LGE_AUDIO_AMP_WM9093
	if (lge_audio_wm9093_platform.set_amp_path)
		lge_audio_wm9093_platform.set_amp_path(ICODEC_SPEAKER_PLAYBACK_RX);
#endif

	return 0;
}

static int msm_snddev_poweramp_on_spk_vzwnavi(void)
{
	pr_debug("%s\n", __func__);

#ifdef CONFIG_LGE_AUDIO_AMP_WM9093
	if (lge_audio_wm9093_platform.set_amp_path)
		lge_audio_wm9093_platform.set_amp_path(ICODEC_SPEAKER_PLAYBACK_VZWNAVI);
#endif

	return 0;
}
#ifdef CONFIG_LGE_AUDIO_AMP_WM9093
static int msm_snddev_poweramp_on_hph(void)
{
	pr_debug("%s\n", __func__);

	if (lge_audio_wm9093_platform.set_amp_path)
		lge_audio_wm9093_platform.set_amp_path(ICODEC_HEADSET_ST_RX);

	return 0;
}

static void msm_snddev_poweramp_off_hph(void)
{
	if (lge_audio_wm9093_platform.set_amp_path)
		lge_audio_wm9093_platform.set_amp_path(ICODEC_AMP_OFF);

	pr_debug("%s\n", __func__);
}

static int msm_snddev_poweramp_on_hph_playback(void)
{
	pr_debug("%s\n", __func__);

	if (lge_audio_wm9093_platform.set_amp_path)
		lge_audio_wm9093_platform.set_amp_path(ICODEC_HEADSET_ST_PLAYBACK_RX);

	return 0;
}

//LGE_UPDATE_S, jeremy.pi@lge.com, 2011-04-09, at&t simultaneously Ringing Headset and SPK
static int msm_snddev_poweramp_on_hph_spk(void)
{
	pr_debug("%s\n", __func__);

	if (lge_audio_wm9093_platform.set_amp_path)
		lge_audio_wm9093_platform.set_amp_path(ICODEC_HEADSET_ST_RX_SPEAKER_RX);

	return 0;
}

static void msm_snddev_poweramp_off_hph_spk(void)
{
	if (lge_audio_wm9093_platform.set_amp_path)
		lge_audio_wm9093_platform.set_amp_path(ICODEC_AMP_OFF);

	pr_debug("%s\n", __func__);
}
//LGE_UPDATE_E, jeremy.pi@lge.com, 2011-04-09, at&t simultaneously Ringing Headset and SPK

//LGE_UPDATE_S, jeremy.pi@lge.com, 2011-04-09, at&t TTY 
static int msm_snddev_poweramp_on_tty(void)
{
	pr_debug("%s\n", __func__);

	if (lge_audio_wm9093_platform.set_amp_path)
		lge_audio_wm9093_platform.set_amp_path(ICODEC_TTY_RX);

	return 0;
}

static void msm_snddev_poweramp_off_tty(void)
{
	if (lge_audio_wm9093_platform.set_amp_path)
		lge_audio_wm9093_platform.set_amp_path(ICODEC_AMP_OFF);

	pr_debug("%s\n", __func__);
}
//LGE_UPDATE_E, jeremy.pi@lge.com, 2011-04-09, at&t TTY 
#endif

/* Regulator 8058_l10 supplies regulator 8058_ncp. */
#ifndef CONFIG_LGE_AUDIO_NO_NCP_MODE
static struct regulator *snddev_reg_ncp;
#endif
static struct regulator *snddev_reg_l10;

static atomic_t preg_ref_cnt;

static int msm_snddev_voltage_on(void)
{
	int rc=0;
	pr_debug("%s\n", __func__);
#if 0
	if (atomic_inc_return(&preg_ref_cnt) > 1)
		return 0;

	snddev_reg_l10 = regulator_get(NULL, "8058_l10");
	if (IS_ERR(snddev_reg_l10)) {
		pr_err("%s: regulator_get(%s) failed (%ld)\n", __func__,
			"l10", PTR_ERR(snddev_reg_l10));
		return -EBUSY;
	}

	rc = regulator_set_voltage(snddev_reg_l10, 2600000, 2600000);
	if (rc < 0)
		pr_err("%s: regulator_set_voltage(l10) failed (%d)\n",
			__func__, rc);

	rc = regulator_enable(snddev_reg_l10);
	if (rc < 0)
		pr_err("%s: regulator_enable(l10) failed (%d)\n", __func__, rc);
#endif
#ifndef CONFIG_LGE_AUDIO_NO_NCP_MODE
	snddev_reg_ncp = regulator_get(NULL, "8058_ncp");
	if (IS_ERR(snddev_reg_ncp)) {
		pr_err("%s: regulator_get(%s) failed (%ld)\n", __func__,
			"ncp", PTR_ERR(snddev_reg_ncp));
		return -EBUSY;
	}

	rc = regulator_set_voltage(snddev_reg_ncp, 1800000, 1800000);
	if (rc < 0) {
		pr_err("%s: regulator_set_voltage(ncp) failed (%d)\n",
			__func__, rc);
		goto regulator_fail;
	}

	rc = regulator_enable(snddev_reg_ncp);
	if (rc < 0) {
		pr_err("%s: regulator_enable(ncp) failed (%d)\n", __func__, rc);
		goto regulator_fail;
	}

	return rc;

regulator_fail:
	regulator_put(snddev_reg_ncp);
	snddev_reg_ncp = NULL;
	return rc;
#else
	return rc;
#endif
}

static void msm_snddev_voltage_off(void)
{
	int rc = 0;
	rc = 0;
	pr_debug("%s\n", __func__);

#ifndef CONFIG_LGE_AUDIO_NO_NCP_MODE
	if (!snddev_reg_ncp)
		goto done;

	if (atomic_dec_return(&preg_ref_cnt) == 0) {
		rc = regulator_disable(snddev_reg_ncp);
		if (rc < 0)
			pr_err("%s: regulator_disable(ncp) failed (%d)\n",
				__func__, rc);
		regulator_put(snddev_reg_ncp);

		snddev_reg_ncp = NULL;
	}

done:
#endif
#if 0
	if (!snddev_reg_l10)
		return;

	rc = regulator_disable(snddev_reg_l10);
	if (rc < 0)
		pr_err("%s: regulator_disable(l10) failed (%d)\n",
			__func__, rc);

	regulator_put(snddev_reg_l10);
#endif
	snddev_reg_l10 = NULL;
}

static int msm_snddev_enable_amic_power(void)
{
/* sungwoo.cho@lge.com 2011. 01. 26 */
#ifdef CONFIG_LGE_AUDIO
	int ret;

#ifdef CONFIG_PMIC8058_OTHC
	ret = pm8058_micbias_enable(OTHC_MICBIAS_MAIN, OTHC_SIGNAL_ALWAYS_ON);

	if (ret)
		pr_err("%s: Enabling OTHC_MICBIAS_MAIN(%d) power failed\n", __func__, OTHC_MICBIAS_MAIN);
#endif
#if 0
	ret = gpio_request(GPIO_CAMCORDER_MIC_EN, "CAMCORDER_MIC_EN");
	if (ret) {
		pr_err("%s: camcorder mic en %d request failed\n",
			__func__, GPIO_CAMCORDER_MIC_EN);
		return ret;
	}
	gpio_direction_output(GPIO_CAMCORDER_MIC_EN, 1);
#endif	
#endif
	return ret;
/* sungwoo.cho@lge.com 2011. 01. 26 */
}

static void msm_snddev_disable_amic_power(void)
{
/* sungwoo.cho@lge.com 2011. 01. 26 */
#ifdef CONFIG_LGE_AUDIO
#if 0
	gpio_set_value_cansleep(GPIO_CAMCORDER_MIC_EN, 0);
	gpio_free(GPIO_CAMCORDER_MIC_EN);
#endif
#ifdef CONFIG_PMIC8058_OTHC
	{
		int ret;

		ret = pm8058_micbias_enable(OTHC_MICBIAS_MAIN, OTHC_SIGNAL_OFF);

		if (ret)
			pr_err("%s: Disabling OTHC_MICBIAS_MAIN(%d) power failed\n", __func__, OTHC_MICBIAS_MAIN);
	}
#endif
#endif
/* sungwoo.cho@lge.com 2011. 01. 26 */

}

#ifdef CONFIG_LGE_AUDIO

#else
static int msm_snddev_enable_rcv_power(void)
{
	int ret;

	ret = gpio_request(GPIO_RCV_AMP_RESET, "CAMCORDER_MIC_EN");
	if (ret) {
		pr_err("%s: rcv power en %d request failed\n",
			__func__, GPIO_RCV_AMP_RESET);
		return 0;
	}

	gpio_direction_output(GPIO_RCV_AMP_RESET, 1);

	return 0;
}

static void msm_snddev_disable_rcv_power(void)
{
	gpio_set_value_cansleep(GPIO_RCV_AMP_RESET, 0);

	gpio_free(GPIO_RCV_AMP_RESET);
}
#endif
static int msm_snddev_enable_adualmic_power(void)
{
	
#if defined(CONFIG_LGE_AUDIO)&&defined(CONFIG_PMIC8058_OTHC)
	int ret;
	ret = pm8058_micbias_enable(OTHC_MICBIAS_MAIN, OTHC_SIGNAL_ALWAYS_ON);
	if (ret)
		pr_err("%s: Enabling OTHC_MICBIAS_MAIN(%d) power failed\n", __func__, OTHC_MICBIAS_MAIN);
	ret = pm8058_micbias_enable(OTHC_MICBIAS_SUB, OTHC_SIGNAL_ALWAYS_ON);
	if (ret)
		pr_err("%s: Enabling OTHC_MICBIAS_SUB(%d) power failed\n", __func__, OTHC_MICBIAS_SUB);

#endif
	return 0;

}

static void msm_snddev_disable_adualmic_power(void)
{
#if defined(CONFIG_LGE_AUDIO)&&defined(CONFIG_PMIC8058_OTHC)
	int ret;

	ret = pm8058_micbias_enable(OTHC_MICBIAS_MAIN, OTHC_SIGNAL_OFF);
	if (ret)
		pr_err("%s: Disabling OTHC_MICBIAS_MAIN(%d) power failed\n", __func__, OTHC_MICBIAS_MAIN);
	ret = pm8058_micbias_enable(OTHC_MICBIAS_SUB, OTHC_SIGNAL_OFF);
	if (ret)
		pr_err("%s: Enabling OTHC_MICBIAS_SUB(%d) power failed\n", __func__, OTHC_MICBIAS_SUB);

#endif
}
#ifdef LGE_AUDIO_COMMENT_OUT_FOR_REFERENCE
static int msm_snddev_enable_anc_power(void)
{
	int ret = 0;
#ifdef CONFIG_PMIC8058_OTHC
	ret = pm8058_micbias_enable(OTHC_MICBIAS_2,
		OTHC_SIGNAL_ALWAYS_ON);
	if (ret)
		pr_err("%s: Enabling anc micbias 2 failed\n", __func__);

	if (machine_is_msm8x60_fluid()) {

		ret = pm8058_micbias_enable(OTHC_MICBIAS_0,
				OTHC_SIGNAL_ALWAYS_ON);
		if (ret)
			pr_err("%s: Enabling anc micbias 0 failed\n", __func__);

		ret = gpio_request(SNDDEV_GPIO_MIC2_ANCR_SEL, "MIC2_ANCR_SEL");
		if (ret) {
			pr_err("%s: mic2 ancr gpio %d request failed\n",
				__func__, SNDDEV_GPIO_MIC2_ANCR_SEL);
			return ret;
		}
		gpio_direction_output(SNDDEV_GPIO_MIC2_ANCR_SEL, 1);

		ret = gpio_request(SNDDEV_GPIO_MIC1_ANCL_SEL, "MIC1_ANCL_SEL");
		if (ret) {
			pr_err("%s: mic1 ancl gpio %d request failed\n",
				__func__, SNDDEV_GPIO_MIC1_ANCL_SEL);
			gpio_free(SNDDEV_GPIO_MIC2_ANCR_SEL);
			return ret;
		}
		gpio_direction_output(SNDDEV_GPIO_MIC1_ANCL_SEL, 1);

	}
#endif
	return ret;
}

static void msm_snddev_disable_anc_power(void)
{
#ifdef CONFIG_PMIC8058_OTHC
	int ret;

	ret = pm8058_micbias_enable(OTHC_MICBIAS_2, OTHC_SIGNAL_OFF);

	if (machine_is_msm8x60_fluid()) {
		ret |= pm8058_micbias_enable(OTHC_MICBIAS_0,
				OTHC_SIGNAL_OFF);
		gpio_free(SNDDEV_GPIO_MIC2_ANCR_SEL);
		gpio_free(SNDDEV_GPIO_MIC1_ANCL_SEL);
	}

	if (ret)
		pr_err("%s: Disabling anc power failed\n", __func__);
#endif
}

static int msm_snddev_enable_amic_sec_power(void)
{
#ifdef CONFIG_PMIC8058_OTHC
	int ret;

	if (machine_is_msm8x60_fluid()) {

		ret = pm8058_micbias_enable(OTHC_MICBIAS_2,
				OTHC_SIGNAL_ALWAYS_ON);
		if (ret)
			pr_err("%s: Enabling amic2 power failed\n", __func__);

		ret = gpio_request(SNDDEV_GPIO_HS_MIC4_SEL,
						"HS_MIC4_SEL");
		if (ret) {
			pr_err("%s: spkr pamp gpio %d request failed\n",
					__func__, SNDDEV_GPIO_HS_MIC4_SEL);
			return ret;
		}
		gpio_direction_output(SNDDEV_GPIO_HS_MIC4_SEL, 1);
	}
#endif

	msm_snddev_enable_amic_power();
	return 0;
}

static void msm_snddev_disable_amic_sec_power(void)
{
#ifdef CONFIG_PMIC8058_OTHC
	int ret;
	if (machine_is_msm8x60_fluid()) {

		ret = pm8058_micbias_enable(OTHC_MICBIAS_2,
					OTHC_SIGNAL_OFF);
		if (ret)
			pr_err("%s: Disabling amic2 power failed\n", __func__);

		gpio_free(SNDDEV_GPIO_HS_MIC4_SEL);
	}
#endif

	msm_snddev_disable_amic_power();
}

static int msm_snddev_enable_dmic_sec_power(void)
{
	int ret;

	ret = msm_snddev_enable_dmic_power();
	if (ret) {
		pr_err("%s: Error: Enabling dmic power failed\n", __func__);
		return ret;
	}
#ifdef CONFIG_PMIC8058_OTHC
	ret = pm8058_micbias_enable(OTHC_MICBIAS_2, OTHC_SIGNAL_ALWAYS_ON);
	if (ret) {
		pr_err("%s: Error: Enabling micbias failed\n", __func__);
		msm_snddev_disable_dmic_power();
		return ret;
	}
#endif
	return 0;
}

static void msm_snddev_disable_dmic_sec_power(void)
{
	msm_snddev_disable_dmic_power();

#ifdef CONFIG_PMIC8058_OTHC
	pm8058_micbias_enable(OTHC_MICBIAS_2, OTHC_SIGNAL_OFF);
#endif
}
#endif

static struct adie_codec_action_unit iearpiece_48KHz_osr256_actions[] =
	HANDSET_RX_MONO_8000_OSR_256;

static struct adie_codec_hwsetting_entry iearpiece_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = iearpiece_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(iearpiece_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile iearpiece_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = iearpiece_settings,
	.setting_sz = ARRAY_SIZE(iearpiece_settings),
};

static struct snddev_icodec_data snddev_iearpiece_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "handset_rx",
	.copp_id = 0,
	.profile = &iearpiece_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
};

static struct platform_device msm_iearpiece_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_iearpiece_data },
};

static struct adie_codec_action_unit imic_48KHz_osr256_actions[] =
	HANDSET_TX_PRI_MONO_OSR_256;

static struct adie_codec_hwsetting_entry imic_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = imic_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(imic_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile imic_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = imic_settings,
	.setting_sz = ARRAY_SIZE(imic_settings),
};

static struct snddev_icodec_data snddev_imic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_tx",
	.copp_id = 1,
	.profile = &imic_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_amic_power,
	.pamp_off = msm_snddev_disable_amic_power,
};

static struct platform_device msm_imic_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_imic_data },
};

#ifdef LGE_AUDIO_COMMENT_OUT_FOR_REFERENCE
static struct snddev_icodec_data snddev_fluid_ispkr_mic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_mono_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &imic_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_amic_power,
	.pamp_off = msm_snddev_disable_amic_power,
};

static struct platform_device msm_fluid_ispkr_mic_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_fluid_ispkr_mic_data },
};
#endif

static struct adie_codec_action_unit headset_ab_cpls_48KHz_osr256_actions[] =
#ifdef CONFIG_LGE_AUDIO_NO_NCP_MODE
	HEADSET_RX_STEREO_AB_LEG;
#else
	HEADSET_AB_CPLS_48000_OSR_256;
#endif

static struct adie_codec_hwsetting_entry headset_ab_cpls_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = headset_ab_cpls_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(headset_ab_cpls_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile headset_ab_cpls_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = headset_ab_cpls_settings,
	.setting_sz = ARRAY_SIZE(headset_ab_cpls_settings),
};

static struct snddev_icodec_data snddev_ihs_stereo_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_stereo_rx",
	.copp_id = 0,
	.profile = &headset_ab_cpls_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
/* LGE_CHANGE 2011. 01. 20 sungwoo.cho, dongsung.shin start */
#ifdef CONFIG_LGE_AUDIO_AMP_WM9093
	.pamp_on = msm_snddev_poweramp_on_hph,
	.pamp_off = msm_snddev_poweramp_off_hph,
#endif	
/* LGE_CHANGE 2011. 01. 20 sungwoo.cho, dongsung.shin end */
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
};

static struct platform_device msm_headset_stereo_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_ihs_stereo_rx_data },
};


/* define the value for Earjack Loopback, RX */
static struct adie_codec_action_unit ihs_stereo_acs_48KHz_osr256_actions[] =
#ifdef CONFIG_LGE_AUDIO_NO_NCP_MODE
	HEADSET_RX_STEREO_AB_LEG;
#else
	HEADSET_AB_CPLS_48000_OSR_256;
#endif
static struct adie_codec_hwsetting_entry ihs_stereo_acs_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ihs_stereo_acs_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ihs_stereo_acs_48KHz_osr256_actions),
	}
};
static struct adie_codec_dev_profile ihs_stereo_acs_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ihs_stereo_acs_rx_settings,
	.setting_sz = ARRAY_SIZE(ihs_stereo_acs_rx_settings),
};
static struct snddev_icodec_data snddev_ihs_stereo_acs_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_acs_rx",
	.copp_id = 0,
	.profile = &ihs_stereo_acs_rx_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
/* LGE_CHANGE 2011. 01. 20 sungwoo.cho, dongsung.shin start */
#ifdef CONFIG_LGE_AUDIO_AMP_WM9093
	.pamp_on = msm_snddev_poweramp_on_hph,
	.pamp_off = msm_snddev_poweramp_off_hph,
#endif	
/* LGE_CHANGE 2011. 01. 20 sungwoo.cho, dongsung.shin end */
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
};
static struct platform_device msm_headset_stereo_acs_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_ihs_stereo_acs_rx_data },
};


static struct adie_codec_action_unit ihs_stereo_playback_48KHz_osr256_actions[] =
#ifdef CONFIG_LGE_AUDIO_NO_NCP_MODE
	HEADSET_RX_STEREO_PLAYBACK_AB_LEG;
#else
	HEADSET_AB_CPLS_48000_OSR_256;
#endif

static struct adie_codec_hwsetting_entry ihs_stereo_playback_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ihs_stereo_playback_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ihs_stereo_playback_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile ihs_stereo_rx_playback_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ihs_stereo_playback_settings,
	.setting_sz = ARRAY_SIZE(ihs_stereo_playback_settings),
};

static struct snddev_icodec_data snddev_ihs_stereo_rx_playbackdata = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_stereo_rx_playback",
	.copp_id = 0,
	.profile = &ihs_stereo_rx_playback_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on_hph_playback,
	.pamp_off = msm_snddev_poweramp_off_hph,
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
};

static struct platform_device msm_headset_stereo_playback_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_ihs_stereo_rx_playbackdata },
};

#ifdef LGE_AUDIO_COMMENT_OUT_FOR_REFERENCE
static struct adie_codec_action_unit headset_anc_48KHz_osr256_actions[] =
	ANC_HEADSET_CPLS_AMIC1_AUXL_RX1_48000_OSR_256;

static struct adie_codec_hwsetting_entry headset_anc_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = headset_anc_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(headset_anc_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile headset_anc_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = headset_anc_settings,
	.setting_sz = ARRAY_SIZE(headset_anc_settings),
};

static struct snddev_icodec_data snddev_anc_headset_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE | SNDDEV_CAP_ANC),
	.name = "anc_headset_stereo_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &headset_anc_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_anc_power,
	.pamp_off = msm_snddev_disable_anc_power,
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
};

static struct platform_device msm_anc_headset_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_anc_headset_data },
};
#endif

static struct adie_codec_action_unit ispkr_stereo_48KHz_osr256_actions[] =
	SPEAKER_RX_STEREO_48000_OSR_256;

static struct adie_codec_hwsetting_entry ispkr_stereo_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ispkr_stereo_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ispkr_stereo_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile ispkr_stereo_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ispkr_stereo_settings,
	.setting_sz = ARRAY_SIZE(ispkr_stereo_settings),
};

static struct snddev_icodec_data snddev_ispkr_stereo_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "speaker_stereo_rx",
	.copp_id = 0,
	.profile = &ispkr_stereo_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on_spk,
	.pamp_off = msm_snddev_poweramp_off_spk,
/* sungwoo.cho@lge.com 2011. 01. 26 */
#ifdef CONFIG_LGE_AUDIO_AMP_WM9093
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
#endif
/* sungwoo.cho@lge.com 2011. 01. 26 */
};

static struct platform_device msm_ispkr_stereo_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_ispkr_stereo_data },
};

static struct adie_codec_action_unit ispkr_stereo_playback_48KHz_osr256_actions[] =
	SPEAKER_RX_STEREO_PLAYBACK_48000_OSR_256;

static struct adie_codec_hwsetting_entry ispkr_stereo_playback_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ispkr_stereo_playback_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ispkr_stereo_playback_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile ispkr_stereo_playback_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ispkr_stereo_playback_settings,
	.setting_sz = ARRAY_SIZE(ispkr_stereo_playback_settings),
};

static struct snddev_icodec_data snddev_ispkr_stereo_playback_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "speaker_stereo_rx_playback",
	.copp_id = 0,
	.profile = &ispkr_stereo_playback_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on_spk_playback,
	.pamp_off = msm_snddev_poweramp_off_spk,
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
};

static struct platform_device msm_ispkr_stereo_playback_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_ispkr_stereo_playback_data },
};
/* start - add VZW navi Device*/
static struct snddev_icodec_data snddev_ispkr_stereo_vzwnavi_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "speaker_stereo_rx_vzwnavi",
	.copp_id = 0,
	.profile = &ispkr_stereo_playback_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on_spk_vzwnavi,
	.pamp_off = msm_snddev_poweramp_off_spk,
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
};

static struct platform_device msm_ispkr_stereo_vzwnavi_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_ispkr_stereo_vzwnavi_data },
};
/* end - add VZW navi Device*/

#ifdef CONFIG_LGE_AUDIO

static int msm_snddev_enable_dualmic_power(void)
{
	int ret;

	ret = pm8058_micbias_enable(OTHC_MICBIAS_SUB, OTHC_SIGNAL_ALWAYS_ON);

	if (ret)
		pr_err("%s: Enabling OTHC_MICBIAS_SUB(%d) power failed\n", __func__, OTHC_MICBIAS_SUB);

	ret = pm8058_micbias_enable(OTHC_MICBIAS_MAIN, OTHC_SIGNAL_ALWAYS_ON);

	if (ret)
		pr_err("%s: Enabling OTHC_MICBIAS_MAIN(%d) power failed\n", __func__, OTHC_MICBIAS_MAIN);

	return ret;
}

static void msm_snddev_disable_dualmic_power(void)
{
	int ret;

	ret = pm8058_micbias_enable(OTHC_MICBIAS_SUB, OTHC_SIGNAL_OFF);

	if (ret)
		pr_err("%s: Disabling OTHC_MICBIAS_SUB(%d) power failed\n", __func__, OTHC_MICBIAS_SUB);

	ret = pm8058_micbias_enable(OTHC_MICBIAS_MAIN, OTHC_SIGNAL_OFF);

	if (ret)
		pr_err("%s: Disabling OTHC_MICBIAS_MAIN(%d) power failed\n", __func__, OTHC_MICBIAS_MAIN);
}

#ifdef CONFIG_LGE_AUDIO_USE_AUXMIC_FOR_SPEAKER_MODE
static int msm_snddev_enable_submic_power(void)
{
	int ret;

	ret = pm8058_micbias_enable(OTHC_MICBIAS_SUB, OTHC_SIGNAL_ALWAYS_ON);

	if (ret)
		pr_err("%s: Enabling OTHC_MICBIAS_SUB(%d) power failed\n", __func__, OTHC_MICBIAS_SUB);

	return ret;
}

static void msm_snddev_disable_submic_power(void)
{
	int ret;

	ret = pm8058_micbias_enable(OTHC_MICBIAS_SUB, OTHC_SIGNAL_OFF);

	if (ret)
		pr_err("%s: Disabling OTHC_MICBIAS_SUB(%d) power failed\n", __func__, OTHC_MICBIAS_SUB);
}

static struct adie_codec_action_unit submic_48KHz_osr256_actions[] =
//	AMIC_SEC_MONO_OSR_256;
	AUXIN_MONO_OSR_256;

static struct adie_codec_hwsetting_entry submic_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = submic_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(submic_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile submic_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = submic_settings,
	.setting_sz = ARRAY_SIZE(submic_settings),
};
#endif

static struct snddev_icodec_data snddev_ispkr_mic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_mono_tx",
	.copp_id = 1,
#ifdef CONFIG_LGE_AUDIO_USE_AUXMIC_FOR_SPEAKER_MODE
	.profile = &submic_profile,
#else
	.profile = &imic_profile,
#endif
	.channel_mode = 1,
	.default_sample_rate = 48000,
#ifdef CONFIG_LGE_AUDIO_USE_AUXMIC_FOR_SPEAKER_MODE
	.pamp_on = msm_snddev_enable_submic_power,
	.pamp_off = msm_snddev_disable_submic_power,
#else
	.pamp_on = msm_snddev_enable_amic_power,
	.pamp_off = msm_snddev_disable_amic_power,
#endif
};

#else

static struct adie_codec_action_unit ispkr_mic_48KHz_osr256_actions[] =
	SPEAKER_TX_PRI_MONO_OSR_256;

static struct adie_codec_hwsetting_entry ispkr_mic_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ispkr_mic_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ispkr_mic_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile ispkr_mic_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ispkr_mic_settings,
	.setting_sz = ARRAY_SIZE(ispkr_mic_settings),
};

static struct snddev_icodec_data snddev_ispkr_mic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_mono_tx",
	.copp_id = 1,
	.profile = &ispkr_mic_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_amic_power,
	.pamp_off = msm_snddev_disable_amic_power,
};

#endif

static struct platform_device msm_ispkr_mic_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_ispkr_mic_data },
};

static struct adie_codec_action_unit dual_mic_endfire_8KHz_osr256_actions[] =
	AMIC_DUAL_8000_OSR_256;

static struct adie_codec_hwsetting_entry dual_mic_endfire_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = dual_mic_endfire_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(dual_mic_endfire_8KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile dual_mic_endfire_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = dual_mic_endfire_settings,
	.setting_sz = ARRAY_SIZE(dual_mic_endfire_settings),
};

static struct snddev_icodec_data snddev_dual_mic_endfire_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_dual_mic_endfire_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &dual_mic_endfire_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
#ifdef CONFIG_LGE_AUDIO
	.pamp_on = msm_snddev_enable_adualmic_power,
	.pamp_off = msm_snddev_disable_adualmic_power,
#else
	.pamp_on = msm_snddev_enable_dmic_power,
	.pamp_off = msm_snddev_disable_dmic_power,
#endif	
};

static struct platform_device msm_hs_dual_mic_endfire_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_dual_mic_endfire_data },
};

#ifdef LGE_AUDIO_COMMENT_OUT_FOR_REFERENCE
static struct adie_codec_action_unit iearpiece_ffa_48KHz_osr256_actions[] =
	EAR_PRI_MONO_8000_OSR_256;

static struct adie_codec_hwsetting_entry iearpiece_ffa_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = iearpiece_ffa_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(iearpiece_ffa_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile iearpiece_ffa_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = iearpiece_ffa_settings,
	.setting_sz = ARRAY_SIZE(iearpiece_ffa_settings),
};

static struct snddev_icodec_data snddev_iearpiece_ffa_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "handset_rx",
	.copp_id = 0,
	.profile = &iearpiece_ffa_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
};

static struct platform_device msm_iearpiece_ffa_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_iearpiece_ffa_data },
};

static struct snddev_icodec_data snddev_imic_ffa_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &idmic_mono_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_power,
	.pamp_off = msm_snddev_disable_dmic_power,
};

static struct platform_device msm_imic_ffa_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_imic_ffa_data },
};

static struct adie_codec_action_unit dual_mic_endfire_8KHz_osr256_actions[] =
	DMIC1_PRI_STEREO_OSR_256;

static struct adie_codec_hwsetting_entry dual_mic_endfire_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = dual_mic_endfire_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(dual_mic_endfire_8KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile dual_mic_endfire_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = dual_mic_endfire_settings,
	.setting_sz = ARRAY_SIZE(dual_mic_endfire_settings),
};

static struct snddev_icodec_data snddev_dual_mic_endfire_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_dual_mic_endfire_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &dual_mic_endfire_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_power,
	.pamp_off = msm_snddev_disable_dmic_power,
};

static struct platform_device msm_hs_dual_mic_endfire_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_dual_mic_endfire_data },
};

static struct snddev_icodec_data snddev_dual_mic_spkr_endfire_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_dual_mic_endfire_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &dual_mic_endfire_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_power,
	.pamp_off = msm_snddev_disable_dmic_power,
};

static struct platform_device msm_spkr_dual_mic_endfire_device = {
	.name = "snddev_icodec",
	.id = 15,
	.dev = { .platform_data = &snddev_dual_mic_spkr_endfire_data },
};

static struct adie_codec_action_unit dual_mic_broadside_8osr256_actions[] =
	HS_DMIC2_STEREO_OSR_256;

static struct adie_codec_hwsetting_entry dual_mic_broadside_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = dual_mic_broadside_8osr256_actions,
		.action_sz = ARRAY_SIZE(dual_mic_broadside_8osr256_actions),
	}
};

static struct adie_codec_dev_profile dual_mic_broadside_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = dual_mic_broadside_settings,
	.setting_sz = ARRAY_SIZE(dual_mic_broadside_settings),
};

static struct snddev_icodec_data snddev_hs_dual_mic_broadside_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_dual_mic_broadside_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &dual_mic_broadside_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_sec_power,
	.pamp_off = msm_snddev_disable_dmic_sec_power,
};

static struct platform_device msm_hs_dual_mic_broadside_device = {
	.name = "snddev_icodec",
	.id = 21,
	.dev = { .platform_data = &snddev_hs_dual_mic_broadside_data },
};

static struct snddev_icodec_data snddev_spkr_dual_mic_broadside_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_dual_mic_broadside_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &dual_mic_broadside_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_sec_power,
	.pamp_off = msm_snddev_disable_dmic_sec_power,
};

static struct platform_device msm_spkr_dual_mic_broadside_device = {
	.name = "snddev_icodec",
	.id = 18,
	.dev = { .platform_data = &snddev_spkr_dual_mic_broadside_data },
};

static struct adie_codec_action_unit
		fluid_dual_mic_endfire_8KHz_osr256_actions[] =
	FLUID_AMIC_DUAL_8000_OSR_256;

static struct adie_codec_hwsetting_entry fluid_dual_mic_endfire_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = fluid_dual_mic_endfire_8KHz_osr256_actions,
		.action_sz =
			ARRAY_SIZE(fluid_dual_mic_endfire_8KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile fluid_dual_mic_endfire_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = fluid_dual_mic_endfire_settings,
	.setting_sz = ARRAY_SIZE(fluid_dual_mic_endfire_settings),
};

static struct snddev_icodec_data snddev_fluid_dual_mic_endfire_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_dual_mic_endfire_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &fluid_dual_mic_endfire_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_amic_sec_power,
	.pamp_off = msm_snddev_disable_amic_sec_power,
};

static struct platform_device msm_fluid_hs_dual_mic_endfire_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_fluid_dual_mic_endfire_data },
};

static struct snddev_icodec_data snddev_fluid_dual_mic_spkr_endfire_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_dual_mic_endfire_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &fluid_dual_mic_endfire_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_amic_sec_power,
	.pamp_off = msm_snddev_disable_amic_sec_power,
};

static struct platform_device msm_fluid_spkr_dual_mic_endfire_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_fluid_dual_mic_spkr_endfire_data },
};

static struct adie_codec_action_unit
		fluid_dual_mic_broadside_8KHz_osr256_actions[] =
	FLUID_AMIC_DUAL_BROADSIDE_8000_OSR_256;

static struct adie_codec_hwsetting_entry fluid_dual_mic_broadside_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = fluid_dual_mic_broadside_8KHz_osr256_actions,
		.action_sz =
		ARRAY_SIZE(fluid_dual_mic_broadside_8KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile fluid_dual_mic_broadside_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = fluid_dual_mic_broadside_settings,
	.setting_sz = ARRAY_SIZE(fluid_dual_mic_broadside_settings),
};

static struct snddev_icodec_data snddev_fluid_dual_mic_broadside_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_dual_mic_broadside_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &fluid_dual_mic_broadside_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_amic_power,
	.pamp_off = msm_snddev_disable_amic_power,
};

static struct platform_device msm_fluid_hs_dual_mic_broadside_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_fluid_dual_mic_broadside_data },
};

static struct snddev_icodec_data snddev_fluid_dual_mic_spkr_broadside_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_dual_mic_broadside_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &fluid_dual_mic_broadside_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_amic_power,
	.pamp_off = msm_snddev_disable_amic_power,
};

static struct platform_device msm_fluid_spkr_dual_mic_broadside_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_fluid_dual_mic_spkr_broadside_data },
};
#endif

static struct snddev_hdmi_data snddev_hdmi_stereo_rx_data = {
	.capability = SNDDEV_CAP_RX ,
	.name = "hdmi_stereo_rx",
	.copp_id = HDMI_RX,
	.channel_mode = 0,
	.default_sample_rate = 48000,
};

static struct platform_device msm_snddev_hdmi_stereo_rx_device = {
	.name = "snddev_hdmi",
	.dev = { .platform_data = &snddev_hdmi_stereo_rx_data },
};

static struct snddev_mi2s_data snddev_mi2s_fm_tx_data = {
	.capability = SNDDEV_CAP_TX ,
	.name = "fmradio_stereo_tx",
	.copp_id = MI2S_TX,
	.channel_mode = 2, /* stereo */
	.sd_lines = MI2S_SD3, /* sd3 */
	.sample_rate = 48000,
};

static struct platform_device msm_mi2s_fm_tx_device = {
	.name = "snddev_mi2s",
	.dev = { .platform_data = &snddev_mi2s_fm_tx_data },
};

static struct snddev_mi2s_data snddev_mi2s_fm_rx_data = {
	.capability = SNDDEV_CAP_RX ,
	.name = "fmradio_stereo_rx",
	.copp_id = MI2S_RX,
	.channel_mode = 2, /* stereo */
	.sd_lines = MI2S_SD3, /* sd3 */
	.sample_rate = 48000,
};

static struct platform_device msm_mi2s_fm_rx_device = {
	.name = "snddev_mi2s",
	.id = 1,
	.dev = { .platform_data = &snddev_mi2s_fm_rx_data },
};

static struct adie_codec_action_unit iheadset_mic_tx_osr256_actions[] =
	HEADSET_TX_MONO_PRI_OSR_256;

static struct adie_codec_hwsetting_entry iheadset_mic_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = iheadset_mic_tx_osr256_actions,
		.action_sz = ARRAY_SIZE(iheadset_mic_tx_osr256_actions),
	}
};

static struct adie_codec_dev_profile iheadset_mic_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = iheadset_mic_tx_settings,
	.setting_sz = ARRAY_SIZE(iheadset_mic_tx_settings),
};

static struct snddev_icodec_data snddev_headset_mic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "headset_mono_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &iheadset_mic_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
};

static struct platform_device msm_headset_mic_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_headset_mic_data },
};

/* Voice memo mic for headset, TX */
static struct adie_codec_action_unit iheadset_mic_vm_tx_osr256_actions[] =
	HEADSET_TX_MONO_PRI_OSR_256_VM;

static struct adie_codec_hwsetting_entry iheadset_mic_vm_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = iheadset_mic_vm_tx_osr256_actions,
		.action_sz = ARRAY_SIZE(iheadset_mic_vm_tx_osr256_actions),
	}
};

static struct adie_codec_dev_profile iheadset_mic_vm_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = iheadset_mic_vm_tx_settings,
	.setting_sz = ARRAY_SIZE(iheadset_mic_vm_tx_settings),
};

static struct snddev_icodec_data snddev_headset_mic_vm_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "headset_lgvm_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &iheadset_mic_vm_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
};

static struct platform_device msm_headset_mic_vm_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_headset_mic_vm_data },
};
static struct adie_codec_action_unit iheadset_mic_record_tx_osr256_actions[] =
	HEADSET_RECORD_TX_MONO_PRI_OSR_256;

static struct adie_codec_hwsetting_entry iheadset_mic_record_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = iheadset_mic_record_tx_osr256_actions,
		.action_sz = ARRAY_SIZE(iheadset_mic_record_tx_osr256_actions),
	}
};

static struct adie_codec_dev_profile iheadset_mic_record_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = iheadset_mic_record_tx_settings,
	.setting_sz = ARRAY_SIZE(iheadset_mic_record_tx_settings),
};

static struct snddev_icodec_data snddev_headset_mic_record_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "headset_record_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &iheadset_mic_record_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
};

static struct platform_device msm_headset_mic_record_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_headset_mic_record_data },
};
/* define the value for Earjack Loopback, TX */
static struct adie_codec_action_unit iheadset_mic_acs_tx_osr256_actions[] =
	HEADSET_TX_MONO_PRI_OSR_256;
static struct adie_codec_hwsetting_entry iheadset_mic_acs_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = iheadset_mic_acs_tx_osr256_actions,
		.action_sz = ARRAY_SIZE(iheadset_mic_acs_tx_osr256_actions),
	}
};
static struct adie_codec_dev_profile iheadset_mic_acs_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = iheadset_mic_acs_tx_settings,
	.setting_sz = ARRAY_SIZE(iheadset_mic_acs_tx_settings),
};
static struct snddev_icodec_data snddev_headset_mic_acs_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "headset_acs_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &iheadset_mic_acs_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
};
static struct platform_device msm_headset_mic_acs_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_headset_mic_acs_data },
};



static struct adie_codec_action_unit
	ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions[] =
#ifdef CONFIG_LGE_AUDIO_NO_NCP_MODE
    HPH_SPEAKER_RX_STEREO_AB_LEG;
#else
	SPEAKER_HPH_AB_CPL_PRI_STEREO_48000_OSR_256;
#endif /* CONFIG_LGE_AUDIO_NO_NCP_MODE */

static struct adie_codec_hwsetting_entry
	ihs_stereo_speaker_stereo_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions,
		.action_sz =
		ARRAY_SIZE(ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile ihs_stereo_speaker_stereo_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ihs_stereo_speaker_stereo_rx_settings,
	.setting_sz = ARRAY_SIZE(ihs_stereo_speaker_stereo_rx_settings),
};

static struct snddev_icodec_data snddev_ihs_stereo_speaker_stereo_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_stereo_speaker_stereo_rx",
	.copp_id = 0,
	.profile = &ihs_stereo_speaker_stereo_rx_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on_hph_spk,
	.pamp_off = msm_snddev_poweramp_off_hph_spk,
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
};

static struct platform_device msm_ihs_stereo_speaker_stereo_rx_device = {
	.name = "snddev_icodec",
	.id = 22,
	.dev = { .platform_data = &snddev_ihs_stereo_speaker_stereo_rx_data },
};

/* define the value for BT_SCO */

static struct snddev_ecodec_data snddev_bt_sco_earpiece_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "bt_sco_rx",
	.copp_id = PCM_RX,
	.channel_mode = 1,
};

static struct snddev_ecodec_data snddev_bt_sco_mic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "bt_sco_tx",
	.copp_id = PCM_TX,
	.channel_mode = 1,
};

struct platform_device msm_bt_sco_earpiece_device = {
	.name = "msm_snddev_ecodec",
	.dev = { .platform_data = &snddev_bt_sco_earpiece_data },
};

struct platform_device msm_bt_sco_mic_device = {
	.name = "msm_snddev_ecodec",
	.dev = { .platform_data = &snddev_bt_sco_mic_data },
};


/* define the value for BT_SCO BTTM(AT command BT Test Mode) */
//BTTM_RX BTTM_TX

static struct snddev_ecodec_data snddev_bt_sco_bttm_earpiece_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "bt_sco_bttm_rx",
	.copp_id = PCM_RX,
	.channel_mode = 1,
};

static struct snddev_ecodec_data snddev_bt_sco_bttm_mic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "bt_sco_bttm_tx",
	.copp_id = PCM_TX,
	.channel_mode = 1,
};

struct platform_device msm_bt_sco_bttm_earpiece_device = {
	.name = "msm_snddev_ecodec",
	.dev = { .platform_data = &snddev_bt_sco_bttm_earpiece_data },
};

struct platform_device msm_bt_sco_bttm_mic_device = {
	.name = "msm_snddev_ecodec",
	.dev = { .platform_data = &snddev_bt_sco_bttm_mic_data },
};

static struct adie_codec_action_unit itty_mic_tx_osr256_actions[] =
	TTY_HEADSET_TX_MONO_OSR_256;

static struct adie_codec_hwsetting_entry itty_mono_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = itty_mic_tx_osr256_actions,
		.action_sz = ARRAY_SIZE(itty_mic_tx_osr256_actions),
	},
};

static struct adie_codec_dev_profile itty_mono_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = itty_mono_tx_settings,
	.setting_sz = ARRAY_SIZE(itty_mono_tx_settings),
};

static struct snddev_icodec_data snddev_itty_mono_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE | SNDDEV_CAP_TTY),
	.name = "tty_headset_mono_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &itty_mono_tx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
};

static struct platform_device msm_itty_mono_tx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_itty_mono_tx_data },
};

static struct adie_codec_action_unit itty_stereo_48KHz_osr256_actions[] =
	TTY_HEADSET_RX_STEREO_AB_LEG;

static struct adie_codec_hwsetting_entry itty_mono_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = itty_stereo_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(itty_stereo_48KHz_osr256_actions),
	},
};

static struct adie_codec_dev_profile itty_mono_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = itty_mono_rx_settings,
	.setting_sz = ARRAY_SIZE(itty_mono_rx_settings),
};

static struct snddev_icodec_data snddev_itty_mono_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE | SNDDEV_CAP_TTY),
	.name = "tty_headset_mono_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &itty_mono_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on_tty,
	.pamp_off = msm_snddev_poweramp_off_tty,	
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
};

static struct platform_device msm_itty_mono_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_itty_mono_rx_data },
};

static struct adie_codec_action_unit linein_pri_actions[] =
	LINEIN_PRI_STEREO_OSR_256;

static struct adie_codec_hwsetting_entry linein_pri_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = linein_pri_actions,
		.action_sz = ARRAY_SIZE(linein_pri_actions),
	},
};

static struct adie_codec_dev_profile linein_pri_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = linein_pri_settings,
	.setting_sz = ARRAY_SIZE(linein_pri_settings),
};

static struct snddev_icodec_data snddev_linein_pri_data = {
	.capability = SNDDEV_CAP_TX,
	.name = "linein_pri_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &linein_pri_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
};

static struct platform_device msm_linein_pri_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_linein_pri_data },
};

static struct adie_codec_action_unit auxpga_lp_lo_actions[] =
	LB_AUXPGA_LO_STEREO;

static struct adie_codec_hwsetting_entry auxpga_lp_lo_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = auxpga_lp_lo_actions,
		.action_sz = ARRAY_SIZE(auxpga_lp_lo_actions),
	},
};

static struct adie_codec_dev_profile auxpga_lp_lo_profile = {
	.path_type = ADIE_CODEC_LB,
	.settings = auxpga_lp_lo_settings,
	.setting_sz = ARRAY_SIZE(auxpga_lp_lo_settings),
};

static struct snddev_icodec_data snddev_auxpga_lp_lo_data = {
	.capability = SNDDEV_CAP_LB,
	.name = "speaker_stereo_lb",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &auxpga_lp_lo_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on_spk,
	.pamp_off = msm_snddev_poweramp_off_spk,
	.dev_vol_type = SNDDEV_DEV_VOL_ANALOG,
};

static struct platform_device msm_auxpga_lp_lo_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_auxpga_lp_lo_data },
};

static struct adie_codec_action_unit auxpga_lp_hs_actions[] =
	LB_AUXPGA_HPH_AB_CPLS_STEREO;

static struct adie_codec_hwsetting_entry auxpga_lp_hs_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = auxpga_lp_hs_actions,
		.action_sz = ARRAY_SIZE(auxpga_lp_hs_actions),
	},
};

static struct adie_codec_dev_profile auxpga_lp_hs_profile = {
	.path_type = ADIE_CODEC_LB,
	.settings = auxpga_lp_hs_settings,
	.setting_sz = ARRAY_SIZE(auxpga_lp_hs_settings),
};

static struct snddev_icodec_data snddev_auxpga_lp_hs_data = {
	.capability = SNDDEV_CAP_LB,
	.name = "hs_stereo_lb",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &auxpga_lp_hs_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
	.dev_vol_type = SNDDEV_DEV_VOL_ANALOG,
};

static struct platform_device msm_auxpga_lp_hs_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_auxpga_lp_hs_data },
};

#ifdef CONFIG_MSM8X60_FTM_AUDIO_DEVICES
static struct adie_codec_action_unit ftm_headset_mono_rx_actions[] =
	HPH_PRI_AB_CPLS_MONO;

static struct adie_codec_hwsetting_entry ftm_headset_mono_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_headset_mono_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_headset_mono_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_headset_mono_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_headset_mono_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_headset_mono_rx_settings),
};

static struct snddev_icodec_data ftm_headset_mono_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_headset_mono_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_headset_mono_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_headset_mono_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_headset_mono_rx_data},
};

static struct adie_codec_action_unit ftm_headset_mono_diff_rx_actions[] =
	HEADSET_MONO_DIFF_RX;

static struct adie_codec_hwsetting_entry ftm_headset_mono_diff_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_headset_mono_diff_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_headset_mono_diff_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_headset_mono_diff_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_headset_mono_diff_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_headset_mono_diff_rx_settings),
};

static struct snddev_icodec_data ftm_headset_mono_diff_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_headset_mono_diff_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_headset_mono_diff_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_headset_mono_diff_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_headset_mono_diff_rx_data},
};

static struct adie_codec_action_unit ftm_spkr_mono_rx_actions[] =
	SPEAKER_PRI_STEREO_48000_OSR_256;

static struct adie_codec_hwsetting_entry ftm_spkr_mono_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_spkr_mono_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_spkr_mono_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_spkr_mono_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_spkr_mono_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_spkr_mono_rx_settings),
};

static struct snddev_icodec_data ftm_spkr_mono_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_spkr_mono_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_spkr_mono_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on,
	.pamp_off = msm_snddev_poweramp_off,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_spkr_mono_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_spkr_mono_rx_data},
};

static struct adie_codec_action_unit ftm_spkr_l_rx_actions[] =
	FTM_SPKR_L_RX;

static struct adie_codec_hwsetting_entry ftm_spkr_l_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_spkr_l_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_spkr_l_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_spkr_l_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_spkr_l_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_spkr_l_rx_settings),
};

static struct snddev_icodec_data ftm_spkr_l_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_spkr_l_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_spkr_l_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on,
	.pamp_off = msm_snddev_poweramp_off,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_spkr_l_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_spkr_l_rx_data},
};

static struct adie_codec_action_unit ftm_spkr_r_rx_actions[] =
	SPKR_R_RX;

static struct adie_codec_hwsetting_entry ftm_spkr_r_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_spkr_r_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_spkr_r_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_spkr_r_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_spkr_r_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_spkr_r_rx_settings),
};

static struct snddev_icodec_data ftm_spkr_r_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_spkr_r_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_spkr_r_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on,
	.pamp_off = msm_snddev_poweramp_off,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_spkr_r_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_spkr_r_rx_data},
};

static struct adie_codec_action_unit ftm_spkr_mono_diff_rx_actions[] =
	SPKR_MONO_DIFF_RX;

static struct adie_codec_hwsetting_entry ftm_spkr_mono_diff_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_spkr_mono_diff_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_spkr_mono_diff_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_spkr_mono_diff_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_spkr_mono_diff_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_spkr_mono_diff_rx_settings),
};

static struct snddev_icodec_data ftm_spkr_mono_diff_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_spkr_mono_diff_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_spkr_mono_diff_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on,
	.pamp_off = msm_snddev_poweramp_off,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_spkr_mono_diff_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_spkr_mono_diff_rx_data},
};

static struct adie_codec_action_unit ftm_headset_mono_l_rx_actions[] =
	HPH_PRI_AB_CPLS_MONO_LEFT;

static struct adie_codec_hwsetting_entry ftm_headset_mono_l_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_headset_mono_l_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_headset_mono_l_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_headset_mono_l_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_headset_mono_l_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_headset_mono_l_rx_settings),
};

static struct snddev_icodec_data ftm_headset_mono_l_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_headset_mono_l_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_headset_mono_l_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_headset_mono_l_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_headset_mono_l_rx_data},
};

static struct adie_codec_action_unit ftm_headset_mono_r_rx_actions[] =
	HPH_PRI_AB_CPLS_MONO_RIGHT;

static struct adie_codec_hwsetting_entry ftm_headset_mono_r_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_headset_mono_r_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_headset_mono_r_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_headset_mono_r_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_headset_mono_r_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_headset_mono_r_rx_settings),
};

static struct snddev_icodec_data ftm_headset_mono_r_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_headset_mono_r_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_headset_mono_r_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_headset_mono_r_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_headset_mono_r_rx_data},
};

static struct adie_codec_action_unit ftm_linein_l_tx_actions[] =
	LINEIN_MONO_L_TX;

static struct adie_codec_hwsetting_entry ftm_linein_l_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_linein_l_tx_actions,
		.action_sz = ARRAY_SIZE(ftm_linein_l_tx_actions),
	},
};

static struct adie_codec_dev_profile ftm_linein_l_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ftm_linein_l_tx_settings,
	.setting_sz = ARRAY_SIZE(ftm_linein_l_tx_settings),
};

static struct snddev_icodec_data ftm_linein_l_tx_data = {
	.capability = SNDDEV_CAP_TX,
	.name = "ftm_linein_l_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &ftm_linein_l_tx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_linein_l_tx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_linein_l_tx_data },
};

static struct adie_codec_action_unit ftm_linein_r_tx_actions[] =
	LINEIN_MONO_R_TX;

static struct adie_codec_hwsetting_entry ftm_linein_r_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_linein_r_tx_actions,
		.action_sz = ARRAY_SIZE(ftm_linein_r_tx_actions),
	},
};

static struct adie_codec_dev_profile ftm_linein_r_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ftm_linein_r_tx_settings,
	.setting_sz = ARRAY_SIZE(ftm_linein_r_tx_settings),
};

static struct snddev_icodec_data ftm_linein_r_tx_data = {
	.capability = SNDDEV_CAP_TX,
	.name = "ftm_linein_r_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &ftm_linein_r_tx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_linein_r_tx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_linein_r_tx_data },
};

static struct adie_codec_action_unit ftm_aux_out_rx_actions[] =
	AUX_OUT_RX;

static struct adie_codec_hwsetting_entry ftm_aux_out_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_aux_out_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_aux_out_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_aux_out_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_aux_out_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_aux_out_rx_settings),
};

static struct snddev_icodec_data ftm_aux_out_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_aux_out_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_aux_out_rx_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_aux_out_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_aux_out_rx_data},
};

static struct adie_codec_action_unit ftm_dmic1_left_tx_actions[] =
	DMIC1_LEFT_TX;

static struct adie_codec_hwsetting_entry ftm_dmic1_left_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_dmic1_left_tx_actions,
		.action_sz = ARRAY_SIZE(ftm_dmic1_left_tx_actions),
	},
};

static struct adie_codec_dev_profile ftm_dmic1_left_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ftm_dmic1_left_tx_settings,
	.setting_sz = ARRAY_SIZE(ftm_dmic1_left_tx_settings),
};

static struct snddev_icodec_data ftm_dmic1_left_tx_data = {
	.capability = SNDDEV_CAP_TX,
	.name = "ftm_dmic1_left_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &ftm_dmic1_left_tx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_power,
	.pamp_off = msm_snddev_disable_dmic_power,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_dmic1_left_tx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_dmic1_left_tx_data},
};

static struct adie_codec_action_unit ftm_dmic1_right_tx_actions[] =
	DMIC1_RIGHT_TX;

static struct adie_codec_hwsetting_entry ftm_dmic1_right_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_dmic1_right_tx_actions,
		.action_sz = ARRAY_SIZE(ftm_dmic1_right_tx_actions),
	},
};

static struct adie_codec_dev_profile ftm_dmic1_right_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ftm_dmic1_right_tx_settings,
	.setting_sz = ARRAY_SIZE(ftm_dmic1_right_tx_settings),
};

static struct snddev_icodec_data ftm_dmic1_right_tx_data = {
	.capability = SNDDEV_CAP_TX,
	.name = "ftm_dmic1_right_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &ftm_dmic1_right_tx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_power,
	.pamp_off = msm_snddev_disable_dmic_power,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_dmic1_right_tx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_dmic1_right_tx_data},
};

static struct adie_codec_action_unit ftm_dmic1_l_and_r_tx_actions[] =
	DMIC1_LEFT_AND_RIGHT_TX;

static struct adie_codec_hwsetting_entry ftm_dmic1_l_and_r_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_dmic1_l_and_r_tx_actions,
		.action_sz = ARRAY_SIZE(ftm_dmic1_l_and_r_tx_actions),
	},
};

static struct adie_codec_dev_profile ftm_dmic1_l_and_r_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ftm_dmic1_l_and_r_tx_settings,
	.setting_sz = ARRAY_SIZE(ftm_dmic1_l_and_r_tx_settings),
};

static struct snddev_icodec_data ftm_dmic1_l_and_r_tx_data = {
	.capability = SNDDEV_CAP_TX,
	.name = "ftm_dmic1_l_and_r_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &ftm_dmic1_l_and_r_tx_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_power,
	.pamp_off = msm_snddev_disable_dmic_power,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_dmic1_l_and_r_tx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_dmic1_l_and_r_tx_data},
};

static struct adie_codec_action_unit ftm_dmic2_left_tx_actions[] =
	DMIC2_LEFT_TX;

static struct adie_codec_hwsetting_entry ftm_dmic2_left_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_dmic2_left_tx_actions,
		.action_sz = ARRAY_SIZE(ftm_dmic2_left_tx_actions),
	},
};

static struct adie_codec_dev_profile ftm_dmic2_left_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ftm_dmic2_left_tx_settings,
	.setting_sz = ARRAY_SIZE(ftm_dmic2_left_tx_settings),
};

static struct snddev_icodec_data ftm_dmic2_left_tx_data = {
	.capability = SNDDEV_CAP_TX,
	.name = "ftm_dmic2_left_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &ftm_dmic2_left_tx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_power,
	.pamp_off = msm_snddev_disable_dmic_power,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_dmic2_left_tx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_dmic2_left_tx_data },
};

static struct adie_codec_action_unit ftm_dmic2_right_tx_actions[] =
	DMIC2_RIGHT_TX;

static struct adie_codec_hwsetting_entry ftm_dmic2_right_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_dmic2_right_tx_actions,
		.action_sz = ARRAY_SIZE(ftm_dmic2_right_tx_actions),
	},
};

static struct adie_codec_dev_profile ftm_dmic2_right_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ftm_dmic2_right_tx_settings,
	.setting_sz = ARRAY_SIZE(ftm_dmic2_right_tx_settings),
};

static struct snddev_icodec_data ftm_dmic2_right_tx_data = {
	.capability = SNDDEV_CAP_TX,
	.name = "ftm_dmic2_right_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &ftm_dmic2_right_tx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_power,
	.pamp_off = msm_snddev_disable_dmic_power,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_dmic2_right_tx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_dmic2_right_tx_data },
};

static struct adie_codec_action_unit ftm_dmic2_l_and_r_tx_actions[] =
	DMIC2_LEFT_AND_RIGHT_TX;

static struct adie_codec_hwsetting_entry ftm_dmic2_l_and_r_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_dmic2_l_and_r_tx_actions,
		.action_sz = ARRAY_SIZE(ftm_dmic2_l_and_r_tx_actions),
	},
};

static struct adie_codec_dev_profile ftm_dmic2_l_and_r_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ftm_dmic2_l_and_r_tx_settings,
	.setting_sz = ARRAY_SIZE(ftm_dmic2_l_and_r_tx_settings),
};

static struct snddev_icodec_data ftm_dmic2_l_and_r_tx_data = {
	.capability = SNDDEV_CAP_TX,
	.name = "ftm_dmic2_l_and_r_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &ftm_dmic2_l_and_r_tx_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_power,
	.pamp_off = msm_snddev_disable_dmic_power,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_dmic2_l_and_r_tx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_dmic2_l_and_r_tx_data},
};

static struct adie_codec_action_unit ftm_handset_mic1_aux_in_actions[] =
	HANDSET_MIC1_AUX_IN;

static struct adie_codec_hwsetting_entry ftm_handset_mic1_aux_in_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_handset_mic1_aux_in_actions,
		.action_sz = ARRAY_SIZE(ftm_handset_mic1_aux_in_actions),
	},
};

static struct adie_codec_dev_profile ftm_handset_mic1_aux_in_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ftm_handset_mic1_aux_in_settings,
	.setting_sz = ARRAY_SIZE(ftm_handset_mic1_aux_in_settings),
};

static struct snddev_icodec_data ftm_handset_mic1_aux_in_data = {
	.capability = SNDDEV_CAP_TX,
	.name = "ftm_handset_mic1_aux_in",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &ftm_handset_mic1_aux_in_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	/* Assumption is that inputs are not tied to analog mic, so
	 * no need to enable mic bias.
	 */
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_handset_mic1_aux_in_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_handset_mic1_aux_in_data},
};

static struct snddev_mi2s_data snddev_mi2s_sd0_rx_data = {
	.capability = SNDDEV_CAP_RX ,
	.name = "mi2s_sd0_rx",
	.copp_id = MI2S_RX,
	.channel_mode = 2, /* stereo */
	.sd_lines = MI2S_SD0, /* sd0 */
	.sample_rate = 48000,
};

static struct platform_device ftm_mi2s_sd0_rx_device = {
	.name = "snddev_mi2s",
	.dev = { .platform_data = &snddev_mi2s_sd0_rx_data },
};

static struct snddev_mi2s_data snddev_mi2s_sd1_rx_data = {
	.capability = SNDDEV_CAP_RX ,
	.name = "mi2s_sd1_rx",
	.copp_id = MI2S_RX,
	.channel_mode = 2, /* stereo */
	.sd_lines = MI2S_SD1, /* sd1 */
	.sample_rate = 48000,
};

static struct platform_device ftm_mi2s_sd1_rx_device = {
	.name = "snddev_mi2s",
	.dev = { .platform_data = &snddev_mi2s_sd1_rx_data },
};

static struct snddev_mi2s_data snddev_mi2s_sd2_rx_data = {
	.capability = SNDDEV_CAP_RX ,
	.name = "mi2s_sd2_rx",
	.copp_id = MI2S_RX,
	.channel_mode = 2, /* stereo */
	.sd_lines = MI2S_SD2, /* sd2 */
	.sample_rate = 48000,
};

static struct platform_device ftm_mi2s_sd2_rx_device = {
	.name = "snddev_mi2s",
	.dev = { .platform_data = &snddev_mi2s_sd2_rx_data },
};

/* earpiece */
static struct adie_codec_action_unit ftm_handset_adie_lp_rx_actions[] =
	EAR_PRI_MONO_LB;

static struct adie_codec_hwsetting_entry ftm_handset_adie_lp_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_handset_adie_lp_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_handset_adie_lp_rx_actions),
	}
};

static struct adie_codec_dev_profile ftm_handset_adie_lp_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_handset_adie_lp_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_handset_adie_lp_rx_settings),
};

static struct snddev_icodec_data ftm_handset_adie_lp_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "ftm_handset_adie_lp_rx",
	.copp_id = 0,
	.profile = &ftm_handset_adie_lp_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_handset_adie_lp_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_handset_adie_lp_rx_data },
};

static struct adie_codec_action_unit ftm_headset_l_adie_lp_rx_actions[] =
	FTM_HPH_PRI_AB_CPLS_MONO_LB_LEFT;

static struct adie_codec_hwsetting_entry ftm_headset_l_adie_lp_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_headset_l_adie_lp_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_headset_l_adie_lp_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_headset_l_adie_lp_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_headset_l_adie_lp_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_headset_l_adie_lp_rx_settings),
};

static struct snddev_icodec_data ftm_headset_l_adie_lp_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_headset_l_adie_lp_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_headset_l_adie_lp_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_headset_l_adie_lp_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_headset_l_adie_lp_rx_data },
};

static struct adie_codec_action_unit ftm_headset_r_adie_lp_rx_actions[] =
	FTM_HPH_PRI_AB_CPLS_MONO_LB_RIGHT;

static struct adie_codec_hwsetting_entry ftm_headset_r_adie_lp_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_headset_r_adie_lp_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_headset_r_adie_lp_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_headset_r_adie_lp_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_headset_r_adie_lp_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_headset_r_adie_lp_rx_settings),
};

static struct snddev_icodec_data ftm_headset_r_adie_lp_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_headset_r_adie_lp_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_headset_r_adie_lp_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_headset_r_adie_lp_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_headset_r_adie_lp_rx_data },
};

static struct adie_codec_action_unit ftm_spkr_l_rx_lp_actions[] =
	FTM_SPKR_L_RX;

static struct adie_codec_hwsetting_entry ftm_spkr_l_rx_lp_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_spkr_l_rx_lp_actions,
		.action_sz = ARRAY_SIZE(ftm_spkr_l_rx_lp_actions),
	},
};

static struct adie_codec_dev_profile ftm_spkr_l_rx_lp_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_spkr_l_rx_lp_settings,
	.setting_sz = ARRAY_SIZE(ftm_spkr_l_rx_lp_settings),
};

static struct snddev_icodec_data ftm_spkr_l_rx_lp_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_spk_l_adie_lp_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_spkr_l_rx_lp_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on,
	.pamp_off = msm_snddev_poweramp_off,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_spk_l_adie_lp_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_spkr_l_rx_lp_data},
};

static struct adie_codec_action_unit ftm_spkr_r_adie_lp_rx_actions[] =
	FTM_SPKR_RX_LB;

static struct adie_codec_hwsetting_entry ftm_spkr_r_adie_lp_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_spkr_r_adie_lp_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_spkr_r_adie_lp_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_spkr_r_adie_lp_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_spkr_r_adie_lp_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_spkr_r_adie_lp_rx_settings),
};

static struct snddev_icodec_data ftm_spkr_r_adie_lp_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_spk_r_adie_lp_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_spkr_r_adie_lp_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on,
	.pamp_off = msm_snddev_poweramp_off,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_spk_r_adie_lp_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_spkr_r_adie_lp_rx_data},
};

static struct adie_codec_action_unit ftm_spkr_adie_lp_rx_actions[] =
	FTM_SPKR_RX_LB;

static struct adie_codec_hwsetting_entry ftm_spkr_adie_lp_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_spkr_adie_lp_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_spkr_adie_lp_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_spkr_adie_lp_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_spkr_adie_lp_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_spkr_adie_lp_rx_settings),
};

static struct snddev_icodec_data ftm_spkr_adie_lp_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_spk_adie_lp_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_spkr_adie_lp_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on,
	.pamp_off = msm_snddev_poweramp_off,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_spk_adie_lp_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_spkr_adie_lp_rx_data},
};

static struct adie_codec_action_unit ftm_handset_dual_tx_lp_actions[] =
	FTM_AMIC_DUAL_HANDSET_TX_LB;

static struct adie_codec_hwsetting_entry ftm_handset_dual_tx_lp_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_handset_dual_tx_lp_actions,
		.action_sz = ARRAY_SIZE(ftm_handset_dual_tx_lp_actions),
	}
};

static struct adie_codec_dev_profile ftm_handset_dual_tx_lp_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ftm_handset_dual_tx_lp_settings,
	.setting_sz = ARRAY_SIZE(ftm_handset_dual_tx_lp_settings),
};

static struct snddev_icodec_data ftm_handset_dual_tx_lp_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_mic1_handset_mic2",
	.copp_id = 1,
	.profile = &ftm_handset_dual_tx_lp_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_amic_power,
	.pamp_off = msm_snddev_disable_amic_power,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_handset_dual_tx_lp_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_handset_dual_tx_lp_data },
};

static struct adie_codec_action_unit ftm_handset_mic_adie_lp_tx_actions[] =
	FTM_HANDSET_LB_TX;

static struct adie_codec_hwsetting_entry
	ftm_handset_mic_adie_lp_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_handset_mic_adie_lp_tx_actions,
		.action_sz = ARRAY_SIZE(ftm_handset_mic_adie_lp_tx_actions),
	}
};

static struct adie_codec_dev_profile ftm_handset_mic_adie_lp_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ftm_handset_mic_adie_lp_tx_settings,
	.setting_sz = ARRAY_SIZE(ftm_handset_mic_adie_lp_tx_settings),
};

static struct snddev_icodec_data ftm_handset_mic_adie_lp_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "ftm_handset_mic_adie_lp_tx",
	.copp_id = 1,
	.profile = &ftm_handset_mic_adie_lp_tx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_amic_power,
	.pamp_off = msm_snddev_disable_amic_power,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_handset_mic_adie_lp_tx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_handset_mic_adie_lp_tx_data },
};

static struct adie_codec_action_unit ftm_headset_mic_adie_lp_tx_actions[] =
	FTM_HEADSET_LB_TX;

static struct adie_codec_hwsetting_entry
			ftm_headset_mic_adie_lp_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_headset_mic_adie_lp_tx_actions,
		.action_sz = ARRAY_SIZE(ftm_headset_mic_adie_lp_tx_actions),
	}
};

static struct adie_codec_dev_profile ftm_headset_mic_adie_lp_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ftm_headset_mic_adie_lp_tx_settings,
	.setting_sz = ARRAY_SIZE(ftm_headset_mic_adie_lp_tx_settings),
};

static struct snddev_icodec_data ftm_headset_mic_adie_lp_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "ftm_headset_mic_adie_lp_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &ftm_headset_mic_adie_lp_tx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.dev_vol_type = SNDDEV_DEV_VOL_DIGITAL,
};

static struct platform_device ftm_headset_mic_adie_lp_tx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_headset_mic_adie_lp_tx_data },
};
#endif /* CONFIG_MSM8X60_FTM_AUDIO_DEVICES */

static struct snddev_virtual_data snddev_uplink_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "uplink_rx",
	.copp_id = VOICE_PLAYBACK_TX,
};

static struct platform_device msm_uplink_rx_device = {
	.name = "snddev_virtual",
	.dev = { .platform_data = &snddev_uplink_rx_data },
};

static struct snddev_hdmi_data snddev_hdmi_non_linear_pcm_rx_data = {
	.capability = SNDDEV_CAP_RX ,
	.name = "hdmi_pass_through",
	.default_sample_rate = 48000,
	.on_apps = 1,
};

static struct platform_device msm_snddev_hdmi_non_linear_pcm_rx_device = {
	.name = "snddev_hdmi",
	.dev = { .platform_data = &snddev_hdmi_non_linear_pcm_rx_data },
};
//LGE_UPDATE_S, jeremy.pi@lge.com, 2011-04-09, at&t Hearing AID
static struct adie_codec_action_unit iearpiece_hac_48KHz_osr256_actions[] =
	HANDSET_RX_MONO_8000_OSR_256_HAC;

static struct adie_codec_hwsetting_entry iearpiece_hac_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = iearpiece_hac_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(iearpiece_hac_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile iearpiece_hac_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = iearpiece_hac_settings,
	.setting_sz = ARRAY_SIZE(iearpiece_hac_settings),
};

static struct snddev_icodec_data snddev_iearpiece_hac_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "handset_rx_hac",
	.copp_id = 0,
	.profile = &iearpiece_hac_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
};

static struct platform_device msm_iearpiece_hac_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_iearpiece_hac_data },
};
//LGE_UPDATE_E, jeremy.pi@lge.com, 2011-04-09, at&t Hearing AID

static struct adie_codec_action_unit dual_nc_48KHz_osr256_actions[] =
	AMIC_DUAL_OSR_256; ///////////////
		
static struct adie_codec_hwsetting_entry dual_nc_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = dual_nc_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(dual_nc_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile dual_nc_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = dual_nc_settings,
	.setting_sz = ARRAY_SIZE(dual_nc_settings),
};

static struct snddev_icodec_data snddev_dual_nc_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_dual_nc_tx",
	.copp_id = 1,
	.profile = &dual_nc_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dualmic_power,
	.pamp_off = msm_snddev_disable_dualmic_power,
};

static struct adie_codec_action_unit mic1_nc_48KHz_osr256_actions[] =
	AMIC_PRI_MONO_OSR_256; ///////////////
		
static struct adie_codec_hwsetting_entry mic1_nc_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = mic1_nc_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(mic1_nc_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile mic1_nc_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = mic1_nc_settings,
	.setting_sz = ARRAY_SIZE(mic1_nc_settings),
};

static struct snddev_icodec_data snddev_mic1_nc_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_mic1_nc_tx",
	.copp_id = 1,
	.profile = &mic1_nc_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_amic_power,
	.pamp_off = msm_snddev_disable_amic_power,
};


static struct adie_codec_action_unit mic2_nc_48KHz_osr256_actions[] =
	AMIC_SEC_MONO_OSR_256; ///////////////
	
static struct adie_codec_hwsetting_entry mic2_nc_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = mic2_nc_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(mic2_nc_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile mic2_nc_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = mic2_nc_settings,
	.setting_sz = ARRAY_SIZE(mic2_nc_settings),
};

static struct snddev_icodec_data snddev_mic2_nc_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_mic2_nc_tx",
	.copp_id = 1,
	.profile = &mic2_nc_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_submic_power,
	.pamp_off = msm_snddev_disable_submic_power,
};

static struct adie_codec_action_unit mic1_acs_48KHz_osr256_actions[] =
	AMIC_PRI_MONO_OSR_256_ACS; ///////////////
		
static struct adie_codec_hwsetting_entry mic1_acs_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = mic1_acs_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(mic1_acs_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile mic1_acs_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = mic1_acs_settings,
	.setting_sz = ARRAY_SIZE(mic1_acs_settings),
};


static struct snddev_icodec_data snddev_mic1_acs_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_mic1_acs_tx",
	.copp_id = 1,
	.profile = &mic1_acs_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_amic_power,
	.pamp_off = msm_snddev_disable_amic_power,
};


static struct platform_device msm_dual_nc_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_dual_nc_data },
};
static struct platform_device msm_mic1_nc_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_mic1_nc_data },
};
static struct platform_device msm_mic2_nc_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_mic2_nc_data },
};
static struct adie_codec_action_unit imic_record_48KHz_osr256_actions[] =
	HANDSET_RECORD_TX_PRI_MONO_OSR_256;

static struct adie_codec_hwsetting_entry imic_record_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = imic_record_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(imic_record_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile imic_record_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = imic_record_settings,
	.setting_sz = ARRAY_SIZE(imic_record_settings),
};

static struct snddev_icodec_data snddev_imic_record_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_record_tx",
	.copp_id = 1,
	.profile = &imic_record_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_amic_power,
	.pamp_off = msm_snddev_disable_amic_power,
};

static struct platform_device msm_imic_record_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_imic_record_data },
};

static struct platform_device msm_mic1_acs_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_mic1_acs_data },
};

#ifdef CONFIG_LGE_AUDIO
static struct adie_codec_action_unit submic_lb_48KHz_osr256_actions[] =
	SUB_MIC_LB_MONO_OSR_256;

static struct adie_codec_hwsetting_entry submic_lb_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = submic_lb_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(submic_lb_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile submic_lb_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = submic_lb_settings,
	.setting_sz = ARRAY_SIZE(submic_lb_settings),
};


static struct snddev_icodec_data snddev_ispkr_mic_lb_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_lb_mono_tx",
	.copp_id = 1,
	.profile = &submic_lb_profile,

	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_submic_power,
	.pamp_off = msm_snddev_disable_submic_power,
};


static struct platform_device msm_mic2_lb_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_ispkr_mic_lb_data },
};
#endif

static struct adie_codec_action_unit imic_vr_48KHz_osr256_actions[] =
	HANDSET_RECORD_TX_PRI_MONO_OSR_256_VR;

static struct adie_codec_hwsetting_entry imic_vr_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = imic_vr_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(imic_vr_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile imic_vr_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = imic_vr_settings,
	.setting_sz = ARRAY_SIZE(imic_vr_settings),
};

static struct snddev_icodec_data snddev_imic_vr_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_vr_tx",
	.copp_id = 1,
	.profile = &imic_vr_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_amic_power,
	.pamp_off = msm_snddev_disable_amic_power,
};

static struct platform_device msm_imic_vr_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_imic_vr_data },
};

//ACS_RX
static struct adie_codec_action_unit iearpiece_acs_48KHz_osr256_actions[] =
	HANDSET_ACS_RX_MONO_8000_OSR_256;
static struct adie_codec_hwsetting_entry iearpiece_acs_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = iearpiece_acs_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(iearpiece_acs_48KHz_osr256_actions),
	}
};
static struct adie_codec_dev_profile iearpiece_acs_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = iearpiece_acs_settings,
	.setting_sz = ARRAY_SIZE(iearpiece_acs_settings),
};

static struct snddev_icodec_data snddev_acs_iearpiece_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "handset_acs_rx",
	.copp_id = 0,
	.profile = &iearpiece_acs_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
};

static struct platform_device msm_iearpiece_acs_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_acs_iearpiece_data },
};


static struct adie_codec_action_unit imic_lgvm_record_48KHz_osr256_actions[] =
	HANDSET_RECORD_TX_PRI_MONO_OSR_256;

static struct adie_codec_hwsetting_entry imic_lgvm_record_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = imic_lgvm_record_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(imic_lgvm_record_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile imic_lgvm_record_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = imic_lgvm_record_settings,
	.setting_sz = ARRAY_SIZE(imic_lgvm_record_settings),
};

static struct snddev_icodec_data snddev_imic_lgvm_record_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_lgvm_tx",
	.copp_id = 1,
	.profile = &imic_lgvm_record_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_amic_power,
	.pamp_off = msm_snddev_disable_amic_power,
};

static struct platform_device msm_imic_lgvm_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_imic_lgvm_record_data },
};

static struct adie_codec_action_unit imic_lgcam_record_48KHz_osr256_actions[] =
	AUXIN_LGCAM_MONO_OSR_256;

static struct adie_codec_hwsetting_entry imic_lgcam_record_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = imic_lgcam_record_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(imic_lgcam_record_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile imic_lgcam_record_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = imic_lgcam_record_settings,
	.setting_sz = ARRAY_SIZE(imic_lgcam_record_settings),
};

static struct snddev_icodec_data snddev_imic_lgcam_record_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_lgcam_tx",
	.copp_id = 1,
	.profile = &imic_lgcam_record_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_submic_power,
	.pamp_off = msm_snddev_disable_submic_power,
};

static struct platform_device msm_imic_lgcam_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_imic_lgcam_record_data },
};

#ifdef CONFIG_DEBUG_FS
static struct adie_codec_action_unit
	ihs_stereo_rx_class_d_legacy_48KHz_osr256_actions[] =
	HPH_PRI_D_LEG_STEREO;

static struct adie_codec_hwsetting_entry
	ihs_stereo_rx_class_d_legacy_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions =
		ihs_stereo_rx_class_d_legacy_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE
		(ihs_stereo_rx_class_d_legacy_48KHz_osr256_actions),
	}
};

static struct adie_codec_action_unit
	ihs_stereo_rx_class_ab_legacy_48KHz_osr256_actions[] =
	HPH_PRI_AB_LEG_STEREO;

static struct adie_codec_hwsetting_entry
	ihs_stereo_rx_class_ab_legacy_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions =
		ihs_stereo_rx_class_ab_legacy_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE
		(ihs_stereo_rx_class_ab_legacy_48KHz_osr256_actions),
	}
};

static void snddev_hsed_config_modify_setting(int type)
{
	struct platform_device *device;
	struct snddev_icodec_data *icodec_data;

	device = &msm_headset_stereo_device;
	icodec_data = (struct snddev_icodec_data *)device->dev.platform_data;

	if (icodec_data) {
		if (type == 1) {
			icodec_data->voltage_on = NULL;
			icodec_data->voltage_off = NULL;
			icodec_data->profile->settings =
				ihs_stereo_rx_class_d_legacy_settings;
			icodec_data->profile->setting_sz =
			ARRAY_SIZE(ihs_stereo_rx_class_d_legacy_settings);
		} else if (type == 2) {
			icodec_data->voltage_on = NULL;
			icodec_data->voltage_off = NULL;
			icodec_data->profile->settings =
				ihs_stereo_rx_class_ab_legacy_settings;
			icodec_data->profile->setting_sz =
			ARRAY_SIZE(ihs_stereo_rx_class_ab_legacy_settings);
		}
	}
}

static void snddev_hsed_config_restore_setting(void)
{
	struct platform_device *device;
	struct snddev_icodec_data *icodec_data;

	device = &msm_headset_stereo_device;
	icodec_data = (struct snddev_icodec_data *)device->dev.platform_data;

	if (icodec_data) {
		icodec_data->voltage_on = msm_snddev_voltage_on;
		icodec_data->voltage_off = msm_snddev_voltage_off;
		icodec_data->profile->settings = headset_ab_cpls_settings;
		icodec_data->profile->setting_sz =
			ARRAY_SIZE(headset_ab_cpls_settings);
	}
}

static ssize_t snddev_hsed_config_debug_write(struct file *filp,
	const char __user *ubuf, size_t cnt, loff_t *ppos)
{
	char *lb_str = filp->private_data;
	char cmd;

	if (get_user(cmd, ubuf))
		return -EFAULT;

	if (!strcmp(lb_str, "msm_hsed_config")) {
		switch (cmd) {
		case '0':
			snddev_hsed_config_restore_setting();
			break;

		case '1':
			snddev_hsed_config_modify_setting(1);
			break;

		case '2':
			snddev_hsed_config_modify_setting(2);
			break;

		default:
			break;
		}
	}
	return cnt;
}

static int snddev_hsed_config_debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static const struct file_operations snddev_hsed_config_debug_fops = {
	.open = snddev_hsed_config_debug_open,
	.write = snddev_hsed_config_debug_write
};
#endif

#ifdef CONFIG_LGE_HEADSET_DETECTION_FSA8008
#include <linux/input.h>

static void fsa8008_set_headset_mic_bias(int enable) {
	int rc;

	if (enable)
		rc = pm8058_micbias_enable(OTHC_MICBIAS_HEADSET, OTHC_SIGNAL_ALWAYS_ON);
	else
		rc = pm8058_micbias_enable(OTHC_MICBIAS_HEADSET, OTHC_SIGNAL_OFF);

	if (rc)
		pr_err("%s: Enabling amic power failed\n", __func__);
}

static struct fsa8008_platform_data fsa8008_platform_data = {
	.switch_name = "h2w",

	.keypad_name = "8660_handset", // "qwerty", // "hsd_headset"; // just for testing

	.key_code = KEY_MEDIA, // just for testing

	.gpio_detect = GPIO_EAR_SENSE_N,
	.gpio_mic_en = GPIO_EAR_MIC_EN,
	.gpio_jpole  = GPIO_EARPOL_DETECT,
	.gpio_key    = GPIO_EAR_KEY_INT,

	.set_headset_mic_bias = fsa8008_set_headset_mic_bias,

	.latency_for_detection = 75,
};

static struct platform_device lge_hsd_device = {
   .name = "fsa8008",
   .id   = -1,
   .dev = {
      .platform_data = &fsa8008_platform_data,
   },
};

#endif

#ifdef LGE_AUDIO_COMMENT_OUT_FOR_REFERENCE
static struct platform_device *snd_devices_ffa[] __initdata = {
	&msm_iearpiece_ffa_device,
	&msm_imic_ffa_device,
	&msm_ispkr_stereo_device,
	&msm_snddev_hdmi_stereo_rx_device,
	&msm_headset_mic_device,
	&msm_ispkr_mic_device,
	&msm_bt_sco_earpiece_device,
	&msm_bt_sco_mic_device,
	&msm_headset_stereo_device,
	&msm_itty_mono_tx_device,
	&msm_itty_mono_rx_device,
	&msm_mi2s_fm_tx_device,
	&msm_mi2s_fm_rx_device,
	&msm_hs_dual_mic_endfire_device,
	&msm_spkr_dual_mic_endfire_device,
	&msm_hs_dual_mic_broadside_device,
	&msm_spkr_dual_mic_broadside_device,
	&msm_ihs_stereo_speaker_stereo_rx_device,
	&msm_anc_headset_device,
	&msm_auxpga_lp_hs_device,
	&msm_auxpga_lp_lo_device,
	&msm_linein_pri_device,
	&msm_icodec_gpio_device,
	&msm_snddev_hdmi_non_linear_pcm_rx_device,
};
#endif

static struct platform_device *snd_devices_lge_i_board[] __initdata = {
	&msm_iearpiece_device,
	&msm_imic_device,
	&msm_ispkr_stereo_device,
	&msm_snddev_hdmi_stereo_rx_device,
	&msm_headset_mic_device,
	&msm_ispkr_mic_device,
	&msm_bt_sco_earpiece_device,
	&msm_bt_sco_mic_device,
	&msm_headset_stereo_device,
	&msm_itty_mono_tx_device,
	&msm_itty_mono_rx_device,
	&msm_mi2s_fm_tx_device,
	&msm_mi2s_fm_rx_device,
	&msm_hs_dual_mic_endfire_device,	
	&msm_ihs_stereo_speaker_stereo_rx_device,
	&msm_auxpga_lp_hs_device,
	&msm_auxpga_lp_lo_device,
	&msm_linein_pri_device,
#ifdef LGE_AUDIO_COMMENT_OUT_FOR_REFERENCE
	&msm_icodec_gpio_device,
#endif
	&msm_snddev_hdmi_non_linear_pcm_rx_device,
#ifdef CONFIG_LGE_HEADSET_DETECTION_FSA8008
	&lge_hsd_device,
#endif
//LGE_UPDATE_S, jeremy.pi@lge.com, 2011-04-09, at&t Hearing AID
    &msm_iearpiece_hac_device,
//LGE_UPDATE_E, jeremy.pi@lge.com, 2011-04-09, at&t Hearing AID
//LGE_UPDATE_S, jeremy.pi@lge.com, 2011-04-12, at&t Playback QTR8615L 
    &msm_ispkr_stereo_playback_device,
    &msm_headset_stereo_playback_device,
//LGE_UPDATE_E, jeremy.pi@lge.com, 2011-04-12, at&t Playback QTR8615L 
	&msm_imic_record_device,
	&msm_dual_nc_device,
	&msm_mic1_nc_device,
	&msm_mic2_nc_device, 
	&msm_mic2_lb_device,
    &msm_iearpiece_acs_device,       //ACS_RX
    &msm_mic1_acs_device,            //ACS_TX
    &msm_imic_vr_device,
    &msm_imic_lgvm_device,
    &msm_imic_lgcam_device,
	&msm_bt_sco_bttm_earpiece_device,//BTTM_RX
	&msm_bt_sco_bttm_mic_device,     //BTTM_TX
  &msm_headset_stereo_acs_device,//EAR ACS_RX
  &msm_headset_mic_acs_device,   //EAR ACS_TX
  &msm_ispkr_stereo_vzwnavi_device,
	&msm_headset_mic_vm_device,
	&msm_headset_mic_record_device,
};

#ifdef LGE_AUDIO_COMMENT_OUT_FOR_REFERENCE
static struct platform_device *snd_devices_fluid[] __initdata = {
	&msm_iearpiece_device,
	&msm_imic_device,
	&msm_ispkr_stereo_device,
	&msm_snddev_hdmi_stereo_rx_device,
	&msm_headset_stereo_device,
	&msm_headset_mic_device,
	&msm_fluid_ispkr_mic_device,
	&msm_bt_sco_earpiece_device,
	&msm_bt_sco_mic_device,
	&msm_mi2s_fm_tx_device,
	&msm_mi2s_fm_rx_device,
	&msm_fluid_hs_dual_mic_endfire_device,
	&msm_fluid_spkr_dual_mic_endfire_device,
	&msm_fluid_hs_dual_mic_broadside_device,
	&msm_fluid_spkr_dual_mic_broadside_device,
	&msm_anc_headset_device,
	&msm_auxpga_lp_hs_device,
	&msm_auxpga_lp_lo_device,
	&msm_icodec_gpio_device,
	&msm_snddev_hdmi_non_linear_pcm_rx_device,
};

#endif

static struct platform_device *snd_devices_common[] __initdata = {
	&msm_aux_pcm_device,
	&msm_cdcclk_ctl_device,
	&msm_mi2s_device,
	&msm_uplink_rx_device,
};

#ifdef CONFIG_MSM8X60_FTM_AUDIO_DEVICES
static struct platform_device *snd_devices_ftm[] __initdata = {
	&ftm_headset_mono_rx_device,
	&ftm_headset_mono_l_rx_device,
	&ftm_headset_mono_r_rx_device,
	&ftm_headset_mono_diff_rx_device,
	&ftm_spkr_mono_rx_device,
	&ftm_spkr_l_rx_device,
	&ftm_spkr_r_rx_device,
	&ftm_spkr_mono_diff_rx_device,
	&ftm_linein_l_tx_device,
	&ftm_linein_r_tx_device,
	&ftm_aux_out_rx_device,
	&ftm_dmic1_left_tx_device,
	&ftm_dmic1_right_tx_device,
	&ftm_dmic1_l_and_r_tx_device,
	&ftm_dmic2_left_tx_device,
	&ftm_dmic2_right_tx_device,
	&ftm_dmic2_l_and_r_tx_device,
	&ftm_handset_mic1_aux_in_device,
	&ftm_mi2s_sd0_rx_device,
	&ftm_mi2s_sd1_rx_device,
	&ftm_mi2s_sd2_rx_device,
	&ftm_handset_mic_adie_lp_tx_device,
	&ftm_headset_mic_adie_lp_tx_device,
	&ftm_handset_adie_lp_rx_device,
	&ftm_headset_l_adie_lp_rx_device,
	&ftm_headset_r_adie_lp_rx_device,
	&ftm_spk_l_adie_lp_rx_device,
	&ftm_spk_r_adie_lp_rx_device,
	&ftm_spk_adie_lp_rx_device,
	&ftm_handset_dual_tx_lp_device,
};
#else
#ifdef LGE_AUDIO_COMMENT_OUT_FOR_REFERENCE
static struct platform_device *snd_devices_ftm[] __initdata = {};
#endif
#endif

void __init msm_snddev_init(void)
{
	int i;
	int dev_id;

#ifdef CONFIG_LGE_AUDIO_AMP_WM9093
	/* wm9093 initialization code */
	i2c_register_board_info(LGE_GSBI_BUS_ID_AUDIO_AMP_WM9093,
		lge_i2c_wm9093_info,
		ARRAY_SIZE(lge_i2c_wm9093_info));
#endif
#ifdef LGE_AUDIO_COMMENT_OUT_FOR_REFERENCE
	atomic_set(&pamp_ref_cnt, 0);
#endif
	atomic_set(&preg_ref_cnt, 0);

	for (i = 0, dev_id = 0; i < ARRAY_SIZE(snd_devices_common); i++)
		snd_devices_common[i]->id = dev_id++;

	platform_add_devices(snd_devices_common,
		ARRAY_SIZE(snd_devices_common));

	for (i = 0; i < ARRAY_SIZE(snd_devices_lge_i_board); i++)
		snd_devices_lge_i_board[i]->id = dev_id++;

	platform_add_devices(snd_devices_lge_i_board, ARRAY_SIZE(snd_devices_lge_i_board));

#ifdef CONFIG_DEBUG_FS
	debugfs_hsed_config = debugfs_create_file("msm_hsed_config",
				S_IFREG | S_IRUGO, NULL,
		(void *) "msm_hsed_config", &snddev_hsed_config_debug_fops);
#endif
}
