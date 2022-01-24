#ifndef RAT_IN_A_TUBE_HPP
#define RAT_IN_A_TUBE_HPP

#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include "riat.h"

namespace RIAT {
    /** Any RIAT exception will subclass this */
    class Exception : public std::exception {};

    /** Allocation error */
    class AllocationException : public Exception {
    public:
        const char *what() const noexcept override {
            return "failed to allocate memory";
        }
        ~AllocationException() noexcept override {}
    };

    /** Syntax exception */
    class SyntaxException : public Exception {
    public:
        SyntaxException(const RIAT_Instance &instance) noexcept {
            const char *file;
            this->reason = ::riat_instance_get_last_compile_error(&instance, &this->line, &this->column, &file);
            this->file = file;

            char what_error_c[512];
            std::snprintf(what_error_c, sizeof(what_error_c), "%s:%zu:%zu: error: %s", this->file.c_str(), this->line, this->column, this->reason.c_str());
            what_error = what_error_c;
        };
        const char *what() const noexcept override {
            return this->what_error.c_str();
        }
        std::size_t get_line() const noexcept {
            return this->line;
        }
        std::size_t get_column() const noexcept {
            return this->column;
        }
        const char *get_file() const noexcept {
            return this->file.c_str();
        }
        const char *get_reason() const noexcept {
            return this->reason.c_str();
        }
        ~SyntaxException() noexcept override {}
    private:
        std::string what_error;
        std::size_t line;
        std::size_t column;
        std::string file;
        std::string reason;
    };

    /**
     * RIAT instance container
     */
    class Instance {
    public:
        /**
         * Load the given script
         * 
         * @param script_source_data   pointer to the script source data
         * @param script_source_length length of the script source data
         * @param file_name            name of the file (for error reporting)
         * 
         * @throws RIAT::Exception on failure
         */
        void load_script_source(const char *script_source_data, std::size_t script_source_length, const char *file_name) {
            this->handle_compile_error(::riat_instance_load_script_source(this->get_instance(), script_source_data, script_source_length, file_name));
        }

        /**
         * Compile the given script and, if successful, clear all loaded scripts.
         * 
         * @throws RIAT::Exception on failure
         */
        void compile_scripts() {
            this->handle_compile_error(::riat_instance_compile_scripts(this->get_instance()));
        }

        /**
         * Get the nodes.
         * 
         * @return vector of nodes
         */
        std::vector<RIAT_Node> get_nodes() const {
            size_t count;
            const auto *nodes = ::riat_instance_get_nodes(this->get_instance(), &count);
            return std::vector<RIAT_Node>(nodes, nodes + count);
        }

        /**
         * Get the scripts.
         * 
         * @return vector of scripts
         */
        std::vector<RIAT_Script> get_scripts() const {
            size_t count;
            const auto *scripts = ::riat_instance_get_scripts(this->get_instance(), &count);
            return std::vector<RIAT_Script>(scripts, scripts + count);
        }

        /**
         * Get the globals.
         * 
         * @return vector of globals
         */
        std::vector<RIAT_Global> get_globals() const {
            size_t count;
            const auto *globals = ::riat_instance_get_globals(this->get_instance(), &count);
            return std::vector<RIAT_Global>(globals, globals + count);
        }

        /**
         * Set the warn calllback
         * 
         * @param callback callback
         */
        void set_warn_callback(RIAT_InstanceWarnCallback callback) noexcept {
            ::riat_instance_set_warn_callback(this->get_instance(), callback);
        }

        /**
         * Get the instance handle
         * 
         * @return instance
         */
        const RIAT_Instance *get_instance() const noexcept {
            return this->instance.get();
        }

        /**
         * Set the optimization level
         * 
         * @param level optimization level
         */
        void set_optimization_level(RIAT_OptimizationLevel level) noexcept {
            ::riat_instance_set_optimization_level(this->get_instance(), level);
        }

        /**
         * Set the compile target
         * 
         * @param target compile target
         */
        void set_compile_target(RIAT_CompileTarget target) noexcept {
            ::riat_instance_set_compile_target(this->get_instance(), target);
        }

        /**
         * Set the user data
         * 
         * @param user_data user data
         */
        void set_user_data(void *user_data) noexcept {
            ::riat_instance_set_user_data(this->get_instance(), user_data);
        }

        /**
         * Get the user data
         * 
         * @return user data
         */
        void *get_user_data() const noexcept {
            return ::riat_instance_get_user_data(this->get_instance());
        }

        /**
         * Get the instance handle
         * 
         * @return instance
         */
        RIAT_Instance *get_instance() noexcept {
            return this->instance.get();
        }

        Instance() : instance(::riat_instance_new(), ::riat_instance_delete) {
            if(this->instance.get() == nullptr) {
                throw AllocationException();
            }
        }
    private:
        std::unique_ptr<RIAT_Instance, void(*)(RIAT_Instance*)> instance;

        void handle_compile_error(RIAT_CompileResult result) {
            switch(result) {
                // Nothing bad happened
                case RIAT_CompileResult::RIAT_COMPILE_OK:
                    return;

                // Allocation error!
                case RIAT_CompileResult::RIAT_COMPILE_ALLOCATION_ERROR:
                    throw AllocationException();

                // Syntax error!
                case RIAT_CompileResult::RIAT_COMPILE_SYNTAX_ERROR:
                    throw SyntaxException(*this->instance);

                default:
                    std::terminate();
            }
        }
    };
};

#endif
