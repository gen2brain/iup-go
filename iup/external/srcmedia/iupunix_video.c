/** \file
 * \brief Video control, libVLC, GStreamer and libmpv engines
 *
 * See Copyright Notice in "iup.h"
 */

#include "iup_video.h"

extern const IvideoBackend iupVideoVlcBackend;
extern const IvideoBackend iupVideoGstreamerBackend;
extern const IvideoBackend iupVideoMpvBackend;

char* iupdrvVideoReadSubtitle(const char* filename)
{
  return iupVideoReadSubtitleStream(fopen(filename, "rb"));
}

const IvideoBackend* iupdrvVideoBackend(int index)
{
  static const IvideoBackend* backends[] = {&iupVideoVlcBackend, &iupVideoGstreamerBackend, &iupVideoMpvBackend};
  if (index < 0 || index >= (int)(sizeof(backends) / sizeof(backends[0])))
    return NULL;
  return backends[index];
}
