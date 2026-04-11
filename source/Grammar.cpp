#include "Grammar.hpp"

#ifdef CLASSY_PATTERNS_TRACE_GRAMMAR
#include <classy-streams/ConsoleStreams.hpp>
#endif

namespace patt {
    namespace __impl {
        accessor<gramm_iter>
        grammar::operator[](const std::string& strKey) {
            return { this->getPatternIter(strKey) };
        }

        const_accessor<const_gramm_iter>
        grammar::operator[](const std::string& strKey) const {
            return { this->getPatternIter(strKey) };
        }

        gramm_iter
        grammar::getPatternIter(const std::string& strKey) {
            gramm_iter
                itPattern   = this->mapPatterns.find(strKey);
            if (itPattern == this->mapPatterns.end()) {
                itPattern   = this->mapPatterns.emplace(strKey, nullptr).first;
            }

            return itPattern;
        }

        const_gramm_iter
        grammar::getPatternIter(const std::string& strKey) const {
            const_gramm_iter
                itPattern   = this->mapPatterns.find(strKey);
            if (itPattern == this->mapPatterns.end()) {
                throw std::out_of_range(
                    std::string("rule \"") + strKey + std::string("\" wasn't found in the grammar"));
            }

            return itPattern;
        }
    }
}