#include <Arduino.h>
#include <math.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_err.h"

#if !CONFIG_IDF_TARGET_ESP32S3
#error "This project requires ESP32-S3."
#endif

static constexpr adc1_channel_t ADC_CHANNEL = ADC1_CHANNEL_3; // GPIO4 on S3
static constexpr adc_bits_width_t ADC_WIDTH = ADC_WIDTH_BIT_12;
static constexpr adc_atten_t ADC_ATTENUATION = ADC_ATTEN_DB_12;
static constexpr uint32_t ADC_MAX = (1U << 12) - 1;
static constexpr uint32_t VREF_NOMINAL_MV = 1100;
static constexpr float VMAX_MV = 3100.0f;
static constexpr uint32_t SAMPLE_PERIOD_MS = 100;

static esp_adc_cal_characteristics_t adcCharacteristics = {};
static bool ready = false;
static uint32_t lastSampleMs = 0;
static uint32_t rowCount = 0;

static void printHeader()
{
    Serial.println("# Module 3, lesson 2: ADC calibration (ESP32-S3)");
    Serial.println("# ADC1 channel 3 / GPIO4; 12 bits; RAW=0..4095");
    Serial.println("# Attenuation: 12 dB (legacy name: 11 dB); Vmax_manual=3100 mV");
    Serial.println("# Vref_nominal=1100 mV; supply=3.3 V; sample_period=100 ms");
    Serial.println("# Calibration: eFuse TP_FIT; both voltages use the SAME RAW sample");
    Serial.println("# Error=abs(U_manual-U_cali)/U_cali*100; N/A when U_cali=0");
    Serial.println("# EDGE means RAW=0 or 4095; check for clipping with a multimeter");
    Serial.println("t_ms       RAW  U_manual(mV)  U_cali(mV)  Error(%)  Status");
    Serial.println("--------------------------------------------------------------");
}

void setup()
{
    Serial.begin(115200);
    delay(1000); // Give the serial terminal time to connect after reset.

    esp_err_t status = adc1_config_width(ADC_WIDTH);
    if (status == ESP_OK) {
        status = adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTENUATION);
    }
    if (status != ESP_OK) {
        Serial.printf("ERROR: ADC configuration: %s\n", esp_err_to_name(status));
        return;
    }

    status = esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP_FIT);
    if (status != ESP_OK) {
        Serial.printf("ERROR: eFuse calibration unavailable: %s\n", esp_err_to_name(status));
        return; // Do not label a nominal conversion as calibrated.
    }
    const esp_adc_cal_value_t source = esp_adc_cal_characterize(
        ADC_UNIT_1, ADC_ATTENUATION, ADC_WIDTH, VREF_NOMINAL_MV,
        &adcCharacteristics);
    if (source != ESP_ADC_CAL_VAL_EFUSE_TP_FIT) {
        Serial.println("ERROR: ADC characterization failed");
        return;
    }

    printHeader();
    ready = true;
    lastSampleMs = millis();
}

void loop()
{
    if (!ready) {
        delay(100);
        return;
    }
    if (Serial.available() && Serial.read() == 'h') {
        printHeader();
    }
    const uint32_t now = millis();
    if (static_cast<uint32_t>(now - lastSampleMs) < SAMPLE_PERIOD_MS) {
        delay(1);
        return;
    }
    lastSampleMs = now; // No burst of catch-up samples after a slow terminal.

    const int raw = adc1_get_raw(ADC_CHANNEL);
    if (raw < 0 || raw > static_cast<int>(ADC_MAX)) {
        Serial.println("ERROR: ADC read failed");
        return;
    }
    const float manualMv = static_cast<float>(raw) * VMAX_MV / ADC_MAX;
    const uint32_t calibratedMv = esp_adc_cal_raw_to_voltage(raw, &adcCharacteristics);
    const char *rangeStatus = (raw == 0 || raw == static_cast<int>(ADC_MAX)) ? "EDGE" : "OK";

    if (rowCount++ % 50 == 0) {
        Serial.println("t_ms       RAW  U_manual(mV)  U_cali(mV)  Error(%)  Status");
    }
    Serial.printf("%10lu %4d %13.1f %11lu ", static_cast<unsigned long>(now), raw,
                  manualMv, static_cast<unsigned long>(calibratedMv));
    if (calibratedMv == 0) {
        Serial.print("      N/A");
    } else {
        const float errorPct = fabsf(manualMv - static_cast<float>(calibratedMv))
                             / static_cast<float>(calibratedMv) * 100.0f;
        Serial.printf("%9.2f", errorPct);
    }
    Serial.printf("  %s\n", rangeStatus);
}
