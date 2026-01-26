#pragma once
#include <any>
#include <map>
#include <memory>
#include <string>
#include <cassert>
#include <flat_set>
#include <optional>
#include <functional>

#include <classy-streams/IOStreams.hpp>
#include <string_view>

namespace patt {
    namespace __impl {
        class Pattern;
        class Grammar;
    }

    using Grammar   =
        __impl::Grammar;
    using Pattern   =
        std::shared_ptr<__impl::Pattern>;

    class Match {
    public:
        Match() :
            iBegin(0), iEnd(0) {}

        Match(intptr_t iBegin, intptr_t iEnd) :
            iBegin(iBegin), iEnd(iEnd) { assert(iEnd >= iBegin); }

        intptr_t
        Begin() const noexcept {
            return this->iBegin;
        }

        intptr_t
        End() const noexcept {
            return this->iEnd;
        }

        size_t
        Length() const noexcept {
            return (size_t)(this->iEnd - this->iBegin);
        }

        std::string
        GetString(io::IStream& is) const {
            intptr_t
                iCurr       = is.GetPosition();
            is.SetPosition(this->iBegin);
                
            std::string
                strMatch    = {};
            size_t
                uMatchLen   = this->Length();
            strMatch.reserve(uMatchLen);
            
            for (size_t i = 0; i != uMatchLen; ++i) {
                std::optional<std::byte>
                    optc    = is.Read();
                if (!optc)
                    break;
                strMatch.push_back(
                    (char)*optc);
            }

            is.SetPosition(iCurr);
            return strMatch;
        }

        size_t
        Forward(io::IStream& from, io::OStream& to) const {
            intptr_t
                iCurr       = from.GetPosition();
            from.SetPosition(this->iBegin);

            size_t
                uMatchLen   = this->Length(),
                uCharCount  = 0;
            while (uCharCount != uMatchLen) {
                std::optional<std::byte>
                    optc    = from.Read();
                if (!optc)
                    break;

                to.Write(*optc);
                uCharCount  += 1;
            }

            from.SetPosition(iCurr);
            return uCharCount;
        }

        auto&
        operator+=(const Match& match) noexcept {
            this->iEnd  =
                match.iEnd;
            return *this;
        }

    private:
        intptr_t
            iBegin, iEnd;
    };

    using OptMatch =
        std::optional<Match>;

    namespace __impl {
        class Pattern {
        public:
            Pattern(const Pattern&) = default;
            Pattern()               = default;
            virtual ~Pattern()      = default;

            OptMatch
            Eval(io::IStream& is, const std::any& usr_val = {}) const {
                return (this->bNegated)
                    ? this->negEval(is, usr_val)
                    : this->normEval(is, usr_val);
            };

            [[nodiscard]]
            virtual patt::Pattern
            Clone() const = 0;

            // pattern inversion
            [[nodiscard]]
            friend patt::Pattern
            operator-(patt::Pattern&& pattern) {
                pattern->bNegated =
                    !pattern->bNegated;
                return pattern;
            }
        
        protected:
            virtual OptMatch
            normEval(io::IStream& is, const std::any& usr_val) const = 0;

            virtual OptMatch
            negEval(io::IStream& is, const std::any& usr_val) const {
                intptr_t
                    iCurr   = is.GetPosition();
                auto
                    optMatch    = this->normEval(is, usr_val);
                is.SetPosition(iCurr);

                if (!optMatch)
                    return Match{ iCurr, iCurr };
                else
                    return std::nullopt;
            }

            bool
                bNegated    = false;
        };

        // pattern inversion
        [[nodiscard]]
        inline patt::Pattern
        operator-(const patt::Pattern& pattern) {
            return -(pattern->Clone());
        }

        class StringPattern :
            public Pattern {
        public:
            StringPattern(std::string_view strv) :
                str(strv) {}
            
            [[nodiscard]]
            patt::Pattern
            Clone() const override {
                return std::make_shared<StringPattern>(*this);
            }

        private:
            OptMatch
            normEval(io::IStream &is, const std::any&) const override {
                intptr_t
                    iBegin  = is.GetPosition();
        
                for (char c : this->str) {
                    auto optc = is.Read();
                    if (!optc || (char)*optc != c)
                        return std::nullopt;
                }
        
                intptr_t
                    iEnd    = is.GetPosition();
                return Match{ iBegin, iEnd };
            }

            std::string
                str;
        };

        class SetPattern :
            public __impl::Pattern {
        public:
            SetPattern(std::string_view strvSet) {
                for (char c : strvSet)
                    this->setChars.emplace(c);
            }

            [[nodiscard]]
            patt::Pattern
            Clone() const override {
                return std::make_shared<SetPattern>(*this);
            }

        private:
            OptMatch
            normEval(io::IStream& is, const std::any&) const override {
                intptr_t
                    iBegin  = is.GetPosition();
                auto optc   = is.Read();
                if (optc && this->setChars.contains((char)*optc)) {
                    intptr_t
                        iEnd    = is.GetPosition();
                    return Match{ iBegin, iEnd };
                }
                else
                    return std::nullopt;
            }

            std::flat_set<char>
                setChars;
        };

        class ConcatPattern :
            public __impl::Pattern {
        public:
            ConcatPattern(const patt::Pattern& lhs, const patt::Pattern& rhs) :
                lhs(lhs), rhs(rhs) {}

            [[nodiscard]]
            patt::Pattern
            Clone() const override {
                return std::make_shared<ConcatPattern>(*this);
            }
        
        private:
            OptMatch
            normEval(io::IStream& is, const std::any& usr_val) const override {
                auto
                    optLhs  = this->lhs->Eval(is, usr_val);
                if (!optLhs)
                    return std::nullopt;
            
                auto
                    optRhs  = this->rhs->Eval(is, usr_val);
                if (!optRhs)
                    return std::nullopt;

                return (*optLhs) += (*optRhs);
            }

            patt::Pattern
                lhs, rhs;
        };

        // lhs followed by rhs
        [[nodiscard]] inline patt::Pattern
        operator>>(const patt::Pattern& lhs, const patt::Pattern& rhs) {
            return std::make_shared<ConcatPattern>(lhs, rhs);
        }

        // lhs excluding rhs
        [[nodiscard]] inline patt::Pattern
        operator-(const patt::Pattern& lhs, const patt::Pattern& rhs) {
            return -rhs >> lhs;
        }

        // lhs excluding rhs
        [[nodiscard]] inline patt::Pattern
        operator-(const patt::Pattern& lhs, patt::Pattern&& rhs) {
            return -std::move(rhs) >> lhs;
        }

        class AnyPattern :
            public Pattern {
        public:
            AnyPattern() = default;

            [[nodiscard]]
            patt::Pattern
            Clone() const override {
                return std::make_shared<AnyPattern>(*this);
            }
        
        private:
            OptMatch
            normEval(io::IStream& is, const std::any&) const override {
                intptr_t
                    iBegin  = is.GetPosition();
                
                if (!is.Read())
                    return std::nullopt;
                
                intptr_t
                    iEnd    = is.GetPosition();
                return Match{ iBegin, iEnd };
            }
        };

        class ChoicePattern :
            public  Pattern {
        public:
            ChoicePattern(const patt::Pattern& lhs, const patt::Pattern& rhs) :
                lhs(lhs), rhs(rhs) {}
        
            [[nodiscard]]
            patt::Pattern
            Clone() const override {
                return std::make_shared<ChoicePattern>(*this);
            }
        
        private:
            OptMatch
            normEval(io::IStream& is, const std::any& usr_val) const override {
                intptr_t
                    iBegin      = is.GetPosition();
                OptMatch
                    optMatch    = this->lhs->Eval(is, usr_val);
                
                if (optMatch) {
                    intptr_t
                        iEnd    = is.GetPosition();
                    return Match{ iBegin, iEnd };
                }
                else
                    is.SetPosition(iBegin);
                
                optMatch        = this->rhs->Eval(is, usr_val);
                if (optMatch) {
                    intptr_t
                        iEnd    = is.GetPosition();
                    return Match{ iBegin, iEnd };
                }
                else {
                    is.SetPosition(iBegin);
                    return std::nullopt;
                }
            }

            patt::Pattern
                lhs, rhs;
        };

        // ordered choice
        [[nodiscard]] inline patt::Pattern
        operator|=(const patt::Pattern& lhs, const patt::Pattern& rhs) {
            return std::make_shared<ChoicePattern>(lhs, rhs);
        }

        class HandlerPattern :
            public Pattern {
        public:
            using Callback =
                std::function<void(io::IStream&, const OptMatch&, const std::any&)>;

            template<typename Fn> requires
                std::is_constructible_v<Callback, Fn>
            HandlerPattern(const patt::Pattern& pattern, Fn&& fnCallback) :
                pattern     (pattern),
                fnCallback  (std::forward<Fn>(fnCallback)) {}
        
            [[nodiscard]]
            patt::Pattern
            Clone() const override {
                return std::make_shared<HandlerPattern>(*this);
            }
        
        private:
            OptMatch
            normEval(io::IStream& is, const std::any& usr_val) const override {
                OptMatch
                    optMatch    = this->pattern->Eval(is, usr_val);
                this->fnCallback(is, optMatch, usr_val);
                return optMatch;
            }
        
            patt::Pattern
                pattern;
            Callback
                fnCallback;
        };

        // wrap pattern with "on pattern evaluation" event handler
        template<typename Fn> requires
            std::is_constructible_v<HandlerPattern::Callback, Fn>
        [[nodiscard]]
        inline patt::Pattern
        operator/(const patt::Pattern& pattern, Fn&& fn) {
            return std::make_shared<HandlerPattern>(
                pattern, fn);
        }

        class LocalePattern :
            public Pattern {
        public:
            using LocaleProc =
                int (*)(int);

            LocalePattern(LocaleProc lpfn) :
                lpfn(lpfn) {}

            [[nodiscard]]
            patt::Pattern
            Clone() const override {
                return std::make_shared<LocalePattern>(*this);
            }
        
        private:
            OptMatch
            normEval(io::IStream& is, const std::any&) const override {
                intptr_t
                    iBegin  = is.GetPosition();
                
                auto
                    optc    = is.Read();
                if (!optc || !this->lpfn((int)*optc))
                    return std::nullopt;

                intptr_t
                    iEnd    = is.GetPosition();
                return Match{ iBegin, iEnd };
            }

            LocaleProc
                lpfn;
        };

        class RepeatPattern :
            public Pattern {
        public:
            RepeatPattern(const patt::Pattern& pattern, size_t uCount) :
                pattern(pattern), uCount(uCount) {}

            [[nodiscard]]
            patt::Pattern
            Clone() const override {
                return std::make_shared<RepeatPattern>(*this);
            }

        private:
            OptMatch
            normEval(io::IStream& is, const std::any& usr_val) const override {
                intptr_t
                    iBegin  = is.GetPosition();
                for (size_t i = 0; i != this->uCount; ++i) {
                    if (!this->pattern->Eval(is, usr_val))
                        return std::nullopt;
                }

                intptr_t
                    iCurr   = is.GetPosition();
                while (true) {
                    if (!this->pattern->Eval(is, usr_val)) {
                        is.SetPosition(iCurr);
                        break;
                    }

                    iCurr   = is.GetPosition();
                }

                return Match{ iBegin, iCurr };
            }

            OptMatch
            negEval(io::IStream& is, const std::any& usr_val) const override {
                intptr_t
                    iBegin  = is.GetPosition();
                for (size_t i = 0; i != this->uCount; ++i) {
                    intptr_t
                        iCurr   = is.GetPosition();
                    if (!this->pattern->Eval(is, usr_val)) {
                        is.SetPosition(iCurr);
                        return Match{ iBegin, iCurr };
                    }
                }

                intptr_t
                    iEnd    = is.GetPosition();
                return Match{ iBegin, iEnd };
            }
        
            patt::Pattern
                pattern;
            size_t
                uCount;
        };

        // repeat pattern:
        //  if rhs is positive, repeat N+ times;
        //  if rhs is negative, repeat 0..N times.
        [[nodiscard]]
        inline patt::Pattern
        operator%(const patt::Pattern& lhs, ssize_t rhs) {
            patt::Pattern
                pattern = std::make_shared<RepeatPattern>(lhs, (size_t)std::abs(rhs));
            if (rhs < 0)
                pattern = -pattern;
            return pattern;
        }

        class RepeatExactPattern :
            public Pattern {
        public:
            RepeatExactPattern(const patt::Pattern& pattern, size_t uCount) :
                pattern(pattern), uCount(uCount) {}

            [[nodiscard]]
            patt::Pattern
            Clone() const override {
                return std::make_shared<RepeatExactPattern>(*this);
            }
        
        private:
            OptMatch
            normEval(io::IStream& is, const std::any& usr_val) const override {
                intptr_t
                    iBegin  = is.GetPosition();
                for (size_t i = 0; i != this->uCount; ++i) {
                    if (!this->pattern->Eval(is, usr_val))
                        return std::nullopt;
                }

                intptr_t
                    iEnd    = is.GetPosition();
                return Match{ iBegin, iEnd };
            }

            patt::Pattern
                pattern;
            size_t
                uCount;
        };

        // repeat pattern exactly N times
        [[nodiscard]]
        inline patt::Pattern
        operator*(const patt::Pattern& lhs, size_t rhs) {
            return std::make_shared<RepeatExactPattern>(lhs, rhs);
        }

        using MapPatterns =
            std::map<std::string, patt::Pattern>;

        class Grammar {
        private:
            class Accessor {
            public:
                Accessor(MapPatterns& mapPatterns, const std::string& strKey) {
                    auto
                        it  = mapPatterns.find(strKey);
                    if (it == mapPatterns.end())
                        it  = mapPatterns.emplace(strKey, nullptr).first;
                    this->itPattern = it;
                }

                [[nodiscard]]
                operator patt::Pattern() &&;

                patt::Pattern&
                operator=(const patt::Pattern& pattern) && {
                    return this->itPattern->second = pattern;
                }
            private:
                MapPatterns::iterator
                    itPattern;
            };
            
        public:
            Grammar() = default;
            Grammar(const Grammar& obj) = default;
            Grammar(Grammar&& obj) noexcept = default;

            auto&
            operator=(const Grammar& obj) {
                Grammar
                    temp    = obj;
                this->mapPatterns.swap(
                    temp.mapPatterns);
                return *this;
            }

            auto&
            operator=(Grammar&& obj) noexcept {
                Grammar
                    temp    = std::move(obj);
                this->mapPatterns.swap(
                    temp.mapPatterns);
                return *this;
            }

            Accessor
            operator[](const std::string& strKey) {
                return Accessor(this->mapPatterns, strKey);
            }

        private:
            MapPatterns
                mapPatterns;
        };

        class GrammarPattern :
            public Pattern {
        public:
            GrammarPattern(MapPatterns::iterator itPattern) :
                itPattern(itPattern) {}

            [[nodiscard]]
            patt::Pattern
            Clone() const override {
                return std::make_shared<GrammarPattern>(*this);
            }

        private:
            OptMatch
            normEval(io::IStream& is, const std::any& usr_val) const override {
                const patt::Pattern&
                    pattern = this->itPattern->second;
                if (pattern == nullptr)
                    return std::nullopt;
                return pattern->Eval(is, usr_val);
            }

            MapPatterns::iterator
                itPattern = {};
        };

        inline Grammar::Accessor::operator patt::Pattern() && {
            return std::make_shared<GrammarPattern>(
                this->itPattern);
        }

        class ForwardPattern :
            public Pattern {
        public:
            ForwardPattern(const patt::Pattern& pattern, io::OStream& refStream) :
                pattern(pattern), refStream(refStream) {}

            patt::Pattern
            Clone() const override {
                return std::make_shared<ForwardPattern>(*this);
            }
            
        private:
            OptMatch
            normEval(io::IStream& is, const std::any& usr_val) const override {
                auto
                    optMatch    = this->pattern->Eval(is, usr_val);
                if (optMatch)
                    optMatch->Forward(is, this->refStream);
                return optMatch;
            }

            patt::Pattern
                pattern;
            io::OStream&
                refStream;
        };

        [[nodiscard]]
        inline patt::Pattern
        operator/(const patt::Pattern& pattern, io::OStream& refStream) {
            return std::make_shared<ForwardPattern>(pattern, refStream);
        }

        class LookAheadPattern :
            public Pattern {
        public:
            LookAheadPattern(const patt::Pattern& pattern) :
                pattern(pattern) {}

            patt::Pattern
            Clone() const override {
                return std::make_shared<LookAheadPattern>(*this);
            }

        private:
            OptMatch
            normEval(io::IStream& is, const std::any& usr_val) const override {
                intptr_t
                    iCurr       = is.GetPosition();
                auto
                    optMatch    = this->pattern->Eval(is, usr_val);
                is.SetPosition(iCurr);
                
                return optMatch;
            }

            patt::Pattern
                pattern;
        };

        inline patt::Pattern
        operator&(const patt::Pattern& pattern) {
            return std::make_shared<LookAheadPattern>(pattern);
        }
    }

    [[nodiscard]]
    inline Pattern
    Str(std::string_view strv) {
        return std::make_shared<__impl::StringPattern>(strv);
    }

    [[nodiscard]]
    inline Pattern
    Any() {
        return std::make_shared<__impl::AnyPattern>();
    }

    [[nodiscard]]
    inline Pattern
    None() {
        return -Any();
    }

    [[nodiscard]]
    inline Pattern
    Alpha() {
        return std::make_shared<__impl::LocalePattern>(isalpha);
    }

    [[nodiscard]]
    inline Pattern
    Alnum() {
        return std::make_shared<__impl::LocalePattern>(isalnum);
    }

    [[nodiscard]]
    inline Pattern
    Digit() {
        return std::make_shared<__impl::LocalePattern>(isdigit);
    }

    [[nodiscard]]
    inline Pattern
    HexDigit() {
        return std::make_shared<__impl::LocalePattern>(isxdigit);
    }

    [[nodiscard]]
    inline Pattern
    LowerCase() {
        return std::make_shared<__impl::LocalePattern>(islower);
    }

    [[nodiscard]]
    inline Pattern
    UpperCase() {
        return std::make_shared<__impl::LocalePattern>(isupper);
    }

    [[nodiscard]]
    inline Pattern
    SpaceOrNewLine() {
        return std::make_shared<__impl::LocalePattern>(isspace);
    }

    [[nodiscard]]
    inline Pattern
    Blank() {
        return std::make_shared<__impl::LocalePattern>(isblank);
    }
    
    [[nodiscard]]
    inline Pattern
    Set(std::string_view strvSet) {
        return std::make_shared<__impl::SetPattern>(strvSet);
    }

    [[nodiscard]]
    inline Pattern
    Space() {
        return Set(" \t\v");
    }

    [[nodiscard]]
    inline OptMatch
    Eval(const Pattern& p, io::IStream& is, const std::any& usr_val = {}) {
        return p->Eval(is, usr_val);
    }
}
