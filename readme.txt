VP8 Video For Windows Codec
===========================

This package contains the Video For Windows driver  of the
Google VP8 (WebM) codec. It is based on the source code of
the Xvid VFW driver, and is released under the GNU Public
license and is linked against the libvpx library.

You can always find the latest source code and binary here:
http://www.optimasc.com/products/vp8vfw/index.html

The VP8 library was created by Google and has a BSD license, for more
information on the VP8 codec, please consult http://www.webmproject.org


The driver has been tested on Windows 98SE, Windows XP and Windows 2000,
but should also work with other windows versions. Please report any
problems found at info@optimasc.com


Known Issues
------------

1) Because of how the VP8 library is designed, that one more call to the
library must be done to get the full statistics packets even after all frames
are processed (NULL parameter of vpx_codec_encode()), therefore 2-pass
encoding will not work on software that use the Video Compression Manager
without setting a valid framecount using the ICM_COMPRESS_FRAMES_INFO.

The following software has been tested:
- Corel Videostudio Pro X3 -> OK in all modes.
- Adobe Premiere 2.0 -> Fails in 2-pass mode due to the above issue.
- Virtualdub 1.9.10 -> OK in all modes.

2) The libvpx library is quite slow for encoding, hence it is strongly
suggested to encode using the "Good" preset quality.

3) All frames sizes must be a mutiplie of 2 in both width and height.

Recompiling
============
- Pre-requisites: Pre-compiled VP8 library or VP8 Visual C++ project, you will need
  to change the project settings (library paths) accordingly.
- Platform SDK, you will need to setup the correct include paths and your libraries
  in your environment.


Changelog
=========
1.2.0.0:
- Linked against libvpx 0.9.7-p1 which should improve encoding speed

1.1.0.0:
- Linked against libvpx 0.9.6.1 which should improve encoding speed
  on some computers and might exceptionally slow it on others
  (like my Core Duo Laptop).
- After some tests and seeing there is not much difference between the
  Best end Good quality encoding, the default configuration is now Good.
- Bugfix: The number of threads used for encoding now depends on the number
  of CPU's detected (the number of threads is now equal to the number of cores - 1)
- Bugfix: Input YV12 and I420 would crash the codec (wrong stride calculation was done)
- Add limitation on frame width and height

1.0.1.0:
- Release for Windows, linked against libvpx 0.9.5
- Full installer
- Bugfix of crash when images must be padded to 16-byte aligned widths and
  heights. From now on, padding is handled directly by libvpx.

1.0.0.0:
- Initial release for Windows, linked against libvpx 0.9.2
