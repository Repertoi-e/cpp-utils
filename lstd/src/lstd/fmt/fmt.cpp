#include "fmt.h"

LSTD_BEGIN_NAMESPACE

void fmt_default_parse_error_handler(const string &message, const string &formatString, s64 position) {
    // An error during formatting occured.
    // If you are running a debugger it has now hit a breakpoint.

    // Make escape characters appear as they would in a string literal
    string str = formatString;
    string_replace_all(str, '\"', "\\\"");
    string_replace_all(str, '\\', "\\\\");
    string_replace_all(str, '\a', "\\a");
    string_replace_all(str, '\b', "\\b");
    string_replace_all(str, '\f', "\\f");
    string_replace_all(str, '\n', "\\n");
    string_replace_all(str, '\r', "\\r");
    string_replace_all(str, '\t', "\\t");
    string_replace_all(str, '\v', "\\v");

    string_builder_writer output;
    fmt_to_writer(&output, "\n\n>>> {!GRAY}An error during formatting occured: {!YELLOW}{}{!GRAY}\n", message);
    fmt_to_writer(&output, "    ... the error happened here:\n");
    fmt_to_writer(&output, "        {!}{}{!GRAY}\n", str);
    fmt_to_writer(&output, "        {: >{}} {!} \n\n", "^", position + 1);
#if defined NDEBUG
    Context.PanicHandler(combine(output.Builder), {});
#else
    print("{}", string_builder_combine(output.Builder));

    // More info has been printed to the console but here's the error message:
    auto errorMessage = message;
    assert(false);
#endif
}

LSTD_END_NAMESPACE
