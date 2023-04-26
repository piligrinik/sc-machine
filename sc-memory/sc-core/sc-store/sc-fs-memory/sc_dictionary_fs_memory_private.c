/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "sc_dictionary_fs_memory_private.h"

#include "../sc_types.h"
#include "../sc-base/sc_allocator.h"
#include "../sc-container/sc-string/sc_string.h"

sc_uint8 _sc_uchar_dictionary_children_size()
{
  sc_uint8 const max_sc_char = 255;
  sc_uint8 const min_sc_char = 1;

  return max_sc_char - min_sc_char + 1;
}

void _sc_uchar_dictionary_sc_char_to_sc_int(sc_char ch, sc_uint8 * ch_num, sc_uint8 const * mask)
{
  *ch_num = 128 + (sc_uint8)ch;
}

sc_bool _sc_uchar_dictionary_initialize(sc_dictionary ** dictionary)
{
  return sc_dictionary_initialize(
      dictionary, _sc_uchar_dictionary_children_size(), _sc_uchar_dictionary_sc_char_to_sc_int);
}

sc_uint8 _sc_number_dictionary_children_size()
{
  sc_uint8 const max_sc_char = 9;
  sc_uint8 const min_sc_char = 0;

  return max_sc_char - min_sc_char + 1;
}

void _sc_number_dictionary_sc_char_to_sc_int(sc_char ch, sc_uint8 * ch_num, sc_uint8 const * mask)
{
  *ch_num = (sc_uint8)ch - '0';
}

sc_bool _sc_number_dictionary_initialize(sc_dictionary ** dictionary)
{
  return sc_dictionary_initialize(
      dictionary, _sc_number_dictionary_children_size(), _sc_number_dictionary_sc_char_to_sc_int);
}

sc_bool _sc_dictionary_fs_memory_node_destroy(sc_dictionary_node * node, void ** args)
{
  (void)args;

  if (node->data != null_ptr)
  {
    sc_mem_free(((sc_list *)node->data)->begin->data);
    sc_list_destroy(node->data);
  }
  node->data = null_ptr;

  sc_mem_free(node->next);
  node->next = null_ptr;

  sc_mem_free(node->offset);
  node->offset = null_ptr;
  node->offset_size = 0;

  sc_mem_free(node);

  return SC_TRUE;
}

sc_bool _sc_dictionary_fs_memory_link_node_destroy(sc_dictionary_node * node, void ** args)
{
  (void)args;

  sc_list * link_hashes = node->data;
  if (link_hashes != null_ptr)
  {
    sc_list_destroy(link_hashes);
  }
  node->data = null_ptr;

  sc_mem_free(node->next);
  node->next = null_ptr;

  sc_mem_free(node->offset);
  node->offset = null_ptr;
  node->offset_size = 0;

  sc_mem_free(node);

  return SC_TRUE;
}

sc_bool _sc_dictionary_fs_memory_string_node_destroy(sc_dictionary_node * node, void ** args)
{
  (void)args;

  void * content = node->data;
  if (content != null_ptr)
  {
    sc_mem_free(content);
  }
  node->data = null_ptr;

  sc_mem_free(node->next);
  node->next = null_ptr;

  sc_mem_free(node->offset);
  node->offset = null_ptr;
  node->offset_size = 0;

  sc_mem_free(node);

  return SC_TRUE;
}

sc_memory_params * _sc_dictionary_fs_memory_get_default_params(sc_char const * path, sc_bool clear)
{
  sc_memory_params * params = sc_mem_new(sc_memory_params, 1);
  params->repo_path = path;
  params->clear = clear;
  params->max_strings_channels = DEFAULT_MAX_STRINGS_CHANNELS;
  params->max_strings_channel_size = DEFAULT_MAX_STRINGS_CHANNEL_SIZE;
  params->max_searchable_string_size = DEFAULT_MAX_SEARCHABLE_STRING_SIZE;
  params->term_separators = DEFAULT_TERM_SEPARATORS;

  return params;
}

sc_char * _sc_dictionary_fs_memory_get_first_term(sc_char const * string, sc_char const * term_separators)
{
  sc_uint32 const size = sc_str_len(string);
  sc_char copied_string[size + 1];
  sc_mem_cpy(copied_string, string, size);
  copied_string[size] = '\0';

  sc_char * term = strtok(copied_string, term_separators);
  sc_char * copied_term;
  term == null_ptr ? sc_string_empty(copied_term) : sc_str_cpy(copied_term, term, sc_str_len(term));
  return copied_term;
}

sc_list * _sc_dictionary_fs_memory_get_string_terms(sc_char const * string, sc_char const * term_separators)
{
  sc_uint32 const size = sc_str_len(string);
  sc_char copied_string[size + 1];
  sc_mem_cpy(copied_string, string, size);
  copied_string[size] = '\0';

  sc_char * term = strtok(copied_string, term_separators);
  sc_list * terms;
  sc_list_init(&terms);

  sc_dictionary * unique_terms;
  sc_dictionary_initialize(&unique_terms, _sc_uchar_dictionary_children_size(), _sc_uchar_dictionary_sc_char_to_sc_int);
  while (term != null_ptr)
  {
    sc_uint64 const term_length = sc_str_len(term);

    if (!sc_dictionary_has(unique_terms, term, term_length))
    {
      sc_char * term_copy;
      sc_str_cpy(term_copy, term, term_length);

      sc_list_push_back(terms, term_copy);
      sc_dictionary_append(unique_terms, term_copy, term_length, null_ptr);
    }

    term = strtok(null_ptr, term_separators);
  }
  sc_dictionary_destroy(unique_terms, sc_dictionary_node_destroy);

  if (terms->size == 0)
  {
    sc_char * str;
    sc_string_empty(str);
    sc_list_push_back(terms, str);
  }

  return terms;
}
