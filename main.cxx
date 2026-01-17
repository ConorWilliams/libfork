#include <string>

void hello_world(std::string const &name);

int main(int argc, char *argv[]) {
  hello_world(argv[0] ? argv[0] : "Voldemort?");
  return 0;
}
