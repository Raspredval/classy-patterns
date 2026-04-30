#include "Pattern.hpp"

namespace patt {
    namespace __impl {
        OptMatch
        pattern::Eval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) {
            return (this->bNegated)
                ? this->negEval(istream, groups, usr_val)
                : this->normEval(istream, groups, usr_val);
        }

        Pattern
        operator-(Pattern&& pt) noexcept {
            pt->bNegated = !pt->bNegated;
            return pt;
        }

        Pattern
        operator-(const Pattern& pt) noexcept {
            return -pt->Clone();
        }

        OptMatch
        pattern::negEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) {
            intptr_t
                iCurPos = istream.GetPosition();
            OptMatch
                optm    = this->normEval(istream, groups, usr_val);
            if (!optm) {
                intptr_t
                    iEndPos = istream.GetPosition();
                return Match(iCurPos, iEndPos);
            }
            else
                return std::nullopt;
        }

        patternsList::patternsList(const Pattern& ptRoot) :
            ptRoot(ptRoot),
            ptLast(ptRoot) {}

        void
        patternsList::append(const Pattern& pt) noexcept {
            this->ptLast = (this->ptLast->ptNext = pt);
        }

        Pattern
        patternsList::first() const noexcept {
            return this->ptRoot;
        }

        Pattern
        patternsList::last() const noexcept {
            return this->ptRoot;
        }
    }

    extern OptMatch
    Eval(io::IStream& istream, const Pattern& pt, CaptureGroupList& groups, const std::any& usr_val) {
        return pt->Eval(istream, groups, usr_val);
    }

    extern OptMatch
    Eval(io::IStream& istream, const Pattern& pt, const std::any& usr_val) {
        CaptureGroupList
            groups = { CaptureGroup{} };
        return Eval(istream, pt, groups, usr_val);
    }
}