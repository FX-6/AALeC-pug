#ifndef TOKEN_H
#define TOKEN_H

#include <Arduino.h>

#include <vector>

/**
 * @brief All types of tokens that can be found in the source code
 */
enum class TokenType {
    Indent,
    Dedent,
    EndOfPart,
    EndOfSource,
    Doctype,
    Tag,
    Text,
    Comment,
    Include,
};

/**
 * @brief Different types of doctypes
 */
enum class DoctypeShorthand {
    Html,
    Xml,
    Transitional,
    Strict,
    Frameset,
    OneDotOne,
    Basic,
    Mobile,
    Plist,
    Other,
};

/**
 * @brief Data about a doctype token
 */
class DoctypeData {
   public:
    /**
     * @brief The value of the doctype as found in the source code
     */
    String value;

    /**
     * @brief The type of the doctype
     */
    DoctypeShorthand doctypeType;

    /**
     * @brief Construct a new empty Doctype Data object
     */
    DoctypeData();

    /**
     * @brief Construct a new Doctype Data object
     *
     * @param value
     * @param doctypeType
     */
    DoctypeData(String value, DoctypeShorthand doctypeType);

    /**
     * @brief Convert the doctype token to an HTML string
     *
     * @return String The HTML string representation of the doctype token
     *                     eg: `<!DOCTYPE html>` or `<?xml version="1.0" encoding="utf-8" ?>`
     */
    String toHTMLString();
};

/**
 * @brief An attribute of a tag token
 */
class Attribute {
   public:
    /**
     * @brief The key of the attribute
     */
    String key;

    /**
     * @brief When the attribute is not a boolean attribute:
     *        The value of the attribute
     */
    String value;

    /**
     * @brief If the attribute is a boolean attribute
     */
    bool booleanAttribute;

    /**
     * @brief When the attribute is a boolean attribute:
     *        Wheter the attribute is checked or not
     */
    bool checked;

    /**
     * @brief If the attribute was generated from an empty commy
     */
    bool emptyAttribute;

    /**
     * @brief Construct a new empty Attribute object
     */
    Attribute();

    /**
     * @brief Construct a new Attribute object, when its not a boolean attribute
     *
     * @param key
     * @param value
     */
    Attribute(String key, String value);

    /**
     * @brief Construct a new Attribute object, when its a boolean attribute
     *
     * @param key
     * @param checked
     */
    Attribute(String key, bool checked);
};

/**
 * @brief Data about a generic tag token
 */
class TagData {
   public:
    /**
     * @brief The name of the tag
     */
    String name;

    /**
     * @brief The attributes of the tag
     */
    std::vector<Attribute> attributes;

    /**
     * @brief If the tag is forced to be a void element
     */
    bool isVoidElement;

    /**
     * @brief The inner text of the tag
     */
    String text;

    /**
     * @brief Construct a new empty Tag Data object
     */
    TagData();

    /**
     * @brief Construct a new Tag Data object
     *
     * @param name
     * @param attributes
     * @param isVoidElement
     * @param text
     */
    TagData(
        String name,
        std::vector<Attribute> attributes,
        bool isVoidElement,
        String text
    );
};

/**
 * @brief Data about a plain text token
 */
class TextData {
   public:
    /**
     * @brief The value of the token
     */
    String value;

    /**
     * @brief If the text was after a pipe
     */
    bool pipeText;

    /**
     * @brief Construct a new empty Text Data object
     */
    TextData();

    /**
     * @brief Construct a new Text Data object
     *
     * @param value
     */
    TextData(String value, bool pipeText);
};

/**
 * @brief Data about a comment token
 */
class CommentData {
   public:
    /**
     * @brief The value of the comment
     */
    String value;

    /**
     * @brief Construct a new empty Comment Data object
     */
    CommentData();

    /**
     * @brief Construct a new Comment Data object
     *
     * @param value
     */
    CommentData(String value);
};

/**
 * @brief Data about an include token
 */
class IncludeData {
   public:
    /**
     * @brief The path of the include
     */
    String path;

    /**
     * @brief Construct a new empty Include Data object
     */
    IncludeData();

    /**
     * @brief Construct a new Include Data object
     *
     * @param path
     */
    IncludeData(String path);
};

/**
 * @brief A token in the source code
 */
class Token {
   public:
    /**
     * @brief The type of the token
     */
    TokenType type;

    /**
     * @brief Specific data for the token
     */
    // ToDo: Use union for optimization?
    // union {
    DoctypeData *doctype;
    TagData *tag;
    TextData *text;
    CommentData *comment;
    IncludeData *include;
    // };

    /**
     * @brief Construct a new Token object, with the given type, start and end.
     *
     * @param type Type of the Token
     */
    Token(TokenType type);

    /**
     * @brief Construct a new Token object, with the given doctype data, start and end.
     *
     * @param data Data about the Doctype Token
     */
    Token(DoctypeData *data);

    /**
     * @brief Construct a new Token object, with the given tag data, start and end.
     *
     * @param data Data about the Tag Token
     */
    Token(TagData *data);

    /**
     * @brief Construct a new Token object, with the given text data, start and end.
     *
     * @param data Data about the Text Token
     */
    Token(TextData *data);

    /**
     * @brief Construct a new Token object, with the given comment data, start and end.
     *
     * @param data Data about the Comment Token
     */
    Token(CommentData *data);

    /**
     * @brief Construct a new Token object, with the given include data, start and end.
     *
     * @param data Data about the Include Token
     */
    Token(IncludeData *data);
};

#endif  // TOKEN_H
