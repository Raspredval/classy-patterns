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
        std::shared_ptr<__impl::pattern>;

    namespace __impl {
        class pattern {
            friend class PatternList;
        public:
            virtual ~pattern() = default;

            virtual Pattern
            Clone() const = 0;

            OptMatch
            Eval(io::IStream& istream, CaptureList& captures, const std::any& usr_val) {
                return (this->bNegated)
                    ? this->negEval(istream, captures, usr_val)
                    : this->normEval(istream, captures, usr_val);
            }

            Pattern
            NextPattern() const noexcept {
                return this->ptNext;
            }

            // pattern negation; inverts the evaluation result of the pattern
            friend Pattern
            operator-(Pattern&& pt) noexcept {
                pt->toggleNegated();
                return pt;
            }

            // pattern negation; inverts the evaluation result of the pattern
            friend Pattern
            operator-(const Pattern& pt) noexcept {
                return -pt->Clone();
            }

        protected:
            virtual OptMatch
            normEval(io::IStream& istream, CaptureList& captures, const std::any& usr_val) = 0;
        
            virtual OptMatch
            negEval(io::IStream& istream, CaptureList& captures, const std::any& usr_val) {
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
            toggleNegated() noexcept {
                this->bNegated = !this->bNegated;
            }

            Pattern
            appendPattern(const Pattern& pt) noexcept {
                return (this->ptNext = pt);
            }

        private:
            Pattern
                ptNext      = nullptr;
            bool
                bNegated    = false;
        };

        class PatternList {
        public:
            PatternList(const Pattern& ptRoot) :
                ptRoot(ptRoot),
                ptLast(ptRoot) {}

            void
            Append(const Pattern& pt) noexcept {
                this->ptLast    =
                    this->ptLast->appendPattern(pt);
            }

            Pattern
            First() const noexcept {
                return this->ptRoot;
            }

            Pattern
            Last() const noexcept {
                return this->ptRoot;
            } 

        private:
            Pattern
                ptRoot  = nullptr,
                ptLast  = nullptr;
        };
    }

    [[nodiscard]]
    inline OptMatch
    Eval(io::IStream& istream, const Pattern& pt, CaptureList& captures, const std::any& usr_val = {}) {
        return pt->Eval(istream, captures, usr_val);
    }

    [[nodiscard]]
    inline OptMatch
    Eval(io::IStream& istream, const Pattern& pt, const std::any& usr_val = {}) {
        CaptureList
            captures;
        return Eval(istream, pt, captures, usr_val);
    }
}