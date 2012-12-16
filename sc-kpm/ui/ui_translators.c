/*
-----------------------------------------------------------------------------
This source file is part of OSTIS (Open Semantic Technology for Intelligent Systems)
For the latest info, see http://www.ostis.net

Copyright (c) 2012 OSTIS

OSTIS is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OSTIS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with OSTIS.  If not, see <http://www.gnu.org/licenses/>.
-----------------------------------------------------------------------------
*/

#include "ui_translators.h"
#include "ui_keynodes.h"

#include "translators/ui_translator_sc2scs.h"

sc_event *ui_translator_sc2scs_event = nullptr;

void ui_initialize_translators()
{
    ui_translator_sc2scs_event = sc_event_new(keynode_ui_command_translate_from_sc, SC_EVENT_ADD_OUTPUT_ARC, 0, ui_translate_sc2scs, 0);
    if (ui_translator_sc2scs_event == nullptr)
        return SC_RESULT_ERROR;
}

void ui_shutdown_translators()
{

}