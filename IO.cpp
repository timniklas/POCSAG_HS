/*
 *   Copyright (C) 2015,2016,2020 by Jonathan Naylor G4KLX
 *   Copyright (C) 2016,2017,2018,2019,2020 by Andy Uribe CA6JAU
 *   Copyright (C) 2017 by Danilo DB4PLE
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
#include "IO.h"

uint32_t    m_frequency_rx;
uint32_t    m_frequency_tx;
uint8_t     m_power;

CIO::CIO():
m_started(false),
m_rxBuffer(1024U),
m_txBuffer(1024U),
m_LoDevYSF(false),
m_ledCount(0U),
m_scanEnable(false),
m_scanPauseCnt(0U),
m_scanPos(0U),
m_ledValue(true),
m_watchdog(0U),
m_int1counter(0U),
m_int2counter(0U)
{
  Init();

  CE_pin(HIGH);
  LED_pin(HIGH);
  PTT_pin(LOW);
  DSTAR_pin(LOW);
  DMR_pin(LOW);
  YSF_pin(LOW);
  P25_pin(LOW);
  NXDN_pin(LOW);
  M17_pin(LOW);
  POCSAG_pin(LOW);
  COS_pin(LOW);
  DEB_pin(LOW);

#if !defined(BIDIR_DATA_PIN)
  TXD_pin(LOW);
#endif

  SCLK_pin(LOW);
  SDATA_pin(LOW);
  SLE_pin(LOW);

  selfTest();

  m_modeTimerCnt = 0U;
}

void CIO::selfTest()
{
  bool ledValue = false;
  uint32_t ledCount = 0U;
  uint32_t blinks = 0U;

  while(true) {
    ledCount++;
    delay_us(1000U);

    if(ledCount >= 125U) {
      ledCount = 0U;
      ledValue = !ledValue;

      LED_pin(!ledValue);
      PTT_pin(ledValue);
      DSTAR_pin(ledValue);
      DMR_pin(ledValue);
      YSF_pin(ledValue);
      P25_pin(ledValue);
      NXDN_pin(ledValue);
      M17_pin(ledValue);
      POCSAG_pin(ledValue);
      COS_pin(ledValue);

      blinks++;

      if(blinks > 5U)
        break;
    }
  }
}

void CIO::process()
{
  uint8_t bit;
  uint32_t scantime;
  uint8_t  control;

  m_ledCount++;

  if (m_started) {
    // Two seconds timeout
    if (m_watchdog >= 19200U) {
      if (m_modemState == STATE_DMR ||m_modemState == STATE_POCSAG) {
        m_modemState = STATE_IDLE;
        setMode(m_modemState);
      }

      m_watchdog = 0U;
    }

#if defined(CONSTANT_SRV_LED)
    LED_pin(HIGH);
#elif defined(CONSTANT_SRV_LED_INVERTED)
    LED_pin(LOW);
#elif defined(DISCREET_SRV_LED)
    if (m_ledCount == 10000U) LED_pin(LOW);
    if (m_ledCount >= 480000U) {
      m_ledCount = 0U;
      LED_pin(HIGH);
    };
#elif defined(DISCREET_SRV_LED_INVERTED)
    if (m_ledCount == 10000U) LED_pin(HIGH);
    if (m_ledCount >= 480000U) {
      m_ledCount = 0U;
      LED_pin(LOW);
    };
#else
    if (m_ledCount >= 24000U) {
      m_ledCount = 0U;
      m_ledValue = !m_ledValue;
      LED_pin(m_ledValue);
    }
#endif
  } else {
    if (m_ledCount >= 240000U) {
      m_ledCount = 0U;
      m_ledValue = !m_ledValue;
      LED_pin(m_ledValue);
    }
    return;
  }

  // Switch off the transmitter if needed
  if (m_txBuffer.getData() == 0U && m_tx) {
    setRX(false);
  }

  scantime = SCAN_TIME;

  if(m_modeTimerCnt >= scantime) {
    m_modeTimerCnt = 0U;
    if( (m_modemState == STATE_IDLE) && (m_scanPauseCnt == 0U) && m_scanEnable) {
      m_scanPos = (m_scanPos + 1U) % m_TotalModes;
      #if !defined(QUIET_MODE_LEDS)
      setMode(m_Modes[m_scanPos]);
      #endif
      io.ifConf(m_Modes[m_scanPos], true);
    }
  }

  if (m_rxBuffer.getData() >= 1U) {
    m_rxBuffer.get(bit, control);

    switch (m_modemState_prev) {
      case STATE_DMR:
        dmrDMORX.databit(bit);
        break;
      case STATE_POCSAG:
        pocsagRX.databit(bit);
        break;
      default:
        break;
    }

  }
}

void CIO::start()
{
  m_TotalModes = 0U;

  if(m_dmrEnable) {
    m_Modes[m_TotalModes] = STATE_DMR;
    m_TotalModes++;
  }

  if(m_pocsagEnable) {
    m_Modes[m_TotalModes] = STATE_POCSAG;
    m_TotalModes++;
  }

#if defined(ENABLE_SCAN_MODE)
  if(m_TotalModes > 1U)
    m_scanEnable = true;
  else {
    m_scanEnable = false;
    setMode(m_modemState);
  }
#else
  m_scanEnable = false;
  setMode(m_modemState);
#endif

  if (m_started)
    return;

  startInt();

  m_started = true;
}

void CIO::write(uint8_t* data, uint16_t length, const uint8_t* control)
{
  if (!m_started)
    return;

  for (uint16_t i = 0U; i < length; i++) {
    if (control == NULL)
      m_txBuffer.put(data[i], MARK_NONE);
    else
      m_txBuffer.put(data[i], control[i]);
  }

  // Switch the transmitter on if needed
  if (!m_tx) {
    setTX();
    m_tx = true;
  }
}

uint16_t CIO::getSpace() const
{
  return m_txBuffer.getSpace();
}

bool CIO::hasTXOverflow()
{
  return m_txBuffer.hasOverflowed();
}

bool CIO::hasRXOverflow()
{
  return m_rxBuffer.hasOverflowed();
}

#if defined(ZUMSPOT_ADF7021) || defined(LONESTAR_USB) || defined(SKYBRIDGE_HS)
void CIO::checkBand(uint32_t frequency_rx, uint32_t frequency_tx) {
  if (!(io.hasSingleADF7021())) {
    // There are two ADF7021s on the board
    if (io.isDualBand()) {
      // Dual band
      if ((frequency_tx <= VHF2_MAX) && (frequency_rx <= VHF2_MAX)) {
        // Turn on VHF side
        io.setBandVHF(true);
      } else if ((frequency_tx >= UHF1_MIN) && (frequency_rx >= UHF1_MIN)) {
        // Turn on UHF side
        io.setBandVHF(false);
      }
    }
  }
}

uint8_t CIO::checkZUMspot(uint32_t frequency_rx, uint32_t frequency_tx) {
  if (!(io.hasSingleADF7021())) {
    // There are two ADF7021s on the board
    if (io.isDualBand()) {
      // Dual band
      if ((frequency_tx <= VHF2_MAX) && (frequency_rx <= VHF2_MAX)) {
        // Turn on VHF side
        io.setBandVHF(true);
      } else if ((frequency_tx >= UHF1_MIN) && (frequency_rx >= UHF1_MIN)) {
        // Turn on UHF side
        io.setBandVHF(false);
      }
    } else if (!io.isDualBand()) {
      // Duplex board
      if ((frequency_tx < UHF1_MIN) || (frequency_rx < UHF1_MIN)) {
        // Reject VHF frequencies
        return 4U;
      }
    }
  }
  return 0U;
}
#endif

uint8_t CIO::setFreq(uint32_t frequency_rx, uint32_t frequency_tx, uint8_t rf_power)
{
  // Configure power level
  setPower(rf_power);

#if !defined(DISABLE_FREQ_CHECK)
  // Check frequency ranges
  if( !( ((frequency_rx >= VHF1_MIN)&&(frequency_rx < VHF1_MAX)) || ((frequency_tx >= VHF1_MIN)&&(frequency_tx < VHF1_MAX)) || \
  ((frequency_rx >= UHF1_MIN)&&(frequency_rx < UHF1_MAX)) || ((frequency_tx >= UHF1_MIN)&&(frequency_tx < UHF1_MAX)) || \
  ((frequency_rx >= VHF2_MIN)&&(frequency_rx < VHF2_MAX)) || ((frequency_tx >= VHF2_MIN)&&(frequency_tx < VHF2_MAX)) || \
  ((frequency_rx >= UHF2_MIN)&&(frequency_rx < UHF2_MAX)) || ((frequency_tx >= UHF2_MIN)&&(frequency_tx < UHF2_MAX)) ) )
    return 4U;
#endif

#if !defined(DISABLE_FREQ_BAN)
  // Check banned frequency ranges
  if( ((frequency_rx >= BAN1_MIN)&&(frequency_rx <= BAN1_MAX)) || ((frequency_tx >= BAN1_MIN)&&(frequency_tx <= BAN1_MAX)) || \
  ((frequency_rx >= BAN2_MIN)&&(frequency_rx <= BAN2_MAX)) || ((frequency_tx >= BAN2_MIN)&&(frequency_tx <= BAN2_MAX)) )
    return 4U;
#endif

// Check if we have a single, dualband or duplex board
#if defined(ZUMSPOT_ADF7021) || defined(LONESTAR_USB) || defined(SKYBRIDGE_HS)
  if (checkZUMspot(frequency_rx, frequency_tx) > 0) {
    return 4U;
  }
#endif

  // Configure frequency
  m_frequency_rx = frequency_rx;
  m_frequency_tx = frequency_tx;

  return 0U;
}

void CIO::setMode(MMDVM_STATE modemState)
{
#if defined(USE_ALTERNATE_POCSAG_LEDS)
  if (modemState != STATE_POCSAG) {
#endif
    DSTAR_pin(modemState  == STATE_DSTAR);
    DMR_pin(modemState    == STATE_DMR);
#if defined(USE_ALTERNATE_POCSAG_LEDS)
  }
#endif
#if defined(USE_ALTERNATE_NXDN_LEDS)
  if (modemState != STATE_NXDN) {
#endif
    YSF_pin(modemState    == STATE_YSF);
    P25_pin(modemState    == STATE_P25);
#if defined(USE_ALTERNATE_NXDN_LEDS)
  }
#endif
#if defined(USE_ALTERNATE_M17_LEDS)
  if (modemState != STATE_M17) {
#endif
    YSF_pin(modemState    == STATE_YSF);
    P25_pin(modemState    == STATE_P25);
#if defined(USE_ALTERNATE_M17_LEDS)
  }
#endif
#if defined(USE_ALTERNATE_NXDN_LEDS)
  if (modemState != STATE_YSF && modemState != STATE_P25) {
#endif
    NXDN_pin(modemState   == STATE_NXDN);
#if defined(USE_ALTERNATE_NXDN_LEDS)
  }
#endif
#if defined(USE_ALTERNATE_POCSAG_LEDS)
  if (modemState != STATE_DSTAR && modemState != STATE_DMR) {
#endif
    POCSAG_pin(modemState == STATE_POCSAG);
#if defined(USE_ALTERNATE_POCSAG_LEDS)
  }
#endif
#if defined(USE_ALTERNATE_M17_LEDS)
  if (modemState != STATE_DSTAR && modemState != STATE_P25) {
#endif
    M17_pin(modemState   == STATE_M17);
#if defined(USE_ALTERNATE_M17_LEDS)
  }
#endif
}

void CIO::setDecode(bool dcd)
{
  if (dcd != m_dcd) {
    m_scanPauseCnt = 1U;
    COS_pin(dcd ? true : false);
  }

  m_dcd = dcd;
}

void CIO::setLoDevYSF(bool on)
{
  m_LoDevYSF = on;
}

void CIO::resetWatchdog()
{
  m_watchdog = 0U;
}

uint32_t CIO::getWatchdog()
{
  return m_watchdog;
}

void CIO::getIntCounter(uint16_t &int1, uint16_t &int2)
{
  int1 = m_int1counter;
  int2 = m_int2counter;
  m_int1counter = 0U;
  m_int2counter = 0U;
}
