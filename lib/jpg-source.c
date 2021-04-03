/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * JPEG still image video source
 *
 * Copyright (C) 2018 Paul Elder
 *
 * Contact: Paul Elder <paul.elder@ideasonboard.com>
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <linux/videodev2.h>

#include "events.h"
#include "tools.h"
#include "v4l2.h"
#include "jpg-source.h"
#include "video-buffers.h"
#include "log.h"

struct jpg_source {
	struct video_source src;

	unsigned int imgsize;
	void *imgdata;
};

#define to_jpg_source(s) container_of(s, struct jpg_source, src)

static void jpg_source_destroy(struct video_source *s)
{
	struct jpg_source *src = to_jpg_source(s);

	if (src->imgdata)
		free(src->imgdata);
	free(src);
}

static int jpg_source_set_format(struct video_source *s,
				  struct v4l2_pix_format *fmt)
{
	if (fmt->pixelformat != v4l2_fourcc('M', 'J', 'P', 'G')) {
		log_error("invalid pixel format");
		return -EINVAL;
	}

	return 0;
}

static int jpg_source_set_frame_rate(struct video_source *s, unsigned int fps)
{ return 0; }

static int jpg_source_free_buffers(struct video_source *s)
{ return 0; }

static int jpg_source_stream_on(struct video_source *s)
{ return 0; }

static int jpg_source_stream_off(struct video_source *s)
{ return 0; }

static void jpg_source_fill_buffer(struct video_source *s,
				   struct video_buffer *buf)
{
	struct jpg_source *src = to_jpg_source(s);

	memcpy(buf->mem, src->imgdata, src->imgsize);
	buf->bytesused = src->imgsize;
}

static const struct video_source_ops jpg_source_ops = {
	.destroy = jpg_source_destroy,
	.set_format = jpg_source_set_format,
	.set_frame_rate = jpg_source_set_frame_rate,
	.alloc_buffers = NULL,
	.export_buffers = NULL,
	.free_buffers = jpg_source_free_buffers,
	.stream_on = jpg_source_stream_on,
	.stream_off = jpg_source_stream_off,
	.queue_buffer = NULL,
	.fill_buffer = jpg_source_fill_buffer,
};

struct video_source *jpg_video_source_create(const char *img_path)
{
	struct jpg_source *src;
	int fd = -1;

	log_debug("Creating JPG video source");

	if (img_path == NULL)
		return NULL;

	src = malloc(sizeof *src);
	if (!src)
		return NULL;

	memset(src, 0, sizeof *src);
	src->src.ops = &jpg_source_ops;

	fd = open(img_path, O_RDONLY);
	if (fd == -1) {
		log_error("Unable to open MJPEG image '%s'", img_path);
		return NULL;
	}

	src->imgsize = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	src->imgdata = malloc(src->imgsize);
	if (src->imgdata == NULL) {
		log_error("Unable to allocate memory for MJPEG image");
		src->imgsize = 0;
		return NULL;
	}

	read(fd, src->imgdata, src->imgsize);
	close(fd);

	return &src->src;
}

void jpg_video_source_init(struct video_source *s, struct events *events)
{
	struct jpg_source *src = to_jpg_source(s);

	src->src.events = events;
}
