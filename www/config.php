<?php
	define("PROGRAM", "dav");
	define("HOST", php_uname("n"));
  	define("TITLE_STRING", PROGRAM . "@" . HOST);

// The 'media' directory in these defines is a link to the media_dir
// which is configured in ~/.dav/dav.conf and the link is
// automatically altered by the startup script if media_dir is changed.
// The videos, stills and timelapse subdirs are fixed to match dav.
// Do not change these here.
//
	define("VIDEO_DIR", "media/videos");
	define("STILL_DIR", "media/stills");
	define("TIMELAPSE_DIR", "media/timelapse");

	define("ARCHIVE_DIR", "archive");

// These are set up by the install or dav.conf and enforced by
// the startup script.  It is no use to change these here.
//
	define("LOG_FILE", "/tmp/dav.log");
	define("MJPEG_FILE", "/run/dav/mjpeg.jpg");
	define("dav", "/home/pi/dav/dav");
	define("FIFO_FILE", "/home/pi/dav/www/FIFO");

	define("SERVOS_ENABLE", "servos_off");

	define("VERSION", "3.0.2");
?>
