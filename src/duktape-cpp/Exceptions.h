#pragma once

namespace duk {

class DuktapeException: public std::runtime_error {
public:
    explicit DuktapeException(const std::string &what)
        : std::runtime_error(what) {}
};

class ScriptEvaluationExcepton: public DuktapeException {
public:
    explicit ScriptEvaluationExcepton(const std::string &what)
        : DuktapeException(what) {}
};

class KeyError: public DuktapeException {
public:
    explicit KeyError(const std::string &what)
        : DuktapeException(what) {}
};

}
