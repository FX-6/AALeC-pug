#include "scanner.h"

#include <AALec-V2.h>

Indentation::Indentation(IndentationType type, int size) :
    type(type),
    size(size) {}

Scanner::Scanner(String inPath) :
    inPath_(inPath),
    lastPosition_(0),
    indentationChar_('.'),
    indentations_(std::vector<Indentation>()),
    inBlockInATag_(false),
    interpolationLevel_(0) {}

bool Scanner::scanPart(std::vector<Token> *tokens) {
    inFile_ = LittleFS.open(inPath_, "r");
    if (!inFile_ || !inFile_.isFile()) {
        Serial.printf(
            "Error 1-1: Failed to open file for reading '%s'\n",
            inPath_.c_str()
        );
        inFile_.close();
        return false;
    }
    inFile_.seek(lastPosition_, SeekSet);

    // Scan the indentation if thers is any
    if (isWhitespace()) {
        if (!scanIndentation(tokens)) {
            inFile_.close();
            // Error output from `scanIndentation()`
            return false;
        }

        // Remove additional indentations from block expansion
        while (indentations_.size() > 0) {
            if (indentations_.back().type == IndentationType::BlockExpansion) {
                indentations_.pop_back();
                tokens->push_back(Token(TokenType::Dedent));
            } else {
                break;
            }
        }
    } else if (check(':')) {
        // Block expansion, add a Indent Token and a 0 size indent level
        tokens->push_back(Token(TokenType::Indent));
        indentations_.push_back(Indentation(IndentationType::BlockExpansion));

        // Ignore the colon and following whitespace
        ignore();
        ignoreWhitespaces();
    } else if (check("#[")) {
        interpolationLevel_++;
        indentations_.push_back(Indentation(IndentationType::TagInterpolation));
        tokens->push_back(Token(TokenType::Indent));
        ignore(2);
    } else if (check(']')) {
        interpolationLevel_--;
        if (interpolationLevel_ > 0) {
            indentations_.pop_back();
            tokens->push_back(Token(TokenType::Dedent));
        }
    } else if (indentations_.size() > 0) {
        // If there is an indentation level, but no indentation, its a dedent
        while (indentations_.size() > 0) {
            if (indentations_.back().type != IndentationType::Conditional) {
                tokens->push_back(Token(TokenType::Dedent));
            }
            indentations_.pop_back();
        }

        // Remove additional indentations from block expansion and conditionals
        while (indentations_.size() > 0) {
            Indentation back = indentations_.back();

            if (back.type == IndentationType::BlockExpansion) {
                indentations_.pop_back();
                tokens->push_back(Token(TokenType::Dedent));
            } else if (back.type == IndentationType::Conditional) {
                indentations_.pop_back();
            } else {
                break;
            }
        }
    }

    // Scan a token if there is a token
    if (check("doctype")) {
        DoctypeData *data = nullptr;
        if (!scanDoctype(data)) {
            delete data;
            inFile_.close();
            // Error output from `scanDoctype()`
            return false;
        }
        tokens->push_back(Token(data));
    } else if (check("<") || check('|') || check(']')) {
        TextData *data = nullptr;
        if (!scanText(data)) {
            delete data;
            inFile_.close();
            // Error output from `scanText()`
            return false;
        }
        tokens->push_back(Token(data));
    } else if (check("//-")) {
        if (!ignoreComment()) {
            inFile_.close();
            // Error output from `ignoreComment()`
            return false;
        }
    } else if (check("//")) {
        CommentData *data = nullptr;
        if (!scanComment(data)) {
            delete data;
            inFile_.close();
            // Error output from `scanComment()`
            return false;
        }
        tokens->push_back(Token(data));
    } else if (check("include")) {
        IncludeData *data = nullptr;
        if (!scanInclude(data)) {
            delete data;
            inFile_.close();
            // Error output from `scanInclude()`
            return false;
        }
        tokens->push_back(Token(data));
    } else if (check("if") || check("unless") || check("else")) {
        if (!scanConditional()) {
            inFile_.close();
            // Error output from `scanConditional()`
            return false;
        }
    } else if (isIdentifierPart() || (check('#') && !check("#["))
               || check('.')) {
        TagData *data = nullptr;
        if (!scanTag(data)) {
            delete data;
            inFile_.close();
            // Error output from `scanTag()`
            return false;
        }
        tokens->push_back(Token(data));
    }

    // Handle the part after the token
    if (isEndOfSource()) {
        tokens->push_back(Token(TokenType::EndOfSource));
    } else if (check('\n')) {
        ignore();
        tokens->push_back(Token(TokenType::EndOfPart));
    } else if (check(':') || check("#[") || check(']')) {
        tokens->push_back(Token(TokenType::EndOfPart));
    } else {
        inFile_.close();
        printErrorUnexpectedChar("Error 1-2");
        return false;
    }

    lastPosition_ = inFile_.position();
    inFile_.close();
    return true;
}

void Scanner::printErrorUnexpectedChar(String name) {
    Serial.printf(
        "%s: Unexpected character (ASCII code: '%d') at %s:%d\n",
        name.c_str(),
        inFile_.peek(),
        inPath_.c_str(),
        inFile_.position()
    );
}

bool Scanner::check(char value) {
    return inFile_.peek() == value;
}

bool Scanner::check(String value) {
    // Remember the start position
    int start = inFile_.position();
    bool res = true;

    // Check each character
    for (char c : value) {
        if (inFile_.read() != c) {
            // Set the return value, end the loop
            res = false;
            break;
        }
    }

    // Reset the stream position
    inFile_.seek(start, SeekSet);
    return res;
}

String Scanner::consume(int amount) {
    String value = "";

    for (int i = 0; i < amount; i++) {
        value += (char)inFile_.read();
    }

    return value;
}

void Scanner::ignore(int amount) {
    inFile_.seek(amount, SeekCur);
}

void Scanner::ignoreWhitespaces(bool includeNewlines) {
    while (check(' ') || check('\t') || (includeNewlines && check('\n'))) {
        ignore();
    }
}

bool Scanner::isWhitespace() {
    return check(' ') || check('\t');
}

bool Scanner::isDigit() {
    return isdigit(inFile_.peek());
}

bool Scanner::isIdentifierPart() {
    return isAlphaNumeric(inFile_.peek()) || check('_');
}

bool Scanner::isEndOfSource() {
    return inFile_.peek() == -1;
}

bool Scanner::nextLineIndentationIsHigher() {
    // Get total size of current indentation
    int currentSize = 0;
    for (Indentation obj : indentations_) {
        currentSize += obj.size;
    }

    // Set the indentation char if not alread set
    if (indentationChar_ == '.') {
        // Not set...
        if (check("\n ")) {
            // ... and next line is indentet with a space
            indentationChar_ = ' ';
        } else if (check("\n\t")) {
            // ... and next line is indentet with a tab
            indentationChar_ = '\t';
        } else {
            // ... but next line isn't indentet anyways
            return false;
        }
    }

    // Generate string with '\n' + indentation size * indentation char + extra indentation char
    String higherIndentation = "\n";

    for (int i = 0; i < currentSize; i++) {
        higherIndentation += indentationChar_;
    }

    higherIndentation += indentationChar_;

    return check(higherIndentation);
}

bool Scanner::nextLineIsPartOfSameConditional() {
    // Get total size of current indentation
    int currentSize = 0;
    for (Indentation obj : indentations_) {
        currentSize += obj.size;
    }

    // Generate string with '\n' + indentation size * indentation char + "else"
    String comparisonString = "\n";

    for (int i = 0; i < currentSize; i++) {
        comparisonString += indentationChar_;
    }

    comparisonString += "else";

    return check(comparisonString);
}

bool Scanner::scanIndentation(std::vector<Token> *tokens) {
    // Set the used indentation char if not already set
    if (indentationChar_ == '.') {
        indentationChar_ = inFile_.peek();
    }

    // Check on what level we are
    uint level;
    for (level = 0; level < indentations_.size(); level++) {
        int levelSize = indentations_[level].size;

        // Create string
        String indentationString = "";
        for (int i = 0; i < levelSize; i++) {
            indentationString += indentationChar_;
        }

        if (check(indentationString)) {
            ignore(levelSize);
        } else {
            break;
        }
    }

    // If there are no more indentation chars
    if (!check(indentationChar_)) {
        // We are either on the same level -> nothing must be done
        // or a smaller level -> generate dedents and remove the levels
        while (level < indentations_.size()) {
            if (indentations_.back().type != IndentationType::Conditional) {
                tokens->push_back(Token(TokenType::Dedent));
            }
            indentations_.pop_back();
        }
    } else {
        // If there are more indentation chars
        // We are either on a new level -> add a new level
        // or the indentation chars are not enough to reach the next level -> error
        if (level == indentations_.size()) {
            int newLevelSize = 0;

            while (check(indentationChar_)) {
                newLevelSize++;
                ignore();
            }

            // Are we in a new conditional?
            Indentation back = indentations_.back();
            if (back.type == IndentationType::Conditional && back.size == 0) {
                indentations_.back().size = newLevelSize;
            } else {
                indentations_.push_back(
                    Indentation(IndentationType::Default, newLevelSize)
                );
                tokens->push_back(Token(TokenType::Indent));
            }
        } else {
            Serial.printf(
                "Error 1-3: Wrong indentation amount at %s:%d\n",
                inPath_.c_str(),
                inFile_.position()
            );
            return false;
        }
    }

    // If there are still spaces or tabs left
    // That means the wrong character was used for indentation
    if (check(' ') || check('\t')) {
        Serial.printf(
            "Error 1-4: Wrong indentation character (ASCII code: '%d') at %s:%d\n",
            inFile_.peek(),
            inPath_.c_str(),
            inFile_.position()
        );
        return false;
    }

    return true;
}

bool Scanner::scanDoctype(DoctypeData *&data) {
    // Ignore the "doctype" keyword and following spaces/tabs
    ignore(7);
    ignoreWhitespaces();

    // Get the doctype value
    String value = "";
    while (!check('\n')) {
        value += consume();
    }

    // Create the data
    if (value == "html" || value == "") {
        data = new DoctypeData(value, DoctypeShorthand::Html);
    } else if (value == "xml") {
        data = new DoctypeData(value, DoctypeShorthand::Xml);
    } else if (value == "transitional") {
        data = new DoctypeData(value, DoctypeShorthand::Transitional);
    } else if (value == "strict") {
        data = new DoctypeData(value, DoctypeShorthand::Strict);
    } else if (value == "frameset") {
        data = new DoctypeData(value, DoctypeShorthand::Frameset);
    } else if (value == "1.1") {
        data = new DoctypeData(value, DoctypeShorthand::OneDotOne);
    } else if (value == "basic") {
        data = new DoctypeData(value, DoctypeShorthand::Basic);
    } else if (value == "mobile") {
        data = new DoctypeData(value, DoctypeShorthand::Mobile);
    } else if (value == "plist") {
        data = new DoctypeData(value, DoctypeShorthand::Plist);
    } else {
        data = new DoctypeData(value, DoctypeShorthand::Other);
    }

    return true;
}

bool Scanner::scanTag(TagData *&data) {
    // Data required for the tag
    String name = "";
    String idLiteral = "";
    String classLiteral = "";
    std::vector<Attribute> attributes = std::vector<Attribute>();
    bool forcedVoidElement = false;
    String *text = new String("");

    // Get the tag name
    if (isIdentifierPart()) {
        while (isIdentifierPart()) {
            name += consume();
        }
    } else if (check('#') || check('.')) {
        name = "div";
    }

    // ID Literal
    if (check('#')) {
        ignore();

        while (isIdentifierPart()) {
            idLiteral += consume();
        }
    }

    // Class Literal
    if (check('.') && !check(".\n")) {
        ignore();

        while (isIdentifierPart()) {
            classLiteral += consume();
        }
    }

    // Add the class literal if it exists (should be first attribute)
    if (classLiteral != "") {
        attributes.push_back(Attribute("class", classLiteral));
    }

    // Add the id literal if it exists (should be after the class literal)
    if (idLiteral != "") {
        attributes.push_back(Attribute("id", idLiteral));
    }

    // Scan attributes if there are any (should be after the id literal)
    if (check('(')) {
        // Get the additional attributes
        if (!scanTagAttributes(&attributes)) {
            delete text;
            // Error output from `scanTagAttributes()`
            return false;
        }
    }

    // Forced void element, text, block expansion, or nothing
    if (check('/')) {
        ignore();
        forcedVoidElement = true;
    } else if (check(' ') || check(".\n")) {
        if (!scanTagText(text)) {
            delete text;
            // Error output from `scanTagText()`
            return false;
        }
    } else if (!check(':') && !check('\n')) {
        delete text;
        printErrorUnexpectedChar("Error 1-5");
        return false;
    }

    data = new TagData(name, attributes, forcedVoidElement, *text);
    delete text;
    return true;
}

bool Scanner::scanTagAttributes(std::vector<Attribute> *attributes) {
    // Ignore the leading '('
    if (check('(')) {
        ignore();
    } else {
        printErrorUnexpectedChar("Error 1-6");
        return false;
    }

    while (!check(')')) {
        // To allow an empty key with value
        bool quotedAttribute = false;

        // Ignore whitespaces before the attribute
        ignoreWhitespaces(true);

        // Get the key (possibly quoted)
        String key = "";
        if (check('"') || check('\'')) {
            quotedAttribute = true;

            // Get the quote
            char quote = consume()[0];

            // Consume until the quote
            // Backslash escaping doesnt matter
            while (!check(quote)) {
                key += consume();
            }

            // Ignore the closing quote
            ignore();
        } else {
            while (isIdentifierPart()) {
                key += consume();
            }
        }

        // Ignore whitespaces after the key
        ignoreWhitespaces(true);

        // Unescaped?
        bool escaped = true;
        if (check("!")) {
            ignore();
            escaped = false;
        }

        // Ignore the "=" if there is one
        String value = "";
        bool booleanAttribute = false;
        bool checked = false;
        if (check('=')) {
            ignore();

            // Ignore the whitespaces between the "=" and the value
            ignoreWhitespaces(true);

            // Get the value
            if (check('"') || check('\'')) {
                // Get the quote
                char quote = consume()[0];

                // Consume until the quote
                // ToDo: Backslash escaping
                while (!check(quote)) {
                    String c = consume();

                    // Replace bad chars if escaped
                    if (escaped) {
                        if (c == "\"") {
                            c = "&quot;";
                        } else if (c == "<") {
                            c = "&lt;";
                        } else if (c == ">") {
                            c = "&gt;";
                        } else if (c == "&") {
                            c = "&amp;";
                        }
                    }

                    value += c;
                }

                // Ignore the closing quote
                ignore();
            } else if (check('(') || check("True") || check("False")
                       || check("IO_") || isDigit()) {
                bool *exprResult = new bool(false);

                if (!scanExpression(exprResult)) {
                    delete exprResult;
                    // Error output from `scanExpression()`
                    return false;
                }

                booleanAttribute = true;
                checked = *exprResult;
                delete exprResult;
            } else {
                printErrorUnexpectedChar("Error 1-7");
                return false;
            }
        } else {
            // Boolean attribute with no value
            booleanAttribute = true;
            checked = true;
        }

        // Ignore whitespaces and the possible commas after the attribute
        ignoreWhitespaces(true);
        if (check(',')) {
            ignore();
            ignoreWhitespaces(true);

            // Was it an empty space before a comma?
            if (key == "" && !quotedAttribute) {
                attributes->push_back(Attribute());
                continue;
            }
        }

        // Add the attribute
        if (booleanAttribute) {
            attributes->push_back(Attribute(key, checked));
        } else {
            attributes->push_back(Attribute(key, value));
        }
    }

    // Consume the trailing ')'
    ignore();

    return true;
}

bool Scanner::scanTagText(String *value) {
    // Inline in a tag or block in a tag?
    if (check(' ')) {
        // Ignore the leading space
        ignore();

        if (!scanTagTextInline(value)) {
            // Error output from `scanTagTextInline()`
            return false;
        }
    } else if (check(".\n")) {
        // Ignore the leading '.'
        ignore();

        if (!scanTagTextBlock(value)) {
            // Error output from `scanTagTextBlock()`
            return false;
        }
    }

    return true;
}

bool Scanner::scanTagTextInline(String *value) {
    // Depending on if we are in a interpolation
    if (interpolationLevel_ > 0) {
        // Consume until the end of the interpolation or the start of a new interpolation
        while (!check(']') && !check("#[")) {
            if (!scanTagTextPart(value)) {
                // Error output from `scanTagTextPart()`
                return false;
            }
        }
    } else {
        // Consume until the '\n' or a tag interpolation start
        while (!check('\n') && !check("#[")) {
            if (!scanTagTextPart(value)) {
                // Error output from `scanTagTextPart()`
                return false;
            }
        }
    }

    return true;
}

bool Scanner::scanTagTextBlock(String *value) {
    // Consume until the end of the first line
    while (!check('\n') && !check("#[")) {
        if (!scanTagTextPart(value)) {
            // Error output from `scanTagTextPart()`
            return false;
        }
    }

    // While indentation is higher, consume lines
    while (nextLineIndentationIsHigher()) {
        // Consume the '\n' of the current line, ignore the first (except when we are already in a block in a tag)
        if (*value != "" || inBlockInATag_) {
            if (!scanTagTextPart(value)) {
                // Error output from `scanTagTextPart()`
                return false;
            }
        } else {
            ignore();
        }

        // Ignore the whitespace between the '\n' and the next line
        ignoreWhitespaces();

        // Consume the next line up until the '\n' or a tag interpolation start
        while (!check('\n') && !check("#[")) {
            if (!scanTagTextPart(value)) {
                // Error output from `scanTagTextPart()`
                return false;
            }
        }
    }

    // Text is done with no following interpolation
    if (check('\n')) {
        inBlockInATag_ = false;
    } else {
        inBlockInATag_ = true;
    }

    return true;
}

bool Scanner::scanTagTextPart(String *value) {
    if (check("#{IO_")) {
        // Ignore the "#{"
        ignore(2);

        // Get the GPIO value
        uint *gpio = new uint(0);

        if (!scanGPIOValue(gpio)) {
            delete gpio;
            // Error output from `scanGPIOValue()`
            return false;
        }

        *value += String(*gpio);
        delete gpio;

        if (!check("}")) {
            printErrorUnexpectedChar("Error 1-8");
            return false;
        } else {
            ignore();
        }
    } else {
        *value += consume();
    }

    return true;
}

bool Scanner::scanText(TextData *&data) {
    // Type of the text
    if (check('<')) {
        // Error output from `scanTextLiteralHTML()`
        return scanTextLiteralHTML(data);
    } else if (check('|')) {
        // Error output from `scanTextPipedText()`
        return scanTextPipedText(data);
    } else if (check(']')) {
        // Error output from `scanTextInterpolationEnd()`
        return scanTextInterpolationEnd(data);
    } else {
        printErrorUnexpectedChar("Error 1-9");
        return false;
    }
}

bool Scanner::scanTextLiteralHTML(TextData *&data) {
    String value = "";

    // Consume until the '\n'
    while (!check('\n')) {
        value += consume();
    }

    data = new TextData(value, TextType::LiteralHTML);
    return true;
}

bool Scanner::scanTextPipedText(TextData *&data) {
    // Ignore the leading '|' and following whitespaces
    ignore();
    ignoreWhitespaces();

    String *value = new String("");

    if (!scanTagTextInline(value)) {
        delete value;
        // Error output from `scanTagTextInline()`
        return false;
    }

    data = new TextData(*value, TextType::PipedText);
    delete value;
    return true;
}

bool Scanner::scanTextInterpolationEnd(TextData *&data) {
    String *value = new String("");

    // Ignore the leading ']'
    ignore();

    // Cunsume debending on if we are in a block in a tag
    if (inBlockInATag_ && interpolationLevel_ == 0) {
        if (!scanTagTextBlock(value)) {
            delete value;
            // Error output from `scanTagTextBlock()`
            return false;
        }
    } else {
        if (!scanTagTextInline(value)) {
            delete value;
            // Error output from `scanTagTextInline()`
            return false;
        }
    }

    data = new TextData(*value, TextType::InnerText);
    delete value;
    return true;
}

bool Scanner::ignoreComment() {
    // Ignore this line up until the '\n'
    while (!check('\n')) {
        ignore();
    }

    // While indentation is higher, ignore lines
    while (nextLineIndentationIsHigher()) {
        // Ignore the '\n' of the current line
        ignore();

        // Ignore the next line up until the '\n'
        while (!check('\n')) {
            ignore();
        }
    }

    return true;
}

bool Scanner::scanComment(CommentData *&data) {
    // Ignore the leading "//"
    ignore(2);

    // Text of the comment
    String value = "";

    // Consume this line up until the '\n'
    while (!check('\n')) {
        value += consume();
    }

    // Ignore the '\n' of the first line (only to mimic PUG closer)
    bool firstLine = true;

    // While indentation is higher, consume lines
    while (nextLineIndentationIsHigher()) {
        // Consume the '\n' of the current line, ignore it if it is the first line
        if (!firstLine) {
            value += consume();
        } else {
            ignore();
            firstLine = false;
        }

        // Ignore the whitespace between the '\n' and the next line
        ignoreWhitespaces();

        // Consume the next line up until the '\n'
        while (!check('\n')) {
            value += consume();
        }
    }

    data = new CommentData(value);
    return true;
}

bool Scanner::scanInclude(IncludeData *&data) {
    // Ignore the leading "include"
    ignore(7);

    // Ignore whitespaces
    ignoreWhitespaces();

    // Get the path
    String path = "";

    while (!check('\n')) {
        path += consume();
    }

    data = new IncludeData(path);
    return true;
}

bool Scanner::scanGPIOValue(uint *&result) {
    if (check("IO_LED")) {
        ignore(6);
        *result = aalec.get_led();
        return true;
    } else if (check("IO_BUTTON")) {
        ignore(9);
        *result = aalec.get_button();
        return true;
    } else if (check("IO_ROTATE")) {
        ignore(9);
        *result = aalec.get_rotate();
        return true;
    } else if (check("IO_TEMP")) {
        ignore(7);
        *result = aalec.get_temp();
        return true;
    } else if (check("IO_HUMIDITY")) {
        ignore(11);
        *result = aalec.get_humidity();
        return true;
    } else if (check("IO_ANALOG")) {
        ignore(9);
        *result = aalec.get_analog();
        return true;
    } else {
        printErrorUnexpectedChar("Error 1-10");
        return false;
    }
}

bool Scanner::scanExpression(bool *&result) {
    if (check('(')) {
        // Ignore the '(' and following whitespace
        ignore();
        ignoreWhitespaces();

        u32 *firstExprResult = new u32(0);
        bool *firstIsTrue = new bool(false);
        u32 *secondExprResult = new u32(0);
        bool *secondIsTrue = new bool(false);

        if (!scanExpressionEvaluate(firstExprResult, firstIsTrue)) {
            delete firstExprResult;
            delete firstIsTrue;
            delete secondExprResult;
            delete secondIsTrue;
            // Error output from `scanExpressionEvaluate()`
            return false;
        }

        // Ignore the '=' and surounding whitespace
        ignoreWhitespaces();
        if (!check('=')) {
            delete firstExprResult;
            delete firstIsTrue;
            delete secondExprResult;
            delete secondIsTrue;

            printErrorUnexpectedChar("Error 1-11");
            return false;
        } else {
            ignore();
        }
        ignoreWhitespaces();

        if (!scanExpressionEvaluate(secondExprResult, secondIsTrue)) {
            delete firstExprResult;
            delete firstIsTrue;
            delete secondExprResult;
            delete secondIsTrue;
            // Error output from `scanExpressionEvaluate()`
            return false;
        }

        if (*firstIsTrue && *secondIsTrue) {
            *result = true;
        } else if (*firstIsTrue) {
            *result = *secondExprResult != 0;
        } else if (*secondIsTrue) {
            *result = *firstExprResult != 0;
        } else {
            *result = *firstExprResult == *secondExprResult;
        }

        // Ignore the closing ')', and whitespace before it
        ignoreWhitespaces();
        if (!check(')')) {
            delete firstExprResult;
            delete firstIsTrue;
            delete secondExprResult;
            delete secondIsTrue;
            printErrorUnexpectedChar("Error 1-12");
            return false;
        } else {
            ignore();
        }

        delete firstExprResult;
        delete firstIsTrue;
        delete secondExprResult;
        delete secondIsTrue;
        return true;
    } else {
        u32 *exprResult = new u32(0);
        bool *isTrue = new bool(false);

        if (!scanExpressionEvaluate(exprResult, isTrue)) {
            delete exprResult;
            delete isTrue;
            // Error output from `scanExpressionEvaluate()`
            return false;
        }

        *result = *isTrue || *exprResult != 0;

        delete exprResult;
        delete isTrue;
        return true;
    }
}

bool Scanner::scanExpressionEvaluate(u32 *&result, bool *&isTrue) {
    if (check("True")) {
        ignore(4);
        *result = 1;
        *isTrue = true;
        return true;
    } else if (check("False")) {
        ignore(5);
        *result = 0;
        *isTrue = false;
        return true;
    } else if (isDigit()) {
        String value = "";
        while (isDigit()) {
            value += consume();
        }
        *result = value.toInt();
        *isTrue = false;
        return true;
    } else if (check("IO_")) {
        if (!scanGPIOValue(result)) {
            return false;
        }
        *isTrue = false;
        return true;
    } else {
        printErrorUnexpectedChar("Error 1-13");
        return false;
    }
}

bool Scanner::scanConditional() {
    // If it starts with else its part of a conditional where one part was already rendered
    if (check("else")) {
        // While parts of this conditional exist, ignore them
        while (check("else")) {
            // Ignore this line until the '\n'
            while (!check('\n')) {
                ignore();
            }

            while (nextLineIndentationIsHigher()) {
                // Ignore the '\n' of the current line
                ignore();

                // Ignore the next line until the '\n'
                while (!check('\n')) {
                    ignore();
                }
            }
        }

        return true;
    }

    // Ignore a new conditional until a part that should be rendered is encountered
    while (check("if") || check("unless") || check("else")) {
        if (check("if") || check("else if")) {
            // Ignore the "if" (or the "else if") and following whitespaces
            if (check("if")) {
                ignore(2);
            } else {
                ignore(7);
            }
            ignoreWhitespaces();

            // Evaluate the expression
            bool *exprResult = new bool(false);
            if (!scanExpression(exprResult)) {
                delete exprResult;
                // Error output from `scanExpression()`
                return false;
            }

            // Ignore the following whitespace and the ':'
            ignoreWhitespaces();
            if (!check(":\n")) {
                delete exprResult;
                printErrorUnexpectedChar("Error 1-14");
                return false;
            } else {
                ignore();
            }

            // If the exrpession evaluated to true, we are done
            if (*exprResult) {
                delete exprResult;
                indentations_.push_back(Indentation(IndentationType::Conditional
                ));
                return true;
            }

            // Expression is false, ignore all indented parts
            while (nextLineIndentationIsHigher()) {
                // Ignore the '\n' of the current line
                ignore();

                // Ignore the next line until the '\n'
                while (!check('\n')) {
                    ignore();
                }
            }

            // Check if the next line is part of the same conditional
            if (nextLineIsPartOfSameConditional()) {
                // If so ignore the newline and whitespaces till the else
                ignoreWhitespaces(true);
            }
        } else if (check("unless") || check("else unless")) {
            // Ignore the "unless" (or the "else unless") and following whitespaces
            if (check("unless")) {
                ignore(6);
            } else {
                ignore(11);
            }
            ignoreWhitespaces();

            // Evaluate the expression
            bool *exprResult = new bool(false);
            if (!scanExpression(exprResult)) {
                delete exprResult;
                // Error output from `scanExpression()`
                return false;
            }

            // Ignore the following whitespace and the ':'
            ignoreWhitespaces();
            if (!check(":\n")) {
                delete exprResult;
                printErrorUnexpectedChar("Error 1-15");
                return false;
            } else {
                ignore();
            }

            // If the exrpession evaluated to false, we are done
            if (!*exprResult) {
                delete exprResult;
                indentations_.push_back(Indentation(IndentationType::Conditional
                ));
                return true;
            }

            // Expression is true, ignore all indented parts
            while (nextLineIndentationIsHigher()) {
                // Ignore the '\n' of the current line
                ignore();

                // Ignore the next line until the '\n'
                while (!check('\n')) {
                    ignore();
                }
            }

            // Check if the next line is part of the same conditional
            if (nextLineIsPartOfSameConditional()) {
                // If so ignore the newline and whitespaces till the else
                ignoreWhitespaces(true);
            }
        } else if (check("else")) {
            // Ignore the "else", following whitespace, and the ':'
            ignore(4);
            ignoreWhitespaces();
            if (!check(":\n")) {
                printErrorUnexpectedChar("Error 1-16");
                return false;
            } else {
                ignore();
            }

            // We are done
            indentations_.push_back(Indentation(IndentationType::Conditional));
            return true;
        }
    }

    if (!check('\n')) {
        printErrorUnexpectedChar("Error 1-17");
        return false;
    }
    return true;
}
