#ifndef SCANNER_H
#define SCANNER_H

#include <LittleFS.h>
#include <token/token.h>

/**
 * @brief Scanner class that tokenizes a .pug file
 */
class Scanner {
   private:
    /**
     * @brief The source file that is being tokenized
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

    /**
     * @brief Wheter we are in a Block in a Tag Text
     *        Used when scanning text after an Interpolation
     */
    bool inBlockInATag_;

    /**
     * @brief Wheter we are in an Interpolation
     */
    int interpolationLevel_;

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
     * @param tokens Appends the scanned tokens to this vector.
     *               Might start with Indent/Dedent Tokens,
     *               followed by another Token,
     *               ends with an EndOfPart or EndOfSource Token.
     * @return true Scanning was successfull
     * @return false Scanning encountered an error, see the serial output for more information
     */
    bool scanPart(std::vector<Token> *tokens);

   private:
    // Helper functions

    /**
     * @brief Compare the nexr char in the source to the given char
     *
     * @param value The character that is being compared
     * @return true The chars match
     * @return false The chars dont match
     */
    bool check(char value);

    /**
     * @brief Compare the next part of the source to the given string
     *
     * @param value The string that is being compared
     * @return true The string matches
     * @return false The string does not match
     */
    bool check(String value);

    /**
     * @brief Removes the specified amount of characters from the source and returns them
     *
     * @param amount The amount of characters to remove, defaults to 1
     * @return String The removed characters
     */
    String consume(int amount = 1);

    /**
     * @brief Removes the specified amount of characters from the source
     *
     * @param amount The amount of characters to remove, defaults to 1
     */
    void ignore(int amount = 1);

    /**
     * @brief Removes all whitespaces (32: ' ', 9: '\t') from the source
     *
     * @param includeNewlines If true: also removes newlines (10: '\n'), defaults to false
     */
    void ignoreWhitespaces(bool includeNewlines = false);

    /**
     * @brief Checks if the source starts with a whitespace (32: ' ', 9: '\t')
     *
     * @return true The source starts with a whitespace (32: ' ', 9: '\t')
     * @return false The source doesnt starts with a whitespace (32: ' ', 9: '\t')
     */
    bool isWhitespace();

    /**
     * @brief Checks if the source starts with a alpha numeric char
     *
     * @return true The source starts with a alpha numeric char
     * @return false The source doesnt starts with a alpha numeric char
     */
    bool isIdentifierPart();

    /**
     * @brief Checks if the source is at the end
     *
     * @return true The source is at the end
     * @return false The source isnt at the end
     */
    bool isEndOfSource();

    /**
     * @brief Checks if the next line starts with more indentation than the current line
     *
     * @return true The next line starts with more indentation than the current line
     * @return false The next line does not start with more indentation than the current line
     */
    bool nextLineIndentationIsHigher();

    // Scan related funcitons

    /**
     * @brief Scans indentation.
     *        Expects a whitespace at the beginning
     *
     * @param tokens Appends the Indent/Dedent Tokens to this vector
     * @return true Scanning was successfull
     * @return false Scanning encountered an error, see the serial output for more information
     */
    bool scanIndentation(std::vector<Token> *tokens);

    /**
     * @brief Scans a doctype tag.
     *        Expects "doctype" at the beginning
     *
     * @param data Writes the data to this pointer
     * @return true Scanning was successfull
     * @return false Scanning encountered an error, see the serial output for more information
     */
    bool scanDoctype(DoctypeData *&data);

    /**
     * @brief Scans a generic tag.
     *        Expects a identifier part at the beginning
     *
     * @param data Writes the data to this pointer
     * @return true Scanning was successfull
     * @return false Scanning encountered an error, see the serial output for more information
     */
    bool scanTag(TagData *&data);

    /**
     * @brief Scans the attributes of a tag.
     *        Expects a '(' at the beginning
     *
     * @param attributes Appends the attributes to this vector
     * @return true Scanning was successfull
     * @return false Scanning encountered an error, see the serial output for more information
     */
    bool scanTagAttributes(std::vector<Attribute> *attributes);

    /**
     * @brief Scans the inner text of a tag.
     *        Expects a space or a ".\n" at the beginning
     *
     * @return String The scanned text
     */
    String scanTagText();

    /**
     * @brief Scans the inline inner text of a tag.
     *        Expects the space already removed
     *
     * @return String The scanned text
     */
    String scanTagTextInline();

    /**
     * @brief Scans the block in a tag inner text of a tag.
     *        Expects the '.' already removed
     *
     * @return String The scanned text
     */
    String scanTagTextBlock();

    /**
     * @brief Scans Text.
     *        Expects a '<', '|', or ']' at the beginning.
     *
     * @param data Writes the data to this pointer
     * @return true Scanning was successfull
     * @return false Scanning encountered an error, see the serial output for more information
     */
    bool scanText(TextData *&data);

    /**
     * @brief Scans Literal HTML Text.
     *        Expects a '<' at the beginning.
     *
     * @param data Writes the data to this pointer
     * @return true Scanning was successfull
     * @return false Scanning encountered an error, see the serial output for more information
     */
    bool scanTextLiteralHTML(TextData *&data);

    /**
     * @brief Scans Piped Text.
     *        Expects a '|' at the beginning.
     *
     * @param data Writes the data to this pointer
     * @return true Scanning was successfull
     * @return false Scanning encountered an error, see the serial output for more information
     */
    bool scanTextPipedText(TextData *&data);

    /**
     * @brief Scans Inner Text after an Interpolation.
     *        Expects a ']' at the beginning.
     *
     * @param data Writes the data to this pointer
     * @return true Scanning was successfull
     * @return false Scanning encountered an error, see the serial output for more information
     */
    bool scanTextInterpolationEnd(TextData *&data);

    /**
     * @brief Ignores a comment.
     *        Expects a "//-" at the beginning
     *
     * @return true Scanning was successfull
     * @return false Scanning encountered an error, see the serial output for more information
     */
    bool ignoreComment();

    /**
     * @brief Scans a comment.
     *        Expects a "//" at the beginning
     *
     * @param data Writes the data to this pointer
     * @return true Scanning was successfull
     * @return false Scanning encountered an error, see the serial output for more information
     */
    bool scanComment(CommentData *&data);

    /**
     * @brief Scans a include.
     *        Expects a "include" at the beginning
     *
     * @param data Writes the data to this pointer
     * @return true Scanning was successfull
     * @return false Scanning encountered an error, see the serial output for more information
     */
    bool scanInclude(IncludeData *&data);
};

#endif  // SCANNER_H
