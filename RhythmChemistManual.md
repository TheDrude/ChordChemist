# Rhythm Chemist

**Rhythm Chemist** is a generative, piano-roll style rhythm sequencer for VCV Rack. It provides a visual, intuitive interface for programming and generating complex rhythmic patterns, polyrhythms, and evolving beats.

![Rhythm Chemist Module](res/RhythmChemist.svg)

## Overview
Unlike standard step sequencers, Rhythm Chemist uses a continuous piano-roll interface that dynamically scales based on your chosen musical Time Signature, Granularity, and Bar length. You can program beats manually by clicking the grid, or let the module's "Chemist" algorithm instantly generate new rhythms based on your desired density.

## Controls & Interfaces

### Timing & Structure
These parameters dictate the mathematical structure of your sequence. Changing them will dynamically update the piano-roll grid.

* **Signature**: Sets the musical time signature. Choose from `Free` (un-quantized grid) or standard signatures like `4/4`, `3/4`, `7/8`, etc.
* **Granularity**: Sets the resolution of each step in the grid. Options range from quarter notes (`1/4`) down to sixty-fourth notes (`1/64`). 
* **Bars**: Sets the total length of the sequence (from `1` to `16` bars).

### Programming the Piano Roll
The large interactive screen displays your current sequence.
* **Add/Remove Notes**: Click any empty square to add a Note (shown as a blue block). Click an active block to remove it.
* **Ties & Legato**: Drag your mouse across multiple squares, or click a square a second time, to create a "Tied" note. Tied notes instruct the Gate output to stay high (100% duty cycle) through the next step, allowing for longer sustained triggers. Distinct notes have an 80% duty cycle.

### Transport
* **Clock (Input)**: Provide a steady clock signal here to drive the sequencer. The module uses interpolation to keep the grid perfectly synced even at high granularities.
* **Reset (Input)**: A trigger received here resets the playhead back to step 1.
* **Run (Button & Input)**: Toggles the sequencer on and off. The green light indicates it is running.

### Generative Controls
* **Density (Knob & CV Input)**: Determines how "busy" the generated rhythm will be, from 0% (empty) to 100% (every step filled).
* **Random (Button & Trigger Input)**: Pressing this button or sending a trigger to the input will instantly wipe the current sequence and generate a new one based on the current Density setting.

### Outputs
* **Gate Out**: Sends a 10V high signal when the playhead crosses an active block. Distinct blocks output an 80% duty cycle gate; tied blocks output a continuous 100% duty cycle gate.
* **EOC Out (End of Cycle)**: Sends a 1ms trigger exactly when the playhead reaches the end of the sequence and loops back to the beginning.

## Tips & Tricks
* **Generative Fills**: Send a slow, occasional trigger to the `Random` input to completely evolve your rhythm every 8 or 16 bars.
* **Swing / Groove**: Because Rhythm Chemist interpolates its own internal clock, you can feed a swung or un-quantized clock into the `Clock` input, and the entire piano roll will follow that groove!

## License
This module is open-source and licensed under the **GPL-3.0-or-later** license.
