#ifndef RAT_IN_A_TUBE_HPP
#define RAT_IN_A_TUBE_HPP

#include <cstdio>
#include <memory>
#include <string>
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
            this->reason = RIAT_instance_get_last_compile_error(&instance, &this->line, &this->column, &file);
            this->file = file;

            char what_error_c[512];
            std::snprintf(what_error_c, sizeof(what_error_c), "%s @ %s:%zu:%zu", this->reason.c_str(), this->file.c_str(), this->line, this->column);
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
        void load_script(const char *script_source_data, size_t script_source_length, const char *file_name) {
            handle_compile_error(RIAT_instance_load_script(instance.get(), script_source_data, script_source_length, file_name));
        }

        /**
         * Load the given script
         * 
         * @param script_source_data script source data
         * @param file_name          name of the file (for error reporting)
         * 
         * @throws RIAT::Exception on failure
         */
        void load_script(const std::string &script_source_data, const char *file_name) {
            return this->load_script(script_source_data.data(), script_source_data.size(), file_name);
        }

        /**
         * Compile the given script and, if successful, clear all loaded scripts.
         * 
         * @throws RIAT::Exception on failure
         */
        void compile_scripts() {
            handle_compile_error(RIAT_instance_compile_scripts(instance.get()));
        }

        Instance(RIAT_CompileTarget compile_target) : instance(RIAT_instance_new(compile_target), RIAT_instance_delete) {
            if(this->instance.get() == nullptr) {
                throw AllocationException();
            }
        }

        Instance() : Instance(RIAT_CompileTarget::RIAT_COMPILE_TARGET_ANY) {}
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
