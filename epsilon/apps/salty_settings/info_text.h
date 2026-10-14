#pragma once

#include <apps/i18n.h>
#include <stdint.h>

namespace SaltySettings {

/* Tiny text formatter (there is no snprintf in liba). Everything written past
 * the capacity of the buffer is silently dropped. */
class Line {
 public:
  Line(char* buffer, int capacity)
      : m_buffer(buffer), m_capacity(capacity), m_length(0) {
    if (m_buffer != nullptr && m_capacity > 0) {
      m_buffer[0] = 0;
    }
  }
  Line& str(const char* s) {
    while (s != nullptr && *s != 0) {
      push(*s++);
    }
    return *this;
  }
  Line& message(I18n::Message m) { return str(I18n::translate(m)); }
  Line& dec(uint32_t value) {
    char digits[10];
    int n = 0;
    do {
      digits[n++] = '0' + value % 10;
      value /= 10;
    } while (value != 0);
    while (n > 0) {
      push(digits[--n]);
    }
    return *this;
  }
  Line& hex(uint32_t value, int minDigits = 1) {
    char digits[8];
    int n = 0;
    do {
      digits[n++] = "0123456789ABCDEF"[value & 0xF];
      value >>= 4;
    } while (value != 0);
    for (int i = n; i < minDigits; i++) {
      push('0');
    }
    while (n > 0) {
      push(digits[--n]);
    }
    return *this;
  }
  // "812 B" or "12.3 KiB"
  Line& size(uint32_t bytes) {
    if (bytes < 1024) {
      return dec(bytes).str(" B");
    }
    return dec(bytes / 1024).str(".").dec((bytes % 1024) * 10 / 1024).str(
        " KiB");
  }

 private:
  void push(char c) {
    if (m_buffer != nullptr && m_length < m_capacity - 1) {
      m_buffer[m_length++] = c;
      m_buffer[m_length] = 0;
    }
  }
  char* m_buffer;
  int m_capacity;
  int m_length;
};

}  // namespace SaltySettings
