#include <gst/gst.h>

int main(int argc, char *argv[]) {
  GstElement *pipeline, *source, *convert, *sink;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;

  gst_init(&argc, &argv);

  /* Tạo từng element riêng lẻ qua factory */
  source = gst_element_factory_make("audiotestsrc", "source");
  convert = gst_element_factory_make("audioconvert", "convert");
  sink = gst_element_factory_make("autoaudiosink", "sink");

  pipeline = gst_pipeline_new("test-pipeline");

  if (!pipeline || !source || !convert || !sink) {
    g_printerr("Không thể tạo một trong các element.\n");
    return -1;
  }

  /* Đưa element vào pipeline (pipeline là một GstBin) */
  gst_bin_add_many(GST_BIN(pipeline), source, convert, sink, NULL);

  /* Liên kết element theo đúng thứ tự dữ liệu chảy qua */
  if (gst_element_link_many(source, convert, sink, NULL) != TRUE) {
    g_printerr("Liên kết element thất bại.\n");
    gst_object_unref(pipeline);
    return -1;
  }

  /* Chỉnh thuộc tính (property) của element qua GObject API */
  g_object_set(source, "freq", 440.0, NULL);

  ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr("Không thể chuyển sang PLAYING.\n");
    gst_object_unref(pipeline);
    return -1;
  }

  bus = gst_element_get_bus(pipeline);
  msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                   GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

  if (msg != NULL) {
    GError *err;
    gchar *debug_info;

    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR:
      gst_message_parse_error(msg, &err, &debug_info);
      g_printerr("Lỗi từ %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
      g_printerr("Chi tiết debug: %s\n", debug_info ? debug_info : "không có");
      g_clear_error(&err);
      g_free(debug_info);
      break;
    case GST_MESSAGE_EOS:
      g_print("Đã đến cuối luồng (EOS).\n");
      break;
    default:
      g_printerr("Message không mong đợi.\n");
      break;
    }
    gst_message_unref(msg);
  }

  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);

  return 0;
}