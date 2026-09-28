// Copyright (c) 2026 P. M. Thompson.
// Licensed under GPL-3.0-or-later (same as the parent Intan-RHX fork).
// This file was written for this fork and is not authored by Intan Technologies.

/**
 * @file ampstimprograms.hpp
 * @brief Multiple stimulation programs (FPGA banks) per amplifier channel.
 * @ingroup RhxStimSequencer
 *
 * @details Maintenance: see docs/stim-sequencer-maintenance.md
 */

#ifndef AMPSTIMPROGRAMS_HPP
#define AMPSTIMPROGRAMS_HPP

#include "stimparameters.h"

#include <array>
#include <cstddef>
#include <memory>

class SystemState;

/**
 * @brief Owns up to four amp-channel stimulation programs (BRAM banks).
 * @ingroup RhxStimSequencer
 */
class AmpStimPrograms
{
public:
    static constexpr std::size_t kMaxPrograms = 4;
    static constexpr std::size_t kActiveProgramCount = 1;

    /**
     * @brief Construct active stimulation programs and register state items on the channel.
     * @pre signalType is AmplifierSignal.
     */
    AmpStimPrograms(SingleItemList& channelItems, SystemState* state, SignalType signalType);

    /**
     * @brief Number of programs constructed in this build (1 today; up to kMaxPrograms).
     */
    std::size_t activeProgramCount() const { return kActiveProgramCount; }

    /**
     * @brief Stimulation program for FPGA bank @p bank.
     * @pre bank < activeProgramCount().
     */
    StimParameters* program(std::size_t bank);

    /**
     * @brief Stimulation program for FPGA bank @p bank.
     * @pre bank < activeProgramCount().
     */
    const StimParameters* program(std::size_t bank) const;

    /**
     * @brief Bank 0 program (legacy alias for channel->stimParameters on amp channels).
     */
    StimParameters* primaryProgram() { return program(0); }

    /**
     * @brief Bank 0 program (legacy alias for channel->stimParameters on amp channels).
     */
    const StimParameters* primaryProgram() const { return program(0); }

    /**
     * @brief Copy all fields from @p source into program @p destBank.
     * @pre destBank < activeProgramCount() and source is non-null.
     */
    void populateProgramFrom(std::size_t destBank, const StimParameters* source);

    /**
     * @brief XML attributes for one program (StimParameters group).
     */
    QStringList getAttributes(XMLGroup xmlGroup, std::size_t bank) const;

    /**
     * @brief Apply one XML attribute to program @p bank.
     * @return true if the attribute was recognized and set.
     */
    bool applyXmlAttribute(std::size_t bank, const QString& attributeName, const QString& attributeValue,
                           SingleItemList& channelItems, SystemState* state) const;

private:
    std::array<std::unique_ptr<StimParameters>, kMaxPrograms> _programs;
};

#endif // AMPSTIMPROGRAMS_HPP
