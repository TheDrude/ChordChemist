\# ChordChemist: Technical Manual



\## Description

\*\*ChordChemist\*\* is a 16-step generative chord sequencer. It generates 4-voice polyphonic chords based on a selected Root Note and Scale. The module outputs both a combined polyphonic signal and four individual monophonic V/Oct signals.



---



\## Signal Flow \& Logic

The sequencer advances one step upon receiving a trigger at the `Trig` input. At each step, the module reads the \*\*Step Degree\*\* parameter (set by the bottom knobs) and generates a 4-note chord constrained to the current \*\*Root\*\* and \*\*Scale\*\*.



\### Absolute CV Behavior ("Motorized" Controls)

The \*\*Steps\*\*, \*\*Spread\*\*, \*\*Root\*\*, and \*\*Scale\*\* parameters utilize absolute CV logic.



\* \*\*If no CV input:\*\* The manual knob sets the value.

\* \*\*Connected:\*\* The CV input \*\*overwrites\*\* the manual knob position. The knob visually snaps to the value corresponding to the incoming voltage. Manual adjustment is disabled while a cable is patched.



---



\## Input Parameters



\### Global Controls



\* \*\*TRIG\*\*

&nbsp;   \* Trigger input (`> 1V`) to advance the sequencer.

\* \*\*RESET\*\*

&nbsp;   \* Trigger input (`> 1V`) to reset the sequence to Step 1.

\* \*\*STEPS\*\*

&nbsp;   \* Sets sequence length (1 to 16 steps).

&nbsp;   \* \*\*CV Standard:\*\* `1V = 1 Step`. (e.g., 4V = 4 Steps).

\* \*\*SPREAD\*\*

&nbsp;   \* Controls the voicing interval distribution.

&nbsp;   \* \*\*CV Standard:\*\* `0V` to `10V` maps to 0% - 100%.

&nbsp;   \* \*\*0.0 - 0.5 (0-5V):\*\* Closed voicing.

&nbsp;   \* \*\*0.5 - 1.0 (5-10V):\*\* Open voicing. The Root note is transposed -1 octave.

\* \*\*RND (Randomize)\*\*

&nbsp;   \* Randomizes the Step Degree (0-6) and internal Chord Quality (Triad, 7th, 9th, 6th, Suspended) for all 16 steps.

&nbsp;   \* \*\*Input:\*\* Button press or Trigger input (`> 1V`).



\### Harmony Controls



\* \*\*ROOT\*\*

&nbsp;   \* Sets the fundamental pitch class (C, C#, D, etc.).

&nbsp;   \* \*\*CV Standard:\*\* `1V/Octave`. (0V = C, 0.083V = C#, etc.).

&nbsp;   \* \*\*Range:\*\* `0V` to `5V` (C0 to C5).

\* \*\*SCALE\*\*

&nbsp;   \* Selects the quantization scale from the internal list.

&nbsp;   \* \*\*CV Standard:\*\* `0V` to `10V` maps linearly across the entire list of available scales.



---



\## Sequencer Interface



\### Step Knobs (1-16)

Selects the diatonic scale degree for the respective step.

\* \*\*Range:\*\* 0 (Root/Tonic) to 6 (Leading Tone/Subtonic).

\* \*\*Indicator:\*\* The current active step is indicated by a \*\*blue halo\*\* around the knob.



\### Center Display

\* Displays the alphanumeric name of the current chord (e.g., "Cm7", "F#maj9").

\* Visualizes the 16 steps and chord intervals in a radial plot.



---



\## Outputs



\### VOICE 1 - 4 (1, 3, 5, Ext)

Individual monophonic outputs for the four voices.

\* \*\*1:\*\* Root (or Root -1 octave if Spread > 50%).

\* \*\*3:\*\* Third interval.

\* \*\*5:\*\* Fifth interval.

\* \*\*Ext:\*\* Extension interval (7th, 9th, 6th, or Octave depending on internal quality).



\### POLY

\* A single polyphonic cable carrying 4 channels corresponding to Voices 1-4.oices 1-4.

