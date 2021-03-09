#include <CTabWebPage.h>
#include <CFile.h>
#include <CStrParse.h>
#include <map>
#include <iostream>
#include <sstream>

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

  CTabWebPage::Type        type        = CTabWebPage::Type::TAB;
  CTabWebPage::Orientation orientation = CTabWebPage::Orientation::HORIZONTAL;

  std::string title;

  bool embed    = false;
  bool fullpage = false;

  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      auto opt = std::string(&argv[i][1]);

      if      (opt == "type") {
        ++i;

        if (i < argc) {
          auto typeName = std::string(argv[i]);

          if      (typeName == "tab")
            type = CTabWebPage::Type::TAB;
          else if (typeName == "accordion")
            type = CTabWebPage::Type::ACCORDION;
          else if (typeName == "images")
            type = CTabWebPage::Type::IMAGES;
          else
            errMsg(strConcat("Invalid type '", typeName, "'"));
        }
        else
          errMsg("Missing arg for -type");
      }
      else if (opt == "orientation" || opt == "orient") {
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

  page.setType(type);
  page.setOrientation(orientation);
  page.setTitle(title);
  page.setEmbed(embed);
  page.setFullPage(fullpage);

  page.init();

  for (const auto &file : files) {
    CFile f(file);

    if (! f.exists())
      errMsg(strConcat("'", file, "' does not exist"));

    page.generate(f);
  }

  page.term();

  return 0;
}

CTabWebPage::
CTabWebPage()
{
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

  if      (type() == Type::TAB) {
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
  else if (type() == Type::ACCORDION) {
    std::cout << "<link href=\"accordion.css\" rel=\"stylesheet\" type=\"text/css\"/>\n";
  }
  else if (type() == Type::IMAGES) {
    std::cout << "<link href=\"images.css\" rel=\"stylesheet\" type=\"text/css\"/>\n";
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
  if      (type() == Type::TAB) {
    if (isFullPage())
      std::cout << "<script src=\"full_tabs.js\" type=\"text/javascript\"></script>\n";
    else
      std::cout << "<script src=\"tabs.js\" type=\"text/javascript\"></script>\n";
  }
  else if (type() == Type::ACCORDION)
    std::cout << "<script src=\"accordion.js\" type=\"text/javascript\"></script>\n";
  else if (type() == Type::IMAGES)
    std::cout << "<script src=\"images.js\" type=\"text/javascript\"></script>\n";

  if (! isEmbed()) {
    std::cout << "</body>\n";
    std::cout << "</html>\n";
  }
}

void
CTabWebPage::
generate(CFile &file)
{
  CFile::Lines lines;

  file.toLines(lines);

  using Lines = std::vector<std::string>;

  struct TabData {
    std::string name;
    std::string desc;
    std::string color { "white" };
    Lines       lines;

    TabData() = default;

    explicit TabData(const std::string &name) :
     name(name) {
    }
  };

  int skipN = 0;

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

  enum class DocPart {
    HEAD,
    BODY_START,
    BODY_END,
    TAIL
  };

  using Tabs = std::vector<TabData *>;

  Lines       startLines, endLines;
  Tabs        tabs;
  TabData*    currentTab = nullptr;
  DocPart     docPart { DocPart::BODY_START };
  std::string tabTitle;

  auto isCommentTag = [](const std::string &line, const std::string &tag,
                         std::string &nameValuesStr) {
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

    nameValuesStr = line.substr(tagLen + 6, lineLen - 9);

    return true;
  };

  for (const auto &line : lines) {
    if (skipN > 0) {
      --skipN;
      continue;
    }

    //---

    // start head lines
    std::string nameValuesStr;

    if (isCommentTag(line, "CTAB_HEAD", nameValuesStr)) {
      NameValues nameValues;

      decodeNameValues(nameValuesStr, nameValues);

      auto title = getNameValue("title", nameValues);

      if (title != "")
        tabTitle = title;

      docPart = DocPart::HEAD;

      currentTab = nullptr;

      continue;
    }

    //---

    // start tail lines
    if (isCommentTag(line, "CTAB_TAIL", nameValuesStr)) {
      docPart = DocPart::TAIL;
      currentTab = nullptr;
      continue;
    }

    // start body lines
    if (isCommentTag(line, "CTAB_BODY", nameValuesStr)) {
      if      (docPart == DocPart::HEAD)
        docPart = DocPart::BODY_START;
      else if (docPart == DocPart::TAIL)
        errMsg("Can't have body contents after tail");
      else
        docPart = DocPart::BODY_END;

      currentTab = nullptr;

      continue;
    }

    //---

    if (isCommentTag(line, "CTAB_SKIP", nameValuesStr)) {
      if (nameValuesStr == "")
        skipN = 1;
      else {
        try {
          skipN = stoi(nameValuesStr);
        }
        catch (...) {
          skipN = -1;
        }

        if (skipN <= 0)
          errMsg(strConcat("Invalid skip value '", nameValuesStr, "'"));
      }

      continue;
    }

    //---

    // start tab lines
    if (isCommentTag(line, "CTAB_TAB", nameValuesStr)) {
      NameValues nameValues;

      decodeNameValues(nameValuesStr, nameValues);

      if      (type() == Type::TAB || type() == Type::ACCORDION) {
        auto tabName = getNameValue("name", nameValues);

        if (tabName == "")
          tabName = strConcat("Tab", tabs.size() + 1);

        auto fileName = getNameValue("file", nameValues);

        auto colorName = getNameValue("color", nameValues);

        if (colorName == "")
          colorName = "white";

        auto *tab = new TabData(tabName);

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

        tabs.push_back(tab);

        currentTab = tab;
      }
      else if (type() == Type::IMAGES) {
        auto imageName = getNameValue("image", nameValues);

        if (imageName == "")
          errMsg("Invalid image name");

        auto desc = getNameValue("desc", nameValues);

        if (desc == "")
          desc = imageName;

        auto *tab = new TabData(imageName);

        tab->desc = desc;

        tabs.push_back(tab);

        currentTab = tab;
      }
    }
    else {
      if (currentTab)
        currentTab->lines.push_back(line);
      else {
        if      (docPart == DocPart::BODY_START)
          startLines.push_back(line);
        else if (docPart == DocPart::BODY_END)
          endLines.push_back(line);
      }
    }
  }

  for (const auto &line : startLines)
    std::cout << line << "\n";

  if (tabTitle != "")
    std::cout << "<h2>" << tabTitle << "</h2>\n";

  if      (type() == Type::TAB) {
    std::cout << "<!-- Tab buttons -->\n";
    std::cout << "<div class=\"tab\">\n";

    bool first = true;

    for (const auto &tab : tabs) {
      std::cout << "  <button class=\"tabs\"";

      if (type() == Type::TAB) {
        if (isFullPage())
          std::cout << " onclick=\"openTab('" << tab->name << "', this, '" << tab->color << "')\"";
        else
          std::cout << " onclick=\"openTab(event, '" << tab->name << "')\"";
      }

      if (first)
        std::cout << " id=\"defaultOpen\"";

      std::cout << ">" << tab->name << "</button>\n";

      first = false;
    }

    std::cout << "</div>\n";

    std::cout << "\n";
    std::cout << "<!-- Tab content -->\n";

    for (const auto &tab : tabs) {
      std::cout << "<div id=\"" << tab->name << "\" class=\"tabcontent\">\n";

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
  else if (type() == Type::ACCORDION) {
    bool first = true;

    std::cout << "<!-- Tab buttons and content -->\n";

    for (const auto &tab : tabs) {
      std::cout << "<button class=\"accordion\"";

      if (first)
        std::cout << " id=\"defaultOpen\"";

      std::cout << ">" << tab->name << "</button>\n";

      first = false;

      //---

      std::cout << "<div class=\"panel\">\n";

      for (const auto &line1 : tab->lines)
        std::cout << line1 << "\n";

      std::cout << "</div>\n";
    }

    //std::cout << "\n";
    //std::cout << "<script>\n";
    //std::cout << "document.getElementById(\"defaultOpen\").click();\n";
    //std::cout << "</script>\n";
  }
  else if (type() == Type::IMAGES) {
    std::cout << "<!-- Slideshow container -->\n";
    std::cout << "<div class=\"slideshow-container\">\n";
    std::cout << "\n";
    std::cout << "  <!-- Full-width images with number and caption text -->\n";

    int n = tabs.size();

    int i = 1;

    for (const auto &tab : tabs) {
      std::cout << "  <div class=\"mySlides fade\">\n";
      std::cout << "    <div class=\"numbertext\">" << i << " / " << n << "</div>\n";
      std::cout << "    <img src=\"" << tab->name << "\" style=\"width:100%\">\n";
      std::cout << "    <div class=\"text\">" << tab->desc << "</div>\n";
      std::cout << "  </div>\n";
      std::cout << "\n";

      ++i;
    }

    std::cout << "  <!-- Next and previous buttons -->\n";
    std::cout << "  <a class=\"prev\" onclick=\"plusSlides(-1)\">&#10094;</a>\n";
    std::cout << "  <a class=\"next\" onclick=\"plusSlides(1)\">&#10095;</a>\n";
    std::cout << "</div>\n";
    std::cout << "<br>\n";
    std::cout << "\n";
    std::cout << "<!-- The dots/circles -->\n";
    std::cout << "<div style=\"text-align:center\">\n";

    i = 1;

    for ( ; i <= n; ++i) {
      std::cout << "  <span class=\"dot\" onclick=\"currentSlide(" << i << ")\"></span>\n";
    }

    std::cout << "</div> \n";
  }

  for (const auto &line : endLines)
    std::cout << line << "\n";
}
