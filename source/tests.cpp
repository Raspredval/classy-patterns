#include <classy-streams/ConsoleStreams.hpp>
#include <classy-streams/BufferStreams.hpp>
#include <Patterns.hpp>

int main() {
    patt::Pattern
        ptName  = patt::Capt(patt::Alpha() % 3) >> patt::Space() % 1 >> patt::Capt(patt::Alpha() % 3);
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

    if (captures.size() != 2) {
        io::cerr.put("failed to capture parts of the name\n");
        return EXIT_FAILURE;
    }
    
    std::string
        strFirstName    = captures[0].GetString(buff),
        strSecondName   = captures[1].GetString(buff);
    io::cout.fmt("parsed first name:  \"{}\"\n", strFirstName);
    io::cout.fmt("parsed second name: \"{}\"\n", strSecondName);

    return EXIT_SUCCESS;
}