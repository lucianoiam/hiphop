/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021-2022 Luciano Iam <oss@lucianoiam.com>
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

#include "NetworkUI.hpp"

USE_NAMESPACE_DISTRHO

NetworkUI::NetworkUI(uint width, uint height)
    : WebUIBase(width, height)
{
    // TODO
}

NetworkUI::~NetworkUI()
{
    // TODO
}

void NetworkUI::uiIdle()
{
    fServer.process();
}

void NetworkUI::postMessage(const JsValueVector& args)
{
    // TODO - broadcast to all clients

    (void)args;
}