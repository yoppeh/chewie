/**
 * @file ollama.h
 * @author Warren Mann (warren@nonvol.io)
 * @brief Interface to Ollama API.
 * @version 0.1.0
 * @date 2024-04-27
 * @copyright Copyright (c) 2024
 */

#ifndef _OLLAMA_H
#define _OLLAMA_H

#include "api.h"

extern const api_interface_t *ollama_get_aip_interface(void);

#endif // _OLLAMA_H
