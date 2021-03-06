#ifndef CTabWebPage_H
#define CTabWebPage_H

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

  void init();
  void term();

  void generate(CFile &file);

 private:
  Type        type_        { Type::TAB };
  Orientation orientation_ { Orientation::HORIZONTAL };
};

#endif
