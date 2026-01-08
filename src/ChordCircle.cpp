/**
 * ChordCircle.cpp
 * * UPDATE: Moved CV inputs (Steps, Spread, Random) another 15px (4mm) to the left.
 * * Current Layout: Knobs at 15.5mm, CV Inputs at 26.5mm.
 */

#include "plugin.hpp"
#include <sstream>
#include <vector>
#include <string>

using namespace rack;

// =============================================================
// 1. EMBEDDED THEORY ENGINE
// =============================================================
struct TheoryEngine {
    std::vector<std::vector<int>> SCALES;
    std::vector<std::string> SCALE_NAMES;

    TheoryEngine() {
        addScale("Chromatic", {0,1,2,3,4,5,6,7,8,9,10,11});
        addScale("Tritone", {0,1,4,6,7,10});
        addScale("Two-Semi Tritone", {0,1,2,6,7,8});
        
        addScale("Major (Ionian)", {0,2,4,5,7,9,11});
        addScale("Major Pentatonic", {0,2,4,7,9});
        addScale("Major Bebop", {0,2,4,5,7,8,9,11});
        addScale("Major Locrian", {0,2,4,5,6,8,10});
        
        addScale("Natural Minor", {0,2,3,5,7,8,10});
        addScale("Minor Pentatonic", {0,3,5,7,10});
        addScale("Harmonic Minor", {0,2,3,5,7,8,11});
        addScale("Melodic Minor", {0,2,3,5,7,9,11});
        
        addScale("Dorian", {0,2,3,5,7,9,10});
        addScale("Phrygian", {0,1,3,5,7,8,10});
        addScale("Lydian", {0,2,4,6,7,9,11});
        addScale("Mixolydian", {0,2,4,5,7,9,10});
        addScale("Locrian", {0,1,3,5,6,8,10});
        
        addScale("Lydian Dominant", {0,2,4,6,7,9,10}); 
        addScale("Lydian Augmented", {0,2,4,6,8,9,11});
        addScale("Lydian Diminished", {0,2,3,6,7,9,11}); 
        
        addScale("Phrygian Dominant", {0,1,4,5,7,8,10});
        addScale("Locrian Nat6", {0,1,3,5,6,9,10});
        addScale("Super Locrian", {0,1,3,4,6,8,10}); 
        
        addScale("Blues", {0,3,5,6,7,10});
        addScale("Double Harmonic", {0,1,4,5,7,8,11}); 
        addScale("Hungarian Minor", {0,2,3,6,7,8,11}); 
        addScale("Hungarian Major", {0,3,4,6,7,9,10});
        addScale("Persian", {0,1,4,5,6,8,11});
        addScale("Hirajoshi", {0,2,3,7,8});
        addScale("Iwato", {0,1,5,6,10});
        addScale("In Sen", {0,1,5,7,10});
        addScale("Yo", {0,2,5,7,9});
        
        addScale("Whole Tone", {0,2,4,6,8,10});
        addScale("Augmented", {0,3,4,7,8,11});
        addScale("Octatonic (H-W)", {0,1,3,4,6,7,9,10});
        addScale("Enigmatic", {0,1,4,6,8,10,11});
        addScale("Prometheus", {0,2,4,6,9,10});
        addScale("Harmonic Major", {0,2,4,5,7,8,11});
        addScale("Neapolitan Maj", {0,1,3,5,7,9,11});
        addScale("Neapolitan Min", {0,1,3,5,7,8,11});
        addScale("Bebop Dominant", {0,2,4,5,7,9,10,11});
        addScale("Algerian", {0,2,3,5,6,7,8,11});
        addScale("Ukrainian Dorian", {0,2,3,6,7,9,10});
        addScale("Istrian", {0,1,3,4,6,7});
    }

    void addScale(std::string name, std::vector<int> notes) {
        SCALE_NAMES.push_back(name);
        SCALES.push_back(notes);
    }

    std::string getChordName(int rootNote, int scaleIdx, int degree) {
        if (scaleIdx < 0 || scaleIdx >= (int)SCALES.size()) return "?";
        const auto& s = SCALES[scaleIdx];
        if (s.empty()) return "?";

        const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        int interval = s[degree % s.size()];
        int absRoot = (rootNote + interval) % 12;
        std::string name = noteNames[absRoot];

        int thirdIdx = (degree + 2) % s.size();
        int fifthIdx = (degree + 4) % s.size();
        
        int i3 = (s[thirdIdx] - interval + 12) % 12;
        int i5 = (s[fifthIdx] - interval + 12) % 12;

        if (i3 == 4) name += "";      
        else if (i3 == 3) name += "m"; 
        else if (i3 == 2) name += "sus2"; 
        else if (i3 == 5) name += "sus4";
        else name += "?";

        if (i5 == 6) name += "b5";
        else if (i5 == 8) name += "#5";

        return name;
    }
};

// =============================================================
// 2. MODULE DEFINITION
// =============================================================

struct ChordCircle : Module {
    enum ParamId {
        STEPS_COUNT_PARAM,    
        ROOT_NOTE_PARAM,      
        SCALE_TYPE_PARAM,     
        SPREAD_PARAM,         
        RANDOMIZE_BTN_PARAM,  
        STEP_DEGREE_PARAM_0,
        STEP_DEGREE_PARAM_15 = STEP_DEGREE_PARAM_0 + 15,
        PARAMS_LEN 
    };

    enum InputId {
        CLOCK_INPUT,      
        RESET_INPUT,      
        ROOT_CV_INPUT,    
        SCALE_CV_INPUT,
        SPREAD_CV_INPUT,
        STEPS_CV_INPUT,
        RANDOM_CV_INPUT,   
        STEP_DEGREE_INPUT_0,
        STEP_DEGREE_INPUT_15 = STEP_DEGREE_INPUT_0 + 15,
        INPUTS_LEN
    };

    enum OutputId {
        VOICE_1_OUTPUT, 
        VOICE_2_OUTPUT, 
        VOICE_3_OUTPUT, 
        VOICE_4_OUTPUT, 
        POLY_OUTPUT,    
        OUTPUTS_LEN
    };

    enum LightId { 
        VOICE_LIGHT_1, 
        VOICE_LIGHT_2, 
        VOICE_LIGHT_3, 
        VOICE_LIGHT_4, 
        LIGHTS_LEN 
    };

    TheoryEngine theory;
    int currentStep = 0;
    std::vector<float> currentVoices = {0.f, 0.f, 0.f, 0.f}; 
    int stepQualities[16];

    dsp::SchmittTrigger clockTrigger;
    dsp::SchmittTrigger resetTrigger;
    dsp::SchmittTrigger randomizeTrigger;

    ChordCircle() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        
        configParam(STEPS_COUNT_PARAM, 1.f, 16.f, 8.f, "Seq Length");
        configParam(ROOT_NOTE_PARAM, 0.f, 60.f, 24.f, "Root Note"); 
        configParam(SCALE_TYPE_PARAM, 0.f, (float)(theory.SCALES.size() - 1), 0.f, "Scale Type"); 
        configParam(SPREAD_PARAM, 0.f, 1.f, 0.f, "Voice Spread");
        configParam(RANDOMIZE_BTN_PARAM, 0.f, 1.f, 0.f, "Randomize Seq");
        
        for (int i = 0; i < 16; i++) {
            configParam(STEP_DEGREE_PARAM_0 + i, 0.f, 6.f, (float)(i % 7), "Step Degree");
            stepQualities[i] = 0; 
        }
    }

    std::string getPentatonicChordName(int rootNote, int scaleType, int degree) {
        const char* notes[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        const auto& s = theory.SCALES[clamp(scaleType, 0, 5)];
        int chordRootIndex = s[degree % s.size()];
        int absRoot = (rootNote + chordRootIndex) % 12;
        std::string noteName = notes[absRoot];
        if(scaleType==4) return noteName + "6"; 
        return noteName + "m7";
    }

    void process(const ProcessArgs& args) override {
        bool trigger = false;
        if (clockTrigger.process(inputs[CLOCK_INPUT].getVoltage())) trigger = true;
        if (resetTrigger.process(inputs[RESET_INPUT].getVoltage())) {
            currentStep = 0; trigger = true; 
        }

        float rndBtn = params[RANDOMIZE_BTN_PARAM].getValue();
        float rndCv = inputs[RANDOM_CV_INPUT].getVoltage();
        if (randomizeTrigger.process(rndBtn + rndCv)) {
            for (int i=0; i<16; i++) {
                params[STEP_DEGREE_PARAM_0 + i].setValue(std::round(random::uniform() * 6.f));
                float r = random::uniform();
                if (r < 0.4) stepQualities[i] = 0;      
                else if (r < 0.5) stepQualities[i] = 1; 
                else if (r < 0.7) stepQualities[i] = 2; 
                else if (r < 0.9) stepQualities[i] = 3; 
                else stepQualities[i] = 4;              
            }
        }
        
        // --- ABSOLUTE CV CONTROL (Motorized Knobs) ---
        // 1. Steps (1V = Step 1, 16V = Step 16)
        if (inputs[STEPS_CV_INPUT].isConnected()) {
            float stepsCv = inputs[STEPS_CV_INPUT].getVoltage();
            params[STEPS_COUNT_PARAM].setValue(clamp(stepsCv, 1.f, 16.f));
        }

        // 2. Spread (0V = 0%, 10V = 100%)
        if (inputs[SPREAD_CV_INPUT].isConnected()) {
            float spreadCv = inputs[SPREAD_CV_INPUT].getVoltage();
            params[SPREAD_PARAM].setValue(clamp(spreadCv / 10.f, 0.f, 1.f));
        }

        // 3. Root (1V/Octave standard: 0V=0, 5V=60)
        if (inputs[ROOT_CV_INPUT].isConnected()) {
            float rootCv = inputs[ROOT_CV_INPUT].getVoltage();
            params[ROOT_NOTE_PARAM].setValue(clamp(rootCv * 12.f, 0.f, 60.f));
        }

        // 4. Scale (Map 0-10V to Full Scale List)
        if (inputs[SCALE_CV_INPUT].isConnected()) {
            float scaleCv = inputs[SCALE_CV_INPUT].getVoltage();
            float maxScale = (float)(theory.SCALES.size() - 1);
            // Map 0V-10V -> 0-MaxScale
            float mappedVal = (scaleCv / 10.f) * maxScale;
            params[SCALE_TYPE_PARAM].setValue(clamp(mappedVal, 0.f, maxScale));
        }

        if (trigger) {
            // Read directly from params (Motorized values)
            int numSteps = clamp((int)params[STEPS_COUNT_PARAM].getValue(), 1, 16);
            
            currentStep = (currentStep + 1) % numSteps;

            int rootInt = (int)params[ROOT_NOTE_PARAM].getValue();
            // Note: inputs[ROOT_CV_INPUT] is NOT added here, it was applied to the knob above.
            
            int scaleInt = (int)params[SCALE_TYPE_PARAM].getValue();
            // Note: inputs[SCALE_CV_INPUT] is NOT added here, it was applied to the knob above.
            
            int degreeParam = STEP_DEGREE_PARAM_0 + currentStep;
            int degree = (int)params[degreeParam].getValue();
            int quality = stepQualities[currentStep];

            int numScales = theory.SCALES.size();
            scaleInt = clamp(scaleInt, 0, numScales - 1);
            
            const auto& s = theory.SCALES[scaleInt];
            currentVoices.clear();
            
            std::vector<int> intervals;
            intervals.push_back(0); 
            intervals.push_back(2); 
            intervals.push_back(4); 
            
            switch (quality) {
                case 0: intervals.push_back(6); break;  
                case 1: intervals.push_back(0); break;  
                case 2: intervals.push_back(5); break;  
                case 3: intervals.push_back(8); break;  
                case 4: intervals.push_back(10); break; 
                default: intervals.push_back(6); break;
            }
            
            for (int k = 0; k < 4; k++) {
                int interval = intervals[k];
                int scaleIndexRaw = degree + interval;
                int scaleIndex = scaleIndexRaw % s.size();
                int octaveShift = scaleIndexRaw / s.size();
                int notePitchClass = s[scaleIndex];
                
                float v = (rootInt / 12.0f) + (notePitchClass / 12.0f) + (float)octaveShift;
                currentVoices.push_back(v);
            }
        }

        outputs[POLY_OUTPUT].setChannels(4);
        
        float spread = params[SPREAD_PARAM].getValue();
        // Spread is already set by CV above
        spread = clamp(spread, 0.f, 1.f);

        for (int i = 0; i < 4; i++) {
            float v = currentVoices[i];
            if (i == 0 && spread > 0.5f) v -= 1.0f; 
            outputs[POLY_OUTPUT].setVoltage(v, i);
            outputs[VOICE_1_OUTPUT + i].setVoltage(v);
            lights[VOICE_LIGHT_1 + i].setBrightness(1.0f);
        }
    }
};

// =============================================================
// 3. UI IMPLEMENTATION
// =============================================================

struct StepKnob : Trimpot {
    int stepIndex = -1;
    
    void draw(const DrawArgs& args) override {
        if (module) {
            ChordCircle* m = dynamic_cast<ChordCircle*>(module);
            if (m && m->currentStep == stepIndex) {
                nvgBeginPath(args.vg);
                nvgCircle(args.vg, box.size.x/2, box.size.y/2, box.size.x/2 + 2.0); 
                nvgFillColor(args.vg, nvgRGBA(21, 55, 227, 255)); // Blue
                nvgFill(args.vg);
            }
        }
        Trimpot::draw(args);
    }
};

struct ValueDisplay : TransparentWidget {
    ChordCircle* module = nullptr;
    int mode = 0; 
    const std::vector<std::string> noteNames = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

    void draw(const DrawArgs& args) override {
        if (!module) return;
        std::string text = "?";
        if (mode == 0) {
            // Reads the param which is now updated by CV automatically
            int val = (int)module->params[ChordCircle::ROOT_NOTE_PARAM].getValue();
            while(val < 0) val += 120;
            int oct = (val / 12) + 1; 
            std::string note = noteNames[val % 12];
            text = note + std::to_string(oct);
        } else {
            // Reads the param which is now updated by CV automatically
            int val = (int)module->params[ChordCircle::SCALE_TYPE_PARAM].getValue();
            int max = module->theory.SCALE_NAMES.size() - 1;
            val = clamp(val, 0, max);
            text = module->theory.SCALE_NAMES[val];
        }
        nvgFontSize(args.vg, 13);
        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgFillColor(args.vg, nvgRGBA(21, 55, 227, 255));
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(args.vg, box.size.x / 2, box.size.y / 2, text.c_str(), NULL);
    }
};

struct CircleDisplay : TransparentWidget {
    ChordCircle* module = nullptr;
    void draw(const DrawArgs& args) override {
        if (!module) return;
        float cx = box.size.x / 2.0;
        float cy = box.size.y / 2.6;
        float radius = 90.0; 
        
        int stepsBase = (int)module->params[ChordCircle::STEPS_COUNT_PARAM].getValue();
        int numSteps = clamp(stepsBase, 1, 16);

        float anglePerStep = (2 * M_PI) / numSteps;
        nvgStrokeWidth(args.vg, 1.5);
        
        int rootVal = (int)module->params[ChordCircle::ROOT_NOTE_PARAM].getValue();
        while(rootVal < 0) rootVal += 120; 
        int rootPitchClass = rootVal % 12;

        int scale = (int)module->params[ChordCircle::SCALE_TYPE_PARAM].getValue();
        int maxScale = module->theory.SCALES.size() - 1;
        scale = clamp(scale, 0, maxScale);

        for (int i = 0; i < numSteps; i++) {
            float start = i * anglePerStep - (M_PI / 2);
            float end = (i + 1) * anglePerStep - (M_PI / 2);

            nvgBeginPath(args.vg);
            if (i == module->currentStep) nvgFillColor(args.vg, nvgRGBA(21, 55, 227, 255));
            else nvgFillColor(args.vg, nvgRGBA(60, 60, 60, 255));
            
            nvgArc(args.vg, cx, cy, radius, start, end, NVG_CW);
            nvgLineTo(args.vg, cx, cy);
            nvgClosePath(args.vg);
            nvgFill(args.vg);
            nvgStrokeColor(args.vg, nvgRGBA(220, 151, 40, 255));
            nvgStroke(args.vg);
            
            if (numSteps <= 16) {
                float textAngle = start + (anglePerStep / 2);
                float tx = cx + cos(textAngle) * (radius * 0.75);
                float ty = cy + sin(textAngle) * (radius * 0.75);
                
                int deg = (int)module->params[ChordCircle::STEP_DEGREE_PARAM_0 + i].getValue();
                std::string name;
                if (scale >= 4 && scale <= 5) name = module->getPentatonicChordName(rootPitchClass, scale, deg); 
                else name = module->theory.getChordName(rootPitchClass, scale, deg);
                
                int qual = module->stepQualities[i];
                if (qual == 2) name += "6";
                if (qual == 3) name += "9";
                if (qual == 4) name += "11";
                
                nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 255));
                nvgFontSize(args.vg, (numSteps > 10) ? 9 : 11);
                nvgFontFaceId(args.vg, APP->window->uiFont->handle);
                nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                nvgText(args.vg, tx, ty, name.c_str(), NULL);
            }
        }
    }
};

struct ChordCircleWidget : ModuleWidget {
    void addLabel(Vec centerPos, std::string text) {
        Label* label = new Label;
        label->box.pos = centerPos.minus(Vec(30, 0)); 
        label->box.size = Vec(60, 10);
        label->text = text;
        label->color = nvgRGBA(220, 220, 220, 255);
        label->fontSize = 10.0f; 
        label->alignment = Label::CENTER_ALIGNMENT;
        addChild(label);
    }

    ChordCircleWidget(ChordCircle* module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ChordCircle.svg")));

        float leftX = 15.5;   
        float cvX = 26; // (Moved left by ~15px/4mm from 30.5)
        
        float rightX = 116; 
        float midX = 65;    
        float yIn = 19; float yGap = 16;
        
        addLabel(mm2px(Vec(leftX+1, yIn-10)), "Trig");
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(leftX, yIn)), module, ChordCircle::CLOCK_INPUT));
        addLabel(mm2px(Vec(leftX+2, (yIn+yGap)-10)), "Reset");
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(leftX, yIn+yGap)), module, ChordCircle::RESET_INPUT));
        
        // --- SPREAD + CV ---
        addLabel(mm2px(Vec(leftX+2, (yIn+(yGap*2))-10)), "SPREAD");
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(leftX, (yIn+(yGap*2)))), module, ChordCircle::SPREAD_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(cvX, (yIn+(yGap*2)))), module, ChordCircle::SPREAD_CV_INPUT));

        // --- STEPS + CV ---
        addLabel(mm2px(Vec(leftX+1, (yIn+(yGap*3))-10)), "Steps");
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(leftX, (yIn+(yGap*3)))), module, ChordCircle::STEPS_COUNT_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(cvX, (yIn+(yGap*3)))), module, ChordCircle::STEPS_CV_INPUT));
        
        // --- RANDOM + CV ---
        addLabel(mm2px(Vec(leftX+1, (yIn+(yGap*4))-10)), "RND");
        addParam(createParamCentered<TL1105>(mm2px(Vec(leftX, yIn+(yGap*4))), module, ChordCircle::RANDOMIZE_BTN_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(cvX, yIn+(yGap*4))), module, ChordCircle::RANDOM_CV_INPUT));
        
        float vStart = 19; float vGap = 15;
        addLabel(mm2px(Vec(rightX+2, vStart-11)), "OUTPUT");
        addLabel(mm2px(Vec(rightX-8, vStart-3)), "1");
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(rightX, vStart)), module, ChordCircle::VOICE_1_OUTPUT));
        addLabel(mm2px(Vec(rightX-8, (vStart+vGap-3))), "3");
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(rightX, vStart+vGap)), module, ChordCircle::VOICE_2_OUTPUT));
        addLabel(mm2px(Vec(rightX-8, (vStart+vGap*2-3))), "5");
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(rightX, vStart+vGap*2)), module, ChordCircle::VOICE_3_OUTPUT));
        addLabel(mm2px(Vec(rightX-8, (vStart+vGap*3-3))), "Ext"); 
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(rightX, vStart+vGap*3)), module, ChordCircle::VOICE_4_OUTPUT));
        addLabel(mm2px(Vec(rightX-11,(vStart+vGap*4-3) )), "POLY");
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(rightX, vStart+vGap*4)), module, ChordCircle::POLY_OUTPUT));

        float topY = 19;
        addLabel(mm2px(Vec(midX-19, topY-11)), "ROOT");
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(midX-21, topY)), module, ChordCircle::ROOT_NOTE_PARAM));
        ValueDisplay* rootDisp = new ValueDisplay();
        rootDisp->box.pos = mm2px(Vec(midX-26, topY+5)); rootDisp->box.size = mm2px(Vec(10, 8));
        rootDisp->module = module; rootDisp->mode = 0; addChild(rootDisp);
        
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(midX-10, topY)), module, ChordCircle::ROOT_CV_INPUT));

        addLabel(mm2px(Vec(midX+17, topY-11)), "SCALE");
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(midX+16, topY)), module, ChordCircle::SCALE_TYPE_PARAM));
        ValueDisplay* scaleDisp = new ValueDisplay();
        scaleDisp->box.pos = mm2px(Vec(midX+11, topY+5)); scaleDisp->box.size = mm2px(Vec(10, 8));
        scaleDisp->module = module; scaleDisp->mode = 1; addChild(scaleDisp);
        
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(midX+27, topY)), module, ChordCircle::SCALE_CV_INPUT));
        
        CircleDisplay* display = new CircleDisplay();
        display->box.pos = mm2px(Vec(midX-40, 32)); 
        display->box.size = mm2px(Vec(80, 80));
        display->module = module;
        addChild(display);

        for (int i = 0; i < 16; i++) {
            int row = i / 8; int col = i % 8; 
            float startX = 14; float spacingX = 14.5; 
            float x = startX + (col * spacingX); float y = 105 + (row * 14);
            addLabel(mm2px(Vec(x+1, y-9)), std::to_string(i+1));
            
            StepKnob* k = createParamCentered<StepKnob>(mm2px(Vec(x, y)), module, ChordCircle::STEP_DEGREE_PARAM_0 + i);
            k->stepIndex = i; 
            addParam(k);
        }
    }
};

Model* modelChordCircle = createModel<ChordCircle, ChordCircleWidget>("ChordChemist");