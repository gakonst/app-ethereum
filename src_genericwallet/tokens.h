/*******************************************************************************
*   Ledger Blue
*   (c) 2016 Ledger
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#include "os.h"

typedef struct tokenDefinition_t {
    uint8_t address[20];
    uint8_t ticker[6];
    uint8_t decimals;
} tokenDefinition_t;

#if defined(CHAIN_TYPE_ETHEREUM)
#define NUM_TOKENS 221
#elif defined(CHAIN_TYPE_UBIQ)
#define NUM_TOKENS 2
#elif defined(CHAIN_TYPE_EXPANSE)
#define NUM_TOKENS 0
#endif

extern tokenDefinition_t const TOKENS[NUM_TOKENS];
