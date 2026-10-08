CC = gcc
CFLAGS = $(shell pkg-config --cflags gstreamer-1.0)
LIBS = $(shell pkg-config --libs gstreamer-1.0)

av_pipeline: av_pipeline.c
	$(CC) av_pipeline.c -o av_pipeline $(CFLAGS) $(LIBS)

run: av_pipeline
	./av_pipeline

clean:
	rm -f av_pipeline video_h264.mp4 audio_configA.wav audio_configB.wav
