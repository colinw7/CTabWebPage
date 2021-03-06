#include <CTabWebPage.h>
#include <CFile.h>
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

//---

int
main(int argc, char **argv)
{
  auto errMsg = [](const std::string &msg) {
    std::cerr << msg << "\n";
    exit(1);
  };

  bool debug = false;

  using Files = std::vector<std::string>;

  Files files;

  CTabWebPage::Type        type        = CTabWebPage::Type::TAB;
  CTabWebPage::Orientation orientation = CTabWebPage::Orientation::HORIZONTAL;

  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      auto opt = std::string(&argv[i][1]);

      if      (opt == "debug")
        debug = true;
      else if (opt == "type") {
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
      else if (opt == "h" || opt == "help") {
        std::cout << "CWebPage [-h]" << "\n";
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

  page.init();

  for (const auto &file : files) {
    CFile f(file);

    if (! f.exists()) {
      std::cerr << "'" << file << "' does not exists\n";
      continue;
    }

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
  std::cout << "<!DOCTYPE html>\n";
  std::cout << "<html>\n";
  std::cout << "<head>\n";

  if      (type_ == Type::TAB) {
    std::cout << "<script src=\"tabs.js\" type=\"text/javascript\"></script>\n";

    if (orientation_ == Orientation::HORIZONTAL)
      std::cout << "<link href=\"htabs.css\" rel=\"stylesheet\" type=\"text/css\"/>\n";
    else
      std::cout << "<link href=\"vtabs.css\" rel=\"stylesheet\" type=\"text/css\"/>\n";
  }
  else if (type_ == Type::ACCORDION) {
    std::cout << "<link href=\"accordion.css\" rel=\"stylesheet\" type=\"text/css\"/>\n";
  }
  else if (type_ == Type::IMAGES) {
    std::cout << "<link href=\"images.css\" rel=\"stylesheet\" type=\"text/css\"/>\n";
  }

  std::cout << "</head>\n";
  std::cout << "<body>\n";
}

void
CTabWebPage::
term()
{
  if      (type_ == Type::ACCORDION)
    std::cout << "<script src=\"accordion.js\" type=\"text/javascript\"></script>\n";
  else if (type_ == Type::IMAGES)
    std::cout << "<script src=\"images.js\" type=\"text/javascript\"></script>\n";

  std::cout << "</body>\n";
  std::cout << "</html>\n";
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
    Lines       lines;

    TabData() = default;

    explicit TabData(const std::string &name) :
     name(name) {
    }
  };

  using Tabs = std::vector<TabData *>;

  Lines    preLines, postLines;
  Tabs     tabs;
  TabData *currentTab = nullptr;

  for (const auto &line : lines) {
    int len = line.length();

    if      (type_ == Type::TAB || type_ == Type::ACCORDION) {
      if (len > 13 && line.substr(0, 9) == "<!-- TAB=" && line.substr(len - 4) == " -->") {
        auto tabName = line.substr(9, len - 13);

        auto *tab = new TabData(tabName);

        tabs.push_back(tab);

        currentTab = tab;
      }
      else {
        if (currentTab)
          currentTab->lines.push_back(line);
        else
          preLines.push_back(line);
      }
    }
    else if (type_ == Type::IMAGES) {
      if (len > 15 && line.substr(0, 11) == "<!-- IMAGE=" && line.substr(len - 4) == " -->") {
        auto imageName = line.substr(11, len - 15);

        auto *tab = new TabData(imageName);

        tab->desc = imageName;

        tabs.push_back(tab);

        currentTab = tab;
      }
      else {
        if (currentTab)
          currentTab->lines.push_back(line);
        else
          preLines.push_back(line);
      }
    }
  }

  if      (type_ == Type::TAB) {
    std::cout << "<!-- Tab buttons -->\n";
    std::cout << "<div class=\"tab\">\n";

    bool first = true;

    for (const auto &tab : tabs) {
      std::cout << "  <button class=\"tabs\"";

      if (type_ == Type::TAB)
        std::cout << " onclick=\"openTab(event, '" << tab->name << "')\"";

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
      std::cout << "<h3>" << tab->name << "</h3>\n";

      for (const auto &line : tab->lines) {
        std::cout << line << "\n";
      }

      std::cout << "</div>\n";
    }

    std::cout << "\n";
    std::cout << "<script>\n";
    std::cout << "document.getElementById(\"defaultOpen\").click();\n";
    std::cout << "</script>\n";
  }
  else if (type_ == Type::ACCORDION) {
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

      for (const auto &line : tab->lines) {
        std::cout << line << "\n";
      }

      std::cout << "</div>\n";
    }

    //std::cout << "\n";
    //std::cout << "<script>\n";
    //std::cout << "document.getElementById(\"defaultOpen\").click();\n";
    //std::cout << "</script>\n";
  }
  else if (type_ == Type::IMAGES) {
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
}
