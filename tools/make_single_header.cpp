// make_single_header.cpp
//
// Copyright (c) 2018 Tristan Brindle (tcbrindle at gmail dot com)
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

#include "libfork/core/macro.hpp"

namespace fs = std::filesystem;

namespace {

constexpr auto &include_regex = R"-((?:^|\n)#include "(libfork(?:[\w/]*)+.hpp)")-";

template <typename Rng, typename Value>
auto contains(Rng const &range, Value const &val) -> bool {
  return std::find(range.begin(), range.end(), val) != std::end(range);
}

struct include_processor {

  static auto run(fs::path const &start_file) -> std::string {
    auto start_path = start_file;
    start_path.remove_filename();
    return include_processor{std::move(start_path)}.process_one(start_file);
  }

 private:
  struct replacement {
    std::ptrdiff_t pos;
    std::ptrdiff_t len;
    std::string text;
  };

  explicit include_processor(fs::path &&start_path) : m_start_path(std::move(start_path)) {}

  auto process_one(fs::path const &path) -> std::string {
    //
    // std::cout << "Processing path " << path << '\n';

    std::ifstream infile(path);

    std::string text(std::istreambuf_iterator<char>{infile}, std::istreambuf_iterator<char>{});

    std::deque<replacement> replacements;

    std::for_each(std::sregex_iterator(text.begin(), text.end(), m_regex),
                  std::sregex_iterator{},
                  [&](auto const &match) {
                    //
                    auto rep = replacement{match.position(), match.length(), {}};

                    auto new_path = m_start_path;

                    new_path = fs::canonical(new_path.append(match.str(1)));

                    if (!contains(m_processed_paths, new_path)) {
                      std::cout << "Found " << match.str(1) << " in " << path << '\n';
                      rep.text = "\n" + process_one(new_path) + "\n";
                    }

                    replacements.push_back(std::move(rep));
                  });

    process_replacements(text, replacements);
    m_processed_paths.push_back(path);
    return text;
  }

  static void process_replacements(std::string &str, std::deque<replacement> &replacements) {

    std::sort(replacements.begin(), replacements.end(), [](const auto &lhs, const auto &rhs) {
      return lhs.pos < rhs.pos;
    });

    while (!replacements.empty()) {

      auto rep = std::move(replacements.front());

      replacements.pop_front();

      str.replace(rep.pos, rep.len, rep.text);

      for (auto &replace : replacements) {
        replace.pos -= rep.len - rep.text.length();
      }
    }
  }

  fs::path m_start_path;
  std::regex m_regex{include_regex};
  std::vector<fs::path> m_processed_paths;
};

} // namespace

auto main(int argc, char **argv) -> int {
  if (argc != 3) {
    LF_THROW(std::invalid_argument{"Usage: make_single_header IN_FILE.hpp OUT_FILE.hpp"});
  }

  const auto infile_path = fs::canonical(fs::path(argv[1]));
  const auto outfile_path = fs::path(argv[2]);

  const auto out_str = include_processor::run(infile_path);

  std::ofstream outfile(outfile_path);

  outfile << std::endl;

  outfile << "//---------------------------------------------------------------//" << std::endl;
  outfile << "//        This is a machine generated file DO NOT EDIT IT        //" << std::endl;
  outfile << "//---------------------------------------------------------------//" << std::endl;

  outfile << std::endl;

  std::copy(out_str.begin(), out_str.end(), std::ostreambuf_iterator<char>(outfile));
}
