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

        OptMatch
        patternJoin::normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) {
            Pattern
                ptCur   = lst.first();
            intptr_t
                iBegin  = istream.GetPosition();
            OptMatch
                optmRet = Match(iBegin, iBegin);
            while (ptCur != nullptr) {
                OptMatch
                    optmCur = ptCur->Eval(istream, groups, usr_val);
                if (!optmCur)
                    return std::nullopt;

                *optmRet    += *optmCur;
                ptCur       = ptCur->ptNext;
            }

            return optmRet;
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

        OptMatch
        patternString::normEval(io::IStream& istream, CaptureGroupList&, const std::any&) {
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

        Pattern
        patternAny::Clone() const {
            return std::make_shared<patternAny>(*this);
        }

        OptMatch
        patternAny::normEval(io::IStream& istream, CaptureGroupList&, const std::any&) {
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

        OptMatch
        patternChoice::normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) {
            intptr_t
                iBegin  = istream.GetPosition();
            Pattern
                ptCur   = this->lst.first();
            while (ptCur) {
                OptMatch
                    optm    = ptCur->Eval(istream, groups, usr_val);
                if (optm)
                    return optm;

                ptCur       = ptCur->ptNext;
                istream.SetPosition(iBegin);
            }

            return std::nullopt;
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

        OptMatch
        patternSet::normEval(io::IStream& istream, CaptureGroupList&, const std::any&) {
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

        OptMatch
        patternRepeat::normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) {
            intptr_t
                iBegin  = istream.GetPosition();
            for (size_t i = 0; i != this->uCount; ++i) {
                if (!this->ptRepeat->Eval(istream, groups, usr_val))
                    return std::nullopt;
            }

            intptr_t
                iEnd    = istream.GetPosition();
            while (true) {
                if (!this->ptRepeat->Eval(istream, groups, usr_val)) {
                    istream.SetPosition(iEnd);
                    break;
                }

                iEnd        = istream.GetPosition();
            }

            return Match(iBegin, iEnd);
        }

        OptMatch
        patternRepeat::negEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) {
            intptr_t
                iBegin  = istream.GetPosition();
            for (size_t i = 0; i != this->uCount; ++i) {
                intptr_t
                    iEnd    = istream.GetPosition();
                if (!this->ptRepeat->Eval(istream, groups, usr_val)) {
                    istream.SetPosition(iEnd);
                    return Match(iBegin, iEnd);
                }
            }

            intptr_t
                iEnd    = istream.GetPosition();
            return Match(iBegin, iEnd);
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

        OptMatch
        patternRepeatExact::normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) {
            intptr_t
                iBegin  = istream.GetPosition();
            for (size_t i = 0; i != this->uCount; ++i) {
                if (!this->ptRepeat->Eval(istream, groups, usr_val))
                    return std::nullopt;
            }
            intptr_t
                iEnd    = istream.GetPosition();
            return Match(iBegin, iEnd);
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

        OptMatch
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

            OptMatch
                optm    = this->itPattern->second->Eval(istream, groups, usr_val);

            #ifdef CLASSY_PATTERNS_TRACE_GRAMMAR
            if (bTraceGrammar) {
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
            }
            #endif

            return optm;
        }

        template<>
        const_accessor<const_gramm_iter>::operator Pattern() const && {
            return std::make_shared<patternGrammar>(this->itPattern);
        }

        template<>
        const_accessor<gramm_iter>::operator Pattern() const && {
            return std::make_shared<patternGrammar>(this->itPattern);
        }

        grammar::operator Pattern() const {
            return std::make_shared<patternGrammar>(
                this->getPatternIter("__eval"));
        }

        patternHandler::patternHandler(const Pattern& ptHandle, Callback fnCallback) :
            ptHandle(ptHandle),
            fnCallback(fnCallback)
        {
            if (fnCallback == nullptr)
                throw std::invalid_argument("invalid callback function pointer");
        }

        Pattern
        patternHandler::Clone() const {
            return std::make_shared<patternHandler>(*this);
        }

        OptMatch
        patternHandler::normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) {
            OptMatch
                optm    = this->ptHandle->Eval(istream, groups, usr_val);
            this->fnCallback(istream, optm, groups, usr_val);
            return optm;
        }

        extern Pattern
        operator/(const Pattern& ptHandle, const Callback& fnCallback) {
            return std::make_shared<patternHandler>(ptHandle, fnCallback);
        }

        extern Pattern
        operator/(const Pattern& ptHandle, Callback&& fnCallback) {
            return std::make_shared<patternHandler>(ptHandle, std::move(fnCallback));
        }

        patternCLocale::patternCLocale(LocaleProc lpfn) :
            lpfn(lpfn) {}

        Pattern
        patternCLocale::Clone() const {
            return std::make_shared<patternCLocale>(*this);
        }

        OptMatch
        patternCLocale::normEval(io::IStream& istream, CaptureGroupList&, const std::any&) {
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

        patternCapture::patternCapture(const Pattern& ptCapture) :
            ptCapture(ptCapture) {}

        Pattern
        patternCapture::Clone() const {
            return std::make_shared<patternCapture>(*this);
        }

        OptMatch
        patternCapture::normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) {
            OptMatch
                optm    = this->ptCapture->Eval(istream, groups, usr_val);
            if (optm) {
                if (groups.empty())
                    groups.emplace_back();
                groups.back().push_back(*optm);
            }
            return optm;
        }

        patternCaptureManip::patternCaptureManip(CaptureManip iCmd) :
            iCmd(iCmd) {};

        Pattern
        patternCaptureManip::Clone() const {
            return std::make_shared<patternCaptureManip>(*this);
        }

        OptMatch
        patternCaptureManip::normEval(io::IStream& istream, CaptureGroupList& groups, const std::any&) {
            intptr_t
                iCurPos = istream.GetPosition();

            switch (this->iCmd) {
            case CaptureManip::PopGroup:
                if (!groups.empty())
                    groups.pop_back();
                break;

            case CaptureManip::PushGroup:
                groups.emplace_back();
                break;

            default:
                throw std::runtime_error(
                    "invalid capture manip cmd");
            }

            return Match(iCurPos, iCurPos);
        }

        patternLookAhead::patternLookAhead(const Pattern& ptLookAhead) :
            ptLookAhead(ptLookAhead) {}

        Pattern
        patternLookAhead::Clone() const {
            return std::make_shared<patternLookAhead>(*this);
        }

        OptMatch
        patternLookAhead::normEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) {
            intptr_t
                iBegin  = istream.GetPosition();

            OptMatch
                optm    = this->ptLookAhead->Eval(istream, groups, usr_val);
            istream.SetPosition(iBegin);

            return optm;
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
    PushCaptGr() {
        return std::make_shared<__impl::patternCaptureManip>(
            __impl::CaptureManip::PushGroup);
    }

    extern Pattern
    PopCaptGr() {
        return std::make_shared<__impl::patternCaptureManip>(
            __impl::CaptureManip::PopGroup);
    }

    extern __impl::JoinPattern
    CaptGr(const Pattern& ptCaptureFrom) {
        return PushCaptGr() >> ptCaptureFrom >> PopCaptGr();
    }
}