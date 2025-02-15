#!/usr/bin/env python3

import re

version_file = "include/libfork/version.hpp"


def extract_version_component(pattern, file_content):
    if match := re.search(pattern, file_content):
        return match.group(1)

    raise ValueError(f"Could not match {pattern=}")


if __name__ == "__main__":
    with open(version_file, "r") as file:
        content = file.read()

    major = extract_version_component(r"#define LF_VERSION_MAJOR (\d+)", content)
    minor = extract_version_component(r"#define LF_VERSION_MINOR (\d+)", content)
    patch = extract_version_component(r"#define LF_VERSION_PATCH (\d+)", content)

    print(f"{major}.{minor}.{patch}")
