#include <unordered_map>
#include <iostream>

//At some point the result of this whole process should be
//to return a token which would be something like
// A Char/Num/Symbol(Token?) -> Its meaning
class Lexer {

  // Should contain all kind char(single tokens) tokens
  enum CHAR_TYPE {
    ALPHABETIC_CHAR,
    DIGIT,
    PUNCTUATOR, // comma, dot ?
    WHITESPACE,
    LINEBREAK
  };

  //members
  //In theory Token should be


  // Should assign each type with its respective type
  // for example ['.'] = PUNCTUATOR
  void setup_char_types(std::unordered_map<char, CHAR_TYPE>&);

  public:
  Lexer() = default;

  
  std::vector<Token> tokenize(const std::string& text);




};
