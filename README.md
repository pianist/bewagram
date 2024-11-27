# bewagram

Beward DS06 (and DS05) open source daemons with Telegram (or any other services) integration, based on OpenIPC firmware.

How it works? You install OpenIPC firmware, which runs Majestic as a video/imagem streamer. Then you add Bewagram daemon, which handles call button, play music, sends notifications and photo to a server (or Telegram bot directly).

TODO:
 * Read voice messages from Telegram group/chat and play it.
 * OD/MD events detect and announce
 * IVE large object detection like people faces, cars, etc...

Done:
 * VENC init/done on image snap (this will allow Majestic restart).
 * Audio mic streaming directly w/o Majestic, this will allow to turn off buggy SDK audio with Majestic segfaults.

