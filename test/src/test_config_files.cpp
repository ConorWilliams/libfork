/**
 * @file test_config_files.cpp
 * @brief Tests for validating configuration files in the repository.
 *
 * These tests verify that configuration files are properly formatted and loadable.
 */

#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

/**
 * @brief Helper function to read a file into a string.
 * @param filepath Path to the file to read
 * @return Contents of the file as a string
 */
auto read_file(std::string const& filepath) -> std::string {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    return "";
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

/**
 * @brief Helper function to check if a file exists.
 * @param filepath Path to check
 * @return true if file exists and is readable, false otherwise
 */
auto file_exists(std::string const& filepath) -> bool {
  std::ifstream file(filepath);
  return file.good();
}

/**
 * @brief Helper to check if a string contains another string.
 */
auto contains(std::string const& haystack, std::string const& needle) -> bool {
  return haystack.find(needle) != std::string::npos;
}

}  // namespace

TEST_CASE(".clang-format configuration is valid", "[config]") {
  std::string const filepath = ".clang-format";

  SECTION("file exists") {
    REQUIRE(file_exists(filepath));
  }

  SECTION("file is readable and not empty") {
    auto content = read_file(filepath);
    REQUIRE(!content.empty());
  }

  SECTION("contains expected YAML document markers") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "---"));
    REQUIRE(contains(content, "..."));
  }

  SECTION("contains essential clang-format keys") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "Language:"));
    REQUIRE(contains(content, "ColumnLimit:"));
  }

  SECTION("Language is set to Cpp") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "Language: Cpp"));
  }

  SECTION("ColumnLimit is defined") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "ColumnLimit:"));
  }
}

TEST_CASE(".clang-tidy configuration is valid", "[config]") {
  std::string const filepath = ".clang-tidy";

  SECTION("file exists") {
    REQUIRE(file_exists(filepath));
  }

  SECTION("file is readable and not empty") {
    auto content = read_file(filepath);
    REQUIRE(!content.empty());
  }

  SECTION("contains Checks configuration") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "Checks:"));
  }

  SECTION("contains CheckOptions") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "CheckOptions:"));
  }

  SECTION("disables specific checks as expected") {
    auto content = read_file(filepath);
    // Should disable some google checks
    REQUIRE(contains(content, "-google-readability-todo"));
  }
}

TEST_CASE(".clangd configuration is valid", "[config]") {
  std::string const filepath = ".clangd";

  SECTION("file exists") {
    REQUIRE(file_exists(filepath));
  }

  SECTION("file is readable and not empty") {
    auto content = read_file(filepath);
    REQUIRE(!content.empty());
  }

  SECTION("contains CompileFlags section") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "CompileFlags:"));
  }

  SECTION("specifies compilation database") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "CompilationDatabase:"));
  }
}

TEST_CASE(".codespellrc configuration is valid", "[config]") {
  std::string const filepath = ".codespellrc";

  SECTION("file exists") {
    REQUIRE(file_exists(filepath));
  }

  SECTION("file is readable and not empty") {
    auto content = read_file(filepath);
    REQUIRE(!content.empty());
  }

  SECTION("contains codespell section") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "[codespell]"));
  }

  SECTION("has builtin configuration") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "builtin"));
  }

  SECTION("has skip configuration") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "skip"));
  }
}

TEST_CASE(".gitignore is valid", "[config]") {
  std::string const filepath = ".gitignore";

  SECTION("file exists") {
    REQUIRE(file_exists(filepath));
  }

  SECTION("file is readable and not empty") {
    auto content = read_file(filepath);
    REQUIRE(!content.empty());
  }

  SECTION("ignores build directories") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "build/"));
  }

  SECTION("ignores IDE directories") {
    auto content = read_file(filepath);
    REQUIRE((contains(content, ".vscode/") || contains(content, ".idea/")));
  }
}

TEST_CASE("GitHub workflow files are valid YAML", "[config][workflows]") {
  std::vector<std::string> workflow_files = {
      ".github/workflows/linear.yml", ".github/workflows/lint.yml",
      ".github/workflows/linux.yml",  ".github/workflows/macos.yml",
  };

  for (auto const& filepath : workflow_files) {
    DYNAMIC_SECTION("Workflow file: " << filepath) {
      SECTION("file exists") {
        REQUIRE(file_exists(filepath));
      }

      SECTION("file is readable and not empty") {
        auto content = read_file(filepath);
        REQUIRE(!content.empty());
      }

      SECTION("has name field") {
        auto content = read_file(filepath);
        REQUIRE(contains(content, "name:"));
      }

      SECTION("has on trigger field") {
        auto content = read_file(filepath);
        REQUIRE(contains(content, "on:"));
      }

      SECTION("has jobs section") {
        auto content = read_file(filepath);
        REQUIRE(contains(content, "jobs:"));
      }

      SECTION("has steps in jobs") {
        auto content = read_file(filepath);
        REQUIRE(contains(content, "steps:"));
      }
    }
  }
}

TEST_CASE("GitHub linear workflow validates merge commits", "[config][workflows]") {
  std::string const filepath = ".github/workflows/linear.yml";

  SECTION("file exists") {
    REQUIRE(file_exists(filepath));
  }

  SECTION("checks for merge commits") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "merge commits"));
    REQUIRE(contains(content, "git rev-list --merges"));
  }

  SECTION("targets modules branch") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "modules"));
  }
}

TEST_CASE("GitHub lint workflow runs linting tools", "[config][workflows]") {
  std::string const filepath = ".github/workflows/lint.yml";

  SECTION("file exists") {
    REQUIRE(file_exists(filepath));
  }

  SECTION("runs codespell") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "codespell"));
  }

  SECTION("runs clang-format") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "clang-format"));
  }
}

TEST_CASE("GitHub linux workflow builds and tests", "[config][workflows]") {
  std::string const filepath = ".github/workflows/linux.yml";

  SECTION("file exists") {
    REQUIRE(file_exists(filepath));
  }

  SECTION("runs on ubuntu") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "ubuntu-latest"));
  }

  SECTION("has build step") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "Build"));
  }

  SECTION("has test step") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "Test"));
    REQUIRE(contains(content, "ctest"));
  }

  SECTION("uses matrix strategy") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "matrix:"));
    REQUIRE(contains(content, "preset:"));
  }
}

TEST_CASE("GitHub macos workflow builds and tests", "[config][workflows]") {
  std::string const filepath = ".github/workflows/macos.yml";

  SECTION("file exists") {
    REQUIRE(file_exists(filepath));
  }

  SECTION("runs on macos") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "macos-latest"));
  }

  SECTION("has build step") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "Build"));
  }

  SECTION("has test step") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "Test"));
    REQUIRE(contains(content, "ctest"));
  }
}

TEST_CASE("CMakePresets.json is valid", "[config][cmake]") {
  std::string const filepath = ".legacy/CMakePresets.json";

  SECTION("file exists") {
    REQUIRE(file_exists(filepath));
  }

  SECTION("file is readable and not empty") {
    auto content = read_file(filepath);
    REQUIRE(!content.empty());
  }

  SECTION("is valid JSON structure") {
    auto content = read_file(filepath);
    // Basic JSON validation - has opening and closing braces
    REQUIRE(contains(content, "{"));
    REQUIRE(contains(content, "}"));
  }

  SECTION("has version field") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "\"version\""));
  }

  SECTION("has configurePresets") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "\"configurePresets\""));
  }

  SECTION("has cmakeMinimumRequired") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "\"cmakeMinimumRequired\""));
  }
}

TEST_CASE(".gemini/settings.json is valid", "[config]") {
  std::string const filepath = ".gemini/settings.json";

  SECTION("file exists") {
    REQUIRE(file_exists(filepath));
  }

  SECTION("file is readable and not empty") {
    auto content = read_file(filepath);
    REQUIRE(!content.empty());
  }

  SECTION("is valid JSON structure") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "{"));
    REQUIRE(contains(content, "}"));
  }

  SECTION("has context section") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "\"context\""));
  }

  SECTION("has ui section") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "\"ui\""));
  }
}

TEST_CASE("Documentation files exist and are valid", "[config][docs]") {
  std::vector<std::string> doc_files = {".legacy/BUILDING.md", ".legacy/CODE_OF_CONDUCT.md",
                                        ".legacy/CONTRIBUTING.md", ".legacy/ChangeLog.md",
                                        ".legacy/TODO.md"};

  for (auto const& filepath : doc_files) {
    DYNAMIC_SECTION("Documentation file: " << filepath) {
      SECTION("file exists") {
        REQUIRE(file_exists(filepath));
      }

      SECTION("file is readable and not empty") {
        auto content = read_file(filepath);
        REQUIRE(!content.empty());
      }

      SECTION("contains markdown headers") {
        auto content = read_file(filepath);
        // Most markdown files should have at least one header
        REQUIRE(contains(content, "#"));
      }
    }
  }
}

TEST_CASE("CITATION.cff is valid", "[config][citation]") {
  std::string const filepath = ".legacy/CITATION.cff";

  SECTION("file exists") {
    REQUIRE(file_exists(filepath));
  }

  SECTION("file is readable and not empty") {
    auto content = read_file(filepath);
    REQUIRE(!content.empty());
  }

  SECTION("has cff-version") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "cff-version:"));
  }

  SECTION("has title") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "title:"));
  }

  SECTION("has authors") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "authors:"));
  }

  SECTION("has license") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "license:"));
  }
}

TEST_CASE("Benchmark CMakeLists.txt is valid", "[config][cmake]") {
  std::string const filepath = ".legacy/bench/CMakeLists.txt";

  SECTION("file exists") {
    REQUIRE(file_exists(filepath));
  }

  SECTION("file is readable and not empty") {
    auto content = read_file(filepath);
    REQUIRE(!content.empty());
  }

  SECTION("has cmake_minimum_required") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "cmake_minimum_required"));
  }

  SECTION("has project declaration") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "project("));
  }

  SECTION("finds required packages") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "find_package"));
  }

  SECTION("links benchmark library") {
    auto content = read_file(filepath);
    REQUIRE(contains(content, "benchmark"));
  }
}

TEST_CASE("Configuration files are mutually consistent", "[config][consistency]") {
  SECTION("clang-format and clang-tidy both exist") {
    REQUIRE(file_exists(".clang-format"));
    REQUIRE(file_exists(".clang-tidy"));
  }

  SECTION("workflow files all target same branch") {
    auto linear = read_file(".github/workflows/linear.yml");
    auto lint = read_file(".github/workflows/lint.yml");
    auto linux_wf = read_file(".github/workflows/linux.yml");
    auto macos = read_file(".github/workflows/macos.yml");

    // All should reference the modules branch
    REQUIRE(contains(linear, "modules"));
    REQUIRE(contains(lint, "modules"));
    REQUIRE(contains(linux_wf, "modules"));
    REQUIRE(contains(macos, "modules"));
  }

  SECTION("both Linux and macOS workflows use similar structure") {
    auto linux_wf = read_file(".github/workflows/linux.yml");
    auto macos = read_file(".github/workflows/macos.yml");

    // Both should have Build and Test steps
    REQUIRE(contains(linux_wf, "Build"));
    REQUIRE(contains(macos, "Build"));
    REQUIRE(contains(linux_wf, "Test"));
    REQUIRE(contains(macos, "Test"));
  }
}