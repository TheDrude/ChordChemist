#include "plugin.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

using namespace rack;

struct RhythmChemist : Module {
    enum ParamId {
        SIGNATURE_PARAM,
        GRANULARITY_PARAM,
        BARS_PARAM,
        RUN_PARAM,
        DENSITY_PARAM,
        RAND_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        CLOCK_INPUT,
        RESET_INPUT,
        RUN_INPUT,
        DENSITY_INPUT,
        RAND_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        GATE_OUTPUT,
        EOC_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        RUN_LIGHT,
        LIGHTS_LEN
    };

    uint8_t steps[8192];
    int currentStep = 0;
    uint32_t lastStepSamples = 0;
    uint32_t currentStepSamples = 0;
    uint32_t samplesSinceLastClock = 0;
    float internalPhase = 0.f;
    float internalPhaseInc = 0.f;
    int subStepsFired = 0;
    bool isRunning = true;
    dsp::SchmittTrigger clockTrigger;
    dsp::SchmittTrigger resetTrigger;
    dsp::SchmittTrigger runTrigger;
    dsp::SchmittTrigger randTrigger;
    dsp::PulseGenerator eocPulse;

    // Constants
    const std::vector<std::string> signatures = {
        "Free", "2/4", "3/4", "4/4", "5/4", "6/4", "7/4", 
        "3/8", "5/8", "6/8", "7/8", "9/8"
    };
    const std::vector<int> signatureBeats = {
        4, 2, 3, 4, 5, 6, 7, 
        3, 5, 6, 7, 9
    };
    const std::vector<int> signatureDenominators = {
        4, 4, 4, 4, 4, 4, 4,
        8, 8, 8, 8, 8
    };

    const std::vector<std::string> granularities = {"1/4", "1/8", "1/16", "1/32", "1/64"};
    const std::vector<int> granularityValues = {4, 8, 16, 32, 64}; // Denominator of the granularity

    RhythmChemist() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(SIGNATURE_PARAM, 0.f, 11.f, 3.f, "Signature"); // Default 4/4
        configParam(GRANULARITY_PARAM, 0.f, 4.f, 2.f, "Granularity"); // Default 1/16
        configParam(BARS_PARAM, 1.f, 16.f, 1.f, "Bars"); // Default 1 bar
        configParam(RUN_PARAM, 0.f, 1.f, 0.f, "Run");
        configParam(DENSITY_PARAM, 0.f, 1.f, 0.5f, "Density");
        configParam(RAND_PARAM, 0.f, 1.f, 0.f, "Randomize");

        configInput(CLOCK_INPUT, "Clock");
        configInput(RESET_INPUT, "Reset");
        configInput(RUN_INPUT, "Run");
        configInput(DENSITY_INPUT, "Density CV");
        configInput(RAND_INPUT, "Random Trigger");
        configOutput(GATE_OUTPUT, "Gate");
        configOutput(EOC_OUTPUT, "End of Cycle");

        std::fill(std::begin(steps), std::end(steps), (uint8_t)0);
    }

    int getNumSteps() {
        int sigIdx = clamp((int)params[SIGNATURE_PARAM].getValue(), 0, 11);
        int granIdx = clamp((int)params[GRANULARITY_PARAM].getValue(), 0, 4);
        int numBars = clamp((int)params[BARS_PARAM].getValue(), 1, 16);

        int beatsPerBar = signatureBeats[sigIdx];
        int beatDenominator = signatureDenominators[sigIdx];
        int granDenominator = granularityValues[granIdx];
        
        // Number of squares per beat = granDenominator / beatDenominator
        // Example: 1/16 granularity, 4/4 sig -> beatDenominator = 4, granDenominator = 16. squaresPerBeat = 4.
        // Example: 1/16 granularity, 6/8 sig -> beatDenominator = 8, granDenominator = 16. squaresPerBeat = 2.
        int squaresPerBeat = std::max(1, granDenominator / beatDenominator);

        return beatsPerBar * squaresPerBeat * numBars;
    }

    void process(const ProcessArgs& args) override {
        // Run/Stop
        if (runTrigger.process(inputs[RUN_INPUT].getVoltage() + params[RUN_PARAM].getValue())) {
            isRunning = !isRunning;
        }
        lights[RUN_LIGHT].setBrightness(isRunning ? 1.f : 0.f);

        // Reset
        if (resetTrigger.process(inputs[RESET_INPUT].getVoltage())) {
            currentStep = 0;
        }

        // Randomizer
        if (randTrigger.process(inputs[RAND_INPUT].getVoltage() + params[RAND_PARAM].getValue())) {
            float density = clamp(params[DENSITY_PARAM].getValue() + inputs[DENSITY_INPUT].getVoltage() / 10.0f, 0.0f, 1.0f);
            int totalSteps = getNumSteps();
            for (int i = 0; i < totalSteps; i++) {
                if (random::uniform() < density) {
                    if (i > 0 && steps[i - 1] > 0 && random::uniform() < 0.3f) {
                        steps[i] = 2; // TIE (30% chance if previous is active)
                    } else {
                        steps[i] = 1; // GATE
                    }
                } else {
                    steps[i] = 0;
                }
            }
            // Clear remaining steps
            for (int i = totalSteps; i < 8192; i++) {
                steps[i] = 0;
            }
        }

        int sigIdx = clamp((int)params[SIGNATURE_PARAM].getValue(), 0, 11);
        int granIdx = clamp((int)params[GRANULARITY_PARAM].getValue(), 0, 4);
        int beatDenominator = signatureDenominators[sigIdx];
        int granDenominator = granularityValues[granIdx];
        int squaresPerBeat = std::max(1, granDenominator / beatDenominator);

        samplesSinceLastClock++;
        currentStepSamples++;

        // Clock Interpolation
        if (isRunning) {
            if (clockTrigger.process(inputs[CLOCK_INPUT].getVoltage())) {
                if (samplesSinceLastClock > 0) {
                    float samplesPerSquare = (float)samplesSinceLastClock / squaresPerBeat;
                    internalPhaseInc = 1.0f / samplesPerSquare;
                }
                samplesSinceLastClock = 0;
                internalPhase = 0.f;
                subStepsFired = 1;

                lastStepSamples = currentStepSamples;
                currentStepSamples = 0;
                currentStep++;
                if (currentStep >= getNumSteps()) {
                    currentStep = 0;
                    eocPulse.trigger(1e-3f); // 1ms trigger
                }
            } else {
                if (internalPhaseInc > 0.f && subStepsFired < squaresPerBeat) {
                    internalPhase += internalPhaseInc;
                    if (internalPhase >= 1.0f) {
                        internalPhase -= 1.0f;
                        subStepsFired++;

                        lastStepSamples = currentStepSamples;
                        currentStepSamples = 0;
                        currentStep++;
                        if (currentStep >= getNumSteps()) {
                            currentStep = 0;
                            eocPulse.trigger(1e-3f);
                        }
                    }
                }
            }
        } else {
            clockTrigger.process(inputs[CLOCK_INPUT].getVoltage());
        }

        // Output logic
        bool gateHigh = false;
        if (currentStep >= 0 && currentStep < 8192) {
            uint8_t state = steps[currentStep];
            if (state > 0) {
                bool isTiedToNext = false;
                if (currentStep + 1 < 8192 && steps[currentStep + 1] == 2) {
                    isTiedToNext = true;
                }
                
                if (isTiedToNext) {
                    gateHigh = true; // 100% duty cycle
                } else {
                    // 80% duty cycle for distinct notes
                    if (lastStepSamples > 0) {
                        if (currentStepSamples < 0.8f * lastStepSamples) {
                            gateHigh = true;
                        }
                    } else {
                        gateHigh = true; // First step, just stay high
                    }
                }
            }
        }

        outputs[GATE_OUTPUT].setVoltage(gateHigh ? 10.f : 0.f);
        outputs[EOC_OUTPUT].setVoltage(eocPulse.process(args.sampleTime) ? 10.f : 0.f);
    }

    // Save/Load notes
    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_t* stepsJ = json_array();
        for (int i = 0; i < 8192; i++) {
            if (steps[i] > 0) {
                json_t* stepObj = json_object();
                json_object_set_new(stepObj, "idx", json_integer(i));
                json_object_set_new(stepObj, "state", json_integer(steps[i]));
                json_array_append_new(stepsJ, stepObj);
            }
        }
        json_object_set_new(rootJ, "steps", stepsJ);
        json_object_set_new(rootJ, "isRunning", json_boolean(isRunning));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        std::fill(std::begin(steps), std::end(steps), (uint8_t)0);
        json_t* stepsJ = json_object_get(rootJ, "steps");
        if (stepsJ) {
            size_t i;
            json_t* stepJ;
            json_array_foreach(stepsJ, i, stepJ) {
                if (json_is_integer(stepJ)) {
                    // Legacy boolean format
                    int stepIdx = json_integer_value(stepJ);
                    if (stepIdx >= 0 && stepIdx < 8192) {
                        steps[stepIdx] = 1; // Default to GATE
                    }
                } else if (json_is_object(stepJ)) {
                    // New state format
                    int stepIdx = json_integer_value(json_object_get(stepJ, "idx"));
                    int state = json_integer_value(json_object_get(stepJ, "state"));
                    if (stepIdx >= 0 && stepIdx < 8192) {
                        steps[stepIdx] = state;
                    }
                }
            }
        }
        json_t* isRunningJ = json_object_get(rootJ, "isRunning");
        if (isRunningJ) {
            isRunning = json_is_true(isRunningJ);
        }
    }
};

// UI implementation
struct ValueDisplayRC : TransparentWidget {
    RhythmChemist* module = nullptr;
    int mode = 0; // 0 = Signature, 1 = Granularity, 2 = Bars

    void draw(const DrawArgs& args) override {
        std::string text = "?";
        if (module) {
            if (mode == 0) {
                int val = clamp((int)module->params[RhythmChemist::SIGNATURE_PARAM].getValue(), 0, 11);
                text = module->signatures[val];
            } else if (mode == 1) {
                int val = clamp((int)module->params[RhythmChemist::GRANULARITY_PARAM].getValue(), 0, 4);
                text = module->granularities[val];
            } else if (mode == 2) {
                int val = clamp((int)module->params[RhythmChemist::BARS_PARAM].getValue(), 1, 16);
                text = std::to_string(val);
            } else if (mode == 3) {
                int val = (int)(clamp(module->params[RhythmChemist::DENSITY_PARAM].getValue(), 0.0f, 1.0f) * 100.f);
                text = std::to_string(val) + "%";
            }
        } else {
            text = (mode == 0) ? "4/4" : ((mode == 1) ? "1/16" : "1");
        }
        nvgFontSize(args.vg, 10);
        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgFillColor(args.vg, nvgRGBA(21, 55, 227, 255)); // Blue
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(args.vg, box.size.x / 2, box.size.y / 2, text.c_str(), NULL);
    }
};

struct PianoRollWidget : Widget {
    RhythmChemist* module = nullptr;
    float squareSize = 15.f;
    float rowGap = 15.f;
    float topPadding = 15.f;
    int numRows = 7;
    uint8_t drawValue = 0;
    int lastStepDragged = -1;

    PianoRollWidget() {
        box.size = Vec(360, topPadding + numRows * squareSize + (numRows - 1) * rowGap + 2);
    }

    int getStepFromMouse(Vec mousePos) {
        if (!module) return -1;
        if (mousePos.y < topPadding) return -1;
        
        int row = clamp((int)((mousePos.y - topPadding) / (squareSize + rowGap)), 0, numRows - 1);
        int col = (int)(mousePos.x / squareSize);
        
        // Total steps per row
        int visibleCols = std::floor(box.size.x / squareSize);
        return (row * visibleCols) + col;
    }

    void draw(const DrawArgs& args) override {
        if (!module) return;

        int numSteps = module->getNumSteps();
        int visibleCols = std::floor(box.size.x / squareSize);

        int sigIdx = clamp((int)module->params[RhythmChemist::SIGNATURE_PARAM].getValue(), 0, 11);
        int granIdx = clamp((int)module->params[RhythmChemist::GRANULARITY_PARAM].getValue(), 0, 4);
        int beatsPerBar = module->signatureBeats[sigIdx];
        int beatDenominator = module->signatureDenominators[sigIdx];
        int granDenominator = module->granularityValues[granIdx];
        int squaresPerBeat = std::max(1, granDenominator / beatDenominator);
        int squaresPerBar = squaresPerBeat * beatsPerBar;

        // Draw background
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGBA(30, 30, 30, 255));
        nvgFill(args.vg);

        nvgScissor(args.vg, 0, 0, box.size.x, box.size.y);



        for (int r = 0; r < numRows; r++) {
            float y = topPadding + r * (squareSize + rowGap);
            
            // Row separator
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, 0, y - (rowGap / 2.f));
            nvgLineTo(args.vg, box.size.x, y - (rowGap / 2.f));
            nvgStrokeColor(args.vg, nvgRGBA(150, 150, 150, 255));
            nvgStrokeWidth(args.vg, 1.0f);
            if (r > 0) nvgStroke(args.vg); // Only draw between rows

            for (int c = 0; c <= visibleCols; c++) {
                int step = (r * visibleCols) + c;
                if (step > numSteps) break;

                float x = c * squareSize;

                // Vertical lines
                nvgBeginPath(args.vg);
                nvgMoveTo(args.vg, x, y);
                nvgLineTo(args.vg, x, y + squareSize);
                
                if (sigIdx != 0) { // Not free
                    if (step % squaresPerBar == 0) {
                        nvgStrokeColor(args.vg, nvgRGBA(150, 150, 150, 255)); // Bar line
                        nvgStrokeWidth(args.vg, 2.0f);
                    } else if (step % squaresPerBeat == 0) {
                        nvgStrokeColor(args.vg, nvgRGBA(80, 80, 80, 255)); // Beat line
                        nvgStrokeWidth(args.vg, 1.0f);
                    } else {
                        nvgStrokeColor(args.vg, nvgRGBA(50, 50, 50, 255)); // Sub grid
                        nvgStrokeWidth(args.vg, 0.5f);
                    }
                } else {
                    nvgStrokeColor(args.vg, nvgRGBA(50, 50, 50, 255));
                    nvgStrokeWidth(args.vg, 0.5f);
                }
                nvgStroke(args.vg);

                // Draw Note if active
                uint8_t state = module->steps[step];
                if (step >= 0 && step < 8192 && state > 0) {
                    nvgBeginPath(args.vg);
                    if (state == 1) { // GATE (Distinct)
                        nvgRect(args.vg, x + 2, y + 2, squareSize - 4, squareSize - 4);
                    } else if (state == 2) { // TIE (Continuous)
                        // Draw connecting block backwards
                        nvgRect(args.vg, x - 2, y + 2, squareSize, squareSize - 4);
                    }
                    nvgFillColor(args.vg, nvgRGBA(21, 55, 227, 200)); // Blue
                    nvgFill(args.vg);
                }

                // Draw playhead cursor
                if (module->isRunning && module->currentStep == step) {
                    nvgBeginPath(args.vg);
                    nvgRect(args.vg, x, y, squareSize, squareSize);
                    nvgFillColor(args.vg, nvgRGBA(255, 100, 0, 150)); // Bright Orange
                    nvgFill(args.vg);
                }
            }
        }

        nvgResetScissor(args.vg);

        // Draw Black Border around the piano roll
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 1.0f, 1.0f, box.size.x - 2.0f, box.size.y - 2.0f);
        nvgStrokeWidth(args.vg, 2.0f);
        nvgStrokeColor(args.vg, nvgRGBA(0, 0, 0, 255)); // Black Border
        nvgStroke(args.vg);
    }

    Vec dragPos;

    void onButton(const ButtonEvent& e) override {
        if (!module) return;
        if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
            if (e.action == GLFW_PRESS) {
                dragPos = e.pos;
                int stepClicked = getStepFromMouse(dragPos);
                if (stepClicked >= 0 && stepClicked < 8192) {
                    if (module->steps[stepClicked] == 0) {
                        module->steps[stepClicked] = 1; // GATE
                        drawValue = 2; // Dragging will TIE
                    } else {
                        module->steps[stepClicked] = 0; // OFF
                        drawValue = 0; // Dragging will OFF
                    }
                    lastStepDragged = stepClicked;
                    e.consume(this);
                }
            }
        }
    }

    void onDragMove(const DragMoveEvent& e) override {
        if (!module) return;
        dragPos = dragPos.plus(e.mouseDelta);
        int stepDragged = getStepFromMouse(dragPos);
        if (stepDragged >= 0 && stepDragged < 8192) {
            if (stepDragged != lastStepDragged) {
                module->steps[stepDragged] = drawValue;
                lastStepDragged = stepDragged;
            }
        }
        e.consume(this);
    }
};

struct RhythmChemistWidget : ModuleWidget {
    void addLabel(Vec centerPos, const std::string& text) {
        Label* label = new Label;
        label->box.pos = centerPos.minus(Vec(35, 0)); 
        label->box.size = Vec(70, 10);
        label->text = text;
        label->color = nvgRGBA(255, 255, 255, 255); // White text
        label->fontSize = 10.0f; 
        label->alignment = Label::CENTER_ALIGNMENT;
        addChild(label);
    }

    RhythmChemistWidget(RhythmChemist* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/RhythmChemist.svg")));

        // Define layout
        float topY = 20;
        
        // --- Top Row ---
        addLabel(mm2px(Vec(12, topY-10)).plus(Vec(8-5, -7)), "Clock");
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12, topY)).plus(Vec(-3, 0)), module, RhythmChemist::CLOCK_INPUT));
        
        addLabel(mm2px(Vec(26, topY-10)).plus(Vec(8-5, -7)), "Reset");
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(26, topY)).plus(Vec(-3, 0)), module, RhythmChemist::RESET_INPUT));

        float sigX = 46;
        addLabel(mm2px(Vec(sigX, topY-10)).plus(Vec(8, -7)), "Signature");
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(sigX, topY)), module, RhythmChemist::SIGNATURE_PARAM));
        ValueDisplayRC* sigDisp = new ValueDisplayRC();
        sigDisp->box.pos = mm2px(Vec(sigX-7, topY+6)).minus(Vec(0, 5));
        sigDisp->box.size = mm2px(Vec(14, 8));
        sigDisp->module = module;
        sigDisp->mode = 0;
        addChild(sigDisp);

        float granX = 68;
        addLabel(mm2px(Vec(granX, topY-10)).plus(Vec(8, -7)), "Granularity");
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(granX, topY)), module, RhythmChemist::GRANULARITY_PARAM));
        ValueDisplayRC* granDisp = new ValueDisplayRC();
        granDisp->box.pos = mm2px(Vec(granX-7, topY+6)).minus(Vec(0, 5));
        granDisp->box.size = mm2px(Vec(14, 8));
        granDisp->module = module;
        granDisp->mode = 1;
        addChild(granDisp);

        float barsX = 90;
        addLabel(mm2px(Vec(barsX, topY-10)).plus(Vec(2+3, -7)), "Bars");
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(barsX, topY)).plus(Vec(3, 0)), module, RhythmChemist::BARS_PARAM));
        ValueDisplayRC* barDisp = new ValueDisplayRC();
        barDisp->box.pos = mm2px(Vec(barsX-7, topY+6)).plus(Vec(3, -5));
        barDisp->box.size = mm2px(Vec(14, 8));
        barDisp->module = module;
        barDisp->mode = 2;
        addChild(barDisp);

        // --- Random & Density (Top Right Stacked) ---
        float randBtnX = 112;
        float randInX = 124;
        addLabel(mm2px(Vec((randBtnX+randInX)/2, topY-10)).plus(Vec(-15+5, -7)), "Random");
        addParam(createParamCentered<BefacoPush>(mm2px(Vec(randBtnX, topY)).plus(Vec(4, 0)), module, RhythmChemist::RAND_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(randInX, topY)).plus(Vec(4, 0)), module, RhythmChemist::RAND_INPUT));

        float denKnobX = 140;
        float denInX = 152;
        addLabel(mm2px(Vec((denKnobX+denInX)/2, topY-10)).plus(Vec(-15+4, -7)), "Density");
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(denKnobX, topY)).plus(Vec(4, 0)), module, RhythmChemist::DENSITY_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(denInX, topY)).plus(Vec(4, 0)), module, RhythmChemist::DENSITY_INPUT));
        ValueDisplayRC* denDisp = new ValueDisplayRC();
        denDisp->box.pos = mm2px(Vec(denKnobX-7, topY+6)).plus(Vec(4, -5));
        denDisp->box.size = mm2px(Vec(14, 8));
        denDisp->module = module;
        denDisp->mode = 3;
        addChild(denDisp);

        // --- Bottom Row ---
        float bottomY = 118;
        addLabel(mm2px(Vec(30, bottomY-10)).plus(Vec(2+2, -9)), "Run");
        addParam(createParamCentered<BefacoPush>(mm2px(Vec(30, bottomY)).plus(Vec(2, -2)), module, RhythmChemist::RUN_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(42, bottomY)).plus(Vec(2, -2)), module, RhythmChemist::RUN_INPUT));
        addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(52, bottomY)).plus(Vec(2, -2)), module, RhythmChemist::RUN_LIGHT));

        addLabel(mm2px(Vec(100, bottomY-10)).plus(Vec(2+5, -9)), "Gate Out");
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(100, bottomY)).plus(Vec(5, -2)), module, RhythmChemist::GATE_OUTPUT));

        addLabel(mm2px(Vec(120, bottomY-10)).plus(Vec(2+5, -9)), "EOC Out");
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(120, bottomY)).plus(Vec(5, -2)), module, RhythmChemist::EOC_OUTPUT));

        // --- Piano Roll Widget ---
        PianoRollWidget* prw = new PianoRollWidget();
        prw->box.pos = mm2px(Vec(15, 32)); 
        prw->box.size.x = 390; // Add sideroom
        prw->module = module;
        addChild(prw);
    }
};

Model* modelRhythmChemist = createModel<RhythmChemist, RhythmChemistWidget>("RhythmChemist");
