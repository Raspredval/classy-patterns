#include "Patterns.hpp"

#ifdef CLASSY_PATTERNS_TRACE_GRAMMAR
#include <classy-streams/ConsoleStreams.hpp>
#endif

namespace patt {
    namespace __impl {
        patternJoin::patternJoin(const Pattern& ptA, const Pattern& ptB) :
            lst(ptA)
        {
            this->lst.append(ptB);
        }

        void
        patternJoin::Append(const Pattern& pt) {
            this->lst.append(pt);
        }

        void
        patternJoin::Append(JoinPattern&& pt) {
            this->lst.append(std::move(pt->lst));
        }

        void
        patternJoin::Prepend(Pattern&& pt) {
            this->lst.prepend(std::move(pt));
        }

        Pattern
        patternJoin::Clone() const {
            return std::make_shared<patternJoin>(*this);
        }

        Match
        patternJoin::normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) {
            intptr_t
                iBegin  = istream.GetPosition();
            Pattern
                ptCur   = lst.first();
            Match
                mCur    = Match(iBegin, iBegin);
            while (ptCur != nullptr) {
                mCur    += ptCur->Eval(istream, groups, usr_val);
                if (!mCur)
                    break;
                ptCur   = ptCur->ptNext;
            }

            return mCur;
        }

        extern JoinPattern
        operator>>(JoinPattern&& ptA, const Pattern& ptB) {
            ptA->Append(ptB);
            return ptA;
        }

        extern JoinPattern
        operator>>(Pattern&& ptA, JoinPattern&& ptB) {
            ptB->Prepend(std::move(ptA));
            return ptB;
        }

        extern JoinPattern
        operator>>(JoinPattern&& ptA, JoinPattern&& ptB) {
            ptA->Append(std::move(ptB));
            return ptA;
        }

        extern JoinPattern
        operator>>(const JoinPattern& ptA, const Pattern& ptB) {
            return std::make_shared<__impl::patternJoin>(*ptA) >> ptB;
        }

        extern JoinPattern
        operator>>(const Pattern& ptA, const Pattern& ptB) {
            return std::make_shared<__impl::patternJoin>(ptA, ptB);
        }

        patternString::patternString(std::string_view strvPattern) :
            strPattern(strvPattern) {}

        Pattern
        patternString::Clone() const {
            return std::make_shared<patternString>(*this);
        }

        Match
        patternString::normEval(io::IStream& istream, CaptureGroupList&, const std::any&) {
            intptr_t
                iBegin  = istream.GetPosition();
            for (size_t i = 0; i != this->strPattern.size(); ++i) {
                std::optional<std::byte>
                    optc    = istream.Read();
                if (!optc || (char)*optc != strPattern[i])
                    return Match(iBegin, iBegin + (intptr_t)(i + 1), true);
            }

            return Match(iBegin, iBegin + (intptr_t)strPattern.size());
        }

        Pattern
        patternAny::Clone() const {
            return std::make_shared<patternAny>(*this);
        }

        Match
        patternAny::normEval(io::IStream& istream, CaptureGroupList&, const std::any&) {
            intptr_t
                iBegin  = istream.GetPosition();
            std::optional<std::byte>
                optc    = istream.Read();
            if (!optc)
                return Match(iBegin, iBegin, true);
            return Match(iBegin, iBegin + 1);
        }

        patternChoice::patternChoice(const Pattern& ptA, const Pattern& ptB) :
            lst(ptA)
        {
            this->lst.append(ptB);
        }

        Pattern
        patternChoice::Clone() const {
            return std::make_shared<patternChoice>(*this);
        }

        void
        patternChoice::Append(const Pattern& pt) {
            this->lst.append(pt);
        }

        Match
        patternChoice::normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) {
            intptr_t
                iBegin      = istream.GetPosition();
            Pattern
                ptCur       = this->lst.first();
            size_t
                uCaptCount  = (!groups.empty())
                                ? groups.back().size() : 0;
            while (ptCur != nullptr) {
                Match
                    optm    = ptCur->Eval(istream, groups, usr_val);
                if (optm)
                    return optm;

                ptCur       = ptCur->ptNext;
                istream.SetPosition(iBegin);

                if (!groups.empty()) {
                    while (groups.back().size() > uCaptCount)
                        groups.back().pop_back();
                }
            }

            return Match(iBegin, iBegin, true);
        }

        extern ChoicePattern
        operator|(ChoicePattern&& ptA, const Pattern& ptB)  {
            ptA->Append(ptB);
            return ptA;
        }

        extern ChoicePattern
        operator|(const ChoicePattern& ptA, const Pattern& ptB) {
            return std::make_shared<__impl::patternChoice>(*ptA) | ptB;
        }

        extern ChoicePattern
        operator|(const Pattern& ptA, const Pattern& ptB) {
            return std::make_shared<__impl::patternChoice>(ptA, ptB);
        }

        patternSet::patternSet(std::string_view strvSet) {
            for (char c : strvSet)
                this->set.emplace(c);
        }

        Match
        patternSet::normEval(io::IStream& istream, CaptureGroupList&, const std::any&) {
            intptr_t
                iBegin  = istream.GetPosition();
            std::optional<std::byte>
                optc    = istream.Read();

            if (!optc)
                return Match(iBegin, iBegin, true);
            if (!this->set.contains((char)*optc))
                return Match(iBegin, iBegin + 1, true);

            return Match(iBegin, iBegin + 1);
        }

        patternRepeat::patternRepeat(const Pattern& ptRepeat, ssize_t iCount) :
            ptRepeat(ptRepeat),
            uCount((size_t)std::abs(iCount))
        {
            if (iCount < 0) {
                this->bNegated  = !this->bNegated;
            }
        }

        Pattern
        patternRepeat::Clone() const {
            return std::make_shared<patternRepeat>(*this);
        }

        Match
        patternRepeat::normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) {
            intptr_t
                iBegin  = istream.GetPosition();
            Match
                mCur    = Match(iBegin, iBegin);
            for (size_t i = 0; i != this->uCount; ++i) {
                mCur    += this->ptRepeat->Eval(istream, groups, usr_val);
                if (!mCur)
                    return mCur;
            }

            while (true) {
                Match
                    mNext   = this->ptRepeat->Eval(istream, groups, usr_val);
                if (!mNext) {
                    istream.SetPosition(mCur.End());
                    break;
                }

                mCur        += mNext;
            }

            return mCur;
        }

        Match
        patternRepeat::negEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) {
            intptr_t
                iBegin  = istream.GetPosition();
            Match
                mCur    = Match(iBegin, iBegin);
            for (size_t i = 0; i != this->uCount; ++i) {
                Match
                    mNext   = this->ptRepeat->Eval(istream, groups, usr_val);
                if (!mNext) {
                    istream.SetPosition(mCur.End());
                    return mCur;
                }
                mCur        += mNext;
            }

            return mCur;
        }

        extern Pattern
        operator%(const patt::Pattern& ptA, ssize_t iCount) {
            return std::make_shared<patternRepeat>(ptA, iCount);
        }

        patternRepeatExact::patternRepeatExact(const Pattern& ptRepeat, size_t uCount) :
            ptRepeat(ptRepeat),
            uCount(uCount) {}

        Pattern
        patternRepeatExact::Clone() const {
            return std::make_shared<patternRepeatExact>(*this);
        }

        Match
        patternRepeatExact::normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) {
            intptr_t
                iBegin  = istream.GetPosition();
            Match
                mCur    = Match(iBegin, iBegin);
            for (size_t i = 0; i != this->uCount; ++i) {
                mCur    += this->ptRepeat->Eval(istream, groups, usr_val);
                if (!mCur)
                    break;
            }

            return mCur;
        }

        extern Pattern
        operator*(const patt::Pattern& ptA, size_t uCount) {
            return std::make_shared<patternRepeatExact>(ptA, uCount);
        }

        patternGrammar::patternGrammar(const_gramm_iter itPattern) :
            itPattern(itPattern) {}

        Pattern
        patternGrammar::Clone() const {
            return std::make_shared<patternGrammar>(*this);
        }

        Match
        patternGrammar::normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) {
            #ifdef CLASSY_PATTERNS_TRACE_GRAMMAR
            const char*
                szEnvVar        = std::getenv("TRACE_GRAMMAR");
            bool
                bTraceGrammar   = std::string_view{ (szEnvVar == nullptr) ? "" : szEnvVar } == "true";
            if (bTraceGrammar) {
                io::cout.fmt("{}:\tbegin\n", this->itPattern->first);
            }
            #endif

            if (this->itPattern->second == nullptr) {
                throw std::out_of_range(
                    std::string("rule \"") + this->itPattern->first + std::string("\" is defined but empty"));
            }

            Match
                mCur    = this->itPattern->second->Eval(istream, groups, usr_val);

            #ifdef CLASSY_PATTERNS_TRACE_GRAMMAR
            if (bTraceGrammar) {
                if (mCur) {
                    io::cout
                        .put(this->itPattern->first)
                        .put(":\tmatched (");
                    mCur.ExportData(istream, io::std_output);
                    io::cout
                        .put(")\n");
                }
                else {
                    io::cout
                        .put(this->itPattern->first)
                        .put(":\tfailed\n");
                }
            }
            #endif

            return mCur;
        }

        template<>
        const_accessor<const_gramm_iter>::operator Pattern() const && {
            return std::make_shared<patternGrammar>(this->itPattern);
        }

        template<>
        const_accessor<gramm_iter>::operator Pattern() const && {
            return std::make_shared<patternGrammar>(this->itPattern);
        }

        Pattern
        grammar::GetDefaultRule() const {
            return std::make_shared<patternGrammar>(
                this->getPatternIter("__eval"));
        }

        patternHandler::patternHandler(const Pattern& ptHandle, Callback lpfnCallback) :
            ptHandle(ptHandle),
            lpfnCallback(lpfnCallback)
        {
            if (lpfnCallback == nullptr)
                throw std::invalid_argument("invalid callback function pointer");
        }

        Pattern
        patternHandler::Clone() const {
            return std::make_shared<patternHandler>(*this);
        }

        Match
        patternHandler::normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) {
            Match
                optm    = this->ptHandle->Eval(istream, groups, usr_val);
            this->lpfnCallback(istream, optm, groups, usr_val);
            return optm;
        }

        extern Pattern
        operator/(const Pattern& ptHandle, Callback lpfnCallback) {
            return std::make_shared<patternHandler>(ptHandle, lpfnCallback);
        }

        patternCLocale::patternCLocale(LocaleProc lpfn) :
            lpfn(lpfn) {}

        Pattern
        patternCLocale::Clone() const {
            return std::make_shared<patternCLocale>(*this);
        }

        Match
        patternCLocale::normEval(io::IStream& istream, CaptureGroupList&, const std::any&) {
            intptr_t
                iBegin  = istream.GetPosition();

            std::optional<std::byte>
                optc    = istream.Read();
            if (!optc)
                return Match(iBegin, iBegin, true);
            if (!this->lpfn((int)*optc))
                return Match(iBegin, iBegin + 1, true);

            return Match(iBegin, iBegin + 1);
        }

        patternCapture::patternCapture(const Pattern& ptCapture) :
            ptCapture(ptCapture) {}

        Pattern
        patternCapture::Clone() const {
            return std::make_shared<patternCapture>(*this);
        }

        Match
        patternCapture::normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) {
            Match
                mCur    = this->ptCapture->Eval(istream, groups, usr_val);
            if (mCur) {
                if (groups.empty())
                    groups.emplace_back();
                groups.back().push_back(mCur);
            }
            return mCur;
        }

        patternCaptureGroup::patternCaptureGroup(const Pattern& ptCaptureFrom) :
            ptCaptureFrom(ptCaptureFrom) {}

        Pattern
        patternCaptureGroup::Clone() const {
            return std::make_shared<patternCaptureGroup>(*this);
        }

        Match
        patternCaptureGroup::normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) {
            groups.emplace_back();
            Match
                mCur    = this->ptCaptureFrom->Eval(istream, groups, usr_val);
            groups.pop_back();
            return mCur;
        }

        patternLookAhead::patternLookAhead(const Pattern& ptLookAhead) :
            ptLookAhead(ptLookAhead) {}

        Pattern
        patternLookAhead::Clone() const {
            return std::make_shared<patternLookAhead>(*this);
        }

        Match
        patternLookAhead::normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) {
            size_t
                uCaptCount  = (!groups.empty())
                                ? groups.back().size() : 0;
            Match
                mCur        = this->ptLookAhead->Eval(istream, groups, usr_val);
            istream.SetPosition(mCur.Begin());
            if (!groups.empty()) {
                while (groups.back().size() > uCaptCount)
                    groups.back().pop_back();
            }
            return mCur;
        }

        extern Pattern
        operator&(const Pattern& ptLookAhead) {
            return std::make_shared<patternLookAhead>(ptLookAhead);
        }
    }

    extern Pattern
    Str(std::string_view strvPattern) {
        return std::make_shared<__impl::patternString>(strvPattern);
    }

    extern Pattern
    Any() {
        return std::make_shared<__impl::patternAny>();
    }

    extern Pattern
    None() {
        return -Any();
    }

    extern Pattern
    Set(std::string_view strvSet) {
        return std::make_shared<__impl::patternSet>(strvSet);
    }

    extern Pattern
    Alpha() {
        return std::make_shared<__impl::patternCLocale>(isalpha);
    }

    extern Pattern
    AlphaNum() {
        return std::make_shared<__impl::patternCLocale>(isalnum);
    }

    extern Pattern
    Digit() {
        return std::make_shared<__impl::patternCLocale>(isdigit);
    }

    extern Pattern
    HexDigit() {
        return std::make_shared<__impl::patternCLocale>(isxdigit);
    }

    extern Pattern
    LowerCase() {
        return std::make_shared<__impl::patternCLocale>(islower);
    }

    extern Pattern
    UpperCase() {
        return std::make_shared<__impl::patternCLocale>(isupper);
    }

    extern Pattern
    SpaceOrNewLine() {
        return std::make_shared<__impl::patternCLocale>(isspace);
    }

    extern Pattern
    Space() {
        return std::make_shared<__impl::patternCLocale>(isblank);
    }

    extern Pattern
    Capt(const Pattern& ptCapture) {
        return std::make_shared<__impl::patternCapture>(ptCapture);
    }

    extern Pattern
    CaptGr(const Pattern& ptCaptureFrom) {
        return std::make_shared<__impl::patternCaptureGroup>(ptCaptureFrom);
    }
}