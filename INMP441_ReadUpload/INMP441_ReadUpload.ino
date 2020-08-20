#include <driver/i2s.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

const char* ssid     = "MaxHome3BB";
const char* password = "987654321";

#define PIN_WS   15
#define PIN_SD   13
#define PIN_CLK  2

#define GAIN 10

#define RECORD_TIME 5
#define SAMPLE_RATE 44100
#define SOUND_DATA_BYTE 2
#define SOUND_DATA_LENGHT (RECORD_TIME * SAMPLE_RATE * SOUND_DATA_BYTE) // Bytes
#define SAMPLE_DIVIDE 100
#define SAMPLE_COUNT ((RECORD_TIME * SAMPLE_RATE) / SAMPLE_DIVIDE)

uint8_t* soundData;

void i2sInit() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // is fixed at 12bit, stereo, MSB
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 2,
    .dma_buf_len = 128,
  };

  i2s_pin_config_t pin_config;
  pin_config.bck_io_num   = PIN_CLK;
  pin_config.ws_io_num    = PIN_WS;
  pin_config.data_out_num = I2S_PIN_NO_CHANGE;
  pin_config.data_in_num  = PIN_SD;


  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_set_clk(I2S_NUM_0, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
}

void setup() {
  Serial.begin(115200);

  soundData = (uint8_t*)ps_malloc(SOUND_DATA_LENGHT);
  i2sInit();

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  Serial.println("Start record");

  size_t readSize;
  uint32_t totalSize = 0;
  for (uint16_t i = 0; i < SAMPLE_COUNT; i++) {
    i2s_read(I2S_NUM_0, (uint8_t*)(&soundData[i *  (SOUND_DATA_LENGHT / SAMPLE_COUNT)]), SOUND_DATA_LENGHT / SAMPLE_COUNT, &readSize, (500 / portTICK_RATE_MS));
    totalSize += readSize;
  }
  if (totalSize != SOUND_DATA_LENGHT) {
    Serial.print("Error: Read Size: ");
    Serial.print(SOUND_DATA_LENGHT);
    Serial.print(", Total Size: ");
    Serial.println(totalSize);

    while (1) delay(100);
  }

  int16_t *soundDataInt16 = (int16_t*)soundData;
  for (int i = 0; i < (totalSize / 2); i++) {
    soundDataInt16[i] *= GAIN;
  }

  uint32_t ChunkSize = 36 + totalSize;
  uint32_t ByteRate = (SAMPLE_RATE * 16 * 1) / 8; //  (Sample Rate * BitsPerSample * Channels) / 8.
  uint32_t BlockAlign = (16 * 1) / 8; // (BitsPerSample * Channels) / 8

  uint8_t headerSize = 44;
  uint8_t soundHeader[44];
  memset(soundHeader, 0, 44);

  memcpy(&soundHeader[0], "RIFF", 4);

  soundHeader[4] = ChunkSize & 0xFF;
  soundHeader[5] = (ChunkSize >> 8) & 0xFF;
  soundHeader[6] = (ChunkSize >> 16) & 0xFF;
  soundHeader[7] = (ChunkSize >> 24) & 0xFF;

  memcpy(&soundHeader[8], "WAVE", 4);

  memcpy(&soundHeader[12], "fmt ", 4);

  soundHeader[16] = 16;
  soundHeader[17] = 0;
  soundHeader[18] = 0;
  soundHeader[19] = 0;

  soundHeader[20] = 1;
  soundHeader[21] = 0;

  soundHeader[22] = 1;
  soundHeader[23] = 0;

  soundHeader[24] = SAMPLE_RATE & 0xFF;
  soundHeader[25] = (SAMPLE_RATE >> 8) & 0xFF;
  soundHeader[26] = (SAMPLE_RATE >> 16) & 0xFF;
  soundHeader[27] = (SAMPLE_RATE >> 24) & 0xFF;

  soundHeader[28] = ByteRate & 0xFF;
  soundHeader[29] = (ByteRate >> 8) & 0xFF;
  soundHeader[30] = (ByteRate >> 16) & 0xFF;
  soundHeader[31] = (ByteRate >> 24) & 0xFF;

  soundHeader[32] = BlockAlign & 0xFF;
  soundHeader[33] = (BlockAlign) >> 8 & 0xFF;

  soundHeader[34] = 16;
  soundHeader[35] = 0;

  memcpy(&soundHeader[36], "data", 4);

  soundHeader[40] = totalSize & 0xFF;
  soundHeader[41] = (totalSize >> 8) & 0xFF;
  soundHeader[42] = (totalSize >> 16) & 0xFF;
  soundHeader[43] = (totalSize >> 24) & 0xFF;


  Serial.println("Connect to Server");

  WiFiClientSecure client;

  if (!client.connect("uploader.dess.web.meca.in.th", 443)) {
    Serial.println("connection failed");
    return;
  }

  String boundary = "----MyBoundaryWithRandomNumber";
  for (int i=0;i<30;i++) {
    boundary += (char)(random('0', '9'));
  }

  String fileHeader = "";
  fileHeader += "--" + boundary + "\r\n";
  fileHeader += "Content-Disposition: form-data; name=\"file\"; filename=\"sound.wav\"\r\n";
  fileHeader += "Content-Type: audio/wav\r\n";
  fileHeader += "\r\n";

  String tokenPayload = "";
  tokenPayload += "--" + boundary + "\r\n";
  tokenPayload += "Content-Disposition: form-data; name=\"token\"\r\n";
  tokenPayload += "\r\n";
  tokenPayload += "hjw6r12za6vsdfqro7100dgr8nl2\r\n";
  
  String endPayload = "--" + boundary + "--";
  
  uint32_t httpPayloadSize = fileHeader.length() + headerSize + totalSize + 2 + tokenPayload.length() + endPayload.length();
  Serial.println(httpPayloadSize);

  client.println("POST /upload HTTP/1.1");
  client.println("Host: uploader.dess.web.meca.in.th");
  client.println("Content-Type: multipart/form-data; boundary=\"" + boundary + "\"");
  client.println("Content-Length: " + String(httpPayloadSize));
  client.println("User-Agent: ESP32");
  client.println("Connection: keep-alive");
  client.println();

  client.print(fileHeader);

  // Start Upload File
  // Header
  client.write(soundHeader, headerSize);

  // Content
  for (int i=0;i<100;i++) {
    client.write(&soundData[i * (totalSize / 100)], totalSize / 100);
  }

  client.print("\r\n");
  
  client.print(tokenPayload);
  client.print(endPayload);

  Serial.println("Write Data END");

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      while (1) delay(100);
    }
  }

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.println("closing connection");

  //delay(5000);
  while (1) delay(100);
}
