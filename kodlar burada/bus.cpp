//BUS TUTORIAL

#include <gst/gst.h>

static GMainLoop *loop;

static gboolean
my_bus_callback (GstBus * bus, GstMessage * message, gpointer data)
{
  // Mesaj tipini ve ismini yazdıralım
  g_print ("Bus Mesaji Alindi: %s (Source: %s)\n", 
            GST_MESSAGE_TYPE_NAME (message), 
            GST_MESSAGE_SRC_NAME (message));

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:{
      GError *err;
      gchar *debug;

      gst_message_parse_error (message, &err, &debug);
      g_print ("Hata: %s\n", err->message);
      g_error_free (err);
      g_free (debug);

      g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_EOS:
      g_print ("Akiş bitti (EOS).\n");
      g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_STATE_CHANGED:
      // Durum değişikliklerini görmek için (NULL -> READY -> PAUSED -> PLAYING)
      {
        GstState old_state, new_state, pending_state;
        gst_message_parse_state_changed (message, &old_state, &new_state, &pending_state);
        g_print ("Durum değişti: %s -> %s\n", 
                  gst_element_state_get_name (old_state), 
                  gst_element_state_get_name (new_state));
      }
      break;
    default:
      break;
  }

  return TRUE;
}

// 3 saniye sonra döngüyü kapatmak için yardımcı fonksiyon (Test amaçlı)
static gboolean
quit_loop_timeout (gpointer data)
{
  g_print ("Test süresi doldu, döngüden çikiliyor...\n");
  g_main_loop_quit (loop);
  return FALSE;
}

gint
main (gint argc, gchar * argv[])
{
  GstElement *pipeline;
  GstBus *bus;
  guint bus_watch_id;
  GstElement *sink;

  /* init */
  gst_init (&argc, &argv);

  /* * Pipeline oluşturma. 
   * Boş bir pipeline yerine basit bir 'fakesrc' elementi ekleyelim ki
   * hata vermeden state değiştirebilsin.
   */
  pipeline = gst_pipeline_new ("my_pipeline");
  GstElement *source = gst_element_factory_make ("fakesrc", "source");
  
  if (!source) {
      g_print ("fakesrc oluşturulamadi.\n");
      return -1;
  }
  g_object_set (source, "num-buffers", 5, NULL);
  sink = gst_element_factory_make ("fakesink", "sink");
  gst_bin_add_many (GST_BIN (pipeline), source,sink,NULL);
  if (gst_element_link (source, sink) != TRUE) {
        g_print ("Elementler bağlanamadı!\n");
        return -1;
    }
  /* Bus izleyiciyi ekle */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, my_bus_callback, NULL);
  gst_object_unref (bus);

  /* loop oluştur */
  loop = g_main_loop_new (NULL, FALSE);

  /* * KRİTİK NOKTA: Pipeline'ı çalıştır!
   * Bunu yapmazsanız Bus'a hiçbir mesaj düşmez.
   */
  g_print ("Pipeline başlatiliyor...\n");
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  /* Test için 3 saniye sonra otomatik çıkış ayarla */
  g_timeout_add_seconds (3, quit_loop_timeout, NULL);

  /* Döngüyü başlat */
  g_main_loop_run (loop);

  /* Temizlik */
  g_print ("Temizlik yapiliyor...\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);

  return 0;
}
