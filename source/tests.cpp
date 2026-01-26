#include <classy-streams/ConsoleStreams.hpp>
#include <classy-streams/BufferStreams.hpp>
#include <Patterns.hpp>

int main() {
    patt::Pattern
        ptName  = patt::Capt(patt::Alpha() % 3);
    io::IOBufferStream
        buff;

    io::cout.put("enter name: ");
    io::TextIO(buff)
        .forward_line_from(io::std_input)
        .go_start();

    patt::CaptureList
        captures;
    auto
        optMatch = patt::Eval(ptName, buff, captures);
    if (!optMatch) {
        io::cerr.put("failed to parse the name\n");
        return EXIT_FAILURE;
    }

    std::string
        strName = captures.back().GetString(buff);
    io::cout.fmt("parsed name: \"{}\"\n", strName);

    return EXIT_SUCCESS;
}