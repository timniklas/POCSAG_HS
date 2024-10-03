/*
 *   Copyright (C) 2015,2016,2018,2020,2021 by Jonathan Naylor G4KLX
 *   Copyright (C) 2018 by Andy Uribe CA6JAU
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#if !defined(SERIALPORT_H)
#define  SERIALPORT_H

#include "Globals.h"

class CSerialPort {
public:
  CSerialPort();

  void start();

  void process();

#if defined(SEND_RSSI_DATA)
  void writeRSSIData(const uint8_t* data, uint8_t length);
#endif

#if defined(ENABLE_DEBUG)
  void writeDebug(const char* text);
  void writeDebug(const char* text, int16_t n1);
  void writeDebugI(const char* text, int32_t n1);
  void writeDebug(const char* text, int16_t n1, int16_t n2, int16_t n3);
  void writeDebug(const char* text, int16_t n1, int16_t n2, int16_t n3, int16_t n4);
#endif
  void writeDebug(const char* text, int16_t n1, int16_t n2);

private:
  uint8_t m_buffer[256U];
  uint8_t m_ptr;
  uint8_t m_len;
  uint8_t m_serial_buffer[128U];
  uint8_t m_serial_len;

  bool    m_debug;
  bool    m_firstCal;

  // Hardware versions
  void    beginInt(uint8_t n, int speed);
  int     availableInt(uint8_t n);
  uint8_t readInt(uint8_t n);
  void    writeInt(uint8_t n, const uint8_t* data, uint16_t length, bool flush = false);
};

#endif
