#include <classy-streams/ConsoleStreams.hpp>
#include <classy-streams/FileStreams.hpp>
#include "Patterns.hpp"

static const patt::Grammar
    gramJSON    =
        std::invoke([] -> patt::Grammar {
            patt::Grammar g;

            g["spacing"]    = patt::SpaceOrNewLine() % 0;

            g["object"]     = patt::Str("{") >> g["spacing"] >> (
                g["field"] >> g["spacing"] >>
                (patt::Str(",") >> g["spacing"] >> g["field"] >> g["spacing"]) % 0
            ) % -1 >> patt::Str("}");
            g["array"]      = patt::Str("[") >> g["spacing"] >> (
                g["value"] >> g["spacing"] >>
                (patt::Str(",") >> g["spacing"] >> g["value"] >> g["spacing"]) % 0
            ) % -1 >> patt::Str("]");
            g["field"]      =
                g["string"] >> g["spacing"]
                >> patt::Str(":") >> g["spacing"] >>
                g["value"];
            g["value"]      =
                g["string"] | g["number"] |
                g["object"] | g["array"] |
                g["boolean"] | patt::Str("null");

            g["boolean"]    = patt::Str("true") | patt::Str("false");
            g["string"]     = patt::Str("\"") >>
                g["strfill"] >> (g["escseq"] >> g["strfill"]) % 0 >>
                patt::Str("\"");
            g["strfill"]    = (&patt::Any() >> -patt::Set("\"\\")) % 0;
            g["escseq"]     = patt::Str("\\") >> (
                patt::Set("/\"\\bfnrt") | (patt::Str("u") >> patt::HexDigit() * 4)
            );

            g["number"]     = g["numint"] >> (patt::Str(".") >> g["numfract"]) % -1;
            g["numint"]     = patt::Str("-") % -1 >> patt::Digit() % 1;
            g["numfract"]   = patt::Digit() % 1 >> (patt::Set("eE") >> g["numexp"]) % -1;
            g["numexp"]     = (patt::Set("+-") % -1) >> patt::Digit() % 1;

            g["__eval"]     = ((g["value"] % -1) >> patt::None()) /
                [] (io::IStream& istream, const patt::OptMatch& optm, const patt::CaptureGroupList&, const std::any&) {
                    if (!optm)
                        io::cerr.fmt("failed to parse JSON at {}\n", istream.GetPosition());
                    else
                        io::cout.put("success\n");
                };

            return g;
        });

int
main() {
    try {
        io::IFileStream
            ifileTestJSON   = io::IFileStream("./assets/test.json");

        patt::OptMatch
            optmJSON        = patt::Eval(ifileTestJSON, gramJSON);

        return ((bool)optmJSON)
            ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    catch (const std::exception& err) {
        io::cerr.fmt("Error: {}\n", err.what());
        return EXIT_FAILURE;
    }
}