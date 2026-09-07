/** \file
 * \brief miniaudio configuration shared by every unit that includes it
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_MINIAUDIO_H
#define __IUP_MINIAUDIO_H

#define MA_NO_ENCODING
#define MA_NO_GENERATION
#define MA_NO_RESOURCE_MANAGER
#if defined(__APPLE__)
#define MA_NO_RUNTIME_LINKING
#endif

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"
#include "miniaudio.h"

#ifdef __cplusplus
extern "C" {
#endif

ma_device* iupdrvAudioDeviceInit(ma_engine* engine);
void iupdrvAudioDeviceRelease(void);

#ifdef __cplusplus
}
#endif

#endif
