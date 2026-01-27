#include <classy-streams/ConsoleStreams.hpp>
#include <classy-streams/BufferStreams.hpp>
#include <Patterns.hpp>

int main() {
    patt::Pattern
        ptID        = (patt::Alpha() |= patt::Str("_")) >> (patt::Alnum() |= patt::Str("_")) % 0,
        ptMacroName = patt::Str("$") >> patt::Capt(patt::Capt(ptID) >> (patt::Str(".") >> patt::Capt(ptID)) % 0) >> patt::None();

    io::IOBufferStream
        buff;
    io::TextIO(buff)
        .forward_line_from(io::std_input)
        .go_start();

    patt::CaptureList
        captures;
    patt::OptMatch
        optm        = patt::Eval(ptMacroName, buff, captures);
    if (!optm) {
        io::cerr.put("failed to parse macro name\n");
        return EXIT_FAILURE;
    }

    for (const patt::Match& cpt : captures) {
        std::string
            strNamePart = cpt.GetString(buff);
        io::cout.put(strNamePart).put_endl();
    }

    return EXIT_SUCCESS;
}