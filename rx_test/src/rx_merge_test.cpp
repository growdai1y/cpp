#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
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

void remove_last_whitespaces(std::string& str) {
  while (!str.empty() && isspace(str.back())) str.pop_back();
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

IconLine icon_line_with_fullpath(const IconLine& icon,
                                 const map<string, string>& paths) {
  auto copy = icon;
  auto itr = paths.find(icon.icon1st);
  if (itr != paths.end()) copy.icon1st = itr->second;

  itr = paths.find(icon.icon2nd);
  if (itr != paths.end()) copy.icon2nd = itr->second;

  return copy;
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

TEST(rxcpp, read_lines) {
  auto lines =
      read_lines("../data/source_icon/Source/Icon/logolist.info").skip(1);

  vector<string> lines_read;
  lines.subscribe([&](auto line) { lines_read.push_back(line); });

  ASSERT_EQ(lines_read[0], "Roku	source_icon_roku.png");
  ASSERT_EQ(lines_read[1], "Game	source_icon_game.png");
  ASSERT_EQ(lines_read[2], "XBox One	source_icon_xboxone.png");
}

TEST(rxcpp, to_icon_line) {
  const auto str = "Roku	source_icon_roku.png";
  auto line = to_icon_line(str);

  ASSERT_EQ(line.name, "Roku");
  ASSERT_EQ(line.icon1st, "source_icon_roku.png");
  ASSERT_EQ(line.icon2nd, "");
}

TEST(rxcpp, build_icon_map) {
  auto rokuSource =
      IconLine{"Roku", "source_icon_roku.png", "", IconLine::Type::Source};
  auto rokuLauncher = IconLine{"Roku", "launcher_f_roku.png",
                               "launcher_n_roku.png", IconLine::Type::Launcher};

  auto iconMap = IconMap();
  iconMap.update(rokuSource);
  iconMap.update(rokuLauncher);

  auto roku = iconMap.get_icon("Roku");
  ASSERT_EQ(roku.name, "Roku");
  ASSERT_EQ(roku.source_icon, "source_icon_roku.png");
  ASSERT_EQ(roku.launcher_f_icon, "launcher_f_roku.png");
  ASSERT_EQ(roku.launcher_n_icon, "launcher_n_roku.png");
}

auto build_path_map(const string& dirpath) {
  map<string, string> pathMap;
  for (const auto& entry : filesystem::directory_iterator(dirpath)) {
    pathMap[entry.path().filename().string()] =
        entry.path().parent_path().string() + '/' +
        entry.path().filename().string();
  }

  return pathMap;
}

TEST(rxcpp, build_absolute_filepaths) {
  auto pathMap = build_path_map("../data/source_icon/Source/Icon");
  auto itr = pathMap.find("source_icon_roku.png");

  ASSERT_EQ(itr->second,
            "../data/source_icon/Source/Icon/source_icon_roku.png");
}

auto read_icon_lines(const string& icondir, const string& logofile) {
  auto pathMap = build_path_map(icondir);
  auto lines =
      read_lines(logofile)
          .skip(1)
          .map(to_icon_line)
          .map(bind(icon_line_with_fullpath, placeholders::_1, pathMap));
  return lines;
}

TEST(rxcpp, read_source_n_launcher_files) {
  auto iconMap =
      read_icon_lines("../data/source_icon/Source/Icon",
                      "../data/source_icon/Source/Icon/logolist.info")
          .merge(read_icon_lines(
              "../data/source_icon/Launcher/Icon/2K",
              "../data/source_icon/Launcher/Icon/logolist.info"))
          .reduce(IconMap(),
                  [](auto container, auto iconLine) {
                    container.update(iconLine);
                    return container;
                  })
          .as_blocking()
          .last();

  auto roku = iconMap.get_icon("Roku");
  ASSERT_EQ(roku.name, "Roku");
  ASSERT_EQ(roku.source_icon,
            "../data/source_icon/Source/Icon/source_icon_roku.png");
  ASSERT_EQ(roku.launcher_f_icon,
            "../data/source_icon/Launcher/Icon/2K/launcher_f_roku.png");
  ASSERT_EQ(roku.launcher_n_icon,
            "../data/source_icon/Launcher/Icon/2K/launcher_n_roku.png");

  auto game = iconMap.get_icon("Game");
  ASSERT_EQ(game.name, "Game");
  ASSERT_EQ(game.source_icon,
            "../data/source_icon/Source/Icon/source_icon_game.png");
  ASSERT_EQ(game.launcher_f_icon, "");
  ASSERT_EQ(game.launcher_n_icon, "");
}
