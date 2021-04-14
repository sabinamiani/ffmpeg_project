#include <iostream> 
#include <string>
#include <fstream> 

extern "C" {
#include <libavcodec/packet.h> // AVPacket
#include <libavutil/frame.h> // AVFrame 
#include <libavformat/avformat.h> // AVFormatContext
#include <libavcodec/avcodec.h> // AVCodecContext
#include <libswscale/swscale.h>
  // #include "bytestream.h"
};

int main(int argc, const char *argv[]) {

  std::string filename = argv[1]; 
  std::cout << filename << std::endl;

  // take the image file and create an AVFormat object
  AVFormatContext *av_format = NULL;

  if (avformat_open_input(&av_format, argv[1], NULL, NULL)!= 0) {
      std::cout << "error" << std::endl;
      return -1;
  }

  avformat_find_stream_info(av_format, NULL);

  av_dump_format(av_format, 0, argv[1], 0);
  
  // create the codec context with correct width, height, and pixel format
  //  AVCodecContext *av_codec_context = av_format->streams[0]->codec;
  
  AVCodec *av_codec = avcodec_find_decoder(av_format->streams[0]->codecpar->codec_id); //av_codec_context->codec_id);
  AVCodecContext *av_codec_context = avcodec_alloc_context3(av_codec);
  avcodec_parameters_to_context(av_codec_context, av_format->streams[0]->codecpar);
  
  // open the codec 
  avcodec_open2(av_codec_context, av_codec, NULL);

  // allocate frame and packet space 
  AVFrame *copy = av_frame_alloc();
  int numBytes = avpicture_get_size(av_codec_context->pix_fmt, av_codec_context->width, av_codec_context->height);
  uint8_t *buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
  avpicture_fill( (AVPicture *)copy, buffer, av_codec_context->pix_fmt, av_codec_context->width, av_codec_context->height);

  // read frame 
  int frame_finished = 0;

  AVPacket packet; 
  packet.data = NULL;
  packet.size = 0;
  av_init_packet(&packet);

  AVFrame *frame = av_frame_alloc();
  
  while (av_read_frame(av_format, &packet) >= 0) {
    if(packet.stream_index != 0)
            continue;
    int ret = avcodec_decode_video2(av_codec_context, frame, &frame_finished, &packet);
    if (ret > 0) {
      printf("Frame is decoded, size %d\n", ret);
      break;
    }
  }

  // make output packet 
  AVPacket output;
  output.data = NULL;
  output.size = 0;
  av_init_packet(&output);

  // convert to .cool encoding 
  AVCodec *encoder = avcodec_find_encoder_by_name("cool");
  // std::cout << encoder->name << std::endl;
  // std::cout << av_codec_context->width << std::endl << +(output.data) << std::endl << frame->pkt_size << std::endl;
  int got_pkt;
  encoder->encode2(av_codec_context, &output, frame, &got_pkt); 

  struct SwsContext *sws_ctx = NULL;
  sws_ctx = sws_getContext(frame->width, frame->height, av_codec_context->pix_fmt, frame->width,
                           frame->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);

  sws_scale(sws_ctx, (uint8_t const * const *)frame->data,
              frame->linesize, 0, frame->height,
              copy->data, copy->linesize);

  // loop x300
  for (int i = 0; i <= 2; i++) {
    // modifying to add ball
    AVPacket mod;// = NULL;
    av_copy_packet(&mod, &output);

    
    
    // export AVFrame to file
    std::string filename = "ball" + std::to_string(i) + ".cool";
    FILE *file = fopen(const_cast<char*>(filename.c_str()), "wb");

    fwrite(mod.data, 1, mod.size, file);

    fclose(file);
  }

  return 0;

}
