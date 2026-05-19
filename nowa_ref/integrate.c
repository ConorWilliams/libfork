#include <stdio.h>
#include "test.h"

int n = 10000;

static double m;
static const double epsilon = 1.0e-9;

static double f(double x)
{
  return (x * x + 1.0) * x;
}

static
double integrate_serial(double x1, double y1, double x2, double y2, double area)
{
  double half = (x2 - x1) / 2;
  double x0 = x1 + half;
  double y0 = f(x0);

  double area_x1x0 = (y1 + y0) / 2 * half;
  double area_x0x2 = (y0 + y2) / 2 * half;
  double area_x1x2 = area_x1x0 + area_x0x2;

  if (area_x1x2 - area < epsilon && area - area_x1x2 < epsilon) {
    return area_x1x2;
  }

  area_x1x0 = integrate_serial(x1, y1, x0, y0, area_x1x0);
  area_x0x2 = integrate_serial(x0, y0, x2, y2, area_x0x2);

  return area_x1x0 + area_x0x2;
}

static fibril
double integrate(double x1, double y1, double x2, double y2, double area)
{
  double half = (x2 - x1) / 2;
  double x0 = x1 + half;
  double y0 = f(x0);

  double area_x1x0 = (y1 + y0) / 2 * half;
  double area_x0x2 = (y0 + y2) / 2 * half;
  double area_x1x2 = area_x1x0 + area_x0x2;

  if (area_x1x2 - area < epsilon && area - area_x1x2 < epsilon) {
    return area_x1x2;
  }

  fibril_t fr;
  fibril_init(&fr);

  fibril_fork(&fr, &area_x1x0, integrate, (x1, y1, x0, y0, area_x1x0));
  area_x0x2 = integrate(x0, y0, x2, y2, area_x0x2);

  fibril_join(&fr);
  return area_x1x0 + area_x0x2;
}

void init() {}
void prep() {}

void test()
{
  m = integrate(0, f(0), n, f(n), 0);
}

int verify()
{
  double expect = integrate_serial(0, f(0), n, f(n), 0);

  if (m - expect < epsilon && expect - m < epsilon) {
    return 0;
  }

  printf("integrate(%d)=%lf (expected %lf)\n", n, m, expect);
  return 1;
}

