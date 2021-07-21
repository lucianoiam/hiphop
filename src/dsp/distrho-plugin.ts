/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021 Luciano Iam <oss@lucianoiam.com>
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

// Everything contained in this file attempts to mimic the C++ plugin interfaces
// Low-level interactions with native container are strictly handled by index.ts

import { dpf_get_sample_rate } from './index'

export default namespace DISTRHO {

    // There is no support for virtual methods in AssemblyScript. Methods that
    // are implemented by the plugin author are declared in PluginInterface
    // and methods provided by the native container implemented in Plugin class.
    // C++ DISTRHO::Plugin = AS Plugin + AS PluginInterface

    export interface PluginInterface {

        // const char* Plugin::getLabel()
        getLabel(): string

        // const char* Plugin::getMaker()
        getMaker(): string

        // const char* Plugin::getLicense()
        getLicense(): string

        // void Plugin::initParameter(uint32_t index, Parameter& parameter)
        initParameter(index: u32, parameter: Parameter): void

        // float Plugin::getParameterValue(uint32_t index)
        getParameterValue(index: u32): f32

        // void Plugin::setParameterValue(uint32_t index, float value)
        setParameterValue(index: u32, value: f32): void

        // void Plugin::activate()
        activate(): void

        // void Plugin::deactivate()
        deactivate(): void

        // void Plugin::run(const float** inputs, float** outputs, uint32_t frames)
        run(inputs: Float32Array[], outputs: Float32Array[]): void

    }

    export class Plugin {
        
        // double Plugin::getSampleRate();
        getSampleRate(): f32 {
            return dpf_get_sample_rate()
        }

    }

    // struct DISTRHO::Parameter
    export class Parameter {

        name: string = ''
        ranges: ParameterRanges = new ParameterRanges

    }

    // struct DISTRHO::ParameterRanges
    export class ParameterRanges {

        def: f32
        min: f32
        max: f32

    }

}
