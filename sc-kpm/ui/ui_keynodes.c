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
#include "ui_keynodes.h"



// ------------- Keynodes ----------------------
const char keynode_ui_user_str[] = "ui_user";
const char keynode_ui_command_translate_from_sc_str[] = "ui_command_translate_from_sc";
const char keynode_ui_nrel_user_answer_formats_str[] = "ui_nrel_user_answer_formats";
const char keynode_ui_rrel_source_sc_construction_str[] = "ui_rrel_source_sc_construction";
const char keynode_ui_rrel_output_format_str[] = "ui_rrel_output_format";


const char keynode_question_nrel_answer_str[] = "question_nrel_answer";
const char keynode_nrel_author_str[] = "nrel_author";


sc_addr keynode_ui_user;
sc_addr keynode_ui_command_translate_from_sc;
sc_addr keynode_ui_nrel_user_answer_formats;
sc_addr keynode_ui_rrel_source_sc_construction;
sc_addr keynode_ui_rrel_output_format;

sc_addr keynode_question_nrel_answer;
sc_addr keynode_nrel_author;

sc_bool initialize_keynodes()
{
    if (sc_helper_resolve_system_identifier(keynode_ui_user_str, &keynode_ui_user) == SC_FALSE)
        return SC_FALSE;

    if (sc_helper_resolve_system_identifier(keynode_question_nrel_answer_str, &keynode_question_nrel_answer) == SC_FALSE)
        return SC_FALSE;

    if (sc_helper_resolve_system_identifier(keynode_ui_command_translate_from_sc_str, &keynode_ui_command_translate_from_sc) == SC_FALSE)
        return SC_FALSE;

    if (sc_helper_resolve_system_identifier(keynode_nrel_author_str, &keynode_nrel_author) == SC_FALSE)
        return SC_FALSE;

    if (sc_helper_resolve_system_identifier(keynode_ui_nrel_user_answer_formats_str, &keynode_ui_nrel_user_answer_formats) == SC_FALSE)
        return SC_FALSE;

    if (sc_helper_resolve_system_identifier(keynode_ui_rrel_source_sc_construction_str, &keynode_ui_rrel_source_sc_construction) == SC_FALSE)
        return SC_FALSE;

    if (sc_helper_resolve_system_identifier(keynode_ui_rrel_output_format_str, &keynode_ui_rrel_output_format) == SC_FALSE)
        return SC_FALSE;

    return SC_TRUE;
}
