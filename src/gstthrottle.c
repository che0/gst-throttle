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

/**
 * SECTION:element-throttle
 *
 * FIXME:Describe throttle here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! throttle ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>

#include "gstthrottle.h"

GST_DEBUG_CATEGORY_STATIC(gst_throttle_debug);
#define GST_CAT_DEFAULT gst_throttle_debug

enum
{
	PROP_0,
	PROP_PRINTONLY,
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE("sink",
	GST_PAD_SINK,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS("ANY")
);

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE("src",
	GST_PAD_SRC,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS("ANY")
);

GST_BOILERPLATE(GstThrottle, gst_throttle, GstElement, GST_TYPE_ELEMENT);

static void gst_throttle_set_property(GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_throttle_get_property(GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static gboolean gst_throttle_set_clock(GstElement *element, GstClock *clock);

static gboolean gst_throttle_handle_event(GstPad * pad, GstEvent * event);
static gboolean gst_throttle_set_caps(GstPad * pad, GstCaps * caps);
static GstFlowReturn gst_throttle_chain(GstPad * pad, GstBuffer * buf);

/* GObject vmethod implementations */

static void gst_throttle_base_init(gpointer gclass)
{
	GstElementClass *element_class = GST_ELEMENT_CLASS(gclass);

	gst_element_class_set_details_simple(element_class,
		"Throttle",
		"Filter/Network",
		"Throttles pipeline processing",
		"Petr Novak <pnovak@koukaam.se>"
	);

	gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&src_factory));
	gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&sink_factory));
}

/* initialize the throttle's class */
static void gst_throttle_class_init(GstThrottleClass * klass)
{
	GObjectClass *gobject_class;
	GstElementClass *gstelement_class;

	gobject_class = (GObjectClass *)klass;
	gstelement_class = (GstElementClass *)klass;

	gobject_class->set_property = gst_throttle_set_property;
	gobject_class->get_property = gst_throttle_get_property;
	
	g_object_class_install_property(gobject_class, PROP_PRINTONLY,
		g_param_spec_boolean("printonly", "Print only", "Do not throttle, just print buffer info", FALSE, G_PARAM_READWRITE)
	);
	
	gstelement_class->set_clock = gst_throttle_set_clock;
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void gst_throttle_init(GstThrottle * filter, GstThrottleClass * gclass)
{
	filter->sinkpad = gst_pad_new_from_static_template(&sink_factory, "sink");
	gst_pad_set_event_function(filter->sinkpad, gst_throttle_handle_event);
	gst_pad_set_setcaps_function(filter->sinkpad, GST_DEBUG_FUNCPTR(gst_throttle_set_caps));
	gst_pad_set_getcaps_function(filter->sinkpad, GST_DEBUG_FUNCPTR(gst_pad_proxy_getcaps));
	gst_pad_set_chain_function(filter->sinkpad, GST_DEBUG_FUNCPTR(gst_throttle_chain));

	filter->srcpad = gst_pad_new_from_static_template(&src_factory, "src");
	gst_pad_set_event_function(filter->srcpad, gst_throttle_handle_event);
	gst_pad_set_getcaps_function(filter->srcpad, GST_DEBUG_FUNCPTR(gst_pad_proxy_getcaps));

	gst_element_add_pad(GST_ELEMENT(filter), filter->sinkpad);
	gst_element_add_pad(GST_ELEMENT(filter), filter->srcpad);
	
	filter->printOnly = FALSE;
	filter->clock = NULL;
	filter->haveStartTime = FALSE;
}

static gboolean gst_throttle_handle_event(GstPad * pad, GstEvent * event)
{
	GstThrottle * filter = GST_THROTTLE(gst_pad_get_parent(pad));
	GST_DEBUG_OBJECT(pad, "passing event %s", GST_EVENT_TYPE_NAME(event));
	
	GstPad * otherpad = (pad == filter->srcpad) ? filter->sinkpad : filter->srcpad;
	gst_object_unref(filter);
	return gst_pad_push_event(otherpad, event);
}


static void gst_throttle_set_property(GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
	GstThrottle *filter = GST_THROTTLE(object);

	switch (prop_id) {
		case PROP_PRINTONLY:
			filter->printOnly = g_value_get_boolean(value);
		break;
		
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void gst_throttle_get_property(GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
	GstThrottle *filter = GST_THROTTLE(object);

	switch (prop_id) {
		case PROP_PRINTONLY:
			g_value_set_boolean(value, filter->printOnly);
		break;
		
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

/* GstElement vmethod implementations */

static gboolean gst_throttle_set_clock(GstElement * element, GstClock * clock)
{
	GstThrottle *filter = GST_THROTTLE(element);
	filter->clock = clock;
	return TRUE;
}

/* this function handles the link with other elements */
static gboolean gst_throttle_set_caps(GstPad * pad, GstCaps * caps)
{
	GstThrottle *filter;
	GstPad *otherpad;

	filter = GST_THROTTLE(gst_pad_get_parent(pad));
	otherpad = (pad == filter->srcpad) ? filter->sinkpad : filter->srcpad;
	gst_object_unref(filter);

	return gst_pad_set_caps(otherpad, caps);
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn gst_throttle_chain(GstPad * pad, GstBuffer * buf)
{
	GstThrottle * filter = GST_THROTTLE(GST_OBJECT_PARENT(pad));
	
	if (filter->printOnly)
	{
		GstCaps * caps = gst_buffer_get_caps(buf);
		gchar * capsStr = gst_caps_to_string(caps);
		gst_caps_unref(caps);
		GST_LOG_OBJECT(filter, "ts: %" GST_TIME_FORMAT " %sof type %s",
			GST_TIME_ARGS(buf->timestamp),
			GST_BUFFER_IS_DISCONT(buf) ? "and discontinuity " : "",
			capsStr
		);
		g_free(capsStr);
		
		GstFlowReturn ret = gst_pad_push(filter->srcpad, buf);
		GST_TRACE_OBJECT(filter, "ts: %" GST_TIME_FORMAT " processed with status %d", GST_TIME_ARGS(buf->timestamp), ret);
		return ret;
	}
	
	if (filter->clock == NULL)
	{
		return gst_pad_push(filter->srcpad, buf);
	}
	
	GstClockTime realTs = gst_clock_get_time(filter->clock);
	
	if (filter->haveStartTime)
	{
		const char * discont = GST_BUFFER_IS_DISCONT(buf) ? " with discotinuity" : "";
		
		GstClockTime expectedRealTs = filter->streamStartRealTime + buf->timestamp;
		gboolean early = realTs < expectedRealTs;
		if (early)
		{
			GstClockID * cid = gst_clock_new_single_shot_id(filter->clock, expectedRealTs);
			GST_TRACE_OBJECT(filter, "ts: %" GST_TIME_FORMAT " %s, waiting for %ld ms", GST_TIME_ARGS(buf->timestamp), discont, (expectedRealTs - realTs)/1000000);
			gst_clock_id_wait(cid, NULL);
			gst_clock_id_unref(cid);
		}
		else
		{
			GST_TRACE_OBJECT(filter, "ts: %" GST_TIME_FORMAT " %s, pad on time", GST_TIME_ARGS(buf->timestamp), discont);
		}
	}
	else
	{
		filter->streamStartRealTime = realTs - buf->timestamp;
		filter->haveStartTime = TRUE;
	}
	
	return gst_pad_push(filter->srcpad, buf);
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean throttle_init(GstPlugin * throttle)
{
	// debug category for filtering log messages
	GST_DEBUG_CATEGORY_INIT(gst_throttle_debug, "throttle", 0, "Throttling filter");

	return gst_element_register(throttle, "throttle", GST_RANK_NONE, GST_TYPE_THROTTLE);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "throttle"
#endif

// gstreamer looks for this structure to register throttles
GST_PLUGIN_DEFINE(
	GST_VERSION_MAJOR,
	GST_VERSION_MINOR,
	"throttle",
	"Filter that throttles pipeline throughput to real time",
	throttle_init,
	VERSION,
	"LGPL",
	"Throttle",
	"https://github.com/che0/gst-throttle"
)
