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
 * @brief The Parser
 */
class Parser {
   private:
    /**
     * @brief Path to the .html file
     */
    File outFile_;

    /**
     * @brief The directory of the currently compiled file, with a trailing slash
     */
    String directory_;

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
     * @brief Wheter there should be a new line before the tag if it is pipe text
     */
    bool addPipeTextNewline_;

    /**
     * @brief Wheter there should be a new line before the tag if it is literal HTML
     */
    bool addLiteralHTMLNewline_;

   public:
    /**
     * @brief Construct a new Parser object
     *
     * @param input
     * @param output
     * @param doctype
     */
    Parser(
        String inPath,
        String outPath,
        DoctypeDialect doctype = DoctypeDialect::None
    );

    /**
     * @brief Destroy the Parser object
     */
    ~Parser();

    /**
     * @brief Parse the source
     */
    bool parse();

   private:
    /**
     * @brief Get the next (aka first) token from the vector and remove it
     * @param tokens
     * @return Token
     */
    Token nextToken(std::vector<Token> *tokens);

    /**
     * @brief Wheter the tag is a void element defaultly
     *        eg: img, br
     *
     * @param tag
     * @return true
     * @return false
     */
    bool isVoidElement(String tag);

    /**
     * @brief Close the last opened tag if it wasnt indentet
     */
    void closeTag();

    /**
     * @brief Close the last opened tag if it wasnt indentet
     */
    void closeTagIfNecessary();

    /**
     * @brief Handle the text newline
     *
     * @param isPipeText
     * @param isLiteralHTML
     */
    void handleTextNewline(bool isPipeText = false, bool isLiteralHTML = false);

    /**
     * @brief Parse a doctype token
     *
     * @param data
     */
    void parseDoctype(DoctypeData *data);

    /**
     * @brief Parse a tag token
     *
     * @param data
     */
    void parseTag(TagData *data);

    /**
     * @brief Parse a literal HTML token
     *
     * @param data
     */
    void parseText(TextData *data);

    /**
     * @brief Parse a comment token
     *
     * @param data
     */
    void parseComment(CommentData *data);

    /**
     * @brief Parse a include token
     *
     * @param data
     */
    bool parseInclude(IncludeData *data);
};

#endif  // PARSER_H
