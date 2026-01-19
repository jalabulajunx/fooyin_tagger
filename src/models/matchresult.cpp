#include "matchresult.h"

static const bool registered = []() {
    qRegisterMetaType<Tagger::MatchResult>("Tagger::MatchResult");
    return true;
}();
