#pragma once
#include <Arduino.h>

#include "QuectelAT.h"

// --- Modem selection ---
#if defined(MODEM_EG800K)
#include "EG800KModem.h"
using _TinyCell_impl = EG800KModem;
#elif defined(MODEM_EC200U)
#include "EC200UModem.h"
using _TinyCell_impl = EC200UModem;
#elif defined(MODEM_EC800Z)
#include "EC800ZModem.h"
using _TinyCell_impl = EC800ZModem;
#else
#error "Please define your modem type, e.g., #define MODEM_EG800K or MODEM_EC800Z"
#endif

// expose a common class name for sketches
using TinyCell = _TinyCell_impl;

#include "QuectelClient.h"
#include "QuectelClientSecure.h"
#include "QuectelConfig.h"  // configuration constants
#include "QuectelUtils.h"
