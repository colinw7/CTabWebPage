#ifndef CTabWebPage_H
#define CTabWebPage_H

#include <string>

class CFile;

class CTabWebPage {
 public:
  enum class Type {
    TAB,
    ACCORDION,
    IMAGES
  };

  enum class Orientation {
    HORIZONTAL,
    VERTICAL
  };

 public:
  CTabWebPage();

  const Type &type() const { return type_; }
  void setType(const Type &t) { type_ = t; }

  const Orientation &orientation() const { return orientation_; }
  void setOrientation(const Orientation &o) { orientation_ = o; }

  const std::string &title() const { return title_; }
  void setTitle(const std::string &s) { title_ = s; }

  bool isEmbed() const { return embed_; }
  void setEmbed(bool b) { embed_ = b; }

  bool isFullPage() const { return fullpage_; }
  void setFullPage(bool b) { fullpage_ = b; }

  void init();
  void term();

  void generate(CFile &file);

 private:
  Type        type_        { Type::TAB };
  Orientation orientation_ { Orientation::HORIZONTAL };
  std::string title_;
  bool        embed_       { false };
  bool        fullpage_    { false };
};

#endif
