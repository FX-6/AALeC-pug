#ifndef PARSER_H
#define PARSER_H

#include <scanner/scanner.h>

/**
 * @brief The different doctypes that influence how the html gets outputted
 */
enum class DoctypeDialect {
    None,
    HTML,
    XML,
};

/**
 * @brief Scanner class that parses a .pug file
 */
class Parser {
   private:
    /**
     * @brief The path to the file that should be compiled
     */
    String inPath_;

    /**
     * @brief The path to the output file
     */
    String outPath_;

    /**
     * @brief The file to write the HTML to
     */
    File outFile_;

    /**
     * @brief The HTML dialect
     */
    DoctypeDialect doctype_;

    /**
     * @brief The scanner
     */
    Scanner scanner_;

    /**
     * @brief Opened but not closed tags
     */
    std::vector<String> tags_;

    /**
     * @brief Wheter this line was indentet
     */
    bool indentet_;

    /**
     * @brief Wheter there should be a new line before the tag if it is this text type
     */
    TextType addNewlineFor_;

   public:
    /**
     * @brief Construct a new Parser object
     *
     * @param inPath Path to the file that should be compiled
     * @param outPath Path to the output file
     * @param doctype The HTML dialect, defaults to no dialect (DoctypeDialect::None)
     */
    Parser(
        String inPath,
        String outPath,
        DoctypeDialect doctype = DoctypeDialect::None
    );

    /**
     * @brief Parse the source
     *
     * @return bool Wheter the parsing was successful, see serial output for errors
     */
    bool parse();

   private:
    /**
     * @brief Get the next (aka first) token from the vector and remove it
     * @param tokens The vector of tokens
     * @return Token The next token
     */
    Token nextToken(std::vector<Token> &tokens);

    /**
     * @brief Whether the tag is a void element by default
     *        eg: img, br
     *
     * @param tag The tag
     * @return bool If it is a void element
     */
    bool isVoidElement(String tag);

    /**
     * @brief Close the last opened tag
     */
    void closeTag();

    /**
     * @brief Close the last opened tag if it wasnt indentet
     */
    void closeTagIfNecessary();

    /**
     * @brief Handle the text newline
     *
     * @param isPipeText Type of the text, defaults to TextType::InnerText
     */
    void handleTextNewline(TextType textType = TextType::InnerText);

    /**
     * @brief Parse a Doctype Token
     *
     * @param data The doctype data
     */
    void parseDoctype(DoctypeData data);

    /**
     * @brief Parse a Tag Token
     *
     * @param data The tag data
     */
    void parseTag(TagData data);

    /**
     * @brief Parse a Text Token
     *
     * @param data The text data
     */
    void parseText(TextData data);

    /**
     * @brief Parse a Comment Token
     *
     * @param data The comment data
     */
    void parseComment(CommentData data);

    /**
     * @brief Parse a Include Token
     *
     * @param data The include data
     * @return bool Wheter the parsing was successful, see serial output for errors
     */
    bool parseInclude(IncludeData data);
};

#endif  // PARSER_H
