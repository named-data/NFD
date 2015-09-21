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

FINAL = '''
class Base
{
  virtual void
  f(int a);
};

class Derived : public Base
{
  virtual void
  f(int a) final;
};
'''

@conf
def check_final(self):
    self.check_cxx(msg='Checking for final specifier on method',
                   fragment=FINAL,
                   define_name='HAVE_CXX_FINAL',
                   features='cxx', mandatory=False)

CLASS_FINAL = '''
class A final
{
};
'''

@conf
def check_class_final(self):
    self.check_cxx(msg='Checking for final specifier on class',
                   fragment=CLASS_FINAL,
                   define_name='HAVE_CXX_CLASS_FINAL',
                   features='cxx', mandatory=False)

def configure(conf):
    conf.check_override()
    conf.check_final()
    conf.check_class_final()
