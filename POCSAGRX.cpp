/*
 *   Copyright (C) 2009-2017 by Jonathan Naylor G4KLX
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

#include "Config.h"
#include "Globals.h"
#include "POCSAGRX.h"
#include "Utils.h"

const uint8_t SYNC_BIT_START_ERRS = 2U;
const uint8_t SYNC_BIT_RUN_ERRS   = 4U;

const unsigned int MAX_SYNC_FRAMES = 3U + 1U;

const uint8_t BIT_MASK_TABLE[] = {0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U};

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])

const uint16_t NOENDPTR = 9999U;

CPOCSAGRX::CPOCSAGRX() :
m_prev(false),
m_state(POCSAGRXS_NONE),
m_bitBuffer(0x00U),
m_buffer(),
m_bufferPtr(0U),
m_lostCount(0U)
{
}

void CPOCSAGRX::reset()
{
  m_prev      = false;
  m_state     = POCSAGRXS_NONE;
  m_bitBuffer = 0x00U;
  m_bufferPtr = 0U;
  m_lostCount = 0U;
}

void CPOCSAGRX::databit(bool bit)
{
  switch (m_state) {
    case POCSAGRXS_DATA:
      processData(bit);
      break;
    default:
      processNone(bit);
      break;
  }
}

void CPOCSAGRX::processNone(bool bit)
{
  m_bitBuffer <<= 1;
  if (bit)
    m_bitBuffer |= 0x01U;

  // Fuzzy matching of the data sync bit sequence
  if (countBits32((m_bitBuffer & POCSAG_FSC_MASK) ^ POCSAG_FSC_WORD) <= SYNC_BIT_START_ERRS) {
    serial.writeDebug("POCSAG: sync found");
    m_state = POCSAGRXS_DATA;
    io.setDecode(true);
  }
}

void CPOCSAGRX::processData(bool bit)
{
  serial.writeDebug("processData");
}
