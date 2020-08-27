/**
 * @file StrixLTLParser.h
 *
 * @brief I adapteed this form STRIX/ltl (https://strix.model.in.tum.de).
 *
 * @author M. Khaled
 *
 */
#pragma once

#include <string>
#include <vector>

#include "StrixOwl.h"

namespace strix_ltl {

    enum class SpecificationType : uint8_t {
        FORMULA,
        AUTOMATON,
    };

    class Specification {
    private:
        Specification(const Specification&) = delete;
        Specification& operator=(const Specification&) = delete;
    public:
        JVM owl;
        void* owl_object;
        SpecificationType type;
        std::vector<std::string> inputs;
        std::vector<std::string> outputs;

        Specification(
            JVM owl,
            void* owl_object,
            SpecificationType type,
            std::vector<std::string> inputs,
            std::vector<std::string> outputs
        ) :
            owl(owl),
            owl_object(owl_object),
            type(type),
            inputs(inputs),
            outputs(outputs)
        {}
        Specification(Specification&& other) :
            owl(other.owl),
            owl_object(other.owl_object),
            type(std::move(other.type)),
            inputs(std::move(other.inputs)),
            outputs(std::move(other.outputs))
        {
            other.owl_object = nullptr;
        }

        ~Specification() {
            if (owl_object != nullptr) {
                destroy_object_handle(owl, owl_object);
            }
        }
    };


    class Parser {
    private:
        JVM owl;

    protected:
        Parser(JVM owl);

        virtual Specification parse_string_with_owl(
                JVM owl,
                const std::string& text
        ) const = 0;

    public:
        virtual ~Parser();

        std::string parse_file(const std::string& input_file);
        Specification parse_string(const std::string& input_string, const int verbosity = 0) const;
    };    

    class LTLParser : public Parser {
    private:
        const std::vector<std::string>& inputs;
        const std::vector<std::string>& outputs;
        const std::string finite;

    protected:
        Specification parse_string_with_owl(
                JVM owl,
                const std::string& text
        ) const;
    public:
        LTLParser(JVM owl, const std::vector<std::string>& inputs, const std::vector<std::string>& outputs, const std::string finite = "");
        ~LTLParser();
    };

}