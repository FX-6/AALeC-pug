#ifndef SCANNER_H
#define SCANNER_H

#include <LittleFS.h>
#include <token/token.h>

/**
 * @brief Different types of indentations
 */
enum class IndentationType {
    Default,
    BlockExpansion,
    TagInterpolation,
    Conditional,
};

/**
 * @brief Info about the an indentation level
 */
class Indentation {
   public:
    /**
     * @brief Type of this level
     */
    IndentationType type;

    /**
     * @brief Size of this level
     */
    int size;

    /**
     * @brief Construct a new Indentation object
     *
     * @param type Type of this indentation
     * @param size Size of this indentation, defaults to 0
     */
    Indentation(IndentationType type, int size = 0);
};

/**
 * @brief Scanner class that tokenizes a .pug file
 */
class Scanner {
   private:
    /**
     * @brief The path to the source file that is being tokenized
     */
    String inPath_;

    /**
     * @brief Position the scanner was at when the if finished scanning the last part
     */
    int lastPosition_;

    /**
     * @brief The character that is used for indentation.
     *        If it is set to '.' no indentation was detected yet
     */
    char indentationChar_;

    /**
     * @brief Info about the indentation levels
     */
    std::vector<Indentation> indentations_;

    /**
     * @brief Wheter we are in a Block in a Tag Text
     *        Used when scanning text after an Interpolation
     */
    bool inBlockInATag_;

    /**
     * @brief Wheter we are in an Interpolation
     */
    int interpolationLevel_;

    /**
     * @brief The source file that is being tokenized
     */
    File inFile_;

   public:
    /**
     * @brief Construct a new Scanner object
     *
     * @param inPath Path to the .pug file
     */
    Scanner(String inPath);

    /**
     * @brief Scan part of the source and return the tokens
     *
     * @param tokens Appends the scanned tokens to this vector.
     *               Might start with Indent/Dedent Tokens,
     *               followed by another Token,
     *               ends with an EndOfPart or EndOfSource Token.
     * @return bool Wether scanning was successfull, see serial output for errors
     */
    bool scanPart(std::vector<Token> &tokens);

   private:
    // Helper functions

    /**
     * @brief Prints the "{name}: Unexpected character (ASCII code: '{code}') at {inPath_}:{position}" message
     */
    void printErrorUnexpectedChar(String name);

    /**
     * @brief Compare the nexr char in the source to the given char
     *
     * @param value The character that is being compared
     * @return bool If the chars match
     */
    bool check(char value);

    /**
     * @brief Compare the next part of the source to the given string
     *
     * @param value The string that is being compared
     * @return bool If the strings matche
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
     * @return bool Wether the source starts with a whitespace (32: ' ', 9: '\t')
     */
    bool isWhitespace();

    /**
     * @brief Checks if the source starts with a digit
     *
     * @return bool Wether the source starts with a digit
     */
    bool isDigit();

    /**
     * @brief Checks if the source starts with a alpha numeric char
     *
     * @return bool Wether the source starts with a alpha numeric char
     */
    bool isIdentifierPart();

    /**
     * @brief Checks if the current line is empty (only whitespaces)
     *
     * @return bool Wether the current line is empty
     */
    bool isEmptyLine();

    /**
     * @brief Checks if the source is at the end
     *
     * @return bool Wether the source is at the end
     */
    bool isEndOfSource();

    /**
     * @brief Checks if the next line starts with more indentation than the current line
     *
     * @return bool Wether the next line starts with more indentation than the current line
     */
    bool nextLineIndentationIsHigher();

    /**
     * @brief Checks if the next line has the same indentation and starts with else
     *        Expects a '\n' at the beginning
     *
     * @return bool Wether the next line is part of the same conditional
     */
    bool nextLineIsPartOfSameConditional();

    // Scan related funcitons

    /**
     * @brief Scans indentation.
     *        Expects a whitespace at the beginning
     *
     * @param tokens Appends the Indent/Dedent Tokens to this vector
     * @return bool Wether scanning was successfull, see serial output for errors
     */
    bool scanIndentation(std::vector<Token> &tokens);

    /**
     * @brief Scans a doctype tag.
     *        Expects "doctype" at the beginning
     *
     * @param data Location to write the data to
     * @return bool Wther scanning was successfull, see serial output for errors
     */
    bool scanDoctype(DoctypeData &data);

    /**
     * @brief Scans a generic tag.
     *        Expects a identifier part at the beginning
     *
     * @param data Location to write the data to
     * @return bool Wether scanning was successfull, see serial output for errors
     */
    bool scanTag(TagData &data);

    /**
     * @brief Scans the attributes of a tag.
     *        Expects a '(' at the beginning
     *
     * @param attributes Appends the attributes to this vector
     * @return bool Wether scanning was successfull, see serial output for errors
     */
    bool scanTagAttributes(std::vector<Attribute> &attributes);

    /**
     * @brief Scans the inner text of a tag.
     *        Expects a space or a ".\n" at the beginning
     *
     * @param value The scanned text
     * @return bool Wether scanning was successfull, see serial output for errors
     */
    bool scanTagText(String &value);

    /**
     * @brief Scans the inline inner text of a tag.
     *        Expects the space already removed
     *
     * @param value The scanned text
     * @return bool Wether scanning was successfull, see serial output for errors
     */
    bool scanTagTextInline(String &value);

    /**
     * @brief Scans the block in a tag inner text of a tag.
     *        Expects the '.' already removed
     *
     * @param value The scanned text
     * @return bool Wether scanning was successfull, see serial output for errors
     */
    bool scanTagTextBlock(String &value);

    /**
     * @brief Scans the next part of the tag text and appends it to the value.
     *
     * @param value The scanned text is appended to this string
     * @return bool Wether scanning was successfull, see serial output for errors
     */
    bool scanTagTextPart(String &value);

    /**
     * @brief Scans Text.
     *        Expects a '<', '|', or ']' at the beginning.
     *
     * @param data Location to write the data to
     * @return bool Wether scanning was successfull, see serial output for errors
     */
    bool scanText(TextData &data);

    /**
     * @brief Scans Literal HTML Text.
     *        Expects a '<' at the beginning.
     *
     * @param data Location to write the data to
     * @return bool Wether scanning was successfull, see serial output for errors
     */
    bool scanTextLiteralHTML(TextData &data);

    /**
     * @brief Scans Piped Text.
     *        Expects a '|' at the beginning.
     *
     * @param data Location to write the data to
     * @return bool Wether scanning was successfull, see serial output for errors
     */
    bool scanTextPipedText(TextData &data);

    /**
     * @brief Scans Inner Text after an Interpolation.
     *        Expects a ']' at the beginning.
     *
     * @param data Location to write the data to
     * @return bool Wether scanning was successfull, see serial output for errors
     */
    bool scanTextInterpolationEnd(TextData &data);

    /**
     * @brief Ignores a comment.
     *        Expects a "//-" at the beginning
     *
     * @return bool Wether scanning was successfull, see serial output for errors
     */
    bool ignoreComment();

    /**
     * @brief Scans a comment.
     *        Expects a "//" at the beginning
     *
     * @param data Location to write the data to
     * @return bool Wether scanning was successfull, see serial output for errors
     */
    bool scanComment(CommentData &data);

    /**
     * @brief Scans a include.
     *        Expects a "include" at the beginning
     *
     * @param data Location to write the data to
     * @return bool Wether scanning was successfull, see serial output for errors
     */
    bool scanInclude(IncludeData &data);

    /**
     * @brief Gets the value of a GPIO Pin.
     *        Expects "IO_" at the beginning
     *
     * @param result Value of the GPIO Pin
     * @return bool Wether scanning was successfull, see serial output for errors
     */
    bool scanGPIOValue(uint &result);

    /**
     * @brief Evaluates an expression.
     *        Expects a '(', "True", "False", "IO_", or digit at the beginning
     *
     * @param result Result of the expression evaluation
     * @return bool Wether scanning was successfull, see the serial output for more information
     */
    bool scanExpression(bool &result);

    /**
     * @brief Evaluates an expression.
     *        Expects a "True", "False", "IO_", or digit at the beginning
     *
     * @param result Result of the expression evaluation
     * @param isTrue Wheter the expression was `True`
     * @return bool Wheter evaluation was successfull, see serial output for errors
     */
    bool scanExpressionEvaluate(u32 &result, bool &isTrue);

    /**
     * @brief Scans a conditional.
     *        Expects "if", "unless", or "else" at the beginning
     *
     * @return bool Wether scanning was successfull, see serial output for errors
     */
    bool scanConditional();
};

#endif  // SCANNER_H
