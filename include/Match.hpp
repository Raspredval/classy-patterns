static_assert(__cplusplus >= 202302, "requires C++23 minimum version");

#pragma once
#include <classy-streams/IOStreams.hpp>
#include <cstdint>
#include <vector>
#include <string>

namespace patt {
    class Match {
    public:
        Match(intptr_t iBegin, intptr_t iEnd, bool bFailed = false);

        intptr_t
        Begin() const noexcept;

        intptr_t
        End() const noexcept;

        size_t
        Length() const noexcept;

        bool
        Empty() const noexcept;

        bool
        Failed() const noexcept;

        operator bool() const noexcept;

        void
        ToggleFailed() noexcept;

        Match&
        operator+=(const Match& m) noexcept;

        std::string
        GetString(io::IStream& istream) const;

        size_t
        ExportData(io::IStream& istream, io::SerialOStream& ostream) const;

    private:
        intptr_t
            iBegin;
        size_t
            uSize   : sizeof(size_t) * 8 - 1,
            bFailed : 1;
    };

    using CaptureGroup  =
        std::vector<Match>;
    using CaptureGroupList  =
        std::vector<CaptureGroup>;
}