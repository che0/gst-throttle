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
 * SECTION:element-splitter
 *
 * FIXME:Describe splitter here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! splitter ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>

#include "gstsplitter.h"

GST_DEBUG_CATEGORY_STATIC(gst_splitter_debug);
#define GST_CAT_DEFAULT gst_splitter_debug

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE("sink",
	GST_PAD_SINK,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS("ANY")
);

static GstStaticPadTemplate mpeg_audio_src_factory = GST_STATIC_PAD_TEMPLATE("audiosrc",
	GST_PAD_SRC,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS("audio/mpeg")
);

static GstStaticPadTemplate h264_src_factory = GST_STATIC_PAD_TEMPLATE("videosrc",
	GST_PAD_SRC,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS("video/x-h264")
);

GST_BOILERPLATE(GstSplitter, gst_splitter, GstElement, GST_TYPE_ELEMENT);

static gboolean gst_splitter_handle_event(GstPad * pad, GstEvent * event);
static GstCaps * gst_splitter_getcaps_any(GstPad * pad);
static GstFlowReturn gst_splitter_chain(GstPad * pad, GstBuffer * buf);

/* GObject vmethod implementations */

static void gst_splitter_base_init(gpointer gclass)
{
	GstElementClass * element_class = GST_ELEMENT_CLASS(gclass);
	gst_element_class_set_details_simple(element_class,
		"Splitter",
		"Filter/Network",
		"Splits muxed input to H.264 and MPEG audio output",
		"Petr Novak <pnovak@koukaam.se>"
	);

	gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&sink_factory));
	gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&mpeg_audio_src_factory));
	gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&h264_src_factory));
	
	GstSplitterClass * splitter_class = GST_SPLITTER_CLASS(gclass);
	splitter_class->videocaps = gst_caps_new_simple("video/x-h264", NULL);
	splitter_class->audiocaps = gst_caps_new_simple("audio/mpeg", NULL);
}

/* initialize the splitter's class */
static void gst_splitter_class_init(GstSplitterClass * klass)
{
	GObjectClass *gobject_class;
	GstElementClass *gstelement_class;

	gobject_class = (GObjectClass *)klass;
	gstelement_class = (GstElementClass *)klass;
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void gst_splitter_init(GstSplitter * splitter, GstSplitterClass * gclass)
{
	splitter->sinkpad = gst_pad_new_from_static_template(&sink_factory, "sink");
	gst_pad_set_event_function(splitter->sinkpad, gst_splitter_handle_event);
	gst_pad_set_getcaps_function(splitter->sinkpad, gst_splitter_getcaps_any);
	gst_pad_set_chain_function(splitter->sinkpad, gst_splitter_chain);

	splitter->audiosrcpad = gst_pad_new_from_static_template(&mpeg_audio_src_factory, "audiosrc");
	gst_pad_set_event_function(splitter->audiosrcpad, gst_splitter_handle_event);
	gst_pad_set_getcaps_function(splitter->audiosrcpad, gst_splitter_getcaps_any);
	
	splitter->videosrcpad = gst_pad_new_from_static_template(&h264_src_factory, "videosrc");
	gst_pad_set_event_function(splitter->videosrcpad, gst_splitter_handle_event);
	gst_pad_set_getcaps_function(splitter->videosrcpad, gst_splitter_getcaps_any);

	gst_element_add_pad(GST_ELEMENT(splitter), splitter->sinkpad);
	gst_element_add_pad(GST_ELEMENT(splitter), splitter->audiosrcpad);
	gst_element_add_pad(GST_ELEMENT(splitter), splitter->videosrcpad);
}

static gboolean gst_splitter_handle_event(GstPad * pad, GstEvent * event)
{
	GstSplitter * splitter = GST_SPLITTER(gst_pad_get_parent(pad));
	
	GstObject * source = GST_EVENT_SRC(event);
	
	
	if (pad == splitter->sinkpad)
	{
		GstPad * destination = NULL;
		
		if ((GST_EVENT_TYPE(event) == GST_EVENT_TAG) && GST_IS_PAD(source))
		{
			GstSplitterClass * splitter_class = GST_SPLITTER_CLASS(G_OBJECT_GET_CLASS(splitter));
			GstCaps * sourceCaps = GST_PAD_CAPS(GST_PAD(source));
			if (GST_IS_CAPS(sourceCaps))
			{
			
				if (gst_caps_can_intersect(splitter_class->videocaps, sourceCaps))
				{
					destination = splitter->videosrcpad;
				}
				else if (gst_caps_can_intersect(splitter_class->audiocaps, sourceCaps))
				{
					destination = splitter->audiosrcpad;
				}
			}
		}
		
		if (destination != NULL)
		{
			GST_DEBUG_OBJECT(pad, "passing event %s from %s:%s to our %s", GST_EVENT_TYPE_NAME(event), GST_OBJECT_NAME(GST_OBJECT_PARENT(source)), GST_OBJECT_NAME(source), GST_OBJECT_NAME(destination));
			gst_pad_push_event(destination, gst_event_copy(event));
		}
		else
		{ // no fixed dest -> forward to all
			GST_DEBUG_OBJECT(pad, "passing event %s from %s:%s to all", GST_EVENT_TYPE_NAME(event), GST_OBJECT_NAME(GST_OBJECT_PARENT(source)), GST_OBJECT_NAME(source));
			gst_pad_push_event(splitter->audiosrcpad, gst_event_copy(event));
			gst_pad_push_event(splitter->videosrcpad, gst_event_copy(event));
		}
	}
	else
	{
		gst_pad_push_event(splitter->sinkpad, gst_event_copy(event));
	}
	
	gst_object_unref(splitter);
	return TRUE;
}

static GstCaps * gst_splitter_getcaps_any(GstPad * pad)
{
	return gst_caps_new_any();
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn gst_splitter_chain(GstPad * pad, GstBuffer * buf)
{
	GstSplitter * splitter = GST_SPLITTER(GST_OBJECT_PARENT(pad));
	
	GstCaps * caps = GST_BUFFER_CAPS(buf);
	if (!GST_IS_CAPS(caps))
	{
		GST_TRACE_OBJECT(splitter, "seen ts: %" GST_TIME_FORMAT", has not caps at all -> /dev/null", GST_TIME_ARGS(buf->timestamp));
		return GST_FLOW_OK;
	}
	
	GstSplitterClass * splitter_class = GST_SPLITTER_CLASS(G_OBJECT_GET_CLASS(splitter));
	if (gst_caps_can_intersect(splitter_class->videocaps, caps))
	{
		GST_TRACE_OBJECT(splitter, "seen ts: %" GST_TIME_FORMAT" -> video source", GST_TIME_ARGS(buf->timestamp));
		return gst_pad_push(splitter->videosrcpad, buf);
	}
	else if (gst_caps_can_intersect(splitter_class->audiocaps, caps))
	{
		GST_TRACE_OBJECT(splitter, "seen ts: %" GST_TIME_FORMAT" -> audio source", GST_TIME_ARGS(buf->timestamp));
		return gst_pad_push(splitter->audiosrcpad, buf);
	}
	
	GST_TRACE_OBJECT(splitter, "seen ts: %" GST_TIME_FORMAT", no compatible caps -> /dev/null", GST_TIME_ARGS(buf->timestamp));
	return GST_FLOW_OK;
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean splitter_init(GstPlugin * splitter)
{
	// debug category for filtering log messages
	GST_DEBUG_CATEGORY_INIT(gst_splitter_debug, "splitter", 0, "Stream splitting filter");

	return gst_element_register(splitter, "splitter", GST_RANK_NONE, GST_TYPE_SPLITTER);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */

#ifndef PACKAGE
#define PACKAGE "splitter"
#endif

// gstreamer looks for this structure to register splitters
GST_PLUGIN_DEFINE(
	GST_VERSION_MAJOR,
	GST_VERSION_MINOR,
	"splitter",
	"Element that splits buffers to H.264 video and MPEG audio outputs",
	splitter_init,
	VERSION,
	"LGPL",
	"Splitter",
	"https://github.com/che0/gst-throttle"
)
