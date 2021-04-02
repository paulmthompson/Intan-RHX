//------------------------------------------------------------------------------
//
//  Intan Technologies RHX Data Acquisition Software
//  Version 3.0.2
//
//  Copyright (c) 2020-2021 Intan Technologies
//
//  This file is part of the Intan Technologies RHX Data Acquisition Software.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published
//  by the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//  This software is provided 'as-is', without any express or implied warranty.
//  In no event will the authors be held liable for any damages arising from
//  the use of this software.
//
//  See <http://www.intantech.com> for documentation and product information.
//
//------------------------------------------------------------------------------

#include <iostream>
#include "rhxdatareader.h"

using namespace std;

RHXDataReader::RHXDataReader(ControllerType type_, int numDataStreams_, const uint16_t* start_, int numSamples_) :
    type(type_),
    numDataStreams(numDataStreams_),
    start(start_),
    numSamples(numSamples_)
{
    channelsPerStream = RHXDataBlock::channelsPerStream(type);
    numAuxChannels = RHXDataBlock::numAuxChannels(type);
    dataFrameSizeInWords = RHXDataBlock::dataBlockSizeInWords(type, numDataStreams) /
            RHXDataBlock::samplesPerDataBlock(type);
    auxChFrameOffset = 1;
}

void RHXDataReader::setNumDataStreams(int numDataStreams_)
{
    dataFrameSizeInWords = RHXDataBlock::dataBlockSizeInWords(type, numDataStreams_) /
            RHXDataBlock::samplesPerDataBlock(type);
}

void RHXDataReader::readTimeStampData(uint32_t* buffer) const
{
    const uint16_t* pRead = start;
    uint32_t* pWrite = buffer;

    pRead += 4; // skip header
    unsigned int w1, w2;
    for (int i = 0; i < numSamples; ++i) {
        w1 = (uint32_t) *pRead;
        w2 = (uint32_t) *(pRead + 1);
        *pWrite = (w2 << 16) | w1;
        pWrite++;
        pRead += dataFrameSizeInWords;
    }
}

// Read one amplifier waveform from raw USB data bytes, converting to microvolts.
void RHXDataReader::readAmplifierData(float* buffer, int stream, int channel) const
{
    const uint16_t* pRead = start;
    float* pWrite = buffer;
    int misoWordSize = ((type == ControllerStimRecordUSB2) ? 2 : 1);

    pRead += 6; // Skip header and timestamp.
    pRead += misoWordSize * (numDataStreams * 3);  // Skip auxillary channels.
    pRead += misoWordSize * ((numDataStreams * channel) + stream);   // Align with selected stream and channel.
    if (type == ControllerStimRecordUSB2) pRead++;  // Skip top 16 bits of 32-bit MISO word from RHS system.
    int adcValue;
    for (int i = 0; i < numSamples; ++i) {
        adcValue = (int) *pRead;
        *pWrite = 0.195F * (float)(adcValue - 32768);     // Return value in microvolts.
        pWrite++;
        pRead += dataFrameSizeInWords;
    }
}

// Read one DC amplifier waveform from raw USB data bytes, converting to volts (ControllerStimRecordUSB2 only).
void RHXDataReader::readDcAmplifierData(float* buffer, int stream, int channel) const
{
    const uint16_t* pRead = start;
    float* pWrite = buffer;

    pRead += 6;    // Skip header and timestamp.
    pRead += 2 * (numDataStreams * 3);  // Skip auxillary channels.
    pRead += 2 * ((numDataStreams * channel) + stream);   // Align with selected stream and channel.
    int adcValue;
    for (int i = 0; i < numSamples; ++i) {
        adcValue = (int) *pRead;
        *pWrite = -0.01923F * (float)(adcValue - 512);     // Return value in volts.
        pWrite++;
        pRead += dataFrameSizeInWords;
    }
}

// Read AuxIn1, 2, or 3 waveform from raw USB data bytes, converting to volts (ControllerRecordUSB2 and ControllerRecordUSB3 only).
void RHXDataReader::readAuxInData(float* buffer, int stream, int auxChannel)
{
    const uint16_t* pRead = start;
    float* pWrite = buffer;

    pRead += 6;    // Skip header and timestamp.
    pRead += (numDataStreams * 1) + stream;     // Align with selected stream and AuxIn data slot.

    // The command string generated by RHXRegisters::createCommandListRHDSampleAuxIns repeats four
    // commands in this data slot: it samples AuxIn1, AuxIn2, AuxIn3, and then it read ROM Register 40,
    // which will always return a value of 0x0049.  We can't count on the first sample always being
    // AuxIn1, because the USB bus sometimes drops bytes and corrupted data frames are thrown away
    // by USBDataThread.  So we need to check for the location of the ROM Register to maintain proper
    // phase.  We remember the current phase in auxChFrameOffet, which maintains a value between 0-3.
    const uint16_t* pReadSaved = pRead;
    const int RomValue = 0x0049;
    bool phaseFound = false;
    int frames = 0;
    while (!phaseFound) {
        int v0, v1, v2, v3;
        v0 = (int) *pRead;
        pRead += dataFrameSizeInWords;
        v1 = (int) *pRead;
        pRead += dataFrameSizeInWords;
        v2 = (int) *pRead;
        pRead += dataFrameSizeInWords;
        v3 = (int) *pRead;
        pRead += dataFrameSizeInWords;

        switch (auxChFrameOffset) {
        case 0:
            if (v3 == RomValue) {
                phaseFound = true;
            } else {
                if (v0 == RomValue && v1 != RomValue && v2 != RomValue) {
                    auxChFrameOffset = 1;
                    phaseFound = true;
                } else if (v1 == RomValue && v0 != RomValue && v2 != RomValue) {
                    auxChFrameOffset = 2;
                    phaseFound = true;
                } else if (v2 == RomValue && v0 != RomValue && v1 != RomValue) {
                    auxChFrameOffset = 3;
                    phaseFound = true;
                }
            }
            break;
        case 1:
            if (v0 == RomValue) {
                phaseFound = true;
            } else {
                if (v1 == RomValue && v2 != RomValue && v3 != RomValue) {
                    auxChFrameOffset = 2;
                    phaseFound = true;
                } else if (v2 == RomValue && v1 != RomValue && v3 != RomValue) {
                    auxChFrameOffset = 3;
                    phaseFound = true;
                } else if (v3 == RomValue && v1 != RomValue && v2 != RomValue) {
                    auxChFrameOffset = 0;
                    phaseFound = true;
                }
            }
            break;
        case 2:
            if (v1 == RomValue) {
                phaseFound = true;
            } else {
                if (v0 == RomValue && v2 != RomValue && v3 != RomValue) {
                    auxChFrameOffset = 1;
                    phaseFound = true;
                } else if (v2 == RomValue && v0 != RomValue && v3 != RomValue) {
                    auxChFrameOffset = 3;
                    phaseFound = true;
                } else if (v3 == RomValue && v0 != RomValue && v2 != RomValue) {
                    auxChFrameOffset = 0;
                    phaseFound = true;
                }
            }
            break;
        case 3:
            if (v2 == RomValue) {
                phaseFound = true;
            } else {
                if (v0 == RomValue && v1 != RomValue && v3 != RomValue) {
                    auxChFrameOffset = 1;
                    phaseFound = true;
                } else if (v1 == RomValue && v0 != RomValue && v3 != RomValue) {
                    auxChFrameOffset = 2;
                    phaseFound = true;
                } else if (v3 == RomValue && v0 != RomValue && v1 != RomValue) {
                    auxChFrameOffset = 0;
                    phaseFound = true;
                }
            }
            break;
        default:
            auxChFrameOffset = 0;
        }
        frames += 4;
        if (frames >= numSamples) {
            cerr << "RHXDataReader::readAuxInData: ROM value not found!\n";
            phaseFound = true;
        }
    }
    int frameOffset = (auxChannel + auxChFrameOffset) % 4;
    pRead = pReadSaved + frameOffset * dataFrameSizeInWords;   // align with data
    float auxInValue;
    for (int i = 0; i < numSamples; i += 4) {
        auxInValue = 0.0000374F * ((float) *pRead); // return value in volts
        *pWrite = auxInValue;     // write same value four times since AuxIn is sampled at fs/4
        pWrite++;
        *pWrite = auxInValue;
        pWrite++;
        *pWrite = auxInValue;
        pWrite++;
        *pWrite = auxInValue;
        pWrite++;
        pRead += 4 * dataFrameSizeInWords;
    }
}

// Read one supply voltage waveform from raw USB data bytes, converting to volts (ControllerRecordUSB2 and ControllerRecordUSB3 only).
void RHXDataReader::readSupplyVoltageData(float* buffer, int stream) const
{
    const uint16_t* pRead = start;
    float* pWrite = buffer;

    pRead += 6; // Skip header and timestamp.
    pRead += (numDataStreams * 1) + stream;     // Align with selected stream and AuxIn data slot.
    pRead += dataFrameSizeInWords * 124;        // Align with "read from Vdd" command.
    float vdd = 0.0000748F * ((float) *pRead);
    for (int i = 0; i < RHXDataBlock::samplesPerDataBlock(type); ++i) { // Write same value 128 times since Vdd is sampled at fs/128.
        *pWrite = vdd;
        pWrite++;
    }
}

void RHXDataReader::readBoardAdcData(float* buffer, int channel) const
{
    const uint16_t* pRead = start;
    float* pWrite = buffer;

    pRead += dataFrameSizeInWords - 10 + channel;

    int adcValue;
    if (type == ControllerRecordUSB2) {
        for (int i = 0; i < numSamples; ++i) {
            adcValue = (int) *pRead;
            *pWrite = 50.354e-6F * adcValue;  // Return value in volts.
            pWrite++;
            pRead += dataFrameSizeInWords;
        }
    } else {
        for (int i = 0; i < numSamples; ++i) {
            adcValue = (int) *pRead;
            *pWrite = 312.5e-6F * (adcValue - 32768);  // Return value in volts.
            pWrite++;
            pRead += dataFrameSizeInWords;
        }
    }
}

void RHXDataReader::readDigInData(uint16_t* buffer) const
{
    const uint16_t* pRead = start;
    uint16_t* pWrite = buffer;

    pRead += dataFrameSizeInWords - 2;

    for (int i = 0; i < numSamples; ++i) {
        *pWrite = *pRead;
        pWrite++;
        pRead += dataFrameSizeInWords;
    }
}

void RHXDataReader::readDigInData(float* buffer, int channel) const
{
    const uint16_t* pRead = start;
    float* pWrite = buffer;
    const uint16_t mask = 1U << channel;

    pRead += dataFrameSizeInWords - 2;

    for (int i = 0; i < numSamples; ++i) {
        *pWrite = (*pRead & mask) ? 1.0F : 0.0F;
        pWrite++;
        pRead += dataFrameSizeInWords;
    }
}

void RHXDataReader::readDigOutData(uint16_t* buffer) const
{
    const uint16_t* pRead = start;
    uint16_t* pWrite = buffer;

    pRead += dataFrameSizeInWords - 1;

    for (int i = 0; i < numSamples; ++i) {
        *pWrite = *pRead;
        pWrite++;
        pRead += dataFrameSizeInWords;
    }
}

void RHXDataReader::readDigOutData(float* buffer, int channel) const
{
    const uint16_t* pRead = start;
    float* pWrite = buffer;
    const uint16_t mask = 1U << channel;

    pRead += dataFrameSizeInWords - 1;

    for (int i = 0; i < numSamples; ++i) {
        *pWrite = (*pRead & mask) ? 1.0F : 0.0F;
        pWrite++;
        pRead += dataFrameSizeInWords;
    }
}

void RHXDataReader::readComplianceLimitData(uint16_t* buffer, int stream) const
{
    const uint16_t* pRead = start;
    uint16_t* pWrite = buffer;

    pRead += 6;    // Skip header and timestamp.
    pRead += 2 * ((numDataStreams * 1) + stream);   // Align with selected stream.

    // The top 16 bits will be either all 1's (results of a WRITE command) or all 0's (results of a READ command).
    for (int i = 0; i < numSamples; ++i) {
        if (*(pRead + 1) == 0) {    // The top 16 bits will be either all 1's (results of a WRITE command)
                                    // or all 0's (results of a READ command).
            *pWrite = *pRead; // Update compliance limit only if a 'read' command was executed, denoting a read from Register 40.
        } else {
            *pWrite = 0;      // If Register 40 was not read, assume no compliance limit violations.
        }
        pWrite++;
        pRead += dataFrameSizeInWords;
    }
}

void RHXDataReader::readComplianceLimitData(uint16_t* buffer, int stream, int channel) const
{
    const uint16_t* pRead = start;
    uint16_t* pWrite = buffer;
    const uint16_t mask = 1U << channel;

    pRead += 6;    // Skip header and timestamp.
    pRead += 2 * ((numDataStreams * 1) + stream);   // Align with selected stream.

    // The top 16 bits will be either all 1's (results of a WRITE command) or all 0's (results of a READ command).
    for (int i = 0; i < numSamples; ++i) {
        if (*(pRead + 1) == 0) {    // The top 16 bits will be either all 1's (results of a WRITE command)
                                    // or all 0's (results of a READ command).
            *pWrite = (*pRead & mask) ? 1 : 0; // Update compliance limit only if a 'read' command was executed, denoting a read from Register 40.
        } else {
            *pWrite = 0;      // If Register 40 was not read, assume no compliance limit violations.
        }
        pWrite++;
        pRead += dataFrameSizeInWords;
    }
}

void RHXDataReader::readStimOnData(uint16_t* buffer, int stream) const
{
    const uint16_t* pRead = start;
    uint16_t* pWrite = buffer;

    pRead += dataFrameSizeInWords - 18 - (numDataStreams * 4) + stream; // Align with selected stream.

    for (int i = 0; i < numSamples; ++i) {
        *pWrite = *pRead;
        pWrite++;
        pRead += dataFrameSizeInWords;
    }
}

void RHXDataReader::readStimOnData(uint16_t* buffer, int stream, int channel) const
{
    const uint16_t* pRead = start;
    uint16_t* pWrite = buffer;
    const uint16_t mask = 1U << channel;

    pRead += dataFrameSizeInWords - 18 - (numDataStreams * 4) + stream; // Align with selected stream.

    for (int i = 0; i < numSamples; ++i) {
        *pWrite = (*pRead & mask) ? 1 : 0;
        pWrite++;
        pRead += dataFrameSizeInWords;
    }
}

void RHXDataReader::readStimPolData(uint16_t* buffer, int stream) const
{
    const uint16_t* pRead = start;
    uint16_t* pWrite = buffer;

    pRead += dataFrameSizeInWords - 18 - (numDataStreams * 3) + stream; // Align with selected stream.

    for (int i = 0; i < numSamples; ++i) {
        *pWrite = *pRead;
        pWrite++;
        pRead += dataFrameSizeInWords;
    }
}

void RHXDataReader::readStimPolData(uint16_t* buffer, int stream, int channel) const
{
    const uint16_t* pRead = start;
    uint16_t* pWrite = buffer;
    const uint16_t mask = 1U << channel;

    pRead += dataFrameSizeInWords - 18 - (numDataStreams * 3) + stream; // Align with selected stream.

    for (int i = 0; i < numSamples; ++i) {
        *pWrite = (*pRead & mask) ? 1 : 0;
        pWrite++;
        pRead += dataFrameSizeInWords;
    }
}

void RHXDataReader::readAmpSettleData(uint16_t* buffer, int stream) const
{
    const uint16_t* pRead = start;
    uint16_t* pWrite = buffer;

    pRead += dataFrameSizeInWords - 18 - (numDataStreams * 2) + stream; // Align with selected stream.

    for (int i = 0; i < numSamples; ++i) {
        *pWrite = *pRead;
        pWrite++;
        pRead += dataFrameSizeInWords;
    }
}

void RHXDataReader::readAmpSettleData(uint16_t* buffer, int stream, int channel) const
{
    const uint16_t* pRead = start;
    uint16_t* pWrite = buffer;
    const uint16_t mask = 1U << channel;

    pRead += dataFrameSizeInWords - 18 - (numDataStreams * 2) + stream; // Align with selected stream.

    for (int i = 0; i < numSamples; ++i) {
        *pWrite = (*pRead & mask) ? 1 : 0;
        pWrite++;
        pRead += dataFrameSizeInWords;
    }
}

void RHXDataReader::readChargeRecovData(uint16_t* buffer, int stream) const
{
    const uint16_t* pRead = start;
    uint16_t* pWrite = buffer;

    pRead += dataFrameSizeInWords - 18 - (numDataStreams * 1) + stream; // Align with selected stream.

    for (int i = 0; i < numSamples; ++i) {
        *pWrite = *pRead;
        pWrite++;
        pRead += dataFrameSizeInWords;
    }
}

void RHXDataReader::readChargeRecovData(uint16_t* buffer, int stream, int channel) const
{
    const uint16_t* pRead = start;
    uint16_t* pWrite = buffer;
    const uint16_t mask = 1U << channel;

    pRead += dataFrameSizeInWords - 18 - (numDataStreams * 1) + stream; // Align with selected stream.

    for (int i = 0; i < numSamples; ++i) {
        *pWrite = (*pRead & mask) ? 1 : 0;
        pWrite++;
        pRead += dataFrameSizeInWords;
    }
}

// Read all five stimulation parameters for an individual RHS channel.  Data returned in uint16 array, where:
// bit 15 (MSB) = compliance limit flag
// bit 14 = charge recovery flag
// bit 13 = amplifier settle flag
// bit 8 = stimulation polarity flag
// bit 0 = stimulation on flag
void RHXDataReader::readStimParamData(uint16_t* buffer, int stream, int channel) const
{
    const uint16_t mask = 1U << channel;
    const uint16_t ComplianceFlag = 1U << 15;
    const uint16_t ChargeRecoveryFlag = 1U << 14;
    const uint16_t AmpSettleFlag = 1U << 13;
    const uint16_t StimPolFlag = 1U << 8;
    const uint16_t StimOnFlag = 1U << 0;
    uint16_t compliance;

    // First, read compliance limit data.
    uint16_t* pWrite = buffer;
    const uint16_t* pReadCompliance = start + 6;  // Skip header and timestamp.
    pReadCompliance += 2 * ((numDataStreams * 1) + stream);  // Align with selected stream.
    const uint16_t* pRead = start;
    pRead += dataFrameSizeInWords - 18 - (numDataStreams * 4) + stream;  // Align with selected stream.

    // The top 16 bits will be either all 1's (results of a WRITE command) or all 0's (results of a READ command).
    for (int i = 0; i < numSamples; ++i) {
        if (*(pReadCompliance + 1) == 0) {  // The top 16 bits will be either all 1's (results of a WRITE command)
                                            // or all 0's (results of a READ command).
            compliance = (*pReadCompliance & mask) ? ComplianceFlag : 0;  // Update compliance limit only if a 'read' command
                                                                          // was executed, denoting a read from Register 40.
        } else {
            compliance = 0;  // If Register 40 was not read, assume no compliance limit violations.
        }
        // The RHS2116 datasheet specifies 0 for negative current and 1 for positive current, so when writing the stim polarity bit, switch it (using 0 : 1) to
        // be consistent with the RHX code's convention of 1 for negative current and 0 for positive current.
        *pWrite = compliance | ((*pRead & mask) ? StimOnFlag : 0)
                             | (((*(pRead + 1 * numDataStreams)) & mask) ? 0 : StimPolFlag)
                             | (((*(pRead + 2 * numDataStreams)) & mask) ? AmpSettleFlag : 0)
                             | (((*(pRead + 3 * numDataStreams)) & mask) ? ChargeRecoveryFlag : 0);
        pWrite++;
        pReadCompliance += dataFrameSizeInWords;
        pRead += dataFrameSizeInWords;
    }
}

// ControllerStimRecordUSB2 only
void RHXDataReader::readBoardDacData(float* buffer, int channel) const
{
    const uint16_t* pRead = start;
    float* pWrite = buffer;

    pRead += dataFrameSizeInWords - 18 + channel;

    int dacValue;
    for (int i = 0; i < numSamples; ++i) {
        dacValue = (int) *pRead;
        *pWrite = 312.5e-6F * (dacValue - 32768);  // Return value in volts.
        pWrite++;
        pRead += dataFrameSizeInWords;
    }
}

