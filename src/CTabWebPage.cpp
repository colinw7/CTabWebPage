#include <CTabWebPage.h>
#include <CFile.h>
#include <CStrParse.h>

#include <iostream>
#include <sstream>
#include <cassert>

template <typename T>
void sstrConcat(std::ostream &o, T t) { // final template
  o << t;
}

template<typename T, typename... Args>
void sstrConcat(std::ostream &o, T t, Args... args) { // recursive variadic function
  sstrConcat(o, t);
  sstrConcat(o, args...);
}

template<typename... Args>
std::string strConcat(Args... args) {
  std::ostringstream ss;
  sstrConcat(ss, args...);
  return ss.str();
}

auto errMsg = [](const std::string &msg) {
  std::cerr << msg << "\n";
  exit(1);
};

//---

int
main(int argc, char **argv)
{
  using Files = std::vector<std::string>;

  Files files;

  CTabWebPage::Orientation orientation = CTabWebPage::Orientation::HORIZONTAL;

  std::string title;

  bool embed    = false;
  bool fullpage = false;

  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      auto opt = std::string(&argv[i][1]);

      if      (opt == "orientation" || opt == "orient") {
        ++i;

        if (i < argc) {
          auto orientationName = std::string(argv[i]);

          if      (orientationName == "h" || orientationName == "horizontal")
            orientation = CTabWebPage::Orientation::HORIZONTAL;
          else if (orientationName == "v" || orientationName == "vertical")
            orientation = CTabWebPage::Orientation::VERTICAL;
          else
            errMsg(strConcat("Invalid orientation '", orientationName, "'"));
        }
        else
          errMsg("Missing arg for -type");
      }
      else if (opt == "title") {
        ++i;

        if (i < argc)
          title = std::string(argv[i]);
      }
      else if (opt == "fullpage" || opt == "full_page") {
        fullpage = true;
      }
      else if (opt == "embed") {
        embed = true;
      }
      else if (opt == "h" || opt == "help") {
        std::cout << "CWebPage"
          " [-type tab|accordion|images]"
          " [-orientation horizontal|vertical]" <<
          " [-title <title>]" <<
          " [-fullpage]" <<
          " [-embed]" <<
          "\n";
        return 0;
      }
      else
        errMsg(strConcat("Invalid option '", argv[i], "'"));
    }
    else {
      files.push_back(std::string(argv[i]));
    }
  }

  CTabWebPage page;

  page.setOrientation(orientation);
  page.setTitle(title);
  page.setEmbed(embed);
  page.setFullPage(fullpage);

  for (const auto &file : files) {
    CFile f(file);

    if (! f.exists())
      errMsg(strConcat("'", file, "' does not exist"));

    page.addFile(f);
  }

  page.generate();

  return 0;
}

//------

CTabWebPage::
CTabWebPage()
{
}

void
CTabWebPage::
generate()
{
  init();

  for (const auto &tabFile : tabFiles_) {
    generateFile(tabFile);
  }

  term();
}

void
CTabWebPage::
init()
{
  if (! isEmbed()) {
    std::cout << "<!DOCTYPE html>\n";
    std::cout << "<html>\n";
    std::cout << "<head>\n";

    if (title() != "")
      std::cout << "<title>" << title() << "</title>\n";
  }

  if (! tabTypeCount_.empty()) {
    std::cout << "\n";

    for (const auto &pt : tabTypeCount_)
      addTypeCss(pt.first);

    std::cout << "\n";
  }

  if (! isEmbed()) {
    std::cout << "</head>\n";
    std::cout << "<body>\n";

    if (title() != "")
      std::cout << "<h1>" << title() << "</h1>\n";
  }
}

void
CTabWebPage::
term()
{
  if (! tabTypeCount_.empty()) {
    std::cout << "\n";

    for (const auto &pt : tabTypeCount_)
      addTypeJS(pt.first);

    std::cout << "\n";
  }

  if (! isEmbed()) {
    std::cout << "</body>\n";
    std::cout << "</html>\n";
  }
}

void
CTabWebPage::
addTypeCss(Type type)
{
  if (! typeCss_[type]) {
    if      (type == Type::TAB) {
      if (isFullPage()) {
        if (orientation() == Orientation::HORIZONTAL)
          std::cout << "<link href=\"full_htabs.css\" rel=\"stylesheet\" type=\"text/css\"/>\n";
      }
      else {
        if (orientation() == Orientation::HORIZONTAL)
          std::cout << "<link href=\"htabs.css\" rel=\"stylesheet\" type=\"text/css\"/>\n";
        else
          std::cout << "<link href=\"vtabs.css\" rel=\"stylesheet\" type=\"text/css\"/>\n";
      }
    }
    else if (type == Type::ACCORDION) {
      std::cout << "<link href=\"accordion.css\" rel=\"stylesheet\" type=\"text/css\"/>\n";
    }
    else if (type == Type::IMAGE) {
      std::cout << "<link href=\"images.css\" rel=\"stylesheet\" type=\"text/css\"/>\n";
    }

    typeCss_[type] = true;
  }
}

void
CTabWebPage::
addTypeJS(Type type)
{
  if (! typeJS_[type]) {
    if      (type == Type::TAB) {
      if (isFullPage())
        std::cout << "<script src=\"full_tabs.js\" type=\"text/javascript\"></script>\n";
      else
        std::cout << "<script src=\"tabs.js\" type=\"text/javascript\"></script>\n";
    }
    else if (type == Type::ACCORDION)
      std::cout << "<script src=\"accordion.js\" type=\"text/javascript\"></script>\n";
    else if (type == Type::IMAGE)
      std::cout << "<script src=\"images.js\" type=\"text/javascript\"></script>\n";

    typeJS_[type] = true;
  }
}

void
CTabWebPage::
addFile(CFile &file)
{
  currentFile_ = new TabFileData;

  tabFiles_.push_back(currentFile_);

  auto *currentTabGroup = addTabGroup(currentFile_);

  //---

  CFile::Lines lines;

  file.toLines(lines);

  //---

  using NameValues = std::map<std::string, std::string>;

  auto decodeNameValues = [](const std::string &str, NameValues &nameValues) {
    //std::cerr << "parse: '" << str << "'\n";

    CStrParse parse(str);

    while (! parse.eof()) {
      parse.skipSpace();

      if (parse.eof())
        break;

      int pos1 = parse.getPos();

      while (! parse.eof() && ! parse.isChar('='))
        parse.skipChar();

      auto name = parse.getBefore(pos1);

      if (parse.isChar('='))
        parse.skipChar();

      parse.skipSpace();

      std::string value;

      if (parse.isChar('"') || parse.isChar('\''))
        parse.readString(value, /*strip_quotes*/true);
      else {
        int pos2 = parse.getPos();

        parse.skipNonSpace();

        value = parse.getBefore(pos2);
      }

      //std::cerr << "add name value: '" << name << "' '" << value << "'\n";

      nameValues[name] = value;
    }
  };

  auto getNameValue = [](const std::string &name, const NameValues &nameValues) {
    auto p = nameValues.find(name);
    if (p == nameValues.end()) return std::string();
    return (*p).second;
  };

  auto isCommentTag = [](const std::string &line, const std::string &tag,
                         std::string &matchTag, std::string &nameValuesStr) {
    auto commentLine = "<!-- " + tag + " -->";

    if (line == commentLine) {
      nameValuesStr = "";
      return true;
    }

    int tagLen  = tag.length();
    int lineLen = line.length();

    if (lineLen <= tagLen + 10)
      return false;

    if (line.substr(0, 5) != "<!-- " || line.substr(lineLen - 4) != " -->")
      return false;

    if (line.substr(5, tagLen) != tag || line[tagLen + 5] != ':')
      return false;

    matchTag      = tag;
    nameValuesStr = line.substr(tagLen + 6, lineLen - 9);

    return true;
  };

  //---

  for (const auto &line : lines) {
    if (currentFile_->skipN > 0) {
      --currentFile_->skipN;
      continue;
    }

    //---

    std::string matchTag;
    std::string nameValuesStr;

    // start head lines
    if (isCommentTag(line, "CTAB_HEAD", matchTag, nameValuesStr)) {
      NameValues nameValues;

      decodeNameValues(nameValuesStr, nameValues);

      auto title = getNameValue("title", nameValues);

      if (title != "")
        currentFile_->tabTitle = title;

      currentFile_->docPart = DocPart::HEAD;

      currentTabGroup->currentTab = nullptr;

      continue;
    }

    //---

    // start tail lines
    if (isCommentTag(line, "CTAB_TAIL", matchTag, nameValuesStr)) {
      currentFile_->docPart = DocPart::TAIL;
      currentTabGroup->currentTab = nullptr;
      continue;
    }

    // start body lines
    if (isCommentTag(line, "CTAB_BODY", matchTag, nameValuesStr)) {
      if      (currentFile_->docPart == DocPart::HEAD)
        currentFile_->docPart = DocPart::BODY_START;
      else if (currentFile_->docPart == DocPart::TAIL)
        errMsg("Can't have body contents after tail");
      else
        currentFile_->docPart = DocPart::BODY_END;

      currentTabGroup->currentTab = nullptr;

      continue;
    }

    //---

    if (isCommentTag(line, "CTAB_SKIP", matchTag, nameValuesStr)) {
      if (nameValuesStr == "")
        currentFile_->skipN = 1;
      else {
        try {
          currentFile_->skipN = stoi(nameValuesStr);
        }
        catch (...) {
          currentFile_->skipN = -1;
        }

        if (currentFile_->skipN <= 0)
          errMsg(strConcat("Invalid skip value '", nameValuesStr, "'"));
      }

      continue;
    }

    //---

    // new tab group
    if      (isCommentTag(line, "CTAB_GROUP", matchTag, nameValuesStr)) {
      currentTabGroup = addTabGroup(currentFile_);
    }
    // start tab lines
    else if (isCommentTag(line, "CTAB_TAB"      , matchTag, nameValuesStr) ||
             isCommentTag(line, "CTAB_ACCORDION", matchTag, nameValuesStr) ||
             isCommentTag(line, "CTAB_IMAGE"    , matchTag, nameValuesStr)) {
      NameValues nameValues;

      decodeNameValues(nameValuesStr, nameValues);

      Type tabType = Type::TAB;

      if      (matchTag == "CTAB_ACCORDION")
        tabType = Type::ACCORDION;
      else if (matchTag == "CTAB_IMAGE")
        tabType = Type::IMAGE;

      if      (tabType == Type::TAB || tabType == Type::ACCORDION) {
        auto tabName = getNameValue("name", nameValues);

        if (tabName == "")
          tabName = strConcat("Tab", currentTabGroup->tabs.size() + 1);

        //---

        // get contents filename
        auto fileName = getNameValue("file", nameValues);

        //---

        // get color
        auto colorName = getNameValue("color", nameValues);

        if (colorName == "")
          colorName = "white";

        //---

        auto mouseOverStr = getNameValue("mouseOver", nameValues);

        bool mouseOver = (mouseOverStr == "1" || mouseOverStr == "true" || mouseOverStr == "yes");

        //---

        auto *tab = addTab(currentTabGroup, tabName, tabType);

        tab->color = colorName;

        if (fileName != "") {
          CFile file1(fileName);

          if (! file1.exists())
            errMsg(strConcat("'", fileName, "' does not exist"));

          CFile::Lines lines1;

          file1.toLines(lines1);

          for (const auto &line1 : lines1)
            tab->lines.push_back(line1);
        }

        tab->mouseOver = mouseOver;
      }
      else if (tabType == Type::IMAGE) {
        auto imageName = getNameValue("image", nameValues);

        if (imageName == "")
          errMsg("Invalid image name");

        auto desc = getNameValue("desc", nameValues);

        if (desc == "")
          desc = imageName;

        auto *tab = addTab(currentTabGroup, imageName, Type::IMAGE);

        tab->desc = desc;
      }
    }
    else {
      if (currentTabGroup->currentTab)
        currentTabGroup->currentTab->lines.push_back(line);
      else {
        if      (currentFile_->docPart == DocPart::BODY_START)
          currentFile_->startLines.push_back(line);
        else if (currentFile_->docPart == DocPart::BODY_END)
          currentFile_->endLines.push_back(line);
      }
    }
  }
}

CTabWebPage::TabGroupData *
CTabWebPage::
addTabGroup(TabFileData *fileData)
{
  fileData->currentTabGroup = new TabGroupData;

  fileData->currentTabGroup->id = ++groupCount_;

  fileData->tabGroups.push_back(fileData->currentTabGroup);

  return fileData->currentTabGroup;
}

CTabWebPage::TabData *
CTabWebPage::
addTab(TabGroupData *tabGroup, const std::string &tabName, Type tabType)
{
  auto *tab = new TabData(tabName);

  tab->type = tabType;

  tab->id = ++tabCount_;

  tabGroup->tabs.push_back(tab);

  tabTypeCount_[tab->type]++;

  tabGroup->currentTab = tab;

  return tab;
}

void
CTabWebPage::
generateFile(TabFileData *fileData)
{
  for (const auto &line : fileData->startLines)
    std::cout << line << "\n";

  if (fileData->tabTitle != "")
    std::cout << "<h2>" << fileData->tabTitle << "</h2>\n";

  //---

  int numSlides = 0;

  for (const auto &tabGroup : fileData->tabGroups) {
    if (tabGroup->tabs.empty())
      continue;

    auto type = tabGroup->tabs[0]->type;

    if      (type == Type::TAB) {
      std::cout << "\n";
      std::cout << "<!-- Tab buttons -->\n";
      std::cout << "<div class=\"tab_buttons\">\n";

      bool firstTab = true;

      for (const auto &tab : tabGroup->tabs) {
        if (tab->type != Type::TAB)
          continue;

        std::cout << "  <button class=\"tab_button\"";

        if (! tab->mouseOver) {
          if (isFullPage())
            std::cout << " onclick=\"openTab(event, '" << tab->name <<
                                             "', this, '" << tab->color << "')\"";
          else
            std::cout << " onclick=\"openTab(event, '" << tab->name <<
                                             "', this, '" << tab->color << "')\"";
        }
        else {
          std::cout << " onmouseover=\"openTab(event, '" << tab->name <<
                                             "', this, '" << tab->color << "')\"";
        }

        if (firstTab)
          std::cout << " id=\"defaultOpen\"";

        std::cout << ">" << tab->name << "</button>\n";

        firstTab = false;
      }

      std::cout << "</div>\n";

      std::cout << "\n";
      std::cout << "<!-- Tab content -->\n";

      for (const auto &tab : tabGroup->tabs) {
        if (tab->type != Type::TAB)
          continue;

        std::cout << "<div id=\"" << tab->name << "\" class=\"tab_content\">\n";

        //std::cout << "<h3>" << tab->name << "</h3>\n";

        for (const auto &line1 : tab->lines)
          std::cout << line1 << "\n";

        std::cout << "</div>\n";
      }

      std::cout << "\n";

      //std::cout << "<script>\n";
      //std::cout << "document.getElementById(\"defaultOpen\").click();\n";
      //std::cout << "</script>\n";
    }
    else if (type == Type::ACCORDION) {
      bool firstAccordian = true;

      std::cout << "<!-- Tab buttons and content -->\n";

      for (const auto &tab : tabGroup->tabs) {
        if (tab->type != Type::ACCORDION)
          continue;

        std::cout << "<button class=\"accordion_button\"";

        if (firstAccordian)
          std::cout << " id=\"defaultOpen\"";

        std::cout << ">" << tab->name << "</button>\n";

        firstAccordian = false;

        //---

        std::cout << "<div class=\"accordian_panel\">\n";

        for (const auto &line1 : tab->lines)
          std::cout << line1 << "\n";

        std::cout << "</div>\n";
      }
    }
    else if (type == Type::IMAGE) {
      ++numSlides;

      std::cout << "\n";
      std::cout << "<!-- Image container -->\n";
      std::cout << "<div class=\"image_container\">\n";
      std::cout << "\n";
      std::cout << "<!-- Full-width images with number and caption text -->\n";

      int n = tabGroup->tabs.size();

      int i = 1;

      std::string slidesName = strConcat("image_slides_", numSlides);

      for (const auto &tab : tabGroup->tabs) {
        std::cout << "<div class=\"" << slidesName << " image_slides image_fade\">\n";
        std::cout << "  <div class=\"image_number\">" << i << " / " << n << "</div>\n";
        std::cout << "  <img src=\"" << tab->name << "\" style=\"width:100%\">\n";
        std::cout << "  <div class=\"image_text\">" << tab->desc << "</div>\n";
        std::cout << "</div>\n";
        std::cout << "\n";

        ++i;
      }

      std::string plusFnName  = strConcat("plusSlides(", numSlides, ", 1)");
      std::string minusFnName = strConcat("plusSlides(", numSlides, ", -1)");

      std::cout << "<!-- Next and previous buttons -->\n";
      std::cout << "<a class=\"image_prev\" onclick=\"" << plusFnName << "\">&#10094;</a>\n";
      std::cout << "<a class=\"image_next\" onclick=\"" << plusFnName << "\">&#10095;</a>\n";
      std::cout << "</div>\n";
      std::cout << "<br>\n";
      std::cout << "\n";
      std::cout << "<!-- The dots/circles -->\n";
      std::cout << "<div style=\"text-align:center\">\n";

      std::string dotName = strConcat("image_dot_", numSlides);

      i = 1;

      for ( ; i <= n; ++i) {
        std::string slideFnName = strConcat("currentSlide(", numSlides, ", ", i, ")");

        std::cout << "  <span class=\"" << dotName << " image_dot\"" <<
                     " onclick=\"" << slideFnName << "\"></span>\n";
      }

      std::cout << "</div> \n";
    }
  }

  for (const auto &line : fileData->endLines)
    std::cout << line << "\n";
}
