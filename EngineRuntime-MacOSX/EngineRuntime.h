#pragma once

#define ENGINE_X64
#define ENGINE_UNIX
#define ENGINE_MACOSX

#include "EngineBase.h"
#include "Streaming.h"
#include "Processes/Process.h"
#include "Processes/Threading.h"
#include "Processes/Shell.h"
#include "Syntax/Grammar.h"
#include "Syntax/MathExpression.h"
#include "Syntax/Regular.h"
#include "Syntax/Tokenization.h"
#include "Miscellaneous/Dictionary.h"
#include "Miscellaneous/DynamicString.h"
#include "Miscellaneous/UndoBuffer.h"
#include "Miscellaneous/Time.h"
#include "Storage/Registry.h"
#include "Storage/TextRegistry.h"