#! /usr/bin/env python
# encoding: utf-8

from waflib import Logs, Utils, Task, TaskGen, Build
from waflib.Tools import c_preproc, cxx

def options(opt):
    opt.add_option('--with-pch', action='store_true', default=False, dest='with_pch',
                   help='''Try to use precompiled header to speed up compilation '''
                        '''(only gcc and clang)''')

def configure(conf):
    conf.env['WITH_PCH'] = conf.options.with_pch

class gchx(Task.Task):
    run_str = '${CXX} -x c++-header ${CXXFLAGS} ${FRAMEWORKPATH_ST:FRAMEWORKPATH} ' + \
              '${CPPPATH_ST:INCPATHS} ${DEFINES_ST:DEFINES} ' + \
              '${CXX_SRC_F}${SRC} ${CXX_TGT_F}${TGT}'
    scan    = c_preproc.scan
    color   = 'BLUE'


PCH_COMPILER_OPTIONS = {
    'clang++': ['-include', '.pch', ''],
    'g++': ['-include', '.gch', ''],
}

@TaskGen.extension('.cpp')
def cxx_hook(self, node):
    if (self.env.WITH_PCH and
        self.env['COMPILER_CXX'] in PCH_COMPILER_OPTIONS.keys() and
        getattr(self, 'pch', None)):

        (flag, pch_ext, include_ext) = PCH_COMPILER_OPTIONS[self.env['COMPILER_CXX']]

        if not getattr(self, 'pch_task', None):
            if isinstance(self.pch, str):
                self.pch = self.path.find_node(self.pch)

            output = self.pch.change_ext('.hpp%s' % pch_ext)

            try:
                # == Will reuse the existing task ==
                # This could cause problem if different compiler flags were used for
                # previous compilation of the precompiled header
                existingTask = self.bld.get_tgen_by_name(output.abspath())
                self.pch_task = existingTask
            except:
                self.pch_task = self.create_task('gchx', self.pch, output)
                self.pch_task.name = output.abspath()

                self.bld.task_gen_cache_names = {}
                self.bld.add_to_group(self.pch_task, group=getattr(self, 'group', None))

        task = self.create_compiled_task('cxx_pch', node)
        # task.run_after.update(set([self.pch_task]))

        task.pch_flag = flag
        task.pch_include_file = self.pch.change_ext('.hpp%s' % include_ext).get_bld()

        out = '%s%s' % (self.pch.name, pch_ext)
        task.pch_file = self.pch.parent.get_bld().find_or_declare(out)
    else:
        self.create_compiled_task('cxx', node)

class cxx_pch(Task.classes['cxx']):
    run_str = '${CXX} ${tsk.pch_flag} ${tsk.pch_include_file.abspath()} '\
              '${ARCH_ST:ARCH} ${CXXFLAGS} ${CPPFLAGS} ' \
              '${FRAMEWORKPATH_ST:FRAMEWORKPATH} ${CPPPATH_ST:INCPATHS} ${DEFINES_ST:DEFINES} ' \
              '${CXX_SRC_F}${SRC} ${CXX_TGT_F}${TGT[0].abspath()}'

    def scan(self):
        (nodes, names) = c_preproc.scan(self)
        nodes.append(self.pch_file)
        return (nodes, names)
