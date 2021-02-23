// https://github.com/pgillan145/AudioPulse

#include <PGardlib.h> // https://github.com/pgillan145/PGardLib
#include <Adafruit_NeoMatrix.h>
#include <gamma.h>
#include <PDM.h>

#define RIGID
#define MAT_W           8           /* Size (columns) of entire matrix */
#define MAT_H           8
#define MATRIX          6
#define MAX_BRIGHT      255

#ifdef RIGID
#define MATRIX_OPTIONS NEO_MATRIX_BOTTOM  + NEO_MATRIX_RIGHT + NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE
#else
#define MATRIX_OPTIONS NEO_MATRIX_TOP  + NEO_MATRIX_LEFT +NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG
#endif
Adafruit_NeoMatrix* matrix = new Adafruit_NeoMatrix(MAT_W, MAT_H, MATRIX, MATRIX_OPTIONS, NEO_GRB + NEO_KHZ800 );

// PDM variables
#define MIN_PDM 200
short sampleBuffer[256];
volatile int samplesRead;
uint32_t last_pdm = 0;
uint16_t max_pdm = 1000;
uint16_t min_pdm = MIN_PDM;
uint8_t brightness = MAX_BRIGHT;
bool verbose = false;

void setup() {
  PGardLibSetup();
  
  PDM.onReceive(onPDMdata);
  PDM.begin(1, 16000);

  matrix->begin();
  matrix->setBrightness(brightness);
}

uint32_t now = millis();
uint32_t last_sec = now;
bool sample = false;

void loop() {
  now = millis();

  if (Serial.available()) {
    delay(10);
    char c = Serial.read();
    String stringText = "";
    while(Serial.available()) { 
        char digit = Serial.read();
        stringText += digit; 
        if(stringText.length() >= 10) {
          break; 
        }
    }
    Serial.flush();
    
    int value_int = stringText.toInt();
    float value_float = stringText.toFloat();
  
    switch(c){
        case 'b':
          if (value_int <= MAX_BRIGHT) {
            brightness = value_int;
            SP("setting brightness to ");
            SPL(brightness);
          }
          break;
        case 's':
          if (sample) {
            sample = false;
            SPL("ending sample");
          }
          else {
            sample = true;
            SPL("starting sample");
          }
          break;
        case 'v':
          if (verbose) {
            verbose = false;
            SPL("verbose mode off");
          }
          else {
            verbose = true;
            SPL("verbose mode on");
          }
          break;
        case '?':
          SP("brightness:"); SP(brightness);
          SP(" verbose:"); SP(verbose);
          SP(" sample:"); SP(sample);
          SPL(" ");
    }
  }
  
  if (samplesRead && brightness) {
    bool one_sec = false;
    if (now > (last_sec + 1000)) {
      one_sec = true;
    }

    min_pdm *= 1.008;
    max_pdm *= .992;
    
    uint32_t total = 0;
    int16_t sample_min = 0;
    int16_t sample_max = 0;
    
    // print samples to the serial monitor or plotter
    for (int i = 0; i < samplesRead; i++) {
      //SPln(sampleBuffer[i]);
      if (sampleBuffer[i] > 0) {
        total += sampleBuffer[i];
      }
      if (sample && one_sec) {
        if (verbose) SPL(sampleBuffer[i]);
        int16_t val = sampleBuffer[i];
        if (val < sample_min) {
          sample_min = val;
        }
        if (val > sample_max) {
          sample_max = val;
        }
      }
    }
    if (sample && one_sec) {
      SP(sample_min); SP(" - "); SPL(sample_max);
    }
    
    if (total > max_pdm) {
      max_pdm = total;
    }
    if (total < min_pdm && total >= MIN_PDM) {
      min_pdm = total;
    }

    if (verbose && one_sec) {
      SP("Volume: ");
      SP(total);
      SP(" ");
      char output_str[20];
      snprintf (output_str, sizeof(output_str), "%.1f%%", ((float)total/(float)max_pdm)*100);
      SP(output_str);
      SP(" (");
      SP(min_pdm);
      SP(" - ");
      SP(max_pdm);
      SPL(")");
      //SP(now); SP(","); SPL(last_sec);
    }
    if ((total > (float)(min_pdm + max_pdm)*.50) && (max_pdm - min_pdm > 2000)) {
      matrix->clear();
      uint8_t volume = map(total,min_pdm,max_pdm,0,brightness);
      matrix->fillRect(0,0, MAT_W, MAT_H, RED4);
      matrix->setBrightness(volume);
      matrix->show();
    }
    else {
      matrix->clear();
      matrix->show();
    }
    samplesRead = 0;
    if (one_sec) {
      last_sec = now;
    }
  }
}

void onPDMdata() {
  // query the number of bytes available
  int bytesAvailable = PDM.available();

  // read into the sample buffer
  PDM.read(sampleBuffer, bytesAvailable);

  // 16-bit, 2 bytes per sample
  samplesRead = bytesAvailable / 2;
}
