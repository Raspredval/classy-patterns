static_assert(__cplusplus >= 202302, "requires C++23 minimum version");

#pragma once
#include <flat_set>
#include <concepts>
#include "Pattern.hpp"
#include "Grammar.hpp"

#ifdef CLASSY_PATTERNS_TRACE_GRAMMAR
#include <classy-streams/ConsoleStreams.hpp>
#endif

namespace patt {
    namespace __impl {
        class grammar;
    }

    // grammar; allows to name and execute groups of patterns;
    // named groups can be executed recursively, DOESNT CHECK FOR INFINITE RECURSION!
    // also, can be converted to a pattern, returns a pattern group named '__eval'
    using Grammar   =
        __impl::grammar;

    namespace __impl {
        class patternJoin :
            public pattern {
        public:
            patternJoin(const Pattern& ptA, const Pattern& ptB) :
                lst(ptA)
            {
                this->lst.Append(ptB);
            }

            void
            Append(const Pattern& pt) {
                this->lst.Append(pt);
            }

            [[nodiscard]] Pattern
            Clone() const override {
                return std::make_shared<patternJoin>(*this);
            }

        protected:
            OptMatch
            normEval(io::IStream& istream, CaptureList& captures, const std::any& usr_val) override {
                Pattern
                    ptCur   = lst.First();
                intptr_t
                    iBegin  = istream.GetPosition();
                OptMatch
                    optmRet = Match(iBegin, iBegin);
                while (ptCur != nullptr) {
                    OptMatch
                        optmCur = ptCur->Eval(istream, captures, usr_val);
                    if (!optmCur)
                        return std::nullopt;

                    *optmRet    += *optmCur;
                    ptCur       = ptCur->NextPattern();
                }

                return optmRet;
            }

        private:
            PatternList
                lst;
        };

        using JoinPattern   =
            std::shared_ptr<__impl::patternJoin>;

        // join pattern; joins several patterns into a chain of patterns to be executed one after the other;
        // stops execution if one of the patterns in the chain fails
        [[nodiscard]]
        inline JoinPattern
        operator>>(JoinPattern&& ptA, const Pattern& ptB) {
            ptA->Append(ptB);
            return ptA;
        }

        // join pattern; joins several patterns into a chain of patterns to be executed one after the other;
        // stops execution if one of the patterns in the chain fails
        [[nodiscard]]
        inline JoinPattern
        operator>>(const JoinPattern& ptA, const Pattern& ptB) {
            return std::make_shared<__impl::patternJoin>(*ptA) >> ptB;
        }

        // join pattern; joins several patterns into a chain of patterns to be executed one after the other;
        // stops execution if one of the patterns in the chain fails
        [[nodiscard]]
        inline JoinPattern
        operator>>(const Pattern& ptA, const Pattern& ptB) {
            return std::make_shared<__impl::patternJoin>(ptA, ptB);
        }

        class patternString :
            public pattern {
        public:
            patternString(std::string_view strvPattern) :
                strPattern(strvPattern) {}

            [[nodiscard]] Pattern
            Clone() const override {
                return std::make_shared<patternString>(*this);
            }

        protected:
            OptMatch
            normEval(io::IStream& istream, CaptureList&, const std::any&) override {
                intptr_t
                    iBegin  = istream.GetPosition();
                for (char c : this->strPattern) {
                    std::optional<std::byte>
                        optc    = istream.Read();
                    if (!optc || (char)*optc != c)
                        return std::nullopt;
                }
                intptr_t
                    iEnd    = istream.GetPosition();
                return Match(iBegin, iEnd);
            }

        private:
            std::string
                strPattern;
        };

        class patternAny :
            public pattern {
        public:
            [[nodiscard]] Pattern
            Clone() const override {
                return std::make_shared<patternAny>(*this);
            }

        protected:
            OptMatch
            normEval(io::IStream& istream, CaptureList&, const std::any&) override {
                intptr_t
                    iBegin  = istream.GetPosition();
                std::optional<std::byte>
                    optc    = istream.Read();
                if (!optc)
                    return std::nullopt;
                intptr_t
                    iEnd    = istream.GetPosition();
                return Match(iBegin, iEnd);
            }
        };

        class patternChoice :
            public pattern {
        public:
            patternChoice(const Pattern& ptA, const Pattern& ptB) :
                lst(ptA)
            {
                this->lst.Append(ptB);
            }

            [[nodiscard]] Pattern
            Clone() const override {
                return std::make_shared<patternChoice>(*this);
            }

            void
            Append(const Pattern& pt) {
                this->lst.Append(pt);
            }

        protected:
            OptMatch
            normEval(io::IStream& istream, CaptureList& captures, const std::any& usr_val) override {
                intptr_t
                    iBegin  = istream.GetPosition();
                Pattern
                    ptCur   = this->lst.First();
                while (ptCur) {
                    OptMatch
                        optm    = ptCur->Eval(istream, captures, usr_val);
                    if (optm)
                        return optm;

                    ptCur       = ptCur->NextPattern();
                    istream.SetPosition(iBegin);
                }

                return std::nullopt;
            }

        private:
            PatternList
                lst;
        };

        using ChoicePattern =
            std::shared_ptr<__impl::patternChoice>;        

        // linear choice pattern; tries to evaluate every choice group until it matches;
        // backtracks if a choice group fails to match and tries to match the next group
        [[nodiscard]]
        inline ChoicePattern
        operator|(ChoicePattern&& ptA, const Pattern& ptB) {
            ptA->Append(ptB);
            return ptA;
        }

        // linear choice pattern; tries to evaluate every choice group until it matches;
        // backtracks if a choice group doesnt match and tries to match the next group
        [[nodiscard]]
        inline ChoicePattern
        operator|(const ChoicePattern& ptA, const Pattern& ptB) {
            return std::make_shared<__impl::patternChoice>(*ptA) | ptB;
        }

        // linear choice pattern; tries to evaluate every choice group until it matches;
        // backtracks if a choice group doesnt match and tries to match the next group
        [[nodiscard]]
        inline ChoicePattern
        operator|(const Pattern& ptA, const Pattern& ptB) {
            return std::make_shared<__impl::patternChoice>(ptA, ptB);
        }

        class patternSet :
            public pattern {
        public:
            patternSet(std::string_view strvSet) {
                for (char c : strvSet)
                    this->set.emplace(c);
            }

            [[nodiscard]] Pattern
            Clone() const override {
                return std::make_shared<patternSet>(*this);
            }

        protected:
            OptMatch
            normEval(io::IStream& istream, CaptureList&, const std::any&) override {
                intptr_t
                    iBegin  = istream.GetPosition();
                std::optional<std::byte>
                    optc    = istream.Read();
                
                if (!optc || !this->set.contains((char)*optc))
                    return std::nullopt;
                
                intptr_t
                    iEnd    = istream.GetPosition();
                return Match(iBegin, iEnd);
            }

        private:
            std::flat_set<char, std::less<char>, std::basic_string<char>>
                set;
        };

        class patternRepeat :
            public pattern {
        public:
            patternRepeat(const Pattern& ptRepeat, ssize_t iCount) :
                ptRepeat(ptRepeat),
                uCount((size_t)std::abs(iCount))
            {
                if (iCount < 0)
                    this->toggleNegated();
            }

            [[nodiscard]] Pattern
            Clone() const override {
                return std::make_shared<patternRepeat>(*this);
            }

        protected:
            OptMatch
            normEval(io::IStream& istream, CaptureList& captures, const std::any& usr_val) override {
                intptr_t
                    iBegin  = istream.GetPosition();
                for (size_t i = 0; i != this->uCount; ++i) {
                    if (!this->ptRepeat->Eval(istream, captures, usr_val))
                        return std::nullopt;
                }

                intptr_t
                    iEnd    = istream.GetPosition();
                while (true) {
                    if (!this->ptRepeat->Eval(istream, captures, usr_val)) {
                        istream.SetPosition(iEnd);
                        break;
                    }

                    iEnd        = istream.GetPosition();
                }

                return Match(iBegin, iEnd);
            }

            OptMatch
            negEval(io::IStream& istream, CaptureList& captures, const std::any& usr_val) override {
                intptr_t
                    iBegin  = istream.GetPosition();
                for (size_t i = 0; i != this->uCount; ++i) {
                    intptr_t
                        iEnd    = istream.GetPosition();
                    if (!this->ptRepeat->Eval(istream, captures, usr_val)) {
                        istream.SetPosition(iEnd);
                        return Match(iBegin, iEnd);
                    }
                }

                intptr_t
                    iEnd    = istream.GetPosition();
                return Match(iBegin, iEnd);
            }

        private:
            Pattern
                ptRepeat;
            size_t
                uCount;
        };

        // repeat pattern; if N is negative, then it repeats no more than N times;
        // if N is not negative, then it repeats at least N times or more
        [[nodiscard]]
        inline Pattern
        operator%(const patt::Pattern& ptA, ssize_t iCount) {
            return std::make_shared<patternRepeat>(ptA, iCount);
        }

        class patternRepeatExact :
            public pattern {
        public:
            patternRepeatExact(const Pattern& ptRepeat, size_t uCount) :
                ptRepeat(ptRepeat),
                uCount(uCount) {}

            [[nodiscard]] Pattern
            Clone() const override {
                return std::make_shared<patternRepeatExact>(*this);
            }

        protected:
            OptMatch
            normEval(io::IStream& istream, CaptureList& captures, const std::any& usr_val) override {
                intptr_t
                    iBegin  = istream.GetPosition();
                for (size_t i = 0; i != this->uCount; ++i) {
                    if (!this->ptRepeat->Eval(istream, captures, usr_val))
                        return std::nullopt;
                }
                intptr_t
                    iEnd    = istream.GetPosition();
                return Match(iBegin, iEnd);
            }

        private:
            Pattern
                ptRepeat;
            size_t
                uCount;
        };

        // repeat exact pattern; repeats given pattern exactly N times
        [[nodiscard]]
        inline Pattern
        operator*(const patt::Pattern& ptA, size_t uCount) {
            return std::make_shared<patternRepeatExact>(ptA, uCount);
        }

        class patternGrammar :
            public pattern {
        public:
            patternGrammar(const_gramm_iter itPattern) :
                itPattern(itPattern) {}

            [[nodiscard]] Pattern
            Clone() const override {
                return std::make_shared<patternGrammar>(*this);
            }

        protected:
            OptMatch
            normEval(io::IStream& istream, CaptureList& captures, const std::any& usr_val) override {
                #ifdef CLASSY_PATTERNS_TRACE_GRAMMAR
                io::cout.fmt("{}:\tbegin\n", this->itPattern->first);
                #endif
                
                if (this->itPattern->second == nullptr) {
                    throw std::out_of_range(
                        std::string("rule \"") + this->itPattern->first + std::string("\" is defined but empty"));
                }

                OptMatch
                    optm    = this->itPattern->second->Eval(istream, captures, usr_val);

                #ifdef CLASSY_PATTERNS_TRACE_GRAMMAR
                if (optm) {
                    io::cout
                        .put(this->itPattern->first)
                        .put(":\tmatched (");
                    optm->ExportData(istream, io::std_output);
                    io::cout
                        .put(")\n");
                }
                else {
                    io::cout
                        .put(this->itPattern->first)
                        .put(":\tfailed\n");
                }
                #endif
                return optm;
            }
        
        private:
            const_gramm_iter
                itPattern;
        };

        template<typename IterT>
        inline __impl::const_accessor<IterT>::operator Pattern() const && {
            return std::make_shared<patternGrammar>(this->itPattern);
        }

        inline __impl::grammar::operator Pattern() const {
            return std::make_shared<patternGrammar>(
                this->getPatternIter("__eval"));
        }

        class patternHandler :
            public pattern {
        public:
            using Callback  =
                std::function<void(io::IStream&, const OptMatch&, CaptureList&, const std::any&)>;

            template<typename Fn> requires
                std::constructible_from<Callback, Fn>
            patternHandler(const Pattern& ptHandle, Fn&& fnCallback) :
                ptHandle(ptHandle),
                fnCallback(std::forward<Fn>(fnCallback)) {}

            [[nodiscard]] Pattern
            Clone() const override {
                return std::make_shared<patternHandler>(*this);
            }

        protected:
            OptMatch
            normEval(io::IStream& istream, CaptureList& captures, const std::any& usr_val) override {
                OptMatch
                    optm    = this->ptHandle->Eval(istream, captures, usr_val);
                this->fnCallback(istream, optm, captures, usr_val);
                return optm;
            }

        private:
            Pattern
                ptHandle;
            Callback
                fnCallback;
        };

        // handler pattern; allows to insert code in between evaluation and observe the evaluation result of a pattern;
        // handler type signature: void (io::IStream&, const patt::OptMatch&, patt::CaptureList&, const std::any&)
        template<typename Fn> requires
            std::constructible_from<patternHandler::Callback, Fn>
        [[nodiscard]]
        inline Pattern
        operator/(const Pattern& ptHandle, Fn&& fnCallback) {
            return std::make_shared<patternHandler>(ptHandle, std::forward<Fn>(fnCallback));
        }

        class patternCLocale :
            public pattern {
        public:
            using LocaleProc    =
                int(*)(int);

            patternCLocale(LocaleProc lpfn) :
                lpfn(lpfn) {}

            [[nodiscard]] Pattern
            Clone() const override {
                return std::make_shared<patternCLocale>(*this);
            }

        protected:
            OptMatch
            normEval(io::IStream& istream, CaptureList&, const std::any&) override {
                intptr_t
                    iBegin  = istream.GetPosition();
                
                std::optional<std::byte>
                    optc    = istream.Read();
                if (!optc || !this->lpfn((int)*optc))
                    return std::nullopt;

                intptr_t
                    iEnd    = istream.GetPosition();
                return Match(iBegin, iEnd);
            }

        private:
            LocaleProc
                lpfn;
        };

        class patternCapture :
            public pattern {
        public:
            patternCapture(const Pattern& ptCapture) :
                ptCapture(ptCapture) {}

            [[nodiscard]] Pattern
            Clone() const override {
                return std::make_shared<patternCapture>(*this);
            }

        protected:
            OptMatch
            normEval(io::IStream& istream, CaptureList& captures, const std::any& usr_val) override {
                OptMatch
                    optm    = this->ptCapture->Eval(istream, captures, usr_val);
                if (optm)
                    captures.push_back(*optm);
                return optm;
            }    

        private:
            Pattern
                ptCapture;
        };

        class patternLookAhead :
            public pattern {
        public:
            patternLookAhead(const Pattern& ptLookAhead) :
                ptLookAhead(ptLookAhead) {}

            [[nodiscard]] Pattern
            Clone() const override {
                return std::make_shared<patternLookAhead>(*this);
            }

        protected:
            OptMatch
            normEval(io::IStream& istream, CaptureList& captures, const std::any& usr_val) override {
                intptr_t
                    iBegin  = istream.GetPosition();
            
                OptMatch
                    optm    = this->ptLookAhead->Eval(istream, captures, usr_val);
                istream.SetPosition(iBegin);

                return optm;
            }

        private:
            Pattern
                ptLookAhead;
        };

        // look ahead operator; evaluates a pattern and backtracks back
        [[nodiscard]]
        inline Pattern
        operator&(const Pattern& ptLookAhead) {
            return std::make_shared<patternLookAhead>(ptLookAhead);
        }
    }

    // matches an exact string of characters
    [[nodiscard]]
    inline Pattern
    Str(std::string_view strvPattern) {
        return std::make_shared<__impl::patternString>(strvPattern);
    }

    // matches if there is at least one character left to match
    [[nodiscard]]
    inline Pattern
    Any() {
        return std::make_shared<__impl::patternAny>();
    }

    // doesnt match if there is at least one character left to match
    [[nodiscard]]
    inline Pattern
    None() {
        return -Any();
    }

    // matches single character if it belongs to a set of characters
    [[nodiscard]]
    inline Pattern
    Set(std::string_view strvSet) {
        return std::make_shared<__impl::patternSet>(strvSet);
    }

    // matches single letter
    [[nodiscard]]
    inline Pattern
    Alpha() {
        return std::make_shared<__impl::patternCLocale>(isalpha);
    }

    // matches single letter or digit
    [[nodiscard]]
    inline Pattern
    AlphaNum() {
        return std::make_shared<__impl::patternCLocale>(isalnum);
    }

    // matches single decimal digit
    [[nodiscard]]
    inline Pattern
    Digit() {
        return std::make_shared<__impl::patternCLocale>(isdigit);
    }

    // matches single hexadecimal digit
    [[nodiscard]]
    inline Pattern
    HexDigit() {
        return std::make_shared<__impl::patternCLocale>(isxdigit);
    }

    // matches single lower case letter
    [[nodiscard]]
    inline Pattern
    LowerCase() {
        return std::make_shared<__impl::patternCLocale>(islower);
    }

    // matches single upper case letter
    [[nodiscard]]
    inline Pattern
    UpperCase() {
        return std::make_shared<__impl::patternCLocale>(isupper);
    }

    // matches single blank or new line character
    [[nodiscard]]
    inline Pattern
    SpaceOrNewLine() {
        return std::make_shared<__impl::patternCLocale>(isspace);
    }

    // matches single blank character
    [[nodiscard]]
    inline Pattern
    Space() {
        return std::make_shared<__impl::patternCLocale>(isblank);
    }

    // captures a successful match and appends it to the capture list
    [[nodiscard]]
    inline Pattern
    Capt(const Pattern& ptCapture) {
        return std::make_shared<__impl::patternCapture>(ptCapture);
    }
}