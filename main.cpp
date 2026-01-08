#include <gst/gst.h>
#include <glib.h>

typedef struct _CustomData {
  GstElement *pipeline;
  GstElement *video_queue;
  GstElement *video_convert;
  GstElement *video_sink;
} CustomData;

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
  GMainLoop *loop = (GMainLoop *)data;

  switch (GST_MESSAGE_TYPE(msg)) {
  case GST_MESSAGE_EOS:
    g_print("Yayın bitti.\n");
    g_main_loop_quit(loop);
    break;

  case GST_MESSAGE_ERROR: {
    gchar *debug;
    GError *error;
    gst_message_parse_error(msg, &error, &debug);
    g_free(debug);
    g_printerr("Hata: %s\n", error->message);
    g_error_free(error);
    g_main_loop_quit(loop);
    break;
  }
  default:
    break;
  }
  return TRUE;
}

static void on_pad_added(GstElement *element, GstPad *pad, gpointer data) {
  CustomData *cdata = (CustomData *)data;
  GstPad *sinkpad = NULL;
  GstCaps *caps;
  GstStructure *str;
  const gchar *name;

  caps = gst_pad_get_current_caps(pad);
  if (!caps) {
    caps = gst_pad_query_caps(pad, NULL);
  }
  
  str = gst_caps_get_structure(caps, 0);
  name = gst_structure_get_name(str);
  g_print("Pad bulundu: %s\n", name);

  if (g_str_has_prefix(name, "video/x-raw")) {
    sinkpad = gst_element_get_static_pad(cdata->video_queue, "sink");
  }

  if (sinkpad) {
    if (!gst_pad_is_linked(sinkpad)) {
      if (gst_pad_link(pad, sinkpad) == GST_PAD_LINK_OK) {
          g_print("+++ BAĞLANDI: %s\n", name);
      } else {
          g_print("!!! BAĞLANTI HATASI: %s\n", name);
      }
    } else {
        g_print("... Zaten bağlı: %s\n", name);
    }
    gst_object_unref(sinkpad);
  }
  
  if (caps) gst_caps_unref(caps);
}

int tutorial_main(int argc, char *argv[], void *user_data) {
  CustomData cdata; 
  GMainLoop *loop;
  GstElement *source, *decoder;
  GstBus *bus;
  guint bus_watch_id;

  // GStreamer'ı başlat
  gst_init(&argc, &argv);

  loop = g_main_loop_new(NULL, FALSE);

  cdata.pipeline = gst_pipeline_new("video-player");
  source = gst_element_factory_make("filesrc", "file-source");
  
  // DÜZELTME 1: decodebin3 YERİNE decodebin KULLANIYORUZ
  decoder = gst_element_factory_make("decodebin", "decoder");

  cdata.video_queue = gst_element_factory_make("queue", "video_queue");

  cdata.video_convert = gst_element_factory_make("videoconvert", "video_converter");
  
  // macOS için Video Sink
  cdata.video_sink = gst_element_factory_make("osxvideosink", "outputvideo");
  if (!cdata.video_sink) cdata.video_sink = gst_element_factory_make("autovideosink", "outputvideo");
  
  // DÜZELTME 2: Ses gelmese bile videonun başlaması için senkronizasyonu gevşetiyoruz
  // Eğer ses akışı bozuksa video onu beklemesin.


  g_object_set(G_OBJECT(source), "location", "./bbb.mp4", NULL);

  bus = gst_pipeline_get_bus(GST_PIPELINE(cdata.pipeline));
  bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
  gst_object_unref(bus);

  gst_bin_add_many(GST_BIN(cdata.pipeline), source, decoder, 
                   cdata.video_queue, cdata.video_convert, cdata.video_sink, 
                    NULL);

  // Statik bağlantılar
  if (!gst_element_link_many(cdata.video_queue, cdata.video_convert, cdata.video_sink, NULL)) {
      g_printerr("Video zinciri bağlanamadı.\n");
  }
  
  if (!gst_element_link(source, decoder)) {
      g_printerr("Source -> Decoder bağlanamadı.\n");
  }

  // Dinamik bağlantı
  g_signal_connect(decoder, "pad-added", G_CALLBACK(on_pad_added), &cdata);

  g_print("Pipeline durumu PLAYING yapılıyor...\n");
  gst_element_set_state(cdata.pipeline, GST_STATE_PLAYING);

  g_print("Main Loop Başlıyor...\n");
  g_main_loop_run(loop);

  gst_element_set_state(cdata.pipeline, GST_STATE_NULL);
  gst_object_unref(GST_OBJECT(cdata.pipeline));
  g_source_remove(bus_watch_id);
  g_main_loop_unref(loop);

  return 0;
}

int main(int argc, char *argv[]) {
  return gst_macos_main(tutorial_main, argc, argv, NULL);
}