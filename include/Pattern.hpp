static_assert(__cplusplus >= 202302, "requires C++23 minimum version");

#pragma once
#include <any>
#include <memory>
#include "Match.hpp"

namespace patt {
    namespace __impl {
        class pattern;
    }

    using Pattern   =
        std::unique_ptr<__impl::pattern>;

    namespace __impl {
        class pattern {
            friend class patternsList;
            friend class patternJoin;
            friend class patternChoice;
        public:
            virtual ~pattern() = default;

            virtual Pattern
            Clone() const = 0;

            OptMatch
            Eval(io::IStream& istream, CaptureList& captures, const std::any& usr_val);

            // pattern negation; inverts the evaluation result of the pattern
            friend Pattern
            operator-(Pattern&& pt) noexcept;

            // pattern negation; inverts the evaluation result of the pattern
            friend Pattern
            operator-(const Pattern& pt) noexcept;

        protected:
            virtual OptMatch
            normEval(io::IStream& istream, CaptureList& captures, const std::any& usr_val) = 0;

            virtual OptMatch
            negEval(io::IStream& istream, CaptureList& captures, const std::any& usr_val);

            Pattern
                ptNext      = nullptr;
            bool
                bNegated    = false;
        };

        class patternsList {
        public:
            patternsList(const Pattern& ptRoot);

            void
            append(const Pattern& pt) noexcept;

            Pattern
            first() const noexcept;

            Pattern
            last() const noexcept;

        private:
            Pattern
                ptRoot  = nullptr,
                ptLast  = nullptr;
        };
    }

    [[nodiscard]]
    extern OptMatch
    Eval(io::IStream& istream, const Pattern& pt, CaptureList& captures, const std::any& usr_val = {});

    [[nodiscard]]
    extern OptMatch
    Eval(io::IStream& istream, const Pattern& pt, const std::any& usr_val = {});
}