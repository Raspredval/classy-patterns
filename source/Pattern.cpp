#include "Pattern.hpp"

namespace patt {
    namespace __impl {
        OptMatch
        pattern::Eval(io::IStream& istream, CaptureList& captures, const std::any& usr_val) {
            return (this->bNegated)
                ? this->negEval(istream, captures, usr_val)
                : this->normEval(istream, captures, usr_val);
        }

        Pattern
        pattern::NextPattern() const noexcept {
            return this->ptNext;
        }

        Pattern
        operator-(Pattern&& pt) noexcept {
            pt->toggleNegated();
            return pt;
        }

        Pattern
        operator-(const Pattern& pt) noexcept {
            return -pt->Clone();
        }

        OptMatch
        pattern::negEval(io::IStream& istream, CaptureList& captures, const std::any& usr_val) {
            intptr_t
                iCurPos = istream.GetPosition();
            OptMatch
                optm    = this->normEval(istream, captures, usr_val);
            if (!optm) {
                intptr_t
                    iEndPos = istream.GetPosition();
                return Match(iCurPos, iEndPos);
            }
            else
                return std::nullopt;
        }

        void
        pattern::toggleNegated() noexcept {
            this->bNegated = !this->bNegated;
        }

        Pattern
        pattern::appendPattern(const Pattern& pt) noexcept {
            return (this->ptNext = pt);
        }

        patternsList::patternsList(const Pattern& ptRoot) :
            ptRoot(ptRoot),
            ptLast(ptRoot) {}

        void
        patternsList::append(const Pattern& pt) noexcept {
            this->ptLast    =
                this->ptLast->appendPattern(pt);
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
    Eval(io::IStream& istream, const Pattern& pt, CaptureList& captures, const std::any& usr_val) {
        return pt->Eval(istream, captures, usr_val);
    }

    extern OptMatch
    Eval(io::IStream& istream, const Pattern& pt, const std::any& usr_val) {
        CaptureList
            captures;
        return Eval(istream, pt, captures, usr_val);
    }
}