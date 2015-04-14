# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

from waflib.Configure import conf

OVERRIDE = '''
class Base
{
  virtual void
  f(int a);
};

class Derived : public Base
{
  virtual void
  f(int a) override;
};
'''

@conf
def check_override(self):
    self.check_cxx(msg='Checking for override specifier',
                   fragment=OVERRIDE,
                   define_name='HAVE_CXX_OVERRIDE',
                   features='cxx', mandatory=False)

STD_TO_STRING = '''
#include <string>
int
main(int argc, char** argv)
{
  std::string s = std::to_string(0);
  s = std::to_string(0l);
  s = std::to_string(0ll);
  s = std::to_string(0u);
  s = std::to_string(0ul);
  s = std::to_string(0ull);
  s = std::to_string(0.0f);
  s = std::to_string(0.0);
  s = std::to_string(0.0l);
  s.clear();
  return 0;
}
'''

@conf
def check_std_to_string(self):
    self.check_cxx(msg='Checking for std::to_string',
                   fragment=STD_TO_STRING,
                   define_name='HAVE_STD_TO_STRING',
                   features='cxx', mandatory=False)

def configure(conf):
    conf.check_override()
    conf.check_std_to_string()
