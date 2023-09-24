#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "rxcpp/rx.hpp"

using namespace std;

struct IconLine {
  enum class Type { Source, Launcher };

  string name;
  string icon1st;
  string icon2nd;
  Type type;

  const string str() const { return name + '\t' + icon1st + '\t' + icon2nd; }
};

std::string remove_last_whitespaces(const std::string& input) {
  std::string result = input;
  size_t lastWhitespace = result.find_last_of(" \t\n\r\f\v");

  if (lastWhitespace != std::string::npos) {
    result.erase(lastWhitespace);
  }

  return result;
}

IconLine to_icon_line(const string& line) {
  string token;
  stringstream ss(line);
  vector<string> tokens;
  while (getline(ss, token, '\t')) {
    remove_last_whitespaces(token);
    tokens.push_back(token);
  }

  auto is_launcher = (tokens.size() == 3);

  // clang-format off
  return IconLine {
    tokens[0],
    tokens[1],
    is_launcher ? tokens[2] : "",
    is_launcher ? IconLine::Type::Launcher : IconLine::Type::Source
  };
  // clang-format on
}

struct IconMap {
  struct Icon {
    string name;
    string source_icon;
    string launcher_f_icon;
    string launcher_n_icon;
  };

  map<string, Icon> icons;
  Icon _invalid_icon;

  const Icon& get_icon(const string& name) const {
    auto itr = icons.find(name);
    auto& icon = (itr == icons.end()) ? _invalid_icon : itr->second;
    return icon;
  }

  void update(const IconLine& line) {
    auto itr = icons.find(line.name);
    if (itr == icons.end()) {
      icons[line.name] = _create_icon(line);
    } else {
      _update_icon(icons[line.name], line);
    }
  }

  Icon _create_icon(const IconLine& line) {
    if (line.type == IconLine::Type::Source) {
      return Icon{line.name, line.icon1st, "", ""};
    } else {
      return Icon{line.name, "", line.icon1st, line.icon2nd};
    }
  }

  void _update_icon(Icon& icon, const IconLine& line) {
    if (line.type == IconLine::Type::Source) {
      icon.source_icon = line.icon1st;
    } else {
      icon.launcher_f_icon = line.icon1st;
      icon.launcher_n_icon = line.icon2nd;
    }
  }
};

rxcpp::observable<string> read_lines(const string& filepath) {
  return rxcpp::observable<>::create<string>([filepath](auto s) {
    ifstream file(filepath);
    if (!file.is_open()) {
      s.on_error(
          std::make_exception_ptr(std::runtime_error("file open failed")));
      return;
    }

    string line;
    while (getline(file, line)) {
      if (line.empty()) continue;
      
      s.on_next(line);
    }

    s.on_completed();
  });
}

TEST(rxcpp, read_source_n_launcher_files) {
  auto sourceLines = read_lines("../data/source_icon/Source/Icon/logolist.info")
                         .skip(1)
                         .map(to_icon_line);

  auto launcherLines =
      read_lines("../data/source_icon/Launcher/Icon/logolist.info")
          .skip(1)
          .map(to_icon_line);

  IconMap iconMap;
  sourceLines.merge(launcherLines)
      .reduce(IconMap(),
              [](auto container, auto iconLine) {
                container.update(iconLine);
                return container;
              })
      .subscribe([&](auto i) { iconMap = i; });

  auto roku = iconMap.get_icon("Roku");
  ASSERT_EQ(roku.name, "Roku");
  ASSERT_EQ(roku.source_icon, "source_icon_roku.png");
  ASSERT_EQ(roku.launcher_f_icon, "launcher_f_roku.png");
  ASSERT_EQ(roku.launcher_n_icon, "launcher_n_roku.png");

  auto game = iconMap.get_icon("Game");
  ASSERT_EQ(game.name, "Game");
  ASSERT_EQ(game.source_icon, "source_icon_game.png");
  ASSERT_EQ(game.launcher_f_icon, "");
  ASSERT_EQ(game.launcher_n_icon, "");
}
