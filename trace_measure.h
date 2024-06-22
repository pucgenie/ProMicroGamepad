//
// file name is a workaround because Arduino IDE is automagically broken
// (#include "trace_measure.inc" looks for a library instead of a file in the same folder)
//
#ifdef TRACE_MEASURE

struct TraceMeasurement {
  uint16_t time1 : 12;
  uint16_t time2 : 12;
};

static struct TraceMeasurement stats1[256];
static uint8_t stats1Idx = 0;

// bloated just for making it possible to see the other timeN for a specific min or max.
static struct TraceMeasurement
  min1 = {0xFFF, 0},
  min2 = {0, 0xFFF},
  max1 = {0, 0xFFF},
  max2 = {0xFFF, 0};
static uint8_t max2Counter = 0;

static bool traceMeasurementMinMaxChanged = false;

inline uint8_t nibble_to_base16(const uint8_t val4) {
  return val4 + ((val4 >= 10) ? 'A' - 10 : '0');
}
/**
Current implementation for 12 bit values
**/
void to_base16(const uint16_t value, void (*consumer)(uint8_t) ) {
  //consumer(nibble_to_base16((value >> 12) & 0x0F));
  consumer(nibble_to_base16((value >> 8) & 0x0F));
  consumer(nibble_to_base16((value >> 4) & 0x0F));
  consumer(nibble_to_base16(value & 0x0F));
}
inline void addStatPoint(const uint16_t t1, const uint16_t t2) {
  #ifdef TRACE_MEASURE_SKIP_SMALL
    if (t1 <= 0x48 && t2 <= 0x4C) {
return;
    }
  #endif
  {
    auto &tmpStats = stats1[stats1Idx];
    tmpStats.time1 = t1;
    tmpStats.time2 = t2;
  }
  if (++stats1Idx == 0) {
    #if defined(TRACE_MEASURE_PRINT_ALL) && !defined(MY_KEEP_SERIAL)
      Serial.begin(9600);
      while (!Serial) {
        delayMicroseconds(125);
      }
    #endif
    for (auto &tmpStats : stats1) {
      if (tmpStats.time2 == max2.time2) {
        ++max2Counter;
        traceMeasurementMinMaxChanged = true;
      }

      if (tmpStats.time1 < min1.time1) {
        min1 = tmpStats;
        traceMeasurementMinMaxChanged = true;
      }
      if (tmpStats.time2 < min2.time2) {
        min2 = tmpStats;
        traceMeasurementMinMaxChanged = true;
      }
      if (tmpStats.time1 > max1.time1) {
        max1 = tmpStats;
        traceMeasurementMinMaxChanged = true;
      }
      if (tmpStats.time2 > max2.time2) {
        max2 = tmpStats;
        max2Counter = 0;
        traceMeasurementMinMaxChanged = true;
      }

      #ifdef TRACE_MEASURE_PRINT_ALL
        to_base16(tmpStats.time1, [](uint8_t character){Serial.write(character);});
        Serial.write(',');
        to_base16(tmpStats.time2, [](uint8_t character){Serial.write(character);});
        Serial.println();
      #endif
    }
    #ifdef TRACE_MEASURE_PRINT_ALL
      Serial.println();
    #endif

    if (traceMeasurementMinMaxChanged) {
      #if !(defined(TRACE_MEASURE_PRINT_ALL) || defined(MY_KEEP_SERIAL)) 
        Serial.begin(9600);
        while (!Serial) {
          delay(3);
        }
      #endif
      to_base16(min1.time1, [](uint8_t character){Serial.write(character);});
      Serial.write('\t');
      to_base16(min2.time2, [](uint8_t character){Serial.write(character);});
      Serial.write('\t');
      to_base16(max1.time1, [](uint8_t character){Serial.write(character);});
      Serial.write('\t');
      to_base16(max2.time2, [](uint8_t character){Serial.write(character);});
      Serial.write('\t');
      to_base16(max2Counter, [](uint8_t character){Serial.write(character);});
      Serial.write("\r\n");
      traceMeasurementMinMaxChanged = false;
    }

    if (Serial) {
      Serial.flush();
      #ifndef MY_KEEP_SERIAL
        Serial.end();
      #endif
    }
  }
}

#endif