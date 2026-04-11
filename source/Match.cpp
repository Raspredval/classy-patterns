#include "Match.hpp"

namespace patt {
    Match::Match(intptr_t iBegin, intptr_t iEnd) :
        iBegin(std::min(iBegin, iEnd)),
        iEnd(std::max(iBegin, iEnd)) {}

    intptr_t
    Match::Begin() const noexcept {
        return this->iBegin;
    }

    intptr_t
    Match::End() const noexcept {
        return this->iEnd;
    }

    size_t
    Match::Length() const noexcept {
        return (size_t)(this->iEnd - this->iBegin);
    }

    bool
    Match::Empty() const noexcept {
        return this->iBegin == this->iEnd;
    }

    Match&
    Match::operator+=(const Match& m) noexcept {
        this->iEnd  = m.iEnd;
        return *this;
    }

    std::string
    Match::GetString(io::IStream& istream) const {
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
    Match::ExportData(io::IStream& istream, io::SerialOStream& ostream) const {
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
}