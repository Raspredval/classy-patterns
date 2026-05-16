#include "Match.hpp"

namespace patt {
    Match::Match(intptr_t iBegin, size_t uSize, bool bFailed) {
        this->iBegin    = iBegin;
        this->uSize     = uSize;
        this->bFailed   = bFailed != 0;
    }

    Match::Match(intptr_t iBegin, intptr_t iEnd, bool bFailed) {
        if (iBegin > iEnd)
            std::swap(iBegin, iEnd);
        this->iBegin    = iBegin;
        this->uSize     = (size_t)(iEnd - iBegin);
        this->bFailed   = bFailed != 0;
    }

    intptr_t
    Match::Begin() const noexcept {
        return this->iBegin;
    }

    intptr_t
    Match::End() const noexcept {
        return this->iBegin + (intptr_t)this->uSize;
    }

    size_t
    Match::Length() const noexcept {
        return this->uSize;
    }

    bool
    Match::Empty() const noexcept {
        return this->uSize == 0;
    }

    bool
    Match::Failed() const noexcept {
        return this->bFailed;
    }

    Match::operator bool() const noexcept {
        return this->Failed();
    }

    void
    Match::ToggleFailed() noexcept {
        this->bFailed   = !this->bFailed;
    }

    Match&
    Match::operator+=(const Match& m) noexcept {
        intptr_t
            iNewEnd = m.End();
        this->uSize = (size_t)(iNewEnd - this->iBegin);
        return *this;
    }

    std::string
    Match::GetString(io::IStream& istream) const {
        intptr_t
            iCurPos = istream.GetPosition();
        std::string
            strResult;
        strResult.reserve(this->uSize);

        istream.SetPosition(this->iBegin);
        for (size_t i = 0; i != this->uSize; ++i) {
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
            uCount;

        istream.SetPosition(this->iBegin);
        for (uCount = 0; uCount != this->uSize; ++uCount) {
            std::optional<std::byte>
                optc    = istream.Read();
            if (!optc)
                break;
            ostream.Write(*optc);
        }

        istream.SetPosition(iCurPos);
        return uCount;
    }
}