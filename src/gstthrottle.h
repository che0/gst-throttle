/*
 * GStreamer
 * Copyright (C) 2012 Petr Novak <pnovak@koukaam.se>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GST_THROTTLE_H__
#define __GST_THROTTLE_H__

#include <gst/gst.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_THROTTLE \
	(gst_throttle_get_type())
#define GST_THROTTLE(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_THROTTLE,GstThrottle))
#define GST_THROTTLE_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_THROTTLE,GstThrottleClass))
#define GST_IS_THROTTLE(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_THROTTLE))
#define GST_IS_THROTTLE_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_THROTTLE))

typedef struct _GstThrottle GstThrottle;
typedef struct _GstThrottleClass GstThrottleClass;

struct _GstThrottle
{
	GstElement element;
	
	GstPad *sinkpad, *srcpad;
	
	gboolean printOnly;
	GstClock * clock;
	gboolean haveStartTime;
	GstClockTimeDiff streamStartRealTime;
};

struct _GstThrottleClass 
{
	GstElementClass parent_class;
};

GType gst_throttle_get_type(void);

G_END_DECLS

#endif /* __GST_THROTTLE_H__ */
