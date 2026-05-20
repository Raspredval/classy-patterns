static_assert(__cplusplus >= 202302, "requires C++23 minimum version");

#pragma once
#include "Pattern.hpp"
#include <string>
#include <map>

namespace patt {
    namespace __impl {
        using gramm_map_type    =
            std::map<std::string, Pattern>;
        using gramm_iter        =
            gramm_map_type::iterator;
        using const_gramm_iter  =
            gramm_map_type::const_iterator;

        template<typename IterT>
        class const_accessor {
        public:
            const_accessor(IterT itPattern) :
                itPattern(itPattern) {}

            operator Pattern() const &&;

        protected:
            IterT
                itPattern;
        };

        template<typename IterT>
        class accessor :
            public const_accessor<IterT> {
        public:
            accessor(IterT itPattern) :
                const_accessor<IterT>(itPattern) {}

            accessor&
            operator=(const Pattern& pt) && {
                this->itPattern->second = pt;
                return *this;
            }
        };

        class grammar {
        public:
            grammar() = default;

            accessor<gramm_iter>
            operator[](const std::string& strKey);

            const_accessor<const_gramm_iter>
            operator[](const std::string& strKey) const;

            Pattern
            GetDefaultRule() const;

            explicit
            operator Pattern() const;

        private:
            gramm_iter
            getPatternIter(const std::string& strKey);

            const_gramm_iter
            getPatternIter(const std::string& strKey) const;

            gramm_map_type
                mapPatterns;
        };
    }
}