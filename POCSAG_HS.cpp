/*
 *   Copyright (C) 2015,2016,2020 by Jonathan Naylor G4KLX
 *   Copyright (C) 2016 by Mathis Schmieder DB9MAT
 *   Copyright (C) 2016 by Colin Durbridge G4EML
 *   Copyright (C) 2016,2017,2018,2019 by Andy Uribe CA6JAU
 *   Copyright (C) 2019 by Florian Wolters DF2ET
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

#include "Config.h"

#include "Globals.h"

// Global variables
MMDVM_STATE m_modemState = STATE_IDLE;
MMDVM_STATE m_calState = STATE_IDLE;
MMDVM_STATE m_modemState_prev = STATE_IDLE;

CPOCSAGRX   pocsagRX;

uint32_t m_modeTimerCnt;

bool m_dmrEnable    = false;
bool m_pocsagEnable = true;

bool m_tx  = false;
bool m_dcd = false;

uint8_t    m_control;

CDMRDMORX  dmrDMORX;

#if defined(SEND_RSSI_DATA)
CCalRSSI   calRSSI;
#endif

CSerialPort serial;
CIO io;

#if defined(STM32_I2C_HOST)
CI2CHost i2c;
#endif

void setup()
{
  serial.start();
  
  dmrDMORX.setColorCode(1);
  io.setFreq(FREQ_RX, FREQ_TX, 255U);
  io.ifConf(STATE_POCSAG, true);
  io.start();

  serial.writeDebug("BOOTING");
}

void loop()
{
  io.process();
  serial.process();

#if defined(SEND_RSSI_DATA)
  if (m_calState == STATE_RSSICAL)
    calRSSI.process();
#endif
}

int main()
{
  setup();

  for (;;)
    loop();
}
