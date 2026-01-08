#pragma once
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <rack.hpp> // Needed for math types if used

struct TheoryEngine {
    // 0: Maj, 1: Min, 2: Dor, 3: Phry, 4: MajPent, 5: MinPent
    const std::vector<std::vector<int>> SCALES = {
        {0, 2, 4, 5, 7, 9, 11},
        {0, 2, 3, 5, 7, 8, 10},
        {0, 2, 3, 5, 7, 9, 10},
        {0, 1, 3, 5, 7, 8, 10},
        {0, 2, 4, 7, 9},
        {0, 3, 5, 7, 10}
    };

    const std::vector<std::string> NOTE_NAMES = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

    // Voice Leading: Returns voltages (1V/oct) optimized for minimal movement
    std::vector<float> applyVoiceLeading(std::vector<float> currentVoicing, int targetRoot, std::vector<int> targetQuality) {
        std::vector<float> nextVoicing;
        
        // Convert current voltages to absolute MIDI notes
        std::vector<float> currentNotes;
        for (float v : currentVoicing) currentNotes.push_back(v * 12.0f + 60.0f);

        // Build target pitch classes
        std::vector<int> targetPitchClasses;
        for (int interval : targetQuality) targetPitchClasses.push_back((targetRoot + interval) % 12);

        // Greedy matching
        std::vector<bool> pitchClassUsed(targetPitchClasses.size(), false);

        for (int i = 0; i < (int)currentVoicing.size(); i++) {
            float currentNote = currentNotes[i];
            float bestNote = -1.0f;
            float minDist = 1000.0f;
            int bestPcIndex = -1;

            for (size_t pcIdx = 0; pcIdx < targetPitchClasses.size(); pcIdx++) {
                if (i < (int)targetPitchClasses.size() && pitchClassUsed[pcIdx]) continue; // Try not to double notes if we have space
                
                int pc = targetPitchClasses[pcIdx];
                float octaveShift = std::round((currentNote - (float)pc) / 12.0f) * 12.0f;
                float candidate = octaveShift + (float)pc;
                float dist = std::abs(candidate - currentNote);

                if (dist < minDist) {
                    minDist = dist;
                    bestNote = candidate;
                    bestPcIndex = pcIdx;
                }
            }
            if (bestPcIndex != -1) pitchClassUsed[bestPcIndex] = true;
            nextVoicing.push_back(bestNote);
        }
        
        // Convert back to 1V/Oct
        std::vector<float> outputVoltages;
        for (float n : nextVoicing) outputVoltages.push_back((n - 60.0f) / 12.0f);
        
        return outputVoltages;
    }

    std::string getChordName(int rootNote, int scaleType, int degree) {
        if (scaleType < 0 || scaleType >= (int)SCALES.size()) return "?";
        const auto& scale = SCALES[scaleType];
        
        int degreeIndex = degree % scale.size(); 
        int chordRootOffset = scale[degreeIndex];
        int actualChordRoot = (rootNote + chordRootOffset) % 12;

        std::string rootName = NOTE_NAMES[actualChordRoot];
        
        int thirdIndex = (degreeIndex + 2) % scale.size();
        int fifthIndex = (degreeIndex + 4) % scale.size();
        int thirdNote = scale[thirdIndex];
        int fifthNote = scale[fifthIndex];
        if (thirdIndex < degreeIndex) thirdNote += 12;
        if (fifthIndex < degreeIndex) fifthNote += 12;

        int i3 = thirdNote - chordRootOffset;
        int i5 = fifthNote - chordRootOffset;

        std::string quality = "";
        if (i3 == 4 && i5 == 7) quality = "";
        else if (i3 == 3 && i5 == 7) quality = "m";
        else if (i3 == 3 && i5 == 6) quality = "dim";
        else if (i3 == 4 && i5 == 8) quality = "aug";
        else quality = "?";

        return rootName + quality;
    }
};