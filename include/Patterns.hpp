static_assert(__cplusplus >= 202302, "requires C++23 minimum version");

#pragma once
#include <flat_set>
#include "Pattern.hpp"
#include "Grammar.hpp"

namespace patt {
    namespace __impl {
        class grammar;
    }

    // grammar; allows to name and execute groups of patterns;
    // named groups can be executed recursively, DOESNT CHECK FOR INFINITE RECURSION!
    using Grammar   =
        __impl::grammar;

    namespace __impl {
        class patternJoin;

        using JoinPattern   =
            std::shared_ptr<__impl::patternJoin>;

        class patternJoin :
            public pattern {
        public:
            patternJoin(const Pattern& ptA, const Pattern& ptB);

            void
            Append(const Pattern& pt);

            void
            Prepend(Pattern&& pt);

            void
            Append(JoinPattern&& pt);

            [[nodiscard]] Pattern
            Clone() const override;

        protected:
            Match
            normEval(io::IStream& istream, CaptureGroupList& capture_groups, const std::any& usr_val) override;

        private:
            patternsList
                lst;
        };

        // join pattern; joins several patterns into a chain of patterns to be executed one after the other;
        // stops execution if one of the patterns in the chain fails
        [[nodiscard]]
        extern JoinPattern
        operator>>(JoinPattern&& ptA, const Pattern& ptB);

        // join pattern; joins several patterns into a chain of patterns to be executed one after the other;
        // stops execution if one of the patterns in the chain fails
        [[nodiscard]]
        extern JoinPattern
        operator>>(Pattern&& ptA, JoinPattern&& ptB);

        // join pattern; joins several patterns into a chain of patterns to be executed one after the other;
        // stops execution if one of the patterns in the chain fails
        [[nodiscard]]
        extern JoinPattern
        operator>>(JoinPattern&& ptA, JoinPattern&& ptB);

        // join pattern; joins several patterns into a chain of patterns to be executed one after the other;
        // stops execution if one of the patterns in the chain fails
        [[nodiscard]]
        extern JoinPattern
        operator>>(const JoinPattern& ptA, const Pattern& ptB);

        // join pattern; joins several patterns into a chain of patterns to be executed one after the other;
        // stops execution if one of the patterns in the chain fails
        [[nodiscard]]
        extern JoinPattern
        operator>>(const Pattern& ptA, const Pattern& ptB);

        class patternString :
            public pattern {
        public:
            patternString(std::string_view strvPattern);

            [[nodiscard]] Pattern
            Clone() const override;

        protected:
            Match
            normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) override;

        private:
            std::string
                strPattern;
        };

        class patternAny :
            public pattern {
        public:
            patternAny() = default;

            [[nodiscard]] Pattern
            Clone() const override;

        protected:
            Match
            normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) override;
        };

        class patternChoice :
            public pattern {
        public:
            patternChoice(const Pattern& ptA, const Pattern& ptB);

            [[nodiscard]] Pattern
            Clone() const override;

            void
            Append(const Pattern& pt);

        protected:
            Match
            normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) override;

        private:
            patternsList
                lst;
        };

        using ChoicePattern =
            std::shared_ptr<__impl::patternChoice>;

        // linear choice pattern; tries to evaluate every choice group until it matches;
        // backtracks if a choice group fails to match and tries to match the next group
        [[nodiscard]]
        extern ChoicePattern
        operator|(ChoicePattern&& ptA, const Pattern& ptB);

        // linear choice pattern; tries to evaluate every choice group until it matches;
        // backtracks if a choice group doesnt match and tries to match the next group
        [[nodiscard]]
        extern ChoicePattern
        operator|(const ChoicePattern& ptA, const Pattern& ptB);

        // linear choice pattern; tries to evaluate every choice group until it matches;
        // backtracks if a choice group doesnt match and tries to match the next group
        [[nodiscard]]
        extern ChoicePattern
        operator|(const Pattern& ptA, const Pattern& ptB);

        class patternSet :
            public pattern {
        public:
            patternSet(std::string_view strvSet);

            [[nodiscard]] Pattern
            Clone() const override;

        protected:
            Match
            normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) override;

        private:
            std::flat_set<char, std::less<char>, std::basic_string<char>>
                set;
        };

        class patternRepeat :
            public pattern {
        public:
            patternRepeat(const Pattern& ptRepeat, ssize_t iCount);

            [[nodiscard]] Pattern
            Clone() const override;

        protected:
            Match
            normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) override;

            Match
            negEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) override;

        private:
            Pattern
                ptRepeat;
            size_t
                uCount;
        };

        // repeat pattern; if N is negative, then it repeats no more than N times;
        // if N is not negative, then it repeats at least N times or more
        [[nodiscard]]
        extern Pattern
        operator%(const patt::Pattern& ptA, ssize_t iCount);

        class patternRepeatExact :
            public pattern {
        public:
            patternRepeatExact(const Pattern& ptRepeat, size_t uCount);

            [[nodiscard]] Pattern
            Clone() const override;

        protected:
            Match
            normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) override;

        private:
            Pattern
                ptRepeat;
            size_t
                uCount;
        };

        // repeat exact pattern; repeats given pattern exactly N times
        [[nodiscard]]
        extern Pattern
        operator*(const patt::Pattern& ptA, size_t uCount);

        class patternGrammar :
            public pattern {
        public:
            patternGrammar(const_gramm_iter itPattern);

            [[nodiscard]] Pattern
            Clone() const override;

        protected:
            Match
            normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) override;

        private:
            const_gramm_iter
                itPattern;
        };

        using Callback  =
            void(*)(io::IStream&, const Match&, const CaptureGroupList&, const std::any&);

        class patternHandler :
            public pattern {
        public:
            patternHandler(const Pattern& ptHandle, Callback lpfnCallback);

            [[nodiscard]] Pattern
            Clone() const override;

        protected:
            Match
            normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) override;

        private:
            Pattern
                ptHandle;
            Callback
                lpfnCallback;
        };

        // handler pattern; allows to insert code in between evaluation and observe the evaluation result of a pattern;
        // handler type signature: void (io::IStream&, const patt::Match&, patt::CaptureList&, const std::any&)
        [[nodiscard]]
        extern Pattern
        operator/(const Pattern& ptHandle, Callback lpfnCallback);

        using LocaleProc    =
            int(*)(int);

        class patternCLocale :
            public pattern {
        public:
            patternCLocale(LocaleProc lpfn);

            [[nodiscard]] Pattern
            Clone() const override;

        protected:
            Match
            normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) override;

        private:
            LocaleProc
                lpfn;
        };

        class patternCapture :
            public pattern {
        public:
            patternCapture(const Pattern& ptCapture);

            [[nodiscard]] Pattern
            Clone() const override;

        protected:
            Match
            normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) override;

        private:
            Pattern
                ptCapture;
        };

        class patternCaptureGroup :
            public pattern {
        public:
            patternCaptureGroup(const Pattern& ptCaptureFrom, Callback lpfnCallback);

            [[nodiscard]] Pattern
            Clone() const override;

        protected:
            Match
            normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) override;

        private:
            Pattern
                ptCaptureFrom;
            Callback
                lpfnCallback;
        };

        class patternLookAhead :
            public pattern {
        public:
            patternLookAhead(const Pattern& ptLookAhead);

            [[nodiscard]] Pattern
            Clone() const override;

        protected:
            Match
            normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) override;

        private:
            Pattern
                ptLookAhead;
        };

        // look ahead operator; evaluates a pattern and backtracks back
        [[nodiscard]]
        extern Pattern
        operator&(const Pattern& ptLookAhead);
    }

    // matches an exact string of characters
    [[nodiscard]]
    extern Pattern
    Str(std::string_view strvPattern);

    // matches if there is at least one character left to match
    [[nodiscard]]
    extern Pattern
    Any();

    // doesnt match if there is at least one character left to match
    [[nodiscard]]
    extern Pattern
    None();

    // matches single character if it belongs to a set of characters
    [[nodiscard]]
    extern Pattern
    Set(std::string_view strvSet);

    // matches single letter
    [[nodiscard]]
    extern Pattern
    Alpha();

    // matches single letter or digit
    [[nodiscard]]
    extern Pattern
    AlphaNum();

    // matches single decimal digit
    [[nodiscard]]
    extern Pattern
    Digit();

    // matches single hexadecimal digit
    [[nodiscard]]
    extern Pattern
    HexDigit();

    // matches single lower case letter
    [[nodiscard]]
    extern Pattern
    LowerCase();

    // matches single upper case letter
    [[nodiscard]]
    extern Pattern
    UpperCase();

    // matches single blank or new line character
    [[nodiscard]]
    extern Pattern
    SpaceOrNewLine();

    // matches single blank character
    [[nodiscard]]
    extern Pattern
    Space();

    // captures a successful match and appends it to
    // the current capture group on the top of the group stack
    [[nodiscard]]
    extern Pattern
    Capt(const Pattern& ptCapture);

    // isolates captures from a pattern to a pattern group,
    // pushes a new capture group to the stack before executing the pattern
    // and pops is after finishing executing the pattern
    [[nodiscard]]
    extern Pattern
    CaptGr(const Pattern& ptCaptureFrom, __impl::Callback lpfnCallback = nullptr);
}