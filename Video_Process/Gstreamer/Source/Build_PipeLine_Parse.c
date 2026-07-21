#include <gst/gst.h>

int main(int argc, char *argv[]) {
  GstElement *pipeline;
  GstBus *bus;
  GstMessage *msg;

  gst_init(&argc,
           &argv); /* Bắt buộc: khởi tạo trước mọi lời gọi GStreamer khác */

  /* Dựng pipeline: nguồn kiểm tra hình -> chuyển đổi màu -> hiển thị */
  pipeline =
      gst_parse_launch("videotestsrc ! videoconvert ! autovideosink", NULL);

  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  /* Chặn (block) luồng hiện tại cho đến khi có lỗi hoặc hết luồng (EOS) */
  bus = gst_element_get_bus(pipeline);
  msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                   GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

  if (msg != NULL)
    gst_message_unref(msg);

  gst_object_unref(bus);
  gst_element_set_state(
      pipeline, GST_STATE_NULL); /* luôn đưa về NULL để giải phóng tài nguyên */
  gst_object_unref(pipeline);

  return 0;
}