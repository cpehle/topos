#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <SDL2/SDL.h>

int main(int argc, char **argv) {
  
  AVFormatContext *format_context = NULL;
  AVCodecContext *video_codec_context;
  AVCodecContext *audio_codec_context;
  AVCodec *video_codec;
  AVCodec *audio_codec;
  AVFrame *frame;
  
  av_register_all();
  avcodec_register_all();

  if (argc < 2) {
    return 0;
  }

  format_context = avformat_alloc_context();
  if (avformat_open_input(&format_context, argv[1], NULL, NULL)) {
    // TODO: Log error.
    printf("Unable to open input.\n");
    exit(1);
  }

  if (avformat_find_stream_info(format_context, NULL)) {
    // TODO: Log error.
    printf("Unable to find stream info\n");
    exit(1);
  }

  int audio_stream_index = -1;
  int video_stream_index = -1;
  for (int i = 0, N = format_context->nb_streams; i < N; ++i) {
    if (format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
      audio_stream_index = i;
    }
    if (format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
      video_stream_index = i;
    }
    if (video_stream_index >= 0 && audio_stream_index >= 0) {
      break;
    }
  }

  if (video_stream_index < 0 || audio_stream_index < 0) {
    // TODO: Log error.
    printf("Unable to find audio or video stream.");
    exit(1);
  }

  video_codec_context = format_context->streams[video_stream_index]->codec;
  if (!video_codec_context) {
    exit(1);
  }
  audio_codec_context = format_context->streams[audio_stream_index]->codec;

  video_codec = avcodec_find_decoder(video_codec_context->codec_id);

  if (avcodec_open2(video_codec_context, video_codec, NULL) < 0) {
    // TODO: Log error.
    exit(1);
  }

  frame = av_frame_alloc();
  if (!frame) {
    // TODO: Log error.
    exit(1);
  }

  SDL_Renderer *renderer;
  SDL_Texture *texture;
  SDL_Window *window;
  {

    int flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;

    if (SDL_Init(flags)) {
      // TODO: Log error.
      exit(1);
    }

    window =
        SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                         video_codec_context->width,
                         video_codec_context->height, SDL_WINDOW_SHOWN);
    if (!window) {
      // TODO: Log error: SDL_GetError();
      SDL_Quit();
      exit(1);
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED |
                                                  SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
      SDL_DestroyWindow(window);
      // Todo: Log error: SDL_GetError();
      SDL_Quit();
      exit(1);
    }

    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer, &info);

    texture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING,
        video_codec_context->width, video_codec_context->height);
    if (!texture) {
      SDL_DestroyRenderer(renderer);
      SDL_DestroyWindow(window);
      SDL_Quit();
      exit(1);
    }
  }

  SDL_Event event;
  int done = SDL_FALSE;
  AVPacket packet;
  av_init_packet(&packet);
  int got_frame;
  while (!done && (av_read_frame(format_context, &packet) >= 0)) {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_ESCAPE) {
          done = SDL_TRUE;
        }
        break;
      case SDL_QUIT:
        done = SDL_TRUE;
        break;
      }
    }

    if (packet.stream_index == video_stream_index) {
      avcodec_decode_video2(video_codec_context, frame, &got_frame, &packet);

      if (got_frame) {

        SDL_UpdateYUVTexture(texture, NULL, frame->data[0], frame->linesize[0],
                             frame->data[1], frame->linesize[1], frame->data[2],
                             frame->linesize[2]);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
      }
    }
    av_free_packet(&packet);
  }

  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
