/// CS3505 FFMPEG Project 2
/// Sabina Miani and Dylan Habersetzer
/// 26 April 2021

/* Citations

https://stackoverflow.com/questions/13163106/ffmpeg-converting-image-sequence-to-video-results-in-blank-video
http://random-stuff-mine.blogspot.com/2014/01/decoding-jpeg-image-file-using-libavcodec.html
https://trac.ffmpeg.org/wiki/Including%20FFmpeg%20headers%20in%20a%20C%2B%2B%20application
https://stackoverflow.com/questions/44048078/how-to-fill-avframe-pixel-by-pixel
https://stackoverflow.com/questions/49492259/writing-binary-data-to-fstream-in-c
https://stackoverflow.com/questions/38808946/copying-a-decoded-ffmpeg-avframe

 */

#include <iostream> 
#include <string>
#include <fstream>
#include <math.h>
#include <cmath>

// external C import libraries 
extern "C" {
#include <libavcodec/packet.h> // AVPacket
#include <libavutil/frame.h> // AVFrame 
#include <libavformat/avformat.h> // AVFormatContext
#include <libavcodec/avcodec.h> // AVCodecContext
#include <libswscale/swscale.h>
};

// forward declarations
void draw_ball(AVFrame* frame);
void write_ball_to_file(AVFrame* frame, AVCodecContext* oldContext, std::string filename);
double distance (int x1, int y1, int x2, int y2);
void read_frame (AVFrame *frame, AVFormatContext *av_format, AVCodecContext *av_codec_context);

int frame_inc = 0;

/* Main execution loop
 */
int main(int argc, const char *argv[]) {

  // take the image file and create an AVFormat object
  AVFormatContext *av_format = NULL;

  if (avformat_open_input(&av_format, argv[1], NULL, NULL)!= 0) {
      std::cout << "error" << std::endl;
      return -1;
  }

  avformat_find_stream_info(av_format, NULL);
  av_dump_format(av_format, 0, argv[1], 0);
  
  // create the codec context with correct width, height, and pixel format
  AVCodec *av_codec = avcodec_find_decoder(av_format->streams[0]->codecpar->codec_id);
  AVCodecContext *av_codec_context = avcodec_alloc_context3(av_codec);
  avcodec_parameters_to_context(av_codec_context, av_format->streams[0]->codecpar);
  
  // open the codec 
  avcodec_open2(av_codec_context, av_codec, NULL);
  
  // read frame
  AVFrame *frame = av_frame_alloc();
  read_frame(frame, av_format, av_codec_context);

  // frame is now read in 
  // copy frame 300 times and draw ball and write out frame as .cool file 

  for(int i = 0; i < 300; i++)
  {
    std::string zeros = "";
    if (i < 10)
      zeros = "00";
    else if (i < 100)
      zeros = "0";
    std::string filename = "ball" + zeros + std::to_string(i) + ".cool";
    write_ball_to_file(frame, av_codec_context, filename);
  }

  return 0;

}

/* Decodes the specified frame 
 */
void read_frame (AVFrame *frame, AVFormatContext *av_format, AVCodecContext *av_codec_context)
{
  AVPacket packet; 
  packet.data = NULL;
  packet.size = 0;
  av_init_packet(&packet);
  
  int frame_finished = 0;
  av_read_frame(av_format, &packet);
  avcodec_decode_video2(av_codec_context, frame, &frame_finished, &packet);
}


/* Write 
 */
void write_ball_to_file(AVFrame* frame, AVCodecContext* oldContext, std::string filename) {
  // receive frame and filename 
  // open encoder 
  AVCodec *encoder = avcodec_find_encoder_by_name("cool");
  
  // create codec context 
  AVCodecContext *context = avcodec_alloc_context3(encoder);
  context->width = frame->width;
  context->height = frame->height; 
  context->pix_fmt = encoder->pix_fmts[0];
  context->time_base.num = 1;
  context->time_base.den = 60;

  // create packet 
  AVPacket output;
  output.data = NULL;
  output.size = 0;
  av_init_packet(&output);

  // allocate frame
  AVFrame *copy = av_frame_alloc();
  copy->height = context->height;
  copy->width = context->width;
  int numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, context->width, context->height);
  uint8_t *buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
  avpicture_fill( (AVPicture *)copy, buffer, AV_PIX_FMT_RGB24, context->width, context->height);

  // start code to copy frame 
  struct SwsContext *sws_ctx = NULL;
  sws_ctx = sws_getContext(frame->width, frame->height, oldContext->pix_fmt, frame->width,
                           frame->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);

  sws_scale(sws_ctx, (uint8_t const * const *)frame->data,
              frame->linesize, 0, frame->height,
              copy->data, copy->linesize);

  draw_ball(copy); 

  // encode the frame
  int got_pkt = 0;
  encoder->encode2(context, &output, copy, &got_pkt); 
 
  // write the packet to the file                                                                                                                   
  FILE *file = fopen(const_cast<char*>(filename.c_str()), "wb");
  fwrite(output.data, 1, output.size, file);
  fclose(file);

  // free packet memory 
  av_free_packet(&output);
  
  return; 
}


/* Write pixel data to an AVFrame copy with a ball in the data
 */
void draw_ball(AVFrame* frame) {

  int ball_diameter = std::min(frame->height, frame->width) * 0.1;

  int centerX = frame->width / 2;
  int centerY = std::abs(sin(frame_inc*0.04))*0.8*frame->height + 0.1*frame->height;

  for (int y = 0; y < frame->height; y++)
      for (int x = 0; x < frame->width; x++)
      {
	if (distance(x, y, centerX, centerY) <= ball_diameter) 
	{
	  frame->data[0][y * frame->linesize[0] + x*3 + 0] = 0; // R
        	  frame->data[0][y * frame->linesize[0] + x*3 + 1] = 0; // G
        	  frame->data[0][y * frame->linesize[0] + x*3 + 2] = 255; // B 
	}
      }
  frame_inc++;
  return;
}

/* Calculates the distance from origin pt to outside of circle
 */
double distance (int x1, int y1, int x2, int y2)
{
  int a = x1 - x2;
  int b = y1 - y2;
  a = a * a;
  b = b * b;
  return std::sqrt(a + b);
}
