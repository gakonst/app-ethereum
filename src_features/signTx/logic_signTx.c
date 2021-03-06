#include "shared_context.h"
#include "utils.h"
#include "ui_callbacks.h"
#include "ui_flow.h"
#ifdef HAVE_STARKWARE
#include "stark_utils.h"
#endif

#define TOKEN_TRANSFER_DATA_SIZE 4 + 32 + 32
static const uint8_t const TOKEN_TRANSFER_ID[] = { 0xa9, 0x05, 0x9c, 0xbb };

#define ALLOWANCE_DATA_SIZE 4 + 32 + 32
static const uint8_t const ALLOWANCE_ID[] = { 0x09, 0x5e, 0xa7, 0xb3 };

#define ERR_SILENT_MODE_CHECK_FAILED 0x6001

#ifdef HAVE_STARKWARE

#define STARKWARE_REGISTER_DATA_SIZE 4 + 32
static const uint8_t const STARKWARE_REGISTER_ID[] = { 0x76, 0x57, 0x18, 0xd7 };
#define STARKWARE_DEPOSIT_TOKEN_DATA_SIZE 4 + 32 + 32 + 32
static const uint8_t const STARKWARE_DEPOSIT_TOKEN_ID[] = { 0x00, 0xae, 0xef, 0x8a };
#define STARKWARE_DEPOSIT_ETH_DATA_SIZE 4 + 32 + 32
static const uint8_t const STARKWARE_DEPOSIT_ETH_ID[] = { 0xe2, 0xbb, 0xb1, 0x58 };
#define STARKWARE_DEPOSIT_CANCEL_DATA_SIZE 4 + 32 + 32
static const uint8_t const STARKWARE_DEPOSIT_CANCEL_ID[] = { 0xc7, 0xfb, 0x11, 0x7c };
#define STARKWARE_DEPOSIT_RECLAIM_DATA_SIZE 4 + 32 + 32
static const uint8_t const STARKWARE_DEPOSIT_RECLAIM_ID[] = { 0x4e, 0xab, 0x38, 0xf4 };
#define STARKWARE_WITHDRAW_DATA_SIZE 4 + 32
static const uint8_t const STARKWARE_WITHDRAW_ID[] = { 0x2e, 0x1a, 0x7d, 0x4d };
#define STARKWARE_FULL_WITHDRAWAL_DATA_SIZE 4 + 32
static const uint8_t const STARKWARE_FULL_WITHDRAWAL_ID[] = { 0x27, 0x6d, 0xd1, 0xde };
#define STARKWARE_FREEZE_DATA_SIZE 4 + 32
static const uint8_t const STARKWARE_FREEZE_ID[] = { 0xb9, 0x10, 0x72, 0x09 };
#define STARKWARE_ESCAPE_DATA_SIZE 4 + 32 + 32 + 32 + 32
static const uint8_t const STARKWARE_ESCAPE_ID[] = { 0x9e, 0x3a, 0xda, 0xc4 };
static const uint8_t const STARKWARE_VERIFY_ESCAPE_ID[] = { 0x2d, 0xd5, 0x30, 0x06 };

#endif

uint32_t splitBinaryParameterPart(char *result, uint8_t *parameter) {
    uint32_t i;
    for (i=0; i<8; i++) {
        if (parameter[i] != 0x00) {
            break;
        }
    }
    if (i == 8) {
        result[0] = '0';
        result[1] = '0';
        result[2] = '\0';
        return 2;
    }
    else {
        array_hexstr(result, parameter + i, 8 - i);
        return ((8 - i) * 2);
    }
}

customStatus_e customProcessor(txContext_t *context) {
    if ((context->currentField == TX_RLP_DATA) &&
        (context->currentFieldLength != 0)) {
        dataPresent = true;
        // If handling a new contract rather than a function call, abort immediately
        if (tmpContent.txContent.destinationLength == 0) {
            return CUSTOM_NOT_HANDLED;
        }
        if (context->currentFieldPos == 0) {
            // If handling the beginning of the data field, assume that the function selector is present
            if (context->commandLength < 4) {
                PRINTF("Missing function selector\n");
                return CUSTOM_FAULT;
            }
            // Initial check to see if the call can be processed
            if ((context->currentFieldLength == TOKEN_TRANSFER_DATA_SIZE) &&
                (os_memcmp(context->workBuffer, TOKEN_TRANSFER_ID, 4) == 0) &&
                (getKnownToken(tmpContent.txContent.destination) != NULL)) {
              contractProvisioned = CONTRACT_ERC20;
            }
            else
            if ((context->currentFieldLength == ALLOWANCE_DATA_SIZE) &&
                (os_memcmp(context->workBuffer, ALLOWANCE_ID, 4) == 0)) {
              contractProvisioned = CONTRACT_ALLOWANCE;
            }
#ifdef HAVE_STARKWARE
            else
            if ((context->currentFieldLength >= STARKWARE_REGISTER_DATA_SIZE) &&
                (os_memcmp(context->workBuffer, STARKWARE_REGISTER_ID, 4) == 0)) {
              contractProvisioned = CONTRACT_STARKWARE_REGISTER;
            }
            else
            if ((context->currentFieldLength == STARKWARE_DEPOSIT_ETH_DATA_SIZE) &&
                (os_memcmp(context->workBuffer, STARKWARE_DEPOSIT_ETH_ID, 4) == 0)) {
              contractProvisioned = CONTRACT_STARKWARE_DEPOSIT_ETH;
            }
            else
            if ((context->currentFieldLength == STARKWARE_DEPOSIT_TOKEN_DATA_SIZE) &&
                (os_memcmp(context->workBuffer, STARKWARE_DEPOSIT_TOKEN_ID, 4) == 0) &&
                quantumSet) {
              contractProvisioned = CONTRACT_STARKWARE_DEPOSIT_TOKEN;
            }
            else
            if ((context->currentFieldLength == STARKWARE_WITHDRAW_DATA_SIZE) &&
                (os_memcmp(context->workBuffer, STARKWARE_WITHDRAW_ID, 4) == 0) &&
                quantumSet) {
              contractProvisioned = CONTRACT_STARKWARE_WITHDRAW;
            }
            else
            if ((context->currentFieldLength == STARKWARE_DEPOSIT_CANCEL_DATA_SIZE) &&
                (os_memcmp(context->workBuffer, STARKWARE_DEPOSIT_CANCEL_ID, 4) == 0)) {
              contractProvisioned = CONTRACT_STARKWARE_DEPOSIT_CANCEL;
            }
            else
            if ((context->currentFieldLength == STARKWARE_DEPOSIT_RECLAIM_DATA_SIZE) &&
                (os_memcmp(context->workBuffer, STARKWARE_DEPOSIT_RECLAIM_ID, 4) == 0)) {
              contractProvisioned = CONTRACT_STARKWARE_DEPOSIT_RECLAIM;
            }
            else
            if ((context->currentFieldLength == STARKWARE_FULL_WITHDRAWAL_DATA_SIZE) &&
                (os_memcmp(context->workBuffer, STARKWARE_FULL_WITHDRAWAL_ID, 4) == 0)) {
              contractProvisioned = CONTRACT_STARKWARE_FULL_WITHDRAWAL;
            }
            else
            if ((context->currentFieldLength == STARKWARE_FREEZE_DATA_SIZE) &&
                (os_memcmp(context->workBuffer, STARKWARE_FREEZE_ID, 4) == 0)) {
              contractProvisioned = CONTRACT_STARKWARE_FREEZE;
            }
            else
            if ((context->currentFieldLength == STARKWARE_ESCAPE_DATA_SIZE) &&
                (os_memcmp(context->workBuffer, STARKWARE_ESCAPE_ID, 4) == 0) &&
                quantumSet) {
              contractProvisioned = CONTRACT_STARKWARE_ESCAPE;
            }
            else
            if (os_memcmp(context->workBuffer, STARKWARE_VERIFY_ESCAPE_ID, 4) == 0) {
              contractProvisioned = CONTRACT_STARKWARE_VERIFY_ESCAPE;
            }

#endif
        }
        // Sanity check
        // Also handle exception that only need to process the beginning of the data
        if ((contractProvisioned != CONTRACT_NONE) &&
#ifdef HAVE_STARKWARE
            (contractProvisioned != CONTRACT_STARKWARE_VERIFY_ESCAPE) &&
            (contractProvisioned != CONTRACT_STARKWARE_REGISTER) &&
#endif
            (context->currentFieldLength > sizeof(dataContext.tokenContext.data))) {
          PRINTF("Data field overflow - dropping customization\n");
          contractProvisioned = CONTRACT_NONE;
        }
        PRINTF("contractProvisioned %d\n", contractProvisioned);
        if (contractProvisioned != CONTRACT_NONE) {
            if (context->currentFieldPos < context->currentFieldLength) {
                uint32_t copySize = MIN(context->commandLength,
                  context->currentFieldLength - context->currentFieldPos);
                // Handle the case where we only need to handle the beginning of the data parameter
                if ((context->currentFieldPos + copySize) < sizeof(dataContext.tokenContext.data)) {
                  copyTxData(context,
                      dataContext.tokenContext.data + context->currentFieldPos,
                      copySize);
                }
                else {
                  if (context->currentFieldPos < sizeof(dataContext.tokenContext.data)) {
                    uint32_t copySize2 = sizeof(dataContext.tokenContext.data) - context->currentFieldPos;
                    copyTxData(context,
                        dataContext.tokenContext.data + context->currentFieldPos,
                        copySize2);
                    copySize -= copySize2;
                  }
                  copyTxData(context, NULL, copySize);
                }
            }
            if (context->currentFieldPos == context->currentFieldLength) {
                context->currentField++;
                context->processingField = false;
            }
            return CUSTOM_HANDLED;
        }
        else {
            uint32_t blockSize;
            uint32_t copySize;
            uint32_t fieldPos = context->currentFieldPos;
            if (fieldPos == 0) {
                if (!N_storage.dataAllowed) {
                  PRINTF("Data field forbidden\n");
                  return CUSTOM_FAULT;
                }
                if (!N_storage.contractDetails) {
                  return CUSTOM_NOT_HANDLED;
                }
                dataContext.rawDataContext.fieldIndex = 0;
                dataContext.rawDataContext.fieldOffset = 0;
                blockSize = 4;
            }
            else {
                if (!N_storage.contractDetails) {
                  return CUSTOM_NOT_HANDLED;
                }
                blockSize = 32 - (dataContext.rawDataContext.fieldOffset % 32);
            }

            // Sanity check
            if ((context->currentFieldLength - fieldPos) < blockSize) {
                PRINTF("Unconsistent data\n");
                return CUSTOM_FAULT;
            }

            copySize = (context->commandLength < blockSize ? context->commandLength : blockSize);
            copyTxData(context,
                        dataContext.rawDataContext.data + dataContext.rawDataContext.fieldOffset,
                        copySize);

            if (context->currentFieldPos == context->currentFieldLength) {
                context->currentField++;
                context->processingField = false;
            }

            dataContext.rawDataContext.fieldOffset += copySize;

            if (copySize == blockSize) {
                // Can display
                if (fieldPos != 0) {
                    dataContext.rawDataContext.fieldIndex++;
                }
                dataContext.rawDataContext.fieldOffset = 0;
                if (fieldPos == 0) {
                    array_hexstr(strings.tmp.tmp, dataContext.rawDataContext.data, 4);
                    ux_flow_init(0, ux_confirm_selector_flow, NULL);
                }
                else {
                    uint32_t offset = 0;
                    uint32_t i;
                    snprintf(strings.tmp.tmp2, sizeof(strings.tmp.tmp2), "Field %d", dataContext.rawDataContext.fieldIndex);
                    for (i=0; i<4; i++) {
                        offset += splitBinaryParameterPart(strings.tmp.tmp + offset, dataContext.rawDataContext.data + 8 * i);
                        if (i != 3) {
                            strings.tmp.tmp[offset++] = ':';
                        }
                    }
                    ux_flow_init(0, ux_confirm_parameter_flow, NULL);
                }
            }
            else {
                return CUSTOM_HANDLED;
            }

            return CUSTOM_SUSPENDED;
        }
    }
    return CUSTOM_NOT_HANDLED;
}

void to_uppercase(char* str, unsigned char size){
    for (unsigned char i = 0; i < size && str[i] != 0; i++)
    {
        str[i] = str[i] >= 'a' ? str[i] - ('a' - 'A') : str[i];
    }
}

void compareOrCopy(char* preapproved_string, char* parsed_string, bool silent_mode){
  if(silent_mode){
    /* ETH address are not fundamentally case sensitive but might
    have some for checksum purpose, so let's get rid of these diffs */
    to_uppercase(preapproved_string, strlen(preapproved_string));
    to_uppercase(parsed_string, strlen(parsed_string));
    if(os_memcmp(preapproved_string, parsed_string, strlen(preapproved_string))){
      THROW(ERR_SILENT_MODE_CHECK_FAILED);
    }
  }
  else{
    strcpy(preapproved_string, parsed_string);
  }
}

void finalizeParsing(bool direct) {
  uint256_t gasPrice, startGas, uint256;
  uint32_t i;
  char displayBuffer[50];
  uint8_t decimals = WEI_TO_ETHER;
  uint8_t *ticker = (uint8_t *)PIC(chainConfig->coinName);
  uint8_t *feeTicker = (uint8_t *)PIC(chainConfig->coinName);
  uint8_t tickerOffset = 0;

  // Verify the chain
  if (chainConfig->chainId != 0) {
    uint32_t v = getV(&tmpContent.txContent);
    if (chainConfig->chainId != v) {
        reset_app_context();
        PRINTF("Invalid chainId %d expected %d\n", v, chainConfig->chainId);
        if (direct) {
            THROW(0x6A80);
        }
        else {
            io_seproxyhal_send_status(0x6A80);
            ui_idle();
            return;
        }
    }
  }
  // Store the hash
  cx_hash((cx_hash_t *)&global_sha3, CX_LAST, tmpCtx.transactionContext.hash, 0, tmpCtx.transactionContext.hash, 32);
#ifdef HAVE_STARKWARE
  if ((contractProvisioned == CONTRACT_STARKWARE_DEPOSIT_ETH) ||
      (contractProvisioned == CONTRACT_STARKWARE_DEPOSIT_TOKEN) ||
      (contractProvisioned == CONTRACT_STARKWARE_WITHDRAW) ||
      (contractProvisioned == CONTRACT_STARKWARE_ESCAPE)) {
    // For a deposit / withdrawal / escape, check if the token ID is known or can't parse
    uint8_t tokenIdOffset = (4 + ((contractProvisioned == CONTRACT_STARKWARE_ESCAPE) ? 32 + 32 : 0));
    if (quantumSet) {
      tokenDefinition_t *currentToken = NULL;
      if (dataContext.tokenContext.quantumIndex != MAX_TOKEN) {
        currentToken = &tmpCtx.transactionContext.tokens[dataContext.tokenContext.quantumIndex];
      }
      compute_token_id(&global_sha3,
        (currentToken != NULL ? currentToken->address : NULL),
        dataContext.tokenContext.quantum, G_io_apdu_buffer + 100);
      if (os_memcmp(dataContext.tokenContext.data + tokenIdOffset, G_io_apdu_buffer + 100, 32) != 0) {
        PRINTF("Token ID not matching - computed %.*H\n", 32, G_io_apdu_buffer + 100);
        PRINTF("Current quantum %.*H\n", 32, dataContext.tokenContext.quantum);
        PRINTF("Requested %.*H\n", 32, dataContext.tokenContext.data + tokenIdOffset);
        contractProvisioned = CONTRACT_NONE;
      }
    }
    else {
      PRINTF("Quantum not set\n");
      contractProvisioned = CONTRACT_NONE;
    }
  }
#endif
    // If there is a token to process, check if it is well known
    if ((contractProvisioned == CONTRACT_ERC20) || (contractProvisioned == CONTRACT_ALLOWANCE)) {
        tokenDefinition_t *currentToken = getKnownToken(tmpContent.txContent.destination);
        if (currentToken != NULL) {
            dataPresent = false;
            decimals = currentToken->decimals;
            ticker = currentToken->ticker;
            tmpContent.txContent.destinationLength = 20;
            os_memmove(tmpContent.txContent.destination, dataContext.tokenContext.data + 4 + 12, 20);
            os_memmove(tmpContent.txContent.value.value, dataContext.tokenContext.data + 4 + 32, 32);
            tmpContent.txContent.value.length = 32;
        }
    }
    else {
      if (dataPresent && contractProvisioned == CONTRACT_NONE && !N_storage.dataAllowed) {
          reset_app_context();
          PRINTF("Data field forbidden\n");
          if (direct) {
            THROW(0x6A80);
          }
          else {
            io_seproxyhal_send_status(0x6A80);
            ui_idle();
            return;
          }
      }
    }
  // Add address
  if (tmpContent.txContent.destinationLength != 0) {
    displayBuffer[0] = '0';
    displayBuffer[1] = 'x';
    getEthAddressStringFromBinary(tmpContent.txContent.destination, (uint8_t*)displayBuffer+2, &global_sha3, chainConfig);
    compareOrCopy(strings.common.fullAddress, displayBuffer, called_from_swap);
  }
  else
  {
    strcpy(strings.common.fullAddress, "Contract");
  }
  if ((contractProvisioned == CONTRACT_NONE) || (contractProvisioned == CONTRACT_ERC20) ||
    (contractProvisioned == CONTRACT_ALLOWANCE)) {
    // Add amount in ethers or tokens
    if ((contractProvisioned == CONTRACT_ALLOWANCE) && ismaxint(tmpContent.txContent.value.value, 32)) {
      i = 0;
      tickerOffset = 0;
      while (ticker[tickerOffset]) {
        displayBuffer[tickerOffset] = ticker[tickerOffset];
        tickerOffset++;
      }
      strcpy(displayBuffer + tickerOffset, "Unlimited");
    }
    else {
      amountToString(tmpContent.txContent.value.value, tmpContent.txContent.value.length, decimals, (char*)ticker, displayBuffer, sizeof(displayBuffer));
    }
    compareOrCopy(strings.common.fullAmount, displayBuffer, called_from_swap);
  }
  // Compute maximum fee
  PRINTF("Max fee\n");
  PRINTF("Gasprice %.*H\n", tmpContent.txContent.gasprice.length, tmpContent.txContent.gasprice.value);
  PRINTF("Startgas %.*H\n", tmpContent.txContent.startgas.length, tmpContent.txContent.startgas.value);
  convertUint256BE(tmpContent.txContent.gasprice.value, tmpContent.txContent.gasprice.length, &gasPrice);
  convertUint256BE(tmpContent.txContent.startgas.value, tmpContent.txContent.startgas.length, &startGas);
  mul256(&gasPrice, &startGas, &uint256);
  tostring256(&uint256, 10, (char *)(G_io_apdu_buffer + 100), 100);
  i = 0;
  while (G_io_apdu_buffer[100 + i]) {
    i++;
  }
  adjustDecimals((char *)(G_io_apdu_buffer + 100), i, (char *)G_io_apdu_buffer, 100, WEI_TO_ETHER);
  i = 0;
  tickerOffset=0;
  memset(displayBuffer, 0, sizeof(displayBuffer));
  while (feeTicker[tickerOffset]) {
      displayBuffer[tickerOffset] = feeTicker[tickerOffset];
      tickerOffset++;
  }
  while (G_io_apdu_buffer[i]) {
    displayBuffer[tickerOffset + i] = G_io_apdu_buffer[i];
    i++;
  }
  displayBuffer[tickerOffset + i] = '\0';
  compareOrCopy(strings.common.maxFee, displayBuffer, called_from_swap);

  bool no_consent = false;

  no_consent = called_from_swap;

#ifdef NO_CONSENT
  no_consent = true;
#endif // NO_CONSENT

  if(no_consent){
    io_seproxyhal_touch_tx_ok(NULL);
  }
  else{

#ifdef HAVE_STARKWARE

    if (contractProvisioned == CONTRACT_STARKWARE_REGISTER) {
      ux_flow_init(0, ux_approval_starkware_register_flow, NULL);
      return;
    }
    else
    if (contractProvisioned == CONTRACT_STARKWARE_DEPOSIT_TOKEN) {
      ux_flow_init(0, ux_approval_starkware_deposit_flow, NULL);
      return;
    }
    else
    if (contractProvisioned == CONTRACT_STARKWARE_DEPOSIT_ETH) {
      ux_flow_init(0, ux_approval_starkware_deposit_flow, NULL);
      return;
    }
    else
    if ((contractProvisioned == CONTRACT_STARKWARE_DEPOSIT_CANCEL) ||
        (contractProvisioned == CONTRACT_STARKWARE_DEPOSIT_RECLAIM) ||
        (contractProvisioned == CONTRACT_STARKWARE_FULL_WITHDRAWAL) ||
        (contractProvisioned == CONTRACT_STARKWARE_FREEZE)) {
      ux_flow_init(0, ux_approval_starkware_verify_vault_id_flow, NULL);
      return;
    }
    else
    if (contractProvisioned == CONTRACT_STARKWARE_WITHDRAW) {
      ux_flow_init(0, ux_approval_starkware_withdraw_flow, NULL);
      return;
    }
    else
    if (contractProvisioned == CONTRACT_STARKWARE_ESCAPE) {
      ux_flow_init(0, ux_approval_starkware_escape_flow, NULL);
      return;
    }
    else
    if (contractProvisioned == CONTRACT_STARKWARE_VERIFY_ESCAPE) {
      ux_flow_init(0, ux_approval_starkware_verify_escape_flow, NULL);
      return;
    }

#endif

    if (contractProvisioned == CONTRACT_ALLOWANCE) {
      ux_flow_init(0, ux_approval_allowance_flow, NULL);
      return;
    }

    ux_flow_init(0,
      ((dataPresent && !N_storage.contractDetails) ? ux_approval_tx_data_warning_flow : ux_approval_tx_flow),
      NULL);
  }

}

