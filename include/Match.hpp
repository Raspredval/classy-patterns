static_assert(__cplusplus >= 202302, "requires C++23 minimum version");

#pragma once
#include <classy-streams/IOStreams.hpp>
#include <optional>
#include <cstdint>
#include <vector>
#include <string>

namespace patt {
    class Match {
    public:
        Match(intptr_t iBegin, intptr_t iEnd) :
            iBegin(std::min(iBegin, iEnd)),
            iEnd(std::max(iBegin, iEnd)) {}

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

        bool
        Empty() const noexcept {
            return this->iBegin == this->iEnd;
        }

        Match&
        operator+=(const Match& m) noexcept {
            this->iEnd  = m.iEnd;
            return *this;
        }

        std::string
        GetString(io::IStream& istream) const {
            intptr_t
                iCurPos = istream.GetPosition();
            std::string
                strResult;
            strResult.reserve(this->Length());
                
            istream.SetPosition(this->Begin());
            while (istream.GetPosition() != this->End()) {
                std::optional<std::byte>
                    optc    = istream.Read();
                if (!optc)
                    break;

                strResult   += (char)*optc;
            }

            istream.SetPosition(iCurPos);
            return strResult;
        }

        size_t
        ExportData(io::IStream& istream, io::SerialOStream& ostream) const {
            intptr_t
                iCurPos = istream.GetPosition();
            size_t
                uCount  = 0;
            
            istream.SetPosition(this->Begin());
            while (istream.GetPosition() != this->End()) {
                std::optional<std::byte>
                    optc    = istream.Read();
                if (!optc)
                    break;
                ostream.Write(*optc);
                uCount      += 1;
            }

            istream.SetPosition(iCurPos);
            return uCount;
        }

    private:
        intptr_t
            iBegin, iEnd;
    };

    using OptMatch      =
        std::optional<Match>;
    using CaptureList   =
        std::vector<Match>;
}