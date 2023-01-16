#ifndef CTabWebPage_H
#define CTabWebPage_H

#include <string>
#include <vector>
#include <map>

class CFile;

class CTabWebPage {
 public:
  enum class Type {
    NONE,
    TAB,
    ACCORDION,
    IMAGE
  };

  enum class Orientation {
    HORIZONTAL,
    VERTICAL
  };

 public:
  CTabWebPage();

  const Orientation &orientation() const { return orientation_; }
  void setOrientation(const Orientation &o) { orientation_ = o; }

  const std::string &title() const { return title_; }
  void setTitle(const std::string &s) { title_ = s; }

  bool isEmbed() const { return embed_; }
  void setEmbed(bool b) { embed_ = b; }

  bool isFullPage() const { return fullpage_; }
  void setFullPage(bool b) { fullpage_ = b; }

  void addFile(CFile &file);

  void generate();

 private:
  using Lines = std::vector<std::string>;

  struct TabData {
    Type        type      { Type::TAB };
    std::string name;
    int         id        { -1 };
    std::string desc;
    std::string color     { "white" };
    Lines       lines;
    bool        mouseOver { false };

    TabData() = default;

    explicit TabData(const std::string &name_) :
     name(name_) {
    }
  };

  using Tabs = std::vector<TabData *>;

  struct TabGroupData {
    Tabs     tabs;
    int      id         { -1 };
    TabData *currentTab { nullptr };
  };

  using TabGroups = std::vector<TabGroupData *>;

  enum class DocPart {
    HEAD,
    BODY_START,
    BODY_END,
    TAIL
  };

  struct TabFileData {
    std::string   tabTitle;
    DocPart       docPart         { DocPart::BODY_START };
    int           skipN           { 0 };
    Lines         startLines;
    Lines         endLines;
    TabGroups     tabGroups;
    TabGroupData* currentTabGroup { nullptr };
  };

  using TabFiles = std::vector<TabFileData *>;

 private:
  void init();
  void term();

  void generateFile(TabFileData *fileData);

  TabGroupData *addTabGroup(TabFileData *fileData);

  TabData *addTab(TabGroupData *tabGroup, const std::string &tabName, Type tabType);

  void addTypeCss(Type type);
  void addTypeJS (Type type);

 private:
  using TypeCss   = std::map<Type, bool>;
  using TypeJS    = std::map<Type, bool>;
  using TypeCount = std::map<Type, int>;

  Orientation  orientation_ { Orientation::HORIZONTAL };
  std::string  title_;
  bool         embed_       { false };
  bool         fullpage_    { false };
  TypeCss      typeCss_;
  TypeJS       typeJS_;
  TypeCount    tabTypeCount_;
  TabFiles     tabFiles_;
  TabFileData* currentFile_ { nullptr };
  int          tabCount_    { 0 };
  int          groupCount_  { 0 };
};

#endif
