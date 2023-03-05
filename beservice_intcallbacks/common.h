#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>

#include <Windows.h>
#include <DbgHelp.h>

#include <fstream>

#include "instrumentation.h"
#include "symbols.h"

static bool g_shouldRun = 0;
static std::ofstream output;