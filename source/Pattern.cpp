#include "Pattern.hpp"

namespace patt {
    namespace __impl {
        Match
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

        Match
        pattern::negEval(io::IStream& istream, CaptureGroupList& groups, const std::any& usr_val) {
            Match
                m   = this->normEval(istream, groups, usr_val);
            m.ToggleFailed();
            return m;
        }

        patternsList::patternsList(const Pattern& ptRoot) :
            ptRoot(ptRoot),
            ptLast(ptRoot) {}

        void
        patternsList::append(const Pattern& pt) noexcept {
            this->ptLast = (this->ptLast->ptNext = pt);
        }

        void
        patternsList::append(patternsList&& lst) noexcept {
            this->ptLast->ptNext = lst.first();
            this->ptLast = lst.last();
        }

        void
        patternsList::prepend(Pattern&& pt) noexcept {
            pt->ptNext = this->ptRoot;
            this->ptRoot = pt;
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

    extern Match
    Eval(io::IStream& istream, const Pattern& pt, CaptureGroupList& groups, const std::any& usr_val) {
        return pt->Eval(istream, groups, usr_val);
    }

    extern Match
    Eval(io::IStream& istream, const Pattern& pt, const std::any& usr_val) {
        CaptureGroupList
            groups;
        return Eval(istream, pt, groups, usr_val);
    }
}