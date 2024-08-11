#ifndef SCANNER_H
#define SCANNER_H

#include <LittleFS.h>
#include <token/token.h>

class Scanner {
   private:
    /**
     * @brief The source that is being tokenized
     */
    File inFile_;

    /**
     * @brief The character that is used for indentation.
     *        If it is set to '.' no indentation was detected yet
     */
    char indentationChar_;

    /**
     * @brief The amount of chars used to indent each level
     */
    std::vector<int> indentationLevel_;

   public:
    /**
     * @brief Construct a new Scanner object
     *
     * @param inPath Path to the .pug file
     */
    Scanner(String inPath);

    /**
     * @brief Destroy the Scanner object
     */
    ~Scanner();

    /**
     * @brief Scan part of the source and return the tokens
     *
     * @return std::vector<Token> Might start with Indent/Dedent Tokens,
     *                            followed by another Token,
     *                            ends with an EndOfPart or EndOfSource Token
     */
    bool scanPart(std::vector<Token> *tokens);

   private:
    // Helper functions

    /**
     * @brief Checks if the source_ starts with a specific character
     *
     * @param value The character that is being checked
     * @return true source_ starts with the value
     * @return false source_ does not start with the value
     */
    bool check(char value);

    /**
     * @brief Checks if the source_ starts with a specific string
     *
     * @param value The string that is being checked
     * @return true source_ starts with the value
     * @return false source_ does not start with the value
     */
    bool check(String value);

    /**
     * @brief Removes the specified amount of characters from the source_.
     *        Also increments the line and column accordingly
     *
     * @param amount
     * @return String The removed characters
     */
    String consume(int amount = 1);

    /**
     * @brief Removes the specified amount of characters from the source_.
     *        Also increments the line and column accordingly
     *
     * @param amount
     */
    void ignore(int amount = 1);

    /**
     * @brief Removes all whitespaces [" "\\t] from the source_
     *
     * @param includeNewlines If true: also removes newlines [\\n], defaults to false
     */
    void ignoreWhitespaces(bool includeNewlines = false);

    /**
     * @brief Checks if the source starts with a whitespace [" "\\t]
     *
     * @return true
     * @return false
     */
    bool isWhitespace();

    /**
     * @brief Checks if the source starts with a valid identifier part [a-zA-Z0-9_]
     *
     * @return true
     * @return false
     */
    bool isIdentifierPart();

    /**
     * @brief Checks if the source is at the end
     *
     * @return true
     * @return false
     */
    bool isEndOfSource();

    /**
     * @brief Checks if the next line starts with more indentation than the current line
     *
     * @return true
     * @return false
     */
    bool nextLineIndentationIsHigher();

    // Scan related funcitons

    /**
     * @brief Scans indentation.
     *        Expects a whitespace at the beginning
     *
     * @return std::vector<Token>
     */
    bool scanIndentation(std::vector<Token> *tokens);

    /**
     * @brief Scans a doctype tag.
     *        Expects "doctype" at the beginning
     *
     * @return DoctypeData
     */
    bool scanDoctype(DoctypeData *&data);

    /**
     * @brief Scans a generic tag.
     *        Expects a identifier part at the beginning
     *
     * @return TagData
     */
    bool scanTag(TagData *&data);

    /**
     * @brief Scans the attributes of a tag.
     *        Expects a opening round bracket at the beginning
     *
     * @return std::vector<Attribute>
     */
    bool scanTagAttributes(std::vector<Attribute> *attributes);

    /**
     * @brief Scans the inner text of a tag.
     *        Expects a space at the beginning
     *
     * @return String
     */
    String scanTagText();

    /**
     * @brief Scans literal HTML.
     *        Expects a less than sign at the beginning
     *
     * @return TextData
     */
    bool scanText(TextData *&data);

    /**
     * @brief Ignores a comment.
     *        Expects a "//-" at the beginning
     */
    bool ignoreComment();

    /**
     * @brief Scans a comment.
     *        Expects a "//" at the beginning
     *
     * @return CommentData
     */
    bool scanComment(CommentData *&data);

    /**
     * @brief Scans a include.
     *        Expects a "include" at the beginning
     *
     * @return IncludeData
     */
    bool scanInclude(IncludeData *&data);
};

#endif  // SCANNER_H
