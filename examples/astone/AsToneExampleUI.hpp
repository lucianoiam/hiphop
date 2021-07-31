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

#ifndef ASTONE_EXAMPLE_UI_HPP
#define ASTONE_EXAMPLE_UI_HPP

#include "DistrhoUI.hpp"
#include "EventHandlers.hpp"

#include "Blendish.hpp"

START_NAMESPACE_DISTRHO

class AsToneExampleUI : public UI, public KnobEventHandler::Callback
{
public:
    AsToneExampleUI();
    ~AsToneExampleUI() {}

    void knobDragStarted(SubWidget*) override {}
    void knobDragFinished(SubWidget*) override {}
    void knobValueChanged(SubWidget*, float value) override;

protected:
    void onDisplay() override {}
    void parameterChanged(uint32_t index, float value) override;

private:
    BlendishSubWidgetSharedContext fBlendish;
    BlendishKnob fKnob;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AsToneExampleUI)

};

END_NAMESPACE_DISTRHO

#endif  // ASTONE_EXAMPLE_UI_HPP
