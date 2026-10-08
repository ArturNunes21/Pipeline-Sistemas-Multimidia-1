#include <gst/gst.h>
#include <glib.h>

static int run_pipeline_and_wait(const char *description, const char *launch_str) {
    GError *parse_err = NULL;

    g_print("\n=== %s ===\n", description);

    GstElement *pipeline = gst_parse_launch(launch_str, &parse_err);
    if (!pipeline) {
        g_printerr("ERRO ao criar a pipeline '%s': %s\n", description,
                   parse_err ? parse_err->message : "motivo nao aparente");
        if (parse_err) g_error_free(parse_err);
        return -1;
    }   

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("ERRO: a pipeline '%s' nao conseguiu ir para o estado PLAYING.\n",
                   description);
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        return -1;
    }

    GstBus *bus = gst_element_get_bus(pipeline);
    GstMessage *msg = gst_bus_timed_pop_filtered(
        bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    int exit_code = 0;
    if (msg != NULL) {
        if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
            GError *err;
            gchar *debug_info;
            gst_message_parse_error(msg, &err, &debug_info);
            g_printerr("ERRO durante a execucao de '%s': %s\n", description, err->message);
            g_printerr("  detalhes do erro: %s\n", debug_info ? debug_info : "nenhum");
            g_error_free(err);
            g_free(debug_info);
            exit_code = -1;
        } else {
            g_print("'%s' concluida com sucesso (EOS).\n", description);
        }
        gst_message_unref(msg);
    }

    gst_object_unref(bus);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return exit_code;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    int r1 = run_pipeline_and_wait(
        "Video H.264 (video_h264.mp4)",
        "videotestsrc pattern=smpte num-buffers=150 ! "
        "video/x-raw,width=320,height=240,framerate=30/1 ! "
        "videoconvert ! "
        "x264enc tune=zerolatency speed-preset=ultrafast ! "
        "h264parse ! "
        "mp4mux ! "
        "filesink location=video_h264.mp4");

    int r2 = run_pipeline_and_wait(
        "Audio PCM - Configuracao A: 44100 Hz / 16 bits / estereo (audio_configA.wav)",
        "audiotestsrc wave=sine freq=440 num-buffers=215 ! "
        "audioconvert ! audioresample ! "
        "audio/x-raw,format=S16LE,rate=44100,channels=2 ! "
        "wavenc ! filesink location=audio_configA.wav");

    int r3 = run_pipeline_and_wait(
        "Audio PCM - Configuracao B: 8000 Hz / 8 bits / mono (audio_configB.wav)",
        "audiotestsrc wave=sine freq=440 num-buffers=39 ! "
        "audioconvert ! audioresample ! "
        "audio/x-raw,format=U8,rate=8000,channels=1 ! "
        "wavenc ! filesink location=audio_configB.wav");

    g_print("\nArquivos gerados: video_h264.mp4, audio_configA.wav, "
            "audio_configB.wav\n");
    g_print("Para ouvir a diferenca: ffplay audio_configA.wav   /   ffplay audio_configB.wav\n");
    g_print("Para ver o video: ffplay video_h264.mp4\n");

    return (r1 == 0 && r2 == 0 && r3 == 0) ? 0 : -1;
}
