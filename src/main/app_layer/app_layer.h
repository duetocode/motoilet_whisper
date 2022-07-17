#ifndef APP_LAYER_H
#define APP_LAYER_H

struct motoilet_whisper_message
{
    unsigned char type;
};

struct whisper_app_layer__config
{
    void (*message_received_cb)(motoilet_whisper_message *message);
};

char whisper_app_layer__init(whisper_app_layer__config *cfg);

char whisper_app_layer__data_received(const unsigned char *data, unsigned char data_length);
char whisper_app_layer__data_send(const motoilet_whisper_message *message);

#endif // APP_LAYER_H