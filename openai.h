/**
 * @file openai.h
 * @author Warren Mann (warren@nonvol.io)
 * @brief Interface to OpenAI API.
 * @version 0.1.0
 * @date 2024-04-27
 * @copyright Copyright (c) 2024
 */

#ifndef _OPENAI_H
#define _OPENAI_H

#include "api.h"

extern const api_interface_t *openai_get_aip_interface(void);

#endif // _OPENAI_H
