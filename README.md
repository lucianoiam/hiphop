Hip-Hop
-------
*High Performance Hybrid Audio Plugins*

*Superseded by [dpfwebui](http://github.com/lucianoiam/dpfwebui).
The new project is a scaled down version (no DSP, only UI) with a
permissive ISC license.*

This project builds on top of the [DPF](http://github.com/DISTRHO/DPF) audio
plugin framework to add web-based UI support. Plugins can leverage JavaScript
and related tech to provide complex user interfaces on the computer running
the plugin and optionally over the local network.

![Screenshot_2022-04-09_13-47-34](https://user-images.githubusercontent.com/930494/162572881-cba8857c-c4d2-444f-8b10-ab27ba86ea30.png)

*Examples running on Bitwig for Linux*

****

### Features

* Based on DISTRHO Plugin Framework (DPF)
* Subset of the `UI` and `Plugin` interfaces ported to JavaScript and [AssemblyScript](https://www.assemblyscript.org)
* C++ sill possible for DSP development
* WebKitGTK or CEF on Linux, WKWebView on macOS, Edge WebView2 on Windows
* VST3 / VST2 / CLAP / LV2 plugin formats
* Network UI support, for example for remote control using a tablet
* Just the powerful basics

The following language combinations are possible:

DSP|UI |Comments
---|---|---------------------------------------------------------------------------
C++|JS |Web view user interface, see examples [zcomp](https://github.com/lucianoiam/hiphop/tree/master/examples/zcomp) and [webgain](https://github.com/lucianoiam/hiphop/tree/master/examples/webgain).
AS |JS |Web view user interface, see example [jitdrum](https://github.com/lucianoiam/hiphop/tree/master/examples/jitdrum).
AS |C++|DPF Graphics Library (DGL), see example [astone](https://github.com/lucianoiam/hiphop/tree/master/examples/astone).
C++|C++|No need for this project, use DPF instead.

### Example JavaScript UI code

```JavaScript
class ExampleUI extends DISTRHO.UI {

    constructor() {
        super();
    
        // Connect <input type="range" id="gain"> element to a parameter

        document.getElementById('gain').addEventListener('input', (ev) => {
            this.setParameterValue(0, parseFloat(ev.target.value));
        });
    }

    parameterChanged(index, value) {
        // Host informs a parameter change, update input element value

        switch (index) {
            case 0:
                document.getElementById('gain').value = value;
                break;
        }
    }
    
}
```

The complete UI interface is defined [here](https://github.com/lucianoiam/hiphop/blob/master/hiphop/src/ui/dpf.js).

### Example AssemblyScript DSP code

```TypeScript
export default class ExamplePlugin extends DISTRHO.Plugin implements DISTRHO.PluginInterface {

    private gain: f32

    setParameterValue(index: u32, value: f32): void {
        // Host informs a parameter change, update local value

        switch (index) {
            case 0:
                this.gain = value
        }
    }

    run(inputs: Float32Array[], outputs: Float32Array[], midiEvents: DISTRHO.MidiEvent[]): void {
        // Process a single audio channel, input and output buffers have equal size

        for (let i = 0; i < inputs[0].length; ++i) {
            outputs[0][i] = this.gain * inputs[0][i]
        }
    }

}
```

The complete plugin interface is defined [here](https://github.com/lucianoiam/hiphop/blob/master/hiphop/src/dsp/dpf.ts).

### Plugin implementations

[Consul](https://github.com/lucianoiam/consul) \
[Castello Reverb](https://github.com/lucianoiam/castello)

### Related projects

[Guinda](https://github.com/lucianoiam/guinda) \
[Pisco](https://github.com/lucianoiam/pisco)

