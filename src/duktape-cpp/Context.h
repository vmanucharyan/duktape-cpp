#pragma once

#include <map>
#include <string>
#include <atomic>
#include <memory>
#include <vector>

#include <duktape.h>

#include "Box.h"

namespace duk {

/**
 * @brief Wrapper around duktape context
 */
class Context {
public:
    static Context & GetSelfFromContext(duk_context *d);

    /**
     * @brief constructor from script id
     * @param scriptId script asset id
     */
    explicit Context(std::string const &scriptId = "");
    ~Context();

    Context(const Context &) = delete;
    Context & operator = (const Context &) = delete;

    Context(Context && that) noexcept;
    Context & operator = (Context && that) noexcept;

    /**
     * @brief Get pointer to duk_context
     */
    duk_context * ptr() { return _ctx; }

    /**
     * @brief Get script id
     */
    const std::string &scriptId() const { return _scriptId; }

    operator duk_context*() const { return _ctx; }

    /**
     * @brief   Store a Box in the context
     * @details Box contains some native resource, used from script,
     *          for example shared or unique pointers.
     * @param box box to store
     * @returns integer key to access box
     */
    int storeBox(std::unique_ptr<BoxBase> box);

    /**
     * @brief Get box from context
     * @param key box key (see `storeBox` method)
     */
    BoxBase & getBox(int key) const;

    /**
     * @brief Remove box from context
     * @param key box key (see `storeBox` method)
     */
    void removeBox(int key);

    /**
     * @brief Add global value to this context
     * @tparam T value type
     * @param name name of the value (must be valid variable name for javascript)
     * @param val value
     */
    template <class T>
    void addGlobal(const char *name, T &&val);

    /**
     * @brief Register a class to this context (class must have `inspect` method)
     * @tparam T class type
     */
    template <class T>
    void registerClass();

    /**
     * @brief Evaluate string and get result
     * @tparam T result type
     * @param[out] res result
     * @param[in] str javascript code
     * @throws ScriptEvaluationExcepton if something goes wrong
     */
    template <class T>
    void evalString(T &res, const char *str);

    /**
     * @brief Evaluate string and ignore the result
     * @param str javascript code
     * @throws ScriptEvaluationExcepton if something goes wrong
     */
    void evalStringNoRes(const char *str);

    /**
     * Store reference to javascript object inside of duktape context
     * @param stackIndex index of the object to store in the current stack
     * @return integer key to stored object, see stashRef
     * @remarks this operation is not thread safe
     */
    int stashRef(int stackIndex);

    /**
     * Delete stashed reference to js object
     * @param refKey key to stashed object (see return value of stashRef method)
     * @remarks this operation is not thread safe
     */
    void unstashRef(int refKey);

    /**
     * Get stored reference to javascript object (see stashRef).
     * Object will be placed at the stack top
     * @param stored object key (see return value of stashRef)
     */
    void getRef(int key);

    /**
     * Get global variable
     * @tparam T global variable type
     * @param[in] name global variable name
     * @param[out] res result
     */
    template <class T>
    void getGlobal(const char *name, T &res);

private:
    duk_context *_ctx;
    std::string _scriptId;
    std::atomic_int _boxCounter { 0 };
    std::map<int, std::unique_ptr<BoxBase>> _boxes;
    int _objectRefCounter { 0 };

    template <class T>
    void push(T &&val);

    int defNamespaces(std::vector<std::string> const &ns);
    void rethrowDukError();
    void assignSelf();
};

}
