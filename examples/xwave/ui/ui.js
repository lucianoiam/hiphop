/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021-2023 Luciano Iam <oss@lucianoiam.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

import '/dpf.js';
import { WaveformElement } from '/thirdparty/x-waveform.js'

const NETWORK_REFRESH_FREQ = 10; /*kFrequencyNetwork*/
const DISPLAY_NUM_BINS     = 8;
const DISPLAY_SCALE_X      = 0.25;

const env = DISTRHO.env, uiHelper = DISTRHO.UIHelper;

function main() {
    new XWaveExampleUI;
}

class XWaveExampleUI extends DISTRHO.UI {

    constructor() {
        super();

        this._sampleRate = 0;
        this._sampleBuffer = [];
        this._prevFrameTimeMs = 0;

        this._initView();
    }

    sampleRateChanged(newSampleRate) {
        this._sampleRate = newSampleRate;
    }

    async messageChannelOpen() {
        this.sampleRateChanged(await this.getSampleRate());

        if (this._prevFrameTimeMs == 0) {
            this._startVisualization();
        }
    }

    // Receive higher latency visualization data sent through WebSocket
    onVisualizationData(data) {
        if (! env.plugin) {
            this._addSamples(data.samples.buffer);
        }
    }

    _initView() {
        if (env.plugin) {
            document.body.appendChild(uiHelper.getNetworkDetailsModalButton(this, {
                fill: '#fff',
                id: 'qr-button',
                modal: {
                    id: 'qr-modal'
                }
            }));
        } else {
            uiHelper.enableOfflineModal(this);
        }

        window.customElements.define('x-waveform', WaveformElement);
        this._waveform = new WaveformElement;
        document.body.prepend(this._waveform);
        this._waveform.setAttribute('autoresize', '');
        this._waveform.setAttribute('width', this._waveform.clientWidth 
                                        * DISPLAY_SCALE_X);

        document.body.style.visibility = 'visible';
    }

    _startVisualization() {
        // Register to receive low latency visualization data sent using local
        // transport, skipping network overhead. Plugin embedded web view only.
        if (env.plugin) {
            window.host.addMessageListener(data => {
                this._addSamples(DISTRHO.Base64.decode(data.samples.$binary));
            });
        }

        this._animate(0);
    }

    _addSamples(/*Uint8Array*/loResSamples) {
        const floatSamples = new Float32Array(loResSamples).map(s => s / 255);
        this._sampleBuffer.push(...floatSamples);

        const maxBufferSize = Math.floor(NETWORK_REFRESH_FREQ / 60 * this._sampleRate),
              numSamples = this._sampleBuffer.length;

        if (numSamples > maxBufferSize) { // buffer overflow?
            this._sampleBuffer.splice(0, numSamples - maxBufferSize);
        }
    }

    _animate(timestampMs) {
        if (this._prevFrameTimeMs > 0) {
            const deltaMs = timestampMs - this._prevFrameTimeMs;

            if (deltaMs <= 1000/NETWORK_REFRESH_FREQ) {
                const numSamples = Math.floor(deltaMs / 1000 * this._sampleRate),
                      binSize = Math.floor(numSamples / DISPLAY_NUM_BINS),
                      bins = new Float32Array(DISPLAY_NUM_BINS);

                let k = 0;

                for (let i = 0; i < DISPLAY_NUM_BINS; i++) {
                    for (let j = 0; j < binSize; j++) {
                        if (k < this._sampleBuffer.length) {
                            bins[i] += this._sampleBuffer[k++];
                        } else {
                            break; // buffer underrun
                        }
                    }
                    bins[i] /= binSize;
                }

                this._sampleBuffer.splice(0, k);
                this._waveform.analyserData = bins;

                //console.log(`DBG : sr=${this._sampleRate} dt=${deltaMs}ms \
                //            pop=${k} left=${this._sampleBuffer.length}`);

            } else {
                this._sampleBuffer = []; // animation was paused
            }
        }

        this._prevFrameTimeMs = timestampMs;

        window.requestAnimationFrame(t => { this._animate(t) });
    }

}

main();
