# PUG Compiler for the AALeC

## The AALeC

This project is build for the [AALeC](https://github.com/informatik-aalen/AALeC-Hardware) and relies on [AALeC-V2](https://github.com/informatik-aalen/AALeC-V2).

## Using this project

### Arduino IDE

1. Download the code as a ZIP file ([see how](https://docs.github.com/en/repositories/working-with-files/using-files/downloading-source-code-archives)).
2. Add the ZIP file to your Arduino IDE ([see how](https://support.arduino.cc/hc/en-us/articles/5145457742236-Add-libraries-to-Arduino-IDE#importing-a-.zip-library)).

### PlatformIO

1. Add `lib_deps = https://github.com/FX-6/AALeC-pug` to your `platformio.ini` file.

## Features

Done: 🟩 \
ToDo: 🟨 \
Wont do: 🟥

- 🟩 Attributes
  - 🟩 Basic
  - 🟩 Multiline Attributes
  - 🟩 Quoted Attributes
  - 🟥 ~~Attribute Interpolation~~
  - 🟩 Unescaped Attributes
  - 🟩 Boolean Attributes
  - 🟥 ~~Style Attributes~~
  - 🟥 ~~Class Attributes~~
  - 🟩 Class Literal
  - 🟩 ID Literal
  - 🟥 ~~&attributes~~
- 🟥 ~~Case~~
  - 🟥 ~~Basic~~
  - 🟥 ~~Case Fall Through~~
  - 🟥 ~~Block Expansion~~
- 🟥 ~~Code~~
  - 🟥 ~~Unbufferd Code~~
  - 🟥 ~~Buffered Code~~
  - 🟥 ~~Unescaped Buffered Code~~
- 🟩 Comments
  - 🟩 Basic
  - 🟩 Block Comments
  - 🟩 Conditional Comments
- 🟨 Conditionals
  - 🟥 ~~Basic~~
  - 🟨 GPIO Pin Conditionals
- 🟩 Doctype
  - 🟩 Doctype Shortcuts
  - 🟩 Custom Doctypes
  - 🟩 Doctype Option
- 🟥 ~~Filters~~
  - 🟥 ~~Basic~~
  - 🟥 ~~Inline Syntax~~
  - 🟥 ~~Filtered Includes~~
  - 🟥 ~~Nested Filters~~
  - 🟥 ~~Custom Filters~~
- 🟩 Includes
  - 🟩 Basic
  - 🟩 Including Plain Text
  - 🟥 ~~Including Filtered Text~~
- 🟥 ~~Inheritance: Extends and Block~~
  - 🟥 ~~Basic~~
  - 🟥 ~~Block append / prepend~~
- 🟩 Interpolation
  - 🟥 ~~String Interpolation, Escaped~~
  - 🟥 ~~String Interpolation, Unescaped~~
  - 🟩 Tag Interpolation
- 🟥 ~~Iteration~~
  - 🟥 ~~each~~
  - 🟥 ~~while~~
- 🟥 ~~Mixins~~
  - 🟥 ~~Basic~~
  - 🟥 ~~Mixin Blocks~~
  - 🟥 ~~Mixin Attributes~~
  - 🟥 ~~Default Argument's Values~~
  - 🟥 ~~Rest Arguments~~
- 🟩 Plain Text
  - 🟩 Inline in a Tag
  - 🟩 Literal HTML
  - 🟩 Piped Text
  - 🟩 Block in a Tag
- 🟩 Tags
  - 🟩 Basic
  - 🟩 Block Expansion
  - 🟩 Self-Closing Tags
